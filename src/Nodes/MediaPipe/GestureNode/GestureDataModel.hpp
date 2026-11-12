#pragma once

/**
 * @file GestureDataModel.hpp
 * @brief 手势识别节点（Gesture）
 *
 * IMAGE 输入 → MediaPipe 手部 21 关键点（本节点私有引擎副本）→ 64 维特征 →
 * 与 .gesture 动作库（GestureRecorder.exe 录制）逐模板余弦匹配 → 滞回确认。
 *
 * 端口：IN IMAGE / ENABLE；OUT IMAGE（叠画标签）/ RESULT + 每个手势 1 个动态端口
 * RESULT：{ "gesture", "confidence", "present", "confirmed", "threshold" }
 * 动态端口：加载动作库后自动按手势数扩展（端口 2..N+1，标签=手势名，
 * 输出 bool = 该手势 confirmed 电平，仅当前确认手势为 true）
 *
 * 动作库从媒体库 Gestures 分类选择（SelectorComboBox，存文件名），
 * 运行时在媒体库 allFiles() 按文件名反查全路径（兜底 Documents/Flow/Medias）。
 *
 * 特征/库格式约定见 GestureFeature.hpp 与 GestureLibrary.hpp（与录制工具严格一致）。
 * 阈值语义：每个手势的文件阈值优先；全局「最低阈值」滑杆是叠加的下限（默认 0=不干预）。
 * 动作库文件支持热重载（外部修改后 ~1s 内生效），识别途中不写库。
 */

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/AppConfig/ConstantDefines.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/ImageReadback.h"
#include "Common/Devices/MediaLibrary/MediaLibrary.h"
#include "NodeDataList.hpp"
#include "PluginDefinition.hpp"
#include "GestureInterface.hpp"
#include "GestureFeature.hpp"
#include "GestureLibrary.hpp"
#include "MediaPipeHandEngine.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QElapsedTimer>
#include <QDateTime>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QObject>
#include <QtNodes/NodeDelegateModel>
#include <algorithm>
#include <atomic>
#include <memory>
#include <opencv2/opencv.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace
{
/** 转 BGR + 长边 1280 缩放；Hand 引擎要求三通道 BGR 输入 */
inline cv::Mat scaleMatForInference(const cv::Mat& src)
{
    if (src.empty()) {
        return {};
    }

    cv::Mat bgr;
    if (src.channels() == 4) {
        cv::cvtColor(src, bgr, cv::COLOR_BGRA2BGR);
    } else if (src.channels() == 1) {
        cv::cvtColor(src, bgr, cv::COLOR_GRAY2BGR);
    } else if (src.channels() == 3) {
        bgr = src;
    } else {
        return {};
    }

    constexpr int kMaxSide = 1280;
    const int maxDim = std::max(bgr.cols, bgr.rows);
    if (maxDim <= kMaxSide) {
        return bgr.clone();
    }
    const double scale = static_cast<double>(kMaxSide) / static_cast<double>(maxDim);
    cv::Mat scaled;
    cv::resize(bgr, scaled, cv::Size(), scale, scale, cv::INTER_AREA);
    return scaled;
}

inline cv::Mat readInputMatForInference(const std::shared_ptr<ImageData>& input)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return {};
    }
    return scaleMatForInference(ImageReadback::matFromFrame(frame));
}

inline bool peekLatestInputTimestamp(const std::shared_ptr<ImageData>& input, qint64& timestamp)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return false;
    }
    timestamp = frame.timestamp;
    return true;
}

inline void pushMatToRingBuffer(std::shared_ptr<ImageData>& outData,
                                std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                qint64& lastPushed,
                                cv::Mat&& mat)
{
    if (mat.empty()) {
        return;
    }
    ensureImageDataBuffer(outData, buffer);
    const qint64 ts = TimestampGenerator::getInstance()->getCurrentFrameCount();
    pushFrameToImageBufferDedup(
        buffer, ImageFrame::fromMat(std::move(mat), ts), lastPushed);
}

inline void clearRingBufferOutput(std::shared_ptr<ImageData>& outData,
                                  std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                  qint64& lastPushed)
{
    lastPushed = -1;
    ensureImageDataBuffer(outData, buffer);
    if (buffer) {
        buffer->clear();
    }
}

/** 单次匹配结果（工作线程产物） */
struct GestureMatch
{
    QString label;
    float score = 0.f;
    float threshold = 0.f;
    bool present = false;
};

/** 特征 × 动作库：逐手势取样本最高相似度（同时测试原始与镜像两个朝向，左右手等效） */
inline GestureMatch matchFeature(const GestureLibrary::Library& library,
                                 const std::array<float, GestureFeature::kDim>& feature,
                                 float globalFloor)
{
    const auto featureMirror = GestureFeature::mirrored(feature);
    GestureMatch best;
    bool hasBest = false;
    for (const auto& gesture : library.gestures) {
        float top = -1.f;
        for (const auto& sample : gesture.samples) {
            const float s = std::max(GestureFeature::cosine(feature, sample),
                                     GestureFeature::cosine(featureMirror, sample));
            if (s > top) {
                top = s;
            }
        }
        if (!hasBest || top > best.score) {
            best.label = gesture.label;
            best.score = top;
            best.threshold = static_cast<float>(gesture.threshold);
            hasBest = true;
        }
    }
    if (!hasBest) {
        best.label.clear();
        best.score = 0.f;
        best.present = false;
        return best;
    }
    const float effective = std::max(best.threshold, globalFloor);
    best.threshold = effective;
    best.present = best.score >= effective;
    if (!best.present) {
        best.label.clear();
    }
    return best;
}

/** 叠画标签文字（QImage 栅格化，支持中文标签） */
inline void drawLabelOverlay(cv::Mat& frame, const GestureMatch& match)
{
    if (frame.empty()) {
        return;
    }
    QImage image(frame.data, frame.cols, frame.rows,
                 static_cast<qsizetype>(frame.step), QImage::Format_BGR888);
    if (image.isNull()) {
        return;
    }
    QPainter painter(&image);
    QFont font;
    font.setPixelSize(std::max(20, frame.rows / 24));
    font.setBold(true);
    painter.setFont(font);
    if (match.present) {
        painter.setPen(QColor(80, 255, 120));
        painter.drawText(16, font.pixelSize() + 16,
                         QStringLiteral("%1  %2%")
                             .arg(match.label)
                             .arg(static_cast<int>(match.score * 100.f + 0.5f)));
    } else {
        painter.setPen(QColor(160, 160, 160));
        painter.drawText(16, font.pixelSize() + 16, QStringLiteral("未匹配"));
    }
    painter.end();
}
} // namespace

namespace Nodes
{
/**
 * @brief 手势识别节点：加载 .gesture 动作库，对画面中的手实时模板匹配
 */
class GestureDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double threshold READ getThreshold WRITE setThreshold NOTIFY thresholdChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(double maxFps READ maxFps WRITE setMaxFps NOTIFY maxFpsChanged)
    Q_PROPERTY(bool drawOverlay READ drawOverlay WRITE setDrawOverlay NOTIFY drawOverlayChanged)
    Q_PROPERTY(QString libraryName READ libraryName WRITE setLibraryName NOTIFY libraryNameChanged)

public:
    GestureDataModel()
    {
        InPortCount = 2;
        OutPortCount = 2;
        CaptionVisible = true;
        Caption = PLUGIN_NAME;
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;
        m_outVariable = std::make_shared<VariableData>();
        ensureImageDataBuffer(m_outImage, m_outImageBuffer);
        m_palmPath = "./plugins/Models/palm_detection_mediapipe_2023feb.onnx";
        m_handPath = "./plugins/Models/handpose_estimation_mediapipe_2023feb.onnx";

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "enabled";
            b.control = widget->enableBtn;
            AbstractDelegateModel::registerExternalBinding("/enable", this, b);
        }

        connect(widget->threshold, &FloatDragValueWidget::valueChanged,
                this, &GestureDataModel::setThreshold);
        connect(widget->stableFrames, &FloatDragValueWidget::valueChanged,
                this, &GestureDataModel::setStableFrames);
        connect(widget->maxFps, &FloatDragValueWidget::valueChanged,
                this, &GestureDataModel::setMaxFps);
        connect(widget->drawOverlayCheck, &QCheckBox::toggled,
                this, &GestureDataModel::setDrawOverlay);
        connect(widget->enableBtn, &QPushButton::clicked,
                this, &GestureDataModel::setEnabled);
        connect(widget->librarySelector, &SelectorComboBox::selectionChanged,
                this, &GestureDataModel::setLibraryName);
        connect(widget->reloadBtn, &QPushButton::clicked, this, [this] { reloadLibrary(true); });
    }

    ~GestureDataModel() override
    {
        cancelPendingInference();
        GlobalEventBus::instance()->unsubscribe(this);
    }

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        ImageGpuUpload::instance().warmup();
        auto bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64) { tryScheduleInference(); },
                Qt::QueuedConnection);
    }

    double getThreshold() const { return m_globalFloor; }
    void setThreshold(double value)
    {
        value = std::clamp(value, 0.0, 1.0);
        if (qFuzzyCompare(m_globalFloor, value)) {
            return;
        }
        m_globalFloor = value;
        if (widget && !qFuzzyCompare(widget->threshold->value(), value)) {
            QSignalBlocker blocker(widget->threshold);
            widget->threshold->setValue(value);
        }
        emit thresholdChanged(value);
        requestInferenceRefresh();
    }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool value)
    {
        if (m_enabled == value) {
            return;
        }
        m_enabled = value;
        if (widget && widget->enableBtn->isChecked() != value) {
            QSignalBlocker blocker(widget->enableBtn);
            widget->enableBtn->setChecked(value);
        }
        emit enabledChanged(value);
        if (!m_enabled) {
            cancelPendingInference();
            m_outVariable = std::make_shared<VariableData>();
            resetGestureOutputs();
            clearRingBufferOutput(m_outImage, m_outImageBuffer, m_lastPushedTimestamp);
            Q_EMIT dataUpdated(1);
            return;
        }
        requestInferenceRefresh();
    }

    double maxFps() const { return m_maxFps; }
    void setMaxFps(double value)
    {
        value = std::clamp(value, 1.0, 30.0);
        if (qFuzzyCompare(m_maxFps, value)) {
            return;
        }
        m_maxFps = value;
        if (widget && !qFuzzyCompare(widget->maxFps->value(), value)) {
            QSignalBlocker blocker(widget->maxFps);
            widget->maxFps->setValue(value);
        }
        emit maxFpsChanged(value);
    }

    bool drawOverlay() const { return m_drawOverlay; }
    void setDrawOverlay(bool value)
    {
        if (m_drawOverlay == value) {
            return;
        }
        m_drawOverlay = value;
        if (widget && widget->drawOverlayCheck->isChecked() != value) {
            QSignalBlocker blocker(widget->drawOverlayCheck);
            widget->drawOverlayCheck->setChecked(value);
        }
        emit drawOverlayChanged(value);
    }

    QString libraryName() const { return m_libraryName; }
    void setLibraryName(const QString& name)
    {
        const QString trimmed = name.trimmed();
        if (m_libraryName == trimmed) {
            return;
        }
        m_libraryName = trimmed;
        m_libraryWarned = false;
        if (widget) {
            widget->setLibraryFile(m_libraryName);
        }
        emit libraryNameChanged(m_libraryName);
        reloadLibrary(true);
    }

    void setStableFrames(double value)
    {
        const int frames = static_cast<int>(std::clamp(std::lround(value), 1L, 30L));
        if (m_stableFramesRequired == frames) {
            return;
        }
        m_stableFramesRequired = frames;
    }

    void requestInferenceRefresh()
    {
        m_lastSeenInputTimestamp = -1;
        tryScheduleInference();
    }

Q_SIGNALS:
    void thresholdChanged(double value);
    void enabledChanged(bool value);
    void maxFpsChanged(double value);
    void drawOverlayChanged(bool value);
    void libraryNameChanged(const QString& name);

private Q_SLOTS:
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
        if (localPath == "enable") {
            setEnabled(ev.payload.toBool());
        }
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
        case PortType::In:
            switch (portIndex) {
            case 0: return "IMAGE";
            case 1: return "ENABLE";
            default: return "";
            }
        case PortType::Out:
            if (portIndex == 0) {
                return "IMAGE 0";
            }
            if (portIndex == 1) {
                return "RESULT";
            }
            // 端口 2..N+1：标签 = 手势名（加载库后动态生成）
            {
                const int idx = static_cast<int>(portIndex) - 2;
                if (idx >= 0 && idx < m_gestureLabels.size()) {
                    return m_gestureLabels.at(idx);
                }
            }
            return "";
        default:
            return "";
        }
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
        case PortType::In:
            if (portIndex == 0) {
                return ImageData().type();
            }
            return VariableData().type();
        case PortType::Out:
            if (portIndex == 0) {
                return ImageData().type();
            }
            return VariableData().type();
        default:
            return ImageData().type();
        }
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        if (port == 0) {
            return m_outImage;
        }
        if (port == 1) {
            return m_outVariable;
        }
        // 动态手势端口：confirmed 电平
        const int idx = static_cast<int>(port) - 2;
        if (idx >= 0 && idx < m_gestureOut.size()) {
            return m_gestureOut.at(idx);
        }
        return m_outVariable;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        switch (portIndex) {
        case 0: {
            m_inImage0 = std::dynamic_pointer_cast<ImageData>(data);
            m_lastSeenInputTimestamp = -1;
            if (!m_inImage0) {
                cancelPendingInference();
                clearRingBufferOutput(m_outImage, m_outImageBuffer, m_lastPushedTimestamp);
                return;
            }
            tryScheduleInference();
            break;
        }
        case 1: {
            auto enableData = std::dynamic_pointer_cast<VariableData>(data);
            if (!enableData) {
                return;
            }
            setEnabled(enableData->asBool());
            break;
        }
        default:
            break;
        }
    }

    QWidget* embeddedWidget() override { return widget; }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        QJsonObject values;
        values["libraryName"] = m_libraryName;
        values["threshold"] = m_globalFloor;
        values["stableFrames"] = m_stableFramesRequired;
        values["enabled"] = m_enabled;
        values["maxFps"] = m_maxFps;
        values["drawOverlay"] = m_drawOverlay;
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        // 加载期不通知端口动态增删（模式同 Curve1D：图形节点创建后读到的已是新端口数）
        m_inLoad = true;
        const QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            const QJsonObject values = v.toObject();
            if (values.contains("threshold")) {
                setThreshold(values["threshold"].toDouble(0.0));
            }
            if (values.contains("stableFrames")) {
                setStableFrames(values["stableFrames"].toDouble(3));
            }
            if (values.contains("maxFps")) {
                setMaxFps(values["maxFps"].toDouble(15.0));
            }
            if (values.contains("drawOverlay")) {
                setDrawOverlay(values["drawOverlay"].toBool(true));
            }
            if (values.contains("libraryName")) {
                setLibraryName(values["libraryName"].toString());
            }
            if (values.contains("enabled")) {
                setEnabled(values["enabled"].toBool(false));
            }
        }
        m_inLoad = false;
    }

private:
    // ---------- 动作库 ----------

    /**
     * @brief 媒体库文件名 → 全路径
     *        优先在媒体库 allFiles() 中按文件名反查；兜底拼接媒体库存储目录
     */
    QString resolveLibraryPath() const
    {
        if (m_libraryName.isEmpty()) {
            return {};
        }
        const QStringList files = MediaLibrary::instance()->allFiles();
        for (const QString& p : files) {
            if (QFileInfo(p).fileName().compare(m_libraryName, Qt::CaseInsensitive) == 0) {
                return p;
            }
        }
        const QString direct =
            AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QLatin1Char('/') + m_libraryName;
        if (QFileInfo::exists(direct)) {
            return direct;
        }
        return {};
    }

    void reloadLibrary(bool force)
    {
        const QString path = resolveLibraryPath();
        if (path.isEmpty()) {
            if (force && !m_libraryName.isEmpty()) {
                widget->statusLabel->setText(
                    QStringLiteral("媒体库中未找到动作库: %1").arg(m_libraryName));
            }
            return;
        }
        const QFileInfo info(path);
        if (!force && m_libraryPath == path) {
            const QDateTime modified = info.lastModified();
            if (modified == m_libraryModified) {
                return;
            }
        }
        GestureLibrary::Library lib;
        QString err;
        if (!GestureLibrary::load(path, lib, &err)) {
            widget->statusLabel->setText(err);
            return;
        }
        {
            QMutexLocker locker(&m_libraryMutex);
            m_library = std::make_shared<const GestureLibrary::Library>(std::move(lib));
        }
        m_libraryPath = path;
        m_libraryModified = info.lastModified();
        QStringList labels;
        QStringList plainLabels;
        int sampleTotal = 0;
        {
            QMutexLocker locker(&m_libraryMutex);
            for (const auto& g : m_library->gestures) {
                labels << QStringLiteral("%1（%2 帧，阈值 %3）")
                              .arg(g.label)
                              .arg(g.samples.size())
                              .arg(g.threshold, 0, 'f', 2);
                plainLabels << g.label;
                sampleTotal += static_cast<int>(g.samples.size());
            }
        }
        widget->gestureList->clear();
        widget->gestureList->addItems(labels);
        // 依据动作数重建动态输出端口（端口 2..N+1，标签=手势名）
        m_gestureLabels = plainLabels;
        applyGestureCount(plainLabels.size(), !m_inLoad);
        resetGestureOutputs();
        QString versionNote;
        {
            QMutexLocker locker(&m_libraryMutex);
            if (m_library->featureVersion != GestureFeature::kFeatureVersion) {
                versionNote = QStringLiteral("（⚠️ 旧版特征库 v%1，识别精度不可信，请用 GestureRecorder 重新录制）")
                                  .arg(m_library->featureVersion);
            }
        }
        widget->statusLabel->setText(
            QStringLiteral("已加载 %1 个标签 / %2 帧样本%3")
                .arg(labels.size())
                .arg(sampleTotal)
                .arg(versionNote));
    }

    /**
     * @brief 调整手势动态输出端口数（OutPortCount = 手势数 + 2：IMAGE / RESULT）。
     *        notifyPorts=true 时通过 portsAboutToBeInserted/Deleted 系列信号通知节点图
     *        动态增删端口（模式同 AudioCrossFaderDataModel::applyPortCount）；
     *        加载期（m_inLoad）传 false，图形节点创建后读到的已是新端口数（同 Curve1D）。
     */
    void applyGestureCount(int gestureCount, bool notifyPorts)
    {
        const unsigned int newCount = static_cast<unsigned int>(qMax(0, gestureCount)) + 2;
        const unsigned int oldCount = OutPortCount;
        if (newCount == oldCount) {
            return;
        }
        if (notifyPorts) {
            if (newCount > oldCount) {
                Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, newCount - 1);
                OutPortCount = newCount;
                Q_EMIT portsInserted();
            } else {
                Q_EMIT portsAboutToBeDeleted(PortType::Out, newCount, oldCount - 1);
                OutPortCount = newCount;
                Q_EMIT portsDeleted();
            }
        } else {
            OutPortCount = newCount;
        }
    }

    /** 全部手势动态端口清零（false 电平）；加载期不向图推送 */
    void resetGestureOutputs()
    {
        m_gestureOut.clear();
        for (int i = 0; i < m_gestureLabels.size(); ++i) {
            m_gestureOut.append(std::make_shared<VariableData>(QVariant(false)));
        }
        if (!m_inLoad) {
            for (int i = 0; i < m_gestureOut.size(); ++i) {
                Q_EMIT dataUpdated(static_cast<PortIndex>(i + 2));
            }
        }
    }

    /** 依据本次匹配结果刷新各手势动态端口的 confirmed 电平（仅变化端口推送） */
    void syncGestureOutputs(const GestureMatch& match)
    {
        for (int i = 0; i < m_gestureLabels.size(); ++i) {
            const bool on = match.present && m_lastConfirmed
                            && match.label == m_gestureLabels.at(i);
            const bool prev = m_gestureOut.at(i) ? m_gestureOut.at(i)->asBool() : false;
            if (prev != on) {
                m_gestureOut[i] = std::make_shared<VariableData>(QVariant(on));
                Q_EMIT dataUpdated(static_cast<PortIndex>(i + 2));
            }
        }
    }

    // ---------- 推理调度（与 HandPose 节点同构） ----------
    void tryScheduleInference()
    {
        if (!m_enabled || !m_inImage0 || imageDataIsEmpty(m_inImage0)) {
            return;
        }

        // 库文件热重载检查（≤1Hz）
        if (m_reloadCheckTimer.elapsed() >= 1000) {
            m_reloadCheckTimer.start();
            reloadLibrary(false);
        }

        if (!m_inferenceWatcher) {
            m_inferenceWatcher = new QFutureWatcher<void>(this);
            connect(m_inferenceWatcher, &QFutureWatcher<void>::finished, this, [this]() {
                tryScheduleInference();
            });
        }

        if (m_inferenceWatcher->isRunning()) {
            return;
        }

        qint64 latestTs = -1;
        if (!peekLatestInputTimestamp(m_inImage0, latestTs)) {
            return;
        }
        if (latestTs >= 0 && latestTs <= m_lastSeenInputTimestamp) {
            return;
        }

        const qint64 intervalMs = static_cast<qint64>(
            std::lround(1000.0 / std::max(1.0, m_maxFps)));
        if (m_inferenceTimer.isValid() && m_inferenceTimer.elapsed() < intervalMs) {
            return;
        }

        cv::Mat frame = readInputMatForInference(m_inImage0);
        if (frame.empty()) {
            return;
        }

        std::shared_ptr<const GestureLibrary::Library> librarySnapshot;
        {
            QMutexLocker locker(&m_libraryMutex);
            librarySnapshot = m_library;
        }

        m_lastSeenInputTimestamp = latestTs;
        m_inferenceTimer.start();
        m_cancelRequested.store(false);
        const double floorValue = m_globalFloor;
        const bool draw = m_drawOverlay;
        const bool hasLibrary = librarySnapshot && !librarySnapshot->gestures.isEmpty();
        if (!hasLibrary && !m_libraryWarned) {
            m_libraryWarned = true;
            widget->statusLabel->setText(
                QStringLiteral("动作库为空：请先把 GestureRecorder 录制的 .gesture 导入媒体库 Gestures 分类后选择"));
        }
        if (!hasLibrary) {
            return;
        }
        const QString palmPath = m_palmPath;
        const QString handPath = m_handPath;

        auto future = QtConcurrent::run([this, frame = std::move(frame), librarySnapshot,
                                         floorValue, draw, palmPath, handPath]() mutable {
            if (m_cancelRequested.load()) {
                return;
            }
            runInferenceOnImage(std::move(frame), *librarySnapshot,
                                static_cast<float>(floorValue), draw, palmPath, handPath);
        });
        m_inferenceWatcher->setFuture(future);
    }

    bool ensureModelsUnlocked(const QString& palmPath, const QString& handPath)
    {
        if (m_modelsReady && m_cachedPalmPath == palmPath && m_cachedHandPath == handPath) {
            return m_palmDetector && m_palmDetector->isLoaded()
                && m_handEstimator && m_handEstimator->isLoaded();
        }
        if (!QFileInfo::exists(palmPath) || !QFileInfo::exists(handPath)) {
            qDebug() << "GestureNode: MediaPipe hand model not found";
            return false;
        }
        // 模板匹配本身极轻，统一走 CPU，避免对 CUDA 的运行时依赖
        m_palmDetector = std::make_unique<MediaPipeHand::PalmDetector>(
            palmPath.toStdString(), 0.3f, 0.3f, 5000, false);
        m_handEstimator = std::make_unique<MediaPipeHand::HandPoseEstimator>(
            handPath.toStdString(), 0.3f, false);
        if (!m_palmDetector->isLoaded() || !m_handEstimator->isLoaded()) {
            qDebug() << "GestureNode: MediaPipe hand model load failed";
            m_palmDetector.reset();
            m_handEstimator.reset();
            return false;
        }
        m_cachedPalmPath = palmPath;
        m_cachedHandPath = handPath;
        m_modelsReady = true;
        return true;
    }

    /** 工作线程：手部关键点 → 特征 → 库匹配 → 叠画 */
    void runInferenceOnImage(cv::Mat inputImage,
                             const GestureLibrary::Library& library,
                             float globalFloor,
                             bool drawOverlay,
                             const QString& palmPath,
                             const QString& handPath)
    {
        if (inputImage.empty() || m_cancelRequested.load()) {
            return;
        }
        try {
            std::vector<MediaPipeHand::HandResult> hands;
            {
                QMutexLocker locker(&m_modelsMutex);
                if (!ensureModelsUnlocked(palmPath, handPath) || m_cancelRequested.load()) {
                    return;
                }
                hands = MediaPipeHand::detectHands(*m_palmDetector, *m_handEstimator, inputImage);
            }

            // 取置信度最高的一只手做匹配（第一版单手语义）
            const MediaPipeHand::HandResult* best = nullptr;
            for (const auto& hand : hands) {
                if (!hand.valid) {
                    continue;
                }
                if (!best || hand.confidence > best->confidence) {
                    best = &hand;
                }
            }

            GestureMatch match;
            std::array<float, GestureFeature::kDim> feature;
            if (best && GestureFeature::extract(best->worldLandmarks, best->handedness, feature)) {
                match = matchFeature(library, feature, globalFloor);
            } else {
                match.label.clear();
                match.present = false;
            }

            cv::Mat resultImage;
            if (drawOverlay) {
                resultImage = inputImage.clone();
                for (const auto& hand : hands) {
                    MediaPipeHand::drawHandOverlay(resultImage, hand);
                }
                drawLabelOverlay(resultImage, match);
            } else {
                resultImage = std::move(inputImage);
            }

            QMetaObject::invokeMethod(
                this,
                [this, resultImage = std::move(resultImage), match, drawOverlay]() mutable {
                    updateMatchState(match);
                    syncGestureOutputs(match);
                    QVariantMap finalResult;
                    finalResult["gesture"] = match.present ? match.label : QString();
                    finalResult["confidence"] = static_cast<double>(match.score);
                    finalResult["present"] = match.present;
                    finalResult["threshold"] = static_cast<double>(match.threshold);
                    finalResult["confirmed"] = m_lastConfirmed;
                    m_outVariable = std::make_shared<VariableData>(finalResult);
                    Q_EMIT dataUpdated(1);
                    if (drawOverlay) {
                        pushMatToRingBuffer(
                            m_outImage, m_outImageBuffer, m_lastPushedTimestamp,
                            std::move(resultImage));
                    }
                },
                Qt::QueuedConnection);
        } catch (const std::exception& e) {
            qDebug() << "GestureNode inference error:" << e.what();
        }
    }

    void updateMatchState(const GestureMatch& match)
    {
        if (match.present && match.label == m_stableLabel) {
            ++m_pendingStable;
        } else {
            m_stableLabel = match.label;
            m_pendingStable = match.present ? 1 : 0;
        }
        m_lastConfirmed = match.present && m_pendingStable >= m_stableFramesRequired;
    }

    void cancelPendingInference()
    {
        m_cancelRequested.store(true);
        if (m_inferenceWatcher && m_inferenceWatcher->isRunning()) {
            m_inferenceWatcher->waitForFinished();
        }
    }

private:
    QFutureWatcher<void>* m_inferenceWatcher = nullptr;

    GestureInterface* widget = new GestureInterface();
    std::shared_ptr<ImageData> m_inImage0;
    std::shared_ptr<VariableData> m_outVariable;
    std::shared_ptr<ImageData> m_outImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outImageBuffer;
    qint64 m_lastPushedTimestamp = -1;
    qint64 m_lastSeenInputTimestamp = -1;
    QString m_palmPath;
    QString m_handPath;

    QString m_libraryPath;   // 解析出的全路径（媒体库反查结果）
    QString m_libraryName;   // 媒体库 Gestures 分类中的文件名
    QDateTime m_libraryModified;
    QStringList m_gestureLabels;                          // 当前库手势名（=动态端口标签）
    QVector<std::shared_ptr<VariableData>> m_gestureOut;  // 每手势 confirmed 电平输出
    bool m_inLoad = false;   // load() 恢复期：动态端口不通知图、输出不推送
    QMutex m_libraryMutex;
    std::shared_ptr<const GestureLibrary::Library> m_library;
    bool m_libraryWarned = false;

    double m_globalFloor = 0.0;
    int m_stableFramesRequired = 3;
    bool m_enabled = false;
    double m_maxFps = 15.0;
    bool m_drawOverlay = true;

    QString m_stableLabel;
    int m_pendingStable = 0;
    bool m_lastConfirmed = false;

    QMutex m_modelsMutex;
    QElapsedTimer m_inferenceTimer;
    QElapsedTimer m_reloadCheckTimer;
    std::atomic<bool> m_cancelRequested{false};

    bool m_modelsReady = false;
    QString m_cachedPalmPath;
    QString m_cachedHandPath;
    std::unique_ptr<MediaPipeHand::PalmDetector> m_palmDetector;
    std::unique_ptr<MediaPipeHand::HandPoseEstimator> m_handEstimator;
};
} // namespace Nodes
