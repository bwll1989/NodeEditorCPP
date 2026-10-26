#pragma once

#include "ObjectTrackInterface.hpp"
#include "PluginDefinition.hpp"

#include "Common/AppConfig/ConstantDefines.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageReadback.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <onnxruntime_cxx_api.h>
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QIcon>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QRecursiveMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QSignalBlocker>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace
{
inline cv::Mat scaleMatForInference(const cv::Mat& src)
{
    if (src.empty()) {
        return {};
    }
    constexpr int kMaxSide = 1280;
    const int maxDim = std::max(src.cols, src.rows);
    if (maxDim <= kMaxSide) {
        return src.clone();
    }
    const double scale = static_cast<double>(kMaxSide) / static_cast<double>(maxDim);
    cv::Mat scaled;
    cv::resize(src, scaled, cv::Size(), scale, scale, cv::INTER_AREA);
    return scaled;
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

inline cv::Mat readInputMatForInference(const std::shared_ptr<ImageData>& input)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return {};
    }
    return scaleMatForInference(ImageReadback::matFromFrame(frame));
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
    pushFrameToImageBufferDedup(buffer, ImageFrame::fromMat(std::move(mat), ts), lastPushed);
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

inline cv::Mat readImageFile(const QString& path, int flags = cv::IMREAD_UNCHANGED)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray bytes = file.readAll();
    if (bytes.isEmpty()) {
        return {};
    }
    std::vector<uchar> buffer(bytes.begin(), bytes.end());
    try {
        return cv::imdecode(buffer, flags);
    } catch (const std::exception&) {
        return {};
    }
}

inline float scoreToUnit(float value)
{
    if (!std::isfinite(value)) {
        return 0.f;
    }
    if (value >= 0.f && value <= 1.f) {
        return value;
    }
    return 1.f / (1.f + std::exp(-value));
}

inline float rectIou(const cv::Rect2f& a, const cv::Rect2f& b)
{
    const float x1 = std::max(a.x, b.x);
    const float y1 = std::max(a.y, b.y);
    const float x2 = std::min(a.x + a.width, b.x + b.width);
    const float y2 = std::min(a.y + a.height, b.y + b.height);
    const float w = x2 - x1;
    const float h = y2 - y1;
    if (w <= 0.f || h <= 0.f) {
        return 0.f;
    }
    const float inter = w * h;
    const float uni = a.area() + b.area() - inter;
    return uni <= 1e-6f ? 0.f : inter / uni;
}

inline cv::Mat asBgr(const cv::Mat& src)
{
    if (src.empty()) {
        return {};
    }
    if (src.type() == CV_8UC3) {
        return src;
    }
    cv::Mat bgr;
    if (src.type() == CV_8UC4) {
        cv::cvtColor(src, bgr, cv::COLOR_BGRA2BGR);
    } else if (src.type() == CV_8UC1) {
        cv::cvtColor(src, bgr, cv::COLOR_GRAY2BGR);
    }
    return bgr;
}

inline QImage imageToQImage(const cv::Mat& src)
{
    cv::Mat rgba;
    if (src.type() == CV_8UC4) {
        cv::cvtColor(src, rgba, cv::COLOR_BGRA2RGBA);
    } else if (src.type() == CV_8UC3) {
        cv::cvtColor(src, rgba, cv::COLOR_BGR2RGBA);
    } else if (src.type() == CV_8UC1) {
        cv::cvtColor(src, rgba, cv::COLOR_GRAY2RGBA);
    } else {
        return {};
    }
    return QImage(rgba.data, rgba.cols, rgba.rows, static_cast<int>(rgba.step), QImage::Format_RGBA8888).copy();
}

struct LetterboxedInput
{
    cv::Mat imageBlob;
    std::vector<float> mask;
    float scale = 1.f;
    float padX = 0.f;
    float padY = 0.f;
    int size = 640;
};

inline LetterboxedInput makeLetterbox(const cv::Mat& src, int size)
{
    LetterboxedInput out;
    out.size = size;
    if (size <= 0) {
        return out;
    }
    out.mask.assign(static_cast<size_t>(size) * static_cast<size_t>(size), 0.f);
    const cv::Mat bgr = asBgr(src);
    if (bgr.empty()) {
        return out;
    }
    const float scale = std::min(static_cast<float>(size) / static_cast<float>(bgr.cols),
                                 static_cast<float>(size) / static_cast<float>(bgr.rows));
    int resizedW = std::clamp(static_cast<int>(std::lround(bgr.cols * scale)), 1, size);
    int resizedH = std::clamp(static_cast<int>(std::lround(bgr.rows * scale)), 1, size);
    const int padX = std::max(0, (size - resizedW) / 2);
    const int padY = std::max(0, (size - resizedH) / 2);
    resizedW = std::min(resizedW, size - padX);
    resizedH = std::min(resizedH, size - padY);
    cv::Mat resized;
    cv::resize(bgr, resized, cv::Size(resizedW, resizedH));
    cv::Mat canvas(size, size, CV_8UC3, cv::Scalar(114, 114, 114));
    resized.copyTo(canvas(cv::Rect(padX, padY, resizedW, resizedH)));
    out.imageBlob = cv::dnn::blobFromImage(canvas, 1.0 / 255.0, cv::Size(), cv::Scalar(), true, false, CV_32F);
    out.scale = scale;
    out.padX = static_cast<float>(padX);
    out.padY = static_cast<float>(padY);
    return out;
}
} // namespace

namespace Nodes
{
class ObjectTrackDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double confidence READ confidence WRITE setConfidence NOTIFY confidenceChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(double maxFps READ maxFps WRITE setMaxFps NOTIFY maxFpsChanged)
    Q_PROPERTY(bool drawOverlay READ drawOverlay WRITE setDrawOverlay NOTIFY drawOverlayChanged)
    Q_PROPERTY(double holdFrames READ holdFrames WRITE setHoldFrames NOTIFY holdFramesChanged)

public:
    ObjectTrackDataModel()
    {
        {
            QMutexLocker locker(&SharedOrt::instance().mutex);
            ++SharedOrt::instance().users;
        }
        InPortCount = 2;
        OutPortCount = 2;
        CaptionVisible = true;
        Caption = QStringLiteral(PLUGIN_NAME);
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;
        m_outVariable = std::make_shared<VariableData>();
        ensureImageDataBuffer(m_outImage, m_outImageBuffer);
        m_widget = new ObjectTrackInterface();

        NodeDelegateModel::ExternalBinding confidenceBinding;
        confidenceBinding.member = "confidence";
        confidenceBinding.control = m_widget->confidence;
        AbstractDelegateModel::registerExternalBinding("/confidence", this, confidenceBinding);

        NodeDelegateModel::ExternalBinding enableBinding;
        enableBinding.member = "enabled";
        enableBinding.control = m_widget->enableBtn;
        AbstractDelegateModel::registerExternalBinding("/enable", this, enableBinding);

        connect(m_widget->confidence, &FloatDragValueWidget::valueChanged, this, &ObjectTrackDataModel::setConfidence);
        connect(m_widget->maxFps, &FloatDragValueWidget::valueChanged, this, &ObjectTrackDataModel::setMaxFps);
        connect(m_widget->holdFrames, &FloatDragValueWidget::valueChanged, this, &ObjectTrackDataModel::setHoldFrames);
        connect(m_widget->drawOverlay, &QCheckBox::toggled, this, &ObjectTrackDataModel::setDrawOverlay);
        connect(m_widget->enableBtn, &QPushButton::clicked, this, [this](bool checked) { setEnabled(checked); });
        connect(m_widget->removeBtn, &QPushButton::clicked, this, &ObjectTrackDataModel::removeSelectedTarget);
        connect(m_widget->regionView, &TrackPickView::pointClicked, this, &ObjectTrackDataModel::lockPoint);
    }

    ~ObjectTrackDataModel() override
    {
        m_cancelRequested.store(true);
        {
            QMutexLocker locker(&m_galleryMutex);
            m_hasPending = false;
            m_pendingFrame.release();
        }
        if (m_inferenceThread) {
            disconnect(m_inferenceThread, nullptr, this, nullptr);
            if (m_inferenceThread->isRunning()) {
                m_inferenceThread->wait();
            }
            delete m_inferenceThread;
            m_inferenceThread = nullptr;
        }
        disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
        GlobalEventBus::instance()->unsubscribe(this);
        {
            QMutexLocker locker(&SharedOrt::instance().mutex);
            auto& shared = SharedOrt::instance();
            if (--shared.users <= 0) {
                shared.users = 0;
                releaseBundle(shared.sam);
                releaseBundle(shared.prompt);
                releaseBundle(shared.detect);
                shared.memory.reset();
                shared.options.reset();
                shared.env.reset();
                shared.samReady = false;
                shared.detectReady = false;
                shared.useCuda = false;
            }
        }
    }

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/confidence"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64) {
                    refreshPreview();
                    tryScheduleInference();
                },
                Qt::QueuedConnection);
    }

    double confidence() const { return m_confThreshold; }
    void setConfidence(double value)
    {
        value = std::clamp(value, 0.0, 1.0);
        if (std::abs(m_confThreshold - value) < 1e-6) {
            return;
        }
        m_confThreshold = value;
        if (m_widget && std::abs(m_widget->confidence->value() - value) >= 1e-6) {
            QSignalBlocker blocker(m_widget->confidence);
            m_widget->confidence->setValue(value);
        }
        emit confidenceChanged(value);
        requestInferenceRefresh();
    }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool value)
    {
        if (m_enabled == value) {
            return;
        }
        if (value) {
            bool hasSamples = false;
            bool hasPending = false;
            {
                QMutexLocker locker(&m_galleryMutex);
                hasSamples = !m_gallery.empty();
                hasPending = m_hasPending;
            }
            const bool segmenting = (m_inferenceThread && m_inferenceThread->isRunning()) || hasPending;
            if (segmenting || !hasSamples) {
                if (m_widget && m_widget->enableBtn->isChecked()) {
                    QSignalBlocker blocker(m_widget->enableBtn);
                    m_widget->enableBtn->setChecked(false);
                }
                setStatusText(segmenting ? QStringLiteral("正在分割，请稍候再开始检测")
                                         : QStringLiteral("请先选择对象"));
                return;
            }
        }
        m_enabled = value;
        if (m_widget && m_widget->enableBtn->isChecked() != value) {
            QSignalBlocker blocker(m_widget->enableBtn);
            m_widget->enableBtn->setChecked(value);
        }
        if (m_widget) {
            m_widget->enableBtn->setText(value ? QStringLiteral("停止检测") : QStringLiteral("开始检测"));
        }
        emit enabledChanged(value);
        if (!m_enabled) {
            {
                QMutexLocker locker(&m_galleryMutex);
                m_hasPending = false;
                m_pendingFrame.release();
            }
            cancelPendingInference();
            if (m_widget && m_widget->regionView) {
                m_widget->regionView->setFrozen(false);
            }
            updatePickControls();
            m_tracks.clear();
            clearRingBufferOutput(m_outImage, m_outImageBuffer, m_lastPushedTimestamp);
            m_outVariable = std::make_shared<VariableData>();
            Q_EMIT dataUpdated(1);
            return;
        }
        updatePickControls();
        m_modelErrorLogged.store(false);
        requestInferenceRefresh();
    }

    double maxFps() const { return m_maxFps; }
    void setMaxFps(double value)
    {
        value = std::clamp(value, 1.0, 30.0);
        if (std::abs(m_maxFps - value) < 1e-6) {
            return;
        }
        m_maxFps = value;
        if (m_widget && std::abs(m_widget->maxFps->value() - value) >= 1e-6) {
            QSignalBlocker blocker(m_widget->maxFps);
            m_widget->maxFps->setValue(value);
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
        if (m_widget && m_widget->drawOverlay->isChecked() != value) {
            QSignalBlocker blocker(m_widget->drawOverlay);
            m_widget->drawOverlay->setChecked(value);
        }
        emit drawOverlayChanged(value);
    }

    double holdFrames() const { return m_holdFrames; }
    void setHoldFrames(double value)
    {
        const int frames = std::clamp(static_cast<int>(std::lround(value)), 0, 60);
        if (m_holdFrames == frames) {
            return;
        }
        m_holdFrames = frames;
        if (m_widget && std::abs(m_widget->holdFrames->value() - frames) >= 1e-6) {
            QSignalBlocker blocker(m_widget->holdFrames);
            m_widget->holdFrames->setValue(frames);
        }
        emit holdFramesChanged(frames);
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            return portIndex == 0 ? QStringLiteral("IMAGE") : QStringLiteral("ENABLE");
        }
        if (portType == PortType::Out) {
            return portIndex == 0 ? QStringLiteral("IMAGE 0") : QStringLiteral("RESULT");
        }
        return {};
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return portIndex == 0 ? ImageData().type() : VariableData().type();
        }
        if (portType == PortType::In) {
            return portIndex == 0 ? ImageData().type() : VariableData().type();
        }
        return ImageData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        return port == 0 ? std::static_pointer_cast<NodeData>(m_outImage)
                         : std::static_pointer_cast<NodeData>(m_outVariable);
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override
    {
        if (!data) {
            if (portIndex == 0) {
                m_inImage.reset();
                cancelPendingInference();
                clearRingBufferOutput(m_outImage, m_outImageBuffer, m_lastPushedTimestamp);
            }
            return;
        }
        if (portIndex == 0) {
            m_inImage = std::dynamic_pointer_cast<ImageData>(data);
            m_lastSeenInputTimestamp = -1;
            if (!m_inImage) {
                cancelPendingInference();
                clearRingBufferOutput(m_outImage, m_outImageBuffer, m_lastPushedTimestamp);
                return;
            }
            refreshPreview();
            tryScheduleInference();
            return;
        }
        if (portIndex == 1) {
            auto enableData = std::dynamic_pointer_cast<VariableData>(data);
            if (enableData) {
                setEnabled(enableData->asBool());
            }
        }
    }

    QWidget* embeddedWidget() override { return m_widget; }

    QJsonObject save() const override
    {
        QJsonObject values;
        values.insert(QStringLiteral("confidence"), m_confThreshold);
        values.insert(QStringLiteral("enabled"), m_enabled);
        values.insert(QStringLiteral("maxFps"), m_maxFps);
        values.insert(QStringLiteral("drawOverlay"), m_drawOverlay);
        values.insert(QStringLiteral("holdFrames"), m_holdFrames);
        QJsonArray targets;
        {
            QMutexLocker locker(&m_galleryMutex);
            for (const TrackTarget& target : m_gallery) {
                QJsonObject item;
                item.insert(QStringLiteral("id"), target.id);
                QJsonArray images;
                for (const ObjectSample& sample : target.samples) {
                    if (!sample.file.isEmpty()) {
                        images.append(relativeObjectRef(sample.file));
                    }
                }
                item.insert(QStringLiteral("images"), images);
                targets.append(item);
            }
        }
        values.insert(QStringLiteral("targets"), targets);
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson.insert(QStringLiteral("values"), values);
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        cancelPendingInference();
        const QJsonValue stored = p.value(QStringLiteral("values"));
        if (!stored.isObject()) {
            return;
        }
        const QJsonObject values = stored.toObject();
        if (values.contains(QStringLiteral("targets"))) {
            std::vector<TrackTarget> loaded;
            const QJsonArray targets = values.value(QStringLiteral("targets")).toArray();
            for (const QJsonValue& entry : targets) {
                if (!entry.isObject()) {
                    continue;
                }
                const QJsonObject item = entry.toObject();
                TrackTarget target;
                target.id = item.value(QStringLiteral("id")).toString().trimmed();
                if (target.id.isEmpty()) {
                    continue;
                }
                if (item.contains(QStringLiteral("images"))) {
                    const QJsonArray images = item.value(QStringLiteral("images")).toArray();
                    for (const QJsonValue& imageValue : images) {
                        const QString stored = imageValue.toString();
                        const QString absolute = resolveObjectRef(stored);
                        cv::Mat image = readImageFile(absolute, cv::IMREAD_UNCHANGED);
                        if (image.empty()) {
                            continue;
                        }
                        QString relative = relativeObjectRef(stored);
                        if (QFileInfo(stored).isAbsolute()) {
                            const QString fileName = QFileInfo(relative).fileName();
                            const QString dest = QDir(AppConstants::OBJECT_TRACK_REFS_DIR).filePath(fileName);
                            if (QDir::cleanPath(absolute) != QDir::cleanPath(dest) && QFileInfo::exists(absolute)) {
                                QDir().mkpath(AppConstants::OBJECT_TRACK_REFS_DIR);
                                if (!QFileInfo::exists(dest)) {
                                    QFile::copy(absolute, dest);
                                }
                            }
                            relative = fileName;
                        }
                        target.samples.push_back(ObjectSample{std::move(image), relative});
                    }
                } else if (item.contains(QStringLiteral("regions"))) {
                    const QJsonArray regions = item.value(QStringLiteral("regions")).toArray();
                    for (const QJsonValue& regionValue : regions) {
                        if (!regionValue.isObject()) {
                            continue;
                        }
                        const QJsonObject region = regionValue.toObject();
                        cv::Mat image = legacyObjectImage(region.value(QStringLiteral("file")).toString(),
                                                          region.value(QStringLiteral("mask")).toString());
                        if (image.empty()) {
                            continue;
                        }
                        const QString file = saveObjectPng(image, target.id);
                        if (file.isEmpty()) {
                            continue;
                        }
                        target.samples.push_back(ObjectSample{std::move(image), file});
                    }
                }
                if (!target.samples.empty()) {
                    loaded.push_back(std::move(target));
                }
            }
            {
                QMutexLocker locker(&m_galleryMutex);
                m_gallery = std::move(loaded);
                m_hasPending = false;
                m_pendingFrame.release();
            }
            m_galleryGen.fetch_add(1);
            m_encodedGen = -1;
            refreshTargetList();
        }
        if (values.contains(QStringLiteral("confidence"))) {
            setConfidence(values.value(QStringLiteral("confidence")).toDouble(0.25));
        }
        if (values.contains(QStringLiteral("maxFps"))) {
            setMaxFps(values.value(QStringLiteral("maxFps")).toDouble(15.0));
        }
        if (values.contains(QStringLiteral("drawOverlay"))) {
            setDrawOverlay(values.value(QStringLiteral("drawOverlay")).toBool(true));
        }
        if (values.contains(QStringLiteral("holdFrames"))) {
            setHoldFrames(values.value(QStringLiteral("holdFrames")).toDouble(8));
        }
        if (values.contains(QStringLiteral("enabled"))) {
            setEnabled(values.value(QStringLiteral("enabled")).toBool(false));
        }
    }

Q_SIGNALS:
    void confidenceChanged(double value);
    void enabledChanged(bool value);
    void maxFpsChanged(double value);
    void drawOverlayChanged(bool value);
    void holdFramesChanged(double value);

private Q_SLOTS:
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
        if (localPath == QStringLiteral("confidence")) {
            setConfidence(ev.payload.toDouble());
        } else if (localPath == QStringLiteral("enable")) {
            setEnabled(ev.payload.toBool());
        }
    }

    void lockPoint(QPoint point)
    {
        if (!m_widget || !m_widget->regionView) {
            return;
        }
        if (m_previewFrame.empty()) {
            setStatusText(QStringLiteral("还没有画面"));
            return;
        }
        if (point.x() < 0 || point.y() < 0 || point.x() >= m_previewFrame.cols || point.y() >= m_previewFrame.rows) {
            return;
        }
        if (m_enabled) {
            setStatusText(QStringLiteral("检测中无法选择对象，请先停止检测"));
            return;
        }
        QString id = m_widget->idEdit->text().trimmed();
        if (id.isEmpty()) {
            id = nextTargetId();
            m_widget->idEdit->setText(id);
        }
        {
            QMutexLocker locker(&m_galleryMutex);
            m_hasPending = true;
            m_pendingId = id;
            m_pendingFrame = m_previewFrame.clone();
            m_pendingClick = cv::Point(point.x(), point.y());
            m_pendingHold = ++m_previewHold;
        }
        m_widget->regionView->setFrozen(true);
        m_inferenceTimer.invalidate();
        setStatusText(QStringLiteral("正在分割 %1").arg(id));
        requestInferenceRefresh();
    }

    void removeSelectedTarget()
    {
        if (!m_widget || !m_widget->targetList) {
            return;
        }
        QListWidgetItem* item = m_widget->targetList->currentItem();
        if (!item) {
            return;
        }
        const QString id = item->data(Qt::UserRole).toString();
        {
            QMutexLocker locker(&m_galleryMutex);
            m_gallery.erase(std::remove_if(m_gallery.begin(),
                                           m_gallery.end(),
                                           [&id](const TrackTarget& target) { return target.id == id; }),
                            m_gallery.end());
        }
        m_galleryGen.fetch_add(1);
        refreshTargetList();
        setStatusText(QStringLiteral("已删除 %1").arg(id));
        requestInferenceRefresh();
    }

private:
    struct ObjectSample
    {
        cv::Mat image;
        QString file;
    };

    struct TrackTarget
    {
        QString id;
        std::vector<ObjectSample> samples;
    };

    struct EncodedTarget
    {
        QString id;
        std::vector<float> embedding;
    };

    struct TrackState
    {
        QString id;
        cv::Rect2f box;
        float score = 0.f;
        int missed = 0;
        float vx = 0.f;
        float vy = 0.f;
        bool lost = false;
    };

    struct LastPosition
    {
        float cx = 0.f;
        float cy = 0.f;
        float score = 0.f;
    };

    struct Detection
    {
        cv::Rect2f box;
        float score = 0.f;
        int classId = -1;
    };

    struct OrtBundle
    {
        std::unique_ptr<Ort::Session> session;
        std::vector<std::string> inputNameStorage;
        std::vector<std::string> outputNameStorage;
        std::vector<const char*> inputNames;
        std::vector<const char*> outputNames;
        std::vector<std::vector<int64_t>> inputShapes;
    };

    struct SharedOrt
    {
        QRecursiveMutex mutex;
        int users = 0;
        std::unique_ptr<Ort::Env> env;
        std::unique_ptr<Ort::SessionOptions> options;
        std::unique_ptr<Ort::MemoryInfo> memory;
        OrtBundle prompt;
        OrtBundle detect;
        OrtBundle sam;
        bool samReady = false;
        bool detectReady = false;
        bool useCuda = false;
        bool cudaUnavailable = false;
        int promptImageIndex = 0;
        int promptMaskIndex = -1;
        int detectImageIndex = 0;
        int detectEmbedIndex = -1;
        int inputSize = 640;
        int fixedSlots = -1;

        static SharedOrt& instance()
        {
            static SharedOrt shared;
            return shared;
        }
    };

    SharedOrt& ort() { return SharedOrt::instance(); }

    struct PendingPick
    {
        bool active = false;
        QString id;
        cv::Mat frame;
        cv::Point click;
        int hold = 0;
    };

    void setStatusText(const QString& text)
    {
        if (m_widget && m_widget->statusLabel && m_widget->statusLabel->text() != text) {
            m_widget->statusLabel->setText(text);
        }
    }

    void updatePickControls()
    {
        if (!m_widget) {
            return;
        }
        const bool picking = !m_enabled;
        if (m_widget->regionView) {
            m_widget->regionView->setPickingEnabled(picking);
        }
        if (m_widget->idEdit) {
            m_widget->idEdit->setEnabled(picking);
        }
        if (m_widget->removeBtn) {
            m_widget->removeBtn->setEnabled(picking);
        }
    }

    void postStatus(const QString& text)
    {
        if (m_workerStatus == text) {
            return;
        }
        m_workerStatus = text;
        QMetaObject::invokeMethod(this, [this, text]() { setStatusText(text); }, Qt::QueuedConnection);
    }

    void refreshTargetList()
    {
        if (!m_widget || !m_widget->targetList) {
            return;
        }
        const QString selected = m_widget->targetList->currentItem()
                                     ? m_widget->targetList->currentItem()->data(Qt::UserRole).toString()
                                     : QString();
        m_widget->targetList->clear();
        QMutexLocker locker(&m_galleryMutex);
        for (const TrackTarget& target : m_gallery) {
            QStringList parts;
            parts << target.id;
            if (!target.samples.empty()) {
                parts << QStringLiteral("取样次数 %1").arg(target.samples.size());
            }
            auto* item = new QListWidgetItem(parts.join(QStringLiteral("  ")), m_widget->targetList);
            item->setData(Qt::UserRole, target.id);
            if (!target.samples.empty()) {
                const QImage icon = imageToQImage(objectThumbnail(target.samples.back().image));
                if (!icon.isNull()) {
                    item->setIcon(QIcon(QPixmap::fromImage(
                        icon.scaled(72, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation))));
                }
                item->setToolTip(target.samples.back().file);
            }
            if (target.id == selected) {
                m_widget->targetList->setCurrentItem(item);
            }
        }
    }

    void refreshTargetListLater()
    {
        QMetaObject::invokeMethod(this, [this]() { refreshTargetList(); }, Qt::QueuedConnection);
    }

    QString nextTargetId() const
    {
        int next = 1;
        QMutexLocker locker(&m_galleryMutex);
        for (const TrackTarget& target : m_gallery) {
            bool numeric = false;
            const int value = target.id.toInt(&numeric);
            if (numeric) {
                next = std::max(next, value + 1);
            }
        }
        return QString::number(next);
    }

    static QString safeToken(QString id)
    {
        if (id.isEmpty()) {
            id = QStringLiteral("target");
        }
        for (QChar& ch : id) {
            if (!ch.isLetterOrNumber() && ch != QLatin1Char('_') && ch != QLatin1Char('-')) {
                ch = QLatin1Char('_');
            }
        }
        return id;
    }

    static QString relativeObjectRef(const QString& stored)
    {
        const QString trimmed = QDir::cleanPath(stored.trimmed());
        if (trimmed.isEmpty()) {
            return {};
        }
        if (!QFileInfo(trimmed).isAbsolute()) {
            return trimmed;
        }
        const QString root = QDir(AppConstants::OBJECT_TRACK_REFS_DIR).absolutePath();
        const QString relative = QDir(root).relativeFilePath(trimmed);
        if (!relative.startsWith(QLatin1String("..")) && !QFileInfo(relative).isAbsolute()) {
            return relative;
        }
        return QFileInfo(trimmed).fileName();
    }

    static QString resolveObjectRef(const QString& stored)
    {
        const QString trimmed = stored.trimmed();
        if (trimmed.isEmpty()) {
            return {};
        }
        const QString cleaned = QDir::cleanPath(trimmed);
        if (QFileInfo(cleaned).isAbsolute()) {
            if (QFileInfo::exists(cleaned)) {
                return cleaned;
            }
            const QString byName = QDir(AppConstants::OBJECT_TRACK_REFS_DIR).filePath(QFileInfo(cleaned).fileName());
            if (QFileInfo::exists(byName)) {
                return byName;
            }
            return cleaned;
        }
        return QDir(AppConstants::OBJECT_TRACK_REFS_DIR).filePath(cleaned);
    }

    QString saveObjectPng(const cv::Mat& image, const QString& id) const
    {
        if (image.empty()) {
            return {};
        }
        std::vector<uchar> encoded;
        try {
            if (!cv::imencode(".png", image, encoded) || encoded.empty()) {
                return {};
            }
        } catch (const std::exception& error) {
            qDebug() << "保存目标图失败:" << error.what();
            return {};
        }
        QDir dir(AppConstants::OBJECT_TRACK_REFS_DIR);
        if (!dir.mkpath(QStringLiteral("."))) {
            return {};
        }
        const QString path = dir.filePath(
            QStringLiteral("%1_%2.png").arg(safeToken(id)).arg(QDateTime::currentMSecsSinceEpoch()));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return {};
        }
        file.write(reinterpret_cast<const char*>(encoded.data()), static_cast<qint64>(encoded.size()));
        return QFileInfo(path).fileName();
    }

    static cv::Mat packObjectImage(const cv::Mat& frame, const cv::Mat& mask)
    {
        const cv::Mat bgr = asBgr(frame);
        if (bgr.empty() || mask.empty()) {
            return {};
        }
        cv::Mat gray;
        if (mask.type() == CV_8UC1) {
            gray = mask;
        } else if (mask.channels() == 4) {
            cv::cvtColor(mask, gray, cv::COLOR_BGRA2GRAY);
        } else if (mask.channels() == 3) {
            cv::cvtColor(mask, gray, cv::COLOR_BGR2GRAY);
        } else {
            return {};
        }
        if (gray.size() != bgr.size()) {
            cv::resize(gray, gray, bgr.size(), 0, 0, cv::INTER_NEAREST);
        }
        if (cv::countNonZero(gray) < 40) {
            return {};
        }
        cv::Mat bgra;
        cv::cvtColor(bgr, bgra, cv::COLOR_BGR2BGRA);
        for (int y = 0; y < bgra.rows; ++y) {
            auto* pixel = bgra.ptr<cv::Vec4b>(y);
            const uchar* alpha = gray.ptr<uchar>(y);
            for (int x = 0; x < bgra.cols; ++x) {
                pixel[x][3] = alpha[x] > 0 ? 255 : 0;
            }
        }
        return bgra;
    }

    static cv::Mat objectThumbnail(const cv::Mat& image)
    {
        if (image.empty() || image.type() != CV_8UC4) {
            return image;
        }
        std::vector<cv::Mat> channels;
        cv::split(image, channels);
        if (channels.size() != 4 || cv::countNonZero(channels[3]) < 40) {
            return image;
        }
        const cv::Rect bounds = cv::boundingRect(channels[3]);
        if (bounds.width < 2 || bounds.height < 2) {
            return image;
        }
        return image(bounds).clone();
    }

    static cv::Mat legacyObjectImage(const QString& framePath, const QString& maskPath)
    {
        if (framePath.isEmpty() || maskPath.isEmpty()) {
            return {};
        }
        const cv::Mat frame = readImageFile(framePath, cv::IMREAD_COLOR);
        const cv::Mat mask = readImageFile(maskPath, cv::IMREAD_GRAYSCALE);
        if (frame.empty() || mask.empty()) {
            return {};
        }
        return packObjectImage(frame, mask);
    }

    static void splitObjectImage(const cv::Mat& image, cv::Mat& bgr, cv::Mat& mask)
    {
        bgr.release();
        mask.release();
        if (image.empty()) {
            return;
        }
        if (image.type() == CV_8UC4) {
            std::vector<cv::Mat> channels;
            cv::split(image, channels);
            if (channels.size() != 4) {
                return;
            }
            cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, bgr);
            mask = channels[3];
            if (cv::countNonZero(mask) < 40) {
                mask = cv::Mat(bgr.size(), CV_8UC1, cv::Scalar(255));
            }
            return;
        }
        bgr = asBgr(image);
        if (!bgr.empty()) {
            mask = cv::Mat(bgr.size(), CV_8UC1, cv::Scalar(255));
        }
    }

    void refreshPreview()
    {
        if (!m_widget || !m_widget->regionView || !m_inImage || imageDataIsEmpty(m_inImage)) {
            return;
        }
        if (m_widget->regionView->isFrozen()) {
            return;
        }
        if (m_previewTimer.isValid() && m_previewTimer.elapsed() < 80) {
            return;
        }
        cv::Mat frame = readInputMatForInference(m_inImage);
        if (frame.empty()) {
            return;
        }
        m_previewTimer.start();
        m_previewFrame = frame;
        m_widget->regionView->setImage(imageToQImage(m_previewFrame));
    }

    void requestInferenceRefresh()
    {
        m_lastSeenInputTimestamp = -1;
        tryScheduleInference();
    }

    qint64 inferenceIntervalMs() const
    {
        return static_cast<qint64>(std::lround(1000.0 / std::max(1.0, m_maxFps)));
    }

    class InferenceThread : public QThread
    {
    public:
        InferenceThread(ObjectTrackDataModel* model, cv::Mat frame, double confidence, bool draw, int hold)
            : m_model(model)
            , m_frame(std::move(frame))
            , m_confidence(confidence)
            , m_draw(draw)
            , m_hold(hold)
        {
            setStackSize(8 * 1024 * 1024);
        }

    protected:
        void run() override
        {
            try {
                m_model->runTrack(std::move(m_frame), m_confidence, m_draw, m_hold);
            } catch (const std::exception& error) {
                qDebug() << "Object Track 推理线程异常:" << error.what();
            } catch (...) {
                qDebug() << "Object Track 推理线程异常";
            }
        }

    private:
        ObjectTrackDataModel* m_model = nullptr;
        cv::Mat m_frame;
        double m_confidence = 0.0;
        bool m_draw = false;
        int m_hold = 0;
    };

    void tryScheduleInference()
    {
        if (m_inferenceThread && m_inferenceThread->isRunning()) {
            return;
        }
        bool hasPick = false;
        bool hasSamples = false;
        {
            QMutexLocker locker(&m_galleryMutex);
            hasPick = m_hasPending;
            hasSamples = !m_gallery.empty();
        }
        if (hasPick && !m_enabled) {
            cv::Mat frame;
            {
                QMutexLocker locker(&m_galleryMutex);
                frame = m_pendingFrame.clone();
            }
            if (frame.empty()) {
                return;
            }
            startInference(std::move(frame));
            return;
        }
        if (!m_enabled || !hasSamples || !m_inImage || imageDataIsEmpty(m_inImage)) {
            return;
        }
        qint64 latestTs = -1;
        if (!peekLatestInputTimestamp(m_inImage, latestTs)) {
            return;
        }
        if (latestTs >= 0 && latestTs <= m_lastSeenInputTimestamp) {
            return;
        }
        if (m_inferenceTimer.isValid() && m_inferenceTimer.elapsed() < inferenceIntervalMs()) {
            return;
        }
        cv::Mat frame = readInputMatForInference(m_inImage);
        if (frame.empty()) {
            return;
        }
        m_lastSeenInputTimestamp = latestTs;
        startInference(std::move(frame));
    }

    void startInference(cv::Mat frame)
    {
        m_inferenceTimer.start();
        m_cancelRequested.store(false);
        auto* thread = new InferenceThread(this, std::move(frame), m_confThreshold, m_drawOverlay, m_holdFrames);
        m_inferenceThread = thread;
        connect(thread, &QThread::finished, this, [this, thread]() {
            if (m_inferenceThread == thread) {
                m_inferenceThread = nullptr;
            }
            thread->deleteLater();
            tryScheduleInference();
        });
        thread->start();
    }

    void cancelPendingInference()
    {
        m_cancelRequested.store(true);
        if (m_inferenceThread && m_inferenceThread->isRunning()) {
            m_inferenceThread->wait();
        }
    }

    PendingPick takePending()
    {
        QMutexLocker locker(&m_galleryMutex);
        PendingPick pick;
        pick.active = m_hasPending;
        if (!m_hasPending) {
            return pick;
        }
        pick.id = m_pendingId;
        pick.frame = std::move(m_pendingFrame);
        pick.click = m_pendingClick;
        pick.hold = m_pendingHold;
        m_hasPending = false;
        return pick;
    }

    bool commitSample(const QString& id, cv::Mat image, const QString& file)
    {
        if (id.isEmpty() || image.empty() || file.isEmpty()) {
            return false;
        }
        QMutexLocker locker(&m_galleryMutex);
        TrackTarget* target = nullptr;
        for (TrackTarget& existing : m_gallery) {
            if (existing.id == id) {
                target = &existing;
                break;
            }
        }
        if (!target) {
            m_gallery.push_back(TrackTarget{id, {}});
            target = &m_gallery.back();
        }
        target->samples.push_back(ObjectSample{std::move(image), file});
        m_galleryGen.fetch_add(1);
        return true;
    }

    void presentSegment(int hold, const cv::Mat& mask)
    {
        QImage overlay;
        if (!mask.empty() && !m_previewFrame.empty() && mask.rows == m_previewFrame.rows
            && mask.cols == m_previewFrame.cols && mask.type() == CV_8UC1) {
            overlay = QImage(mask.cols, mask.rows, QImage::Format_ARGB32);
            overlay.fill(Qt::transparent);
            for (int y = 0; y < mask.rows; ++y) {
                const uchar* src = mask.ptr<uchar>(y);
                auto* dst = reinterpret_cast<QRgb*>(overlay.scanLine(y));
                for (int x = 0; x < mask.cols; ++x) {
                    if (src[x] > 0) {
                        dst[x] = qRgba(0, 220, 120, 110);
                    }
                }
            }
        }
        QMetaObject::invokeMethod(this, [this, hold, overlay]() {
            if (hold != m_previewHold.load() || !m_widget || !m_widget->regionView) {
                return;
            }
            m_widget->regionView->setOverlay(overlay);
            QTimer::singleShot(1200, this, [this, hold]() {
                if (hold != m_previewHold.load() || !m_widget || !m_widget->regionView) {
                    return;
                }
                m_widget->regionView->setFrozen(false);
                m_widget->regionView->setOverlay(QImage());
            });
        }, Qt::QueuedConnection);
    }

    void releasePreview(int hold)
    {
        QMetaObject::invokeMethod(this, [this, hold]() {
            if (hold != m_previewHold.load() || !m_widget || !m_widget->regionView) {
                return;
            }
            m_widget->regionView->setFrozen(false);
        }, Qt::QueuedConnection);
    }

    void configureOrtSessionOptions(bool tryCuda)
    {
        const unsigned int cores = std::max(1u, std::thread::hardware_concurrency());
        ort().options = std::make_unique<Ort::SessionOptions>();
        ort().options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        ort().options->SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        ort().useCuda = false;
        if (tryCuda) {
            try {
                const auto providers = Ort::GetAvailableProviders();
                for (const auto& provider : providers) {
                    if (provider == "CUDAExecutionProvider") {
                        OrtCUDAProviderOptions cudaOptions{};
                        cudaOptions.device_id = 0;
                        cudaOptions.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchHeuristic;
                        cudaOptions.arena_extend_strategy = 0;
                        cudaOptions.do_copy_in_default_stream = 1;
                        cudaOptions.gpu_mem_limit = static_cast<size_t>(512) * 1024 * 1024;
                        ort().options->AppendExecutionProvider_CUDA(cudaOptions);
                        ort().useCuda = true;
                        break;
                    }
                }
            } catch (const std::exception& error) {
                qDebug() << "CUDA EP 注册失败，将使用 CPU:" << error.what();
            }
        }
        if (ort().useCuda) {
            ort().options->SetIntraOpNumThreads(1);
            ort().options->SetInterOpNumThreads(1);
        } else {
            const int cpuThreads = static_cast<int>(std::min(2u, std::max(1u, cores / 2)));
            ort().options->SetIntraOpNumThreads(cpuThreads);
            ort().options->SetInterOpNumThreads(1);
        }
    }

    static void releaseBundle(OrtBundle& bundle)
    {
        bundle.inputNames.clear();
        bundle.outputNames.clear();
        bundle.inputNameStorage.clear();
        bundle.outputNameStorage.clear();
        bundle.inputShapes.clear();
        bundle.session.reset();
    }

    bool fillBundle(const QString& path, OrtBundle& bundle, const Ort::SessionOptions* options = nullptr)
    {
        if (bundle.session) {
            return true;
        }
        if (!ort().env || (options == nullptr && !ort().options)) {
            return false;
        }
        const Ort::SessionOptions& used = options ? *options : *ort().options;
        try {
            bundle.session = std::make_unique<Ort::Session>(*ort().env, path.toStdWString().c_str(), used);
            Ort::AllocatorWithDefaultOptions allocator;
            const size_t inputCount = bundle.session->GetInputCount();
            bundle.inputNameStorage.reserve(inputCount);
            bundle.inputShapes.reserve(inputCount);
            for (size_t i = 0; i < inputCount; ++i) {
                auto name = bundle.session->GetInputNameAllocated(i, allocator);
                bundle.inputNameStorage.emplace_back(name.get());
                bundle.inputShapes.push_back(bundle.session->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
            }
            const size_t outputCount = bundle.session->GetOutputCount();
            bundle.outputNameStorage.reserve(outputCount);
            for (size_t i = 0; i < outputCount; ++i) {
                auto name = bundle.session->GetOutputNameAllocated(i, allocator);
                bundle.outputNameStorage.emplace_back(name.get());
            }
            bundle.inputNames.reserve(bundle.inputNameStorage.size());
            for (const std::string& name : bundle.inputNameStorage) {
                bundle.inputNames.push_back(name.c_str());
            }
            bundle.outputNames.reserve(bundle.outputNameStorage.size());
            for (const std::string& name : bundle.outputNameStorage) {
                bundle.outputNames.push_back(name.c_str());
            }
        } catch (...) {
            releaseBundle(bundle);
            throw;
        }
        return bundle.session != nullptr && !bundle.inputNames.empty() && !bundle.outputNames.empty();
    }

    static int inputIndex(const OrtBundle& bundle, const char* name)
    {
        for (size_t i = 0; i < bundle.inputNames.size(); ++i) {
            if (std::strcmp(bundle.inputNames[i], name) == 0) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    bool ensureEnv()
    {
        if (!ort().env) {
            ort().env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ObjectTrack");
        }
        if (!ort().memory) {
            ort().memory.reset(new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)));
        }
        return ort().env != nullptr && ort().memory != nullptr;
    }

    bool ensureSam()
    {
        if (ort().samReady && ort().sam.session && ort().memory) {
            return true;
        }
        const QString samPath = QStringLiteral("./plugins/Models/mobile-sam-click.onnx");
        if (!QFileInfo::exists(samPath)) {
            postStatus(QStringLiteral("缺少分割模型 mobile-sam-click.onnx"));
            return false;
        }
        if (!ensureEnv()) {
            return false;
        }
        try {
            Ort::SessionOptions cpuOptions;
            cpuOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            cpuOptions.SetIntraOpNumThreads(2);
            cpuOptions.SetInterOpNumThreads(1);
            ort().samReady = fillBundle(samPath, ort().sam, &cpuOptions);
        } catch (const std::exception& error) {
            qDebug() << "点击分割模型加载失败:" << error.what();
            releaseBundle(ort().sam);
            ort().samReady = false;
            postStatus(QStringLiteral("分割模型加载失败"));
            return false;
        }
        return ort().samReady;
    }

    bool ensureDetect()
    {
        if (ort().detectReady && ort().memory && ort().prompt.session && ort().detect.session) {
            return true;
        }
        const QString promptPath = QStringLiteral("./plugins/Models/yoloe-prompt.onnx");
        const QString detectPath = QStringLiteral("./plugins/Models/yoloe-detect.onnx");
        if (!QFileInfo::exists(promptPath) || !QFileInfo::exists(detectPath)) {
            if (!m_modelErrorLogged.load()) {
                m_modelErrorLogged.store(true);
                postStatus(QStringLiteral("缺少 ONNX：plugins/Models/yoloe-prompt.onnx 与 yoloe-detect.onnx"));
            }
            return false;
        }
        if (!ensureEnv()) {
            return false;
        }
        const bool tryCuda = !ort().cudaUnavailable;
        try {
            configureOrtSessionOptions(tryCuda);
            if (!fillBundle(promptPath, ort().prompt) || !fillBundle(detectPath, ort().detect)) {
                releaseBundle(ort().prompt);
                releaseBundle(ort().detect);
                return false;
            }
        } catch (const Ort::Exception& error) {
            releaseBundle(ort().prompt);
            releaseBundle(ort().detect);
            if (!tryCuda) {
                if (!m_modelErrorLogged.load()) {
                    m_modelErrorLogged.store(true);
                    postStatus(QStringLiteral("ONNX 加载失败"));
                }
                qDebug() << "Object Track ONNX 初始化失败:" << error.what();
                return false;
            }
            qDebug() << "CUDA 不可用，回退 CPU:" << error.what();
            ort().cudaUnavailable = true;
            try {
                configureOrtSessionOptions(false);
                if (!fillBundle(promptPath, ort().prompt) || !fillBundle(detectPath, ort().detect)) {
                    releaseBundle(ort().prompt);
                    releaseBundle(ort().detect);
                    return false;
                }
            } catch (const std::exception& cpuError) {
                releaseBundle(ort().prompt);
                releaseBundle(ort().detect);
                if (!m_modelErrorLogged.load()) {
                    m_modelErrorLogged.store(true);
                    postStatus(QStringLiteral("ONNX 加载失败"));
                }
                qDebug() << "Object Track ONNX 初始化失败:" << cpuError.what();
                return false;
            }
        }
        ort().promptImageIndex = std::max(0, inputIndex(ort().prompt, "image"));
        ort().promptMaskIndex = inputIndex(ort().prompt, "mask");
        ort().detectImageIndex = std::max(0, inputIndex(ort().detect, "image"));
        ort().detectEmbedIndex = inputIndex(ort().detect, "embeddings");
        if (ort().detectEmbedIndex < 0 && ort().detect.inputNames.size() > 1) {
            ort().detectEmbedIndex = 1;
        }
        if (ort().detectImageIndex >= 0
            && static_cast<size_t>(ort().detectImageIndex) < ort().detect.inputShapes.size()) {
            const auto& imageShape = ort().detect.inputShapes[static_cast<size_t>(ort().detectImageIndex)];
            if (imageShape.size() == 4 && imageShape[2] > 0 && imageShape[3] > 0) {
                ort().inputSize = static_cast<int>(imageShape[2]);
            }
        }
        ort().fixedSlots = -1;
        if (ort().detectEmbedIndex >= 0
            && static_cast<size_t>(ort().detectEmbedIndex) < ort().detect.inputShapes.size()
            && ort().detect.inputShapes[static_cast<size_t>(ort().detectEmbedIndex)].size() >= 2) {
            const int64_t idSlots = ort().detect.inputShapes[static_cast<size_t>(ort().detectEmbedIndex)][1];
            if (idSlots > 0) {
                ort().fixedSlots = static_cast<int>(idSlots);
            }
        }
        ort().detectReady = ort().prompt.session != nullptr && ort().detect.session != nullptr && ort().memory != nullptr;
        return ort().detectReady;
    }

    std::vector<float> runPrompt(LetterboxedInput& prepared)
    {
        if (prepared.imageBlob.empty() || !ort().memory || !ort().prompt.session) {
            return {};
        }
        std::vector<Ort::Value> inputs;
        std::vector<const char*> names;
        const int64_t imageShape[4] = {1, 3, ort().inputSize, ort().inputSize};
        inputs.push_back(Ort::Value::CreateTensor<float>(
            *ort().memory, prepared.imageBlob.ptr<float>(), prepared.imageBlob.total(), imageShape, 4));
        names.push_back(ort().prompt.inputNames[static_cast<size_t>(ort().promptImageIndex)]);
        const int64_t maskShape[4] = {1, 1, ort().inputSize, ort().inputSize};
        if (ort().promptMaskIndex >= 0) {
            inputs.push_back(Ort::Value::CreateTensor<float>(
                *ort().memory, prepared.mask.data(), prepared.mask.size(), maskShape, 4));
            names.push_back(ort().prompt.inputNames[static_cast<size_t>(ort().promptMaskIndex)]);
        }
        auto outputs = ort().prompt.session->Run(
            Ort::RunOptions{nullptr}, names.data(), inputs.data(), names.size(),
            ort().prompt.outputNames.data(), ort().prompt.outputNames.size());
        if (outputs.empty() || !outputs[0].IsTensor()) {
            return {};
        }
        const float* data = outputs[0].GetTensorMutableData<float>();
        const size_t count = outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
        if (!data || count == 0) {
            return {};
        }
        std::vector<float> embedding(data, data + count);
        double sum = 0.0;
        for (float value : embedding) {
            sum += static_cast<double>(value) * static_cast<double>(value);
        }
        if (sum > 1e-12) {
            const float inv = static_cast<float>(1.0 / std::sqrt(sum));
            for (float& value : embedding) {
                value *= inv;
            }
        }
        return embedding;
    }

    std::vector<float> encodeObject(const cv::Mat& image)
    {
        cv::Mat bgr;
        cv::Mat mask;
        splitObjectImage(image, bgr, mask);
        LetterboxedInput prepared = makeLetterbox(bgr, ort().inputSize);
        if (prepared.imageBlob.empty() || mask.empty() || prepared.mask.empty()) {
            return {};
        }
        std::fill(prepared.mask.begin(), prepared.mask.end(), 0.f);
        cv::Mat binary;
        if (mask.type() == CV_8UC1) {
            binary = mask;
        } else if (mask.channels() == 4) {
            cv::cvtColor(mask, binary, cv::COLOR_BGRA2GRAY);
        } else if (mask.channels() == 3) {
            cv::cvtColor(mask, binary, cv::COLOR_BGR2GRAY);
        } else {
            return {};
        }
        const int resizedW = std::clamp(static_cast<int>(std::lround(bgr.cols * prepared.scale)), 1, ort().inputSize);
        const int resizedH = std::clamp(static_cast<int>(std::lround(bgr.rows * prepared.scale)), 1, ort().inputSize);
        cv::Mat small;
        cv::resize(binary, small, cv::Size(resizedW, resizedH), 0, 0, cv::INTER_NEAREST);
        const int padX = static_cast<int>(prepared.padX);
        const int padY = static_cast<int>(prepared.padY);
        if (padX < 0 || padY < 0 || padX >= ort().inputSize || padY >= ort().inputSize) {
            return {};
        }
        const int yLimit = std::min(small.rows, ort().inputSize - padY);
        const int xLimit = std::min(small.cols, ort().inputSize - padX);
        for (int y = 0; y < yLimit; ++y) {
            const uchar* src = small.ptr<uchar>(y);
            float* dst = prepared.mask.data()
                + static_cast<size_t>(padY + y) * static_cast<size_t>(ort().inputSize)
                + static_cast<size_t>(padX);
            for (int x = 0; x < xLimit; ++x) {
                if (src[x] > 0) {
                    dst[x] = 1.f;
                }
            }
        }
        return runPrompt(prepared);
    }

    cv::Mat segmentClick(const cv::Mat& frame, const cv::Point& click)
    {
        if (!ort().samReady || !ort().sam.session || !ort().memory || frame.empty()
            || ort().sam.inputNames.size() < 2 || ort().sam.outputNames.empty()) {
            return {};
        }
        const cv::Mat bgr = asBgr(frame);
        if (bgr.empty()) {
            return {};
        }
        try {
            constexpr int kSize = 1024;
            const float scale = std::min(static_cast<float>(kSize) / static_cast<float>(bgr.cols),
                                         static_cast<float>(kSize) / static_cast<float>(bgr.rows));
            const int resizedW = std::clamp(static_cast<int>(std::lround(bgr.cols * scale)), 1, kSize);
            const int resizedH = std::clamp(static_cast<int>(std::lround(bgr.rows * scale)), 1, kSize);
            cv::Mat resized;
            cv::resize(bgr, resized, cv::Size(resizedW, resizedH));
            cv::Mat canvas(kSize, kSize, CV_8UC3, cv::Scalar(0, 0, 0));
            resized.copyTo(canvas(cv::Rect(0, 0, resizedW, resizedH)));
            cv::Mat blob = cv::dnn::blobFromImage(canvas, 1.0, cv::Size(), cv::Scalar(), true, false, CV_32F);
            const float pointData[2] = {
                std::clamp(static_cast<float>(click.x) * scale, 0.f, static_cast<float>(kSize - 1)),
                std::clamp(static_cast<float>(click.y) * scale, 0.f, static_cast<float>(kSize - 1))};
            const int64_t imageShape[4] = {1, 3, kSize, kSize};
            const int64_t pointShape[2] = {1, 2};
            const int imageIndex = std::max(0, inputIndex(ort().sam, "image"));
            const int pointIndex = std::max(0, inputIndex(ort().sam, "point"));
            std::vector<Ort::Value> inputs;
            std::vector<const char*> names;
            inputs.push_back(Ort::Value::CreateTensor<float>(
                *ort().memory, blob.ptr<float>(), blob.total(), imageShape, 4));
            names.push_back(ort().sam.inputNames[static_cast<size_t>(imageIndex)]);
            inputs.push_back(Ort::Value::CreateTensor<float>(
                *ort().memory, const_cast<float*>(pointData), 2, pointShape, 2));
            names.push_back(ort().sam.inputNames[static_cast<size_t>(pointIndex)]);
            if (imageIndex > pointIndex) {
                std::swap(inputs[0], inputs[1]);
                std::swap(names[0], names[1]);
            }
            auto outputs = ort().sam.session->Run(
                Ort::RunOptions{nullptr}, names.data(), inputs.data(), names.size(),
                ort().sam.outputNames.data(), ort().sam.outputNames.size());
            int maskOutput = -1;
            int scoreOutput = -1;
            for (size_t i = 0; i < outputs.size() && i < ort().sam.outputNames.size(); ++i) {
                const std::string name = ort().sam.outputNames[i];
                if (name == "masks") {
                    maskOutput = static_cast<int>(i);
                } else if (name == "scores") {
                    scoreOutput = static_cast<int>(i);
                }
            }
            if (maskOutput < 0 || scoreOutput < 0 || !outputs[static_cast<size_t>(maskOutput)].IsTensor()) {
                return {};
            }
            const auto maskShape = outputs[static_cast<size_t>(maskOutput)].GetTensorTypeAndShapeInfo().GetShape();
            const float* maskData = outputs[static_cast<size_t>(maskOutput)].GetTensorMutableData<float>();
            const float* scoreData = outputs[static_cast<size_t>(scoreOutput)].GetTensorMutableData<float>();
            if (maskShape.size() != 4 || maskShape[0] != 1 || maskShape[2] != kSize || maskShape[3] != kSize
                || !maskData || !scoreData) {
                return {};
            }
            const int candidates = static_cast<int>(maskShape[1]);
            const size_t maskCount = outputs[static_cast<size_t>(maskOutput)].GetTensorTypeAndShapeInfo().GetElementCount();
            const size_t scoreCount = outputs[static_cast<size_t>(scoreOutput)].GetTensorTypeAndShapeInfo().GetElementCount();
            if (candidates <= 0 || candidates > 8
                || maskCount < static_cast<size_t>(candidates) * kSize * kSize
                || scoreCount < static_cast<size_t>(candidates)) {
                return {};
            }
            const int px = std::clamp(static_cast<int>(std::lround(pointData[0])), 0, kSize - 1);
            const int py = std::clamp(static_cast<int>(std::lround(pointData[1])), 0, kSize - 1);
            const int imageArea = std::max(1, resizedW * resizedH);
            const int areaLimit = std::max(64, imageArea * 35 / 100);
            int chosen = -1;
            float chosenScore = -1e9f;
            int smallest = -1;
            int smallestArea = imageArea + 1;
            for (int i = 0; i < candidates; ++i) {
                const float* plane = maskData + static_cast<size_t>(i) * kSize * kSize;
                if (plane[static_cast<size_t>(py) * kSize + px] <= 0.f) {
                    continue;
                }
                int area = 0;
                for (int y = 0; y < resizedH; ++y) {
                    const float* row = plane + static_cast<size_t>(y) * kSize;
                    for (int x = 0; x < resizedW; ++x) {
                        if (row[x] > 0.f) {
                            ++area;
                        }
                    }
                }
                if (area < smallestArea) {
                    smallest = i;
                    smallestArea = area;
                }
                if (area <= areaLimit && scoreData[i] > chosenScore) {
                    chosen = i;
                    chosenScore = scoreData[i];
                }
            }
            if (chosen < 0) {
                chosen = smallest;
            }
            if (chosen < 0) {
                return {};
            }
            cv::Mat full(kSize, kSize, CV_8UC1);
            const float* plane = maskData + static_cast<size_t>(chosen) * kSize * kSize;
            for (int y = 0; y < kSize; ++y) {
                uchar* dst = full.ptr<uchar>(y);
                for (int x = 0; x < kSize; ++x) {
                    dst[x] = plane[static_cast<size_t>(y) * kSize + x] > 0.f ? 255 : 0;
                }
            }
            cv::Mat cropped = full(cv::Rect(0, 0, resizedW, resizedH)).clone();
            cv::Mat mask;
            cv::resize(cropped, mask, bgr.size(), 0, 0, cv::INTER_NEAREST);
            if (cv::countNonZero(mask) < 40) {
                return {};
            }
            return mask;
        } catch (const std::exception& error) {
            qDebug() << "点击分割失败:" << error.what();
            return {};
        }
    }

    bool rebuildEncodings(const std::vector<TrackTarget>& gallery)
    {
        m_encoded.clear();
        m_embedDim = 0;
        int sampleCount = 0;
        for (const TrackTarget& target : gallery) {
            sampleCount += static_cast<int>(target.samples.size());
        }
        postStatus(sampleCount > 0 ? QStringLiteral("正在编码 %1 张目标图").arg(sampleCount)
                                  : QStringLiteral("点击画面中的物体"));
        int failed = 0;
        int encodedPhotos = 0;
        bool truncated = false;
        for (const TrackTarget& target : gallery) {
            for (const ObjectSample& sample : target.samples) {
                if (m_cancelRequested.load()) {
                    return false;
                }
                std::vector<float> embedding = encodeObject(sample.image);
                const bool ok = !embedding.empty()
                    && (m_embedDim == 0 || static_cast<int>(embedding.size()) == m_embedDim);
                if (ok) {
                    if (m_embedDim == 0) {
                        m_embedDim = static_cast<int>(embedding.size());
                    }
                    if (ort().fixedSlots > 0 && static_cast<int>(m_encoded.size()) >= ort().fixedSlots) {
                        truncated = true;
                        break;
                    }
                    m_encoded.push_back(EncodedTarget{target.id, std::move(embedding)});
                    ++encodedPhotos;
                } else {
                    ++failed;
                }
                postStatus(QStringLiteral("正在编码 %1/%2").arg(encodedPhotos + failed).arg(sampleCount));
            }
            if (truncated) {
                break;
            }
        }
        if (m_encoded.empty()) {
            postStatus(failed > 0 ? QStringLiteral("目标图无法编码") : QStringLiteral("点击画面中的物体"));
            return false;
        }
        if (truncated) {
            postStatus(QStringLiteral("样本过多，只使用了前 %1 个向量").arg(m_encoded.size()));
        }
        return true;
    }

    std::vector<Detection> decodeDetections(Ort::Value& output, const cv::Size& frameSize, float confidence, float& maxScore)
    {
        std::vector<Detection> detections;
        maxScore = 0.f;
        if (!output.IsTensor()) {
            return detections;
        }
        const auto shape = output.GetTensorTypeAndShapeInfo().GetShape();
        if (shape.size() != 3 || shape[0] != 1) {
            return detections;
        }
        const float* data = output.GetTensorMutableData<float>();
        if (!data) {
            return detections;
        }
        const int dim1 = static_cast<int>(shape[1]);
        const int dim2 = static_cast<int>(shape[2]);
        const bool channelMajor = dim2 > dim1;
        const int channels = channelMajor ? dim1 : dim2;
        const int anchors = channelMajor ? dim2 : dim1;
        if (channels < 6 || anchors <= 0) {
            return detections;
        }
        const int activeClasses = static_cast<int>(m_encoded.size());
        const bool e2e = !channelMajor && channels == 6 && anchors <= 1000;
        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        std::vector<int> classIds;
        bool normalized = true;
        const int sampleCount = std::min(anchors, 16);
        for (int i = 0; i < sampleCount; ++i) {
            const float probe = channelMajor ? data[i] : data[static_cast<size_t>(i) * channels];
            if (std::abs(probe) > 2.5f) {
                normalized = false;
                break;
            }
        }
        const float coordScale = normalized ? static_cast<float>(ort().inputSize) : 1.f;
        auto valueAt = [&](int channel, int anchor) -> float {
            if (channelMajor) {
                return data[static_cast<size_t>(channel) * anchors + anchor];
            }
            return data[static_cast<size_t>(anchor) * channels + channel];
        };
        for (int i = 0; i < anchors; ++i) {
            int classId = -1;
            float score = 0.f;
            float x1 = 0.f;
            float y1 = 0.f;
            float x2 = 0.f;
            float y2 = 0.f;
            if (e2e) {
                x1 = valueAt(0, i) * coordScale;
                y1 = valueAt(1, i) * coordScale;
                x2 = valueAt(2, i) * coordScale;
                y2 = valueAt(3, i) * coordScale;
                score = scoreToUnit(valueAt(4, i));
                classId = static_cast<int>(std::lround(valueAt(5, i)));
            } else {
                const int classChannels = channels - 4;
                float best = 0.f;
                for (int c = 0; c < classChannels && c < activeClasses; ++c) {
                    const float classScore = scoreToUnit(valueAt(4 + c, i));
                    if (classScore > best) {
                        best = classScore;
                        classId = c;
                    }
                }
                score = best;
                const float cx = valueAt(0, i) * coordScale;
                const float cy = valueAt(1, i) * coordScale;
                const float w = valueAt(2, i) * coordScale;
                const float h = valueAt(3, i) * coordScale;
                x1 = cx - w * 0.5f;
                y1 = cy - h * 0.5f;
                x2 = cx + w * 0.5f;
                y2 = cy + h * 0.5f;
            }
            if (score > maxScore) {
                maxScore = score;
            }
            if (!std::isfinite(score) || !std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) {
                continue;
            }
            if (classId < 0 || classId >= activeClasses || score < confidence) {
                continue;
            }
            const float scale = m_letterScale > 1e-6f ? m_letterScale : 1.f;
            float left = (x1 - m_letterPadX) / scale;
            float top = (y1 - m_letterPadY) / scale;
            float right = (x2 - m_letterPadX) / scale;
            float bottom = (y2 - m_letterPadY) / scale;
            left = std::clamp(left, 0.f, static_cast<float>(frameSize.width));
            top = std::clamp(top, 0.f, static_cast<float>(frameSize.height));
            right = std::clamp(right, 0.f, static_cast<float>(frameSize.width));
            bottom = std::clamp(bottom, 0.f, static_cast<float>(frameSize.height));
            if (right - left < 2.f || bottom - top < 2.f) {
                continue;
            }
            boxes.emplace_back(cv::Point(static_cast<int>(std::lround(left)), static_cast<int>(std::lround(top))),
                               cv::Point(static_cast<int>(std::lround(right)), static_cast<int>(std::lround(bottom))));
            scores.push_back(score);
            classIds.push_back(classId);
        }
        if (boxes.empty()) {
            return detections;
        }
        std::vector<int> kept;
        cv::dnn::NMSBoxes(boxes, scores, confidence, 0.45f, kept);
        detections.reserve(kept.size());
        for (int index : kept) {
            if (index < 0 || index >= static_cast<int>(boxes.size())) {
                continue;
            }
            const cv::Rect& box = boxes[static_cast<size_t>(index)];
            detections.push_back(Detection{
                cv::Rect2f(static_cast<float>(box.x),
                           static_cast<float>(box.y),
                           static_cast<float>(box.width),
                           static_cast<float>(box.height)),
                scores[static_cast<size_t>(index)],
                classIds[static_cast<size_t>(index)]});
        }
        return detections;
    }

    void updateTracks(const std::vector<Detection>& detections, const cv::Size& frameSize, int hold)
    {
        std::vector<QString> ids;
        ids.reserve(m_encoded.size());
        for (const EncodedTarget& prompt : m_encoded) {
            if (ids.empty() || ids.back() != prompt.id) {
                ids.push_back(prompt.id);
            }
        }
        std::vector<TrackState> next;
        next.reserve(ids.size());
        for (const QString& id : ids) {
            TrackState* previous = nullptr;
            for (TrackState& track : m_tracks) {
                if (track.id == id) {
                    previous = &track;
                    break;
                }
            }
            int best = -1;
            float bestScore = -1.f;
            for (int i = 0; i < static_cast<int>(detections.size()); ++i) {
                const int classId = detections[static_cast<size_t>(i)].classId;
                if (classId < 0 || classId >= static_cast<int>(m_encoded.size())) {
                    continue;
                }
                if (m_encoded[static_cast<size_t>(classId)].id != id) {
                    continue;
                }
                float score = detections[static_cast<size_t>(i)].score;
                if (previous) {
                    score += 0.15f * rectIou(detections[static_cast<size_t>(i)].box, previous->box);
                }
                if (score > bestScore) {
                    bestScore = score;
                    best = i;
                }
            }
            if (best >= 0) {
                const Detection& det = detections[static_cast<size_t>(best)];
                TrackState track;
                track.id = id;
                track.box = det.box;
                track.score = det.score;
                const float newCx = det.box.x + det.box.width * 0.5f;
                const float newCy = det.box.y + det.box.height * 0.5f;
                if (previous) {
                    const float oldCx = previous->box.x + previous->box.width * 0.5f;
                    const float oldCy = previous->box.y + previous->box.height * 0.5f;
                    track.vx = previous->vx * 0.5f + (newCx - oldCx) * 0.5f;
                    track.vy = previous->vy * 0.5f + (newCy - oldCy) * 0.5f;
                }
                m_lastPosition.insert(id, LastPosition{newCx, newCy, det.score});
                next.push_back(track);
                continue;
            }
            if (!previous) {
                continue;
            }
            TrackState track = *previous;
            track.box.x += track.vx;
            track.box.y += track.vy;
            track.vx *= 0.6f;
            track.vy *= 0.6f;
            track.box.x = std::clamp(track.box.x, 0.f, std::max(0.f, static_cast<float>(frameSize.width) - 1.f));
            track.box.y = std::clamp(track.box.y, 0.f, std::max(0.f, static_cast<float>(frameSize.height) - 1.f));
            track.missed += 1;
            track.lost = true;
            if (track.missed <= hold) {
                next.push_back(track);
            }
        }
        m_tracks = std::move(next);
    }

    static double normalized(float value, float size)
    {
        if (size <= 1.f) {
            return 0.0;
        }
        const double ratio = std::clamp(static_cast<double>(value) / static_cast<double>(size), 0.0, 1.0);
        return std::round(ratio * 10000.0) / 10000.0;
    }

    QVariantMap buildResult(const cv::Size& frameSize, const std::vector<TrackTarget>& gallery) const
    {
        QHash<QString, const TrackState*> tracksById;
        for (const TrackState& track : m_tracks) {
            tracksById.insert(track.id, &track);
        }
        const float width = static_cast<float>(frameSize.width);
        const float height = static_cast<float>(frameSize.height);
        QVariantMap targets;
        QStringList reported;
        auto appendTarget = [&](const QString& id, const TrackState* track) {
            if (id.isEmpty() || reported.contains(id)) {
                return;
            }
            reported.push_back(id);
            const bool present = track && !track->lost;
            double cx = 0.0;
            double cy = 0.0;
            double px = 0.0;
            double py = 0.0;
            double confidence = 0.0;
            const auto remembered = m_lastPosition.constFind(id);
            if (present) {
                confidence = std::round(static_cast<double>(track->score) * 10000.0) / 10000.0;
                const double centerX = static_cast<double>(track->box.x + track->box.width * 0.5f);
                const double centerY = static_cast<double>(track->box.y + track->box.height * 0.5f);
                cx = normalized(static_cast<float>(centerX), width);
                cy = normalized(static_cast<float>(centerY), height);
                px = std::round(centerX);
                py = std::round(centerY);
            } else if (remembered != m_lastPosition.cend()) {
                confidence = std::round(static_cast<double>(remembered->score) * 10000.0) / 10000.0;
                cx = normalized(remembered->cx, width);
                cy = normalized(remembered->cy, height);
                px = std::round(static_cast<double>(remembered->cx));
                py = std::round(static_cast<double>(remembered->cy));
            }
            QVariantMap body;
            body.insert(QStringLiteral("confidence"), confidence);
            body.insert(QStringLiteral("position"), QVariantList{cx, cy});
            body.insert(QStringLiteral("pixel"), QVariantList{px, py});
            body.insert(QStringLiteral("present"), present);
            targets.insert(id, body);
        };
        for (const TrackTarget& target : gallery) {
            appendTarget(target.id, tracksById.value(target.id, nullptr));
        }
        QVariantMap result;
        result.insert(QStringLiteral("default"), targets);
        return result;
    }

    cv::Mat drawTracks(const cv::Mat& frame) const
    {
        cv::Mat canvas = asBgr(frame);
        if (canvas.empty()) {
            return {};
        }
        canvas = canvas.clone();
        static const cv::Scalar palette[] = {
            cv::Scalar(255, 128, 0), cv::Scalar(0, 200, 255), cv::Scalar(80, 220, 80),
            cv::Scalar(255, 80, 160), cv::Scalar(180, 120, 255), cv::Scalar(0, 180, 180),
            cv::Scalar(255, 200, 0), cv::Scalar(120, 120, 255)};
        const cv::Rect bounds(0, 0, canvas.cols, canvas.rows);
        for (const TrackState& track : m_tracks) {
            if (track.lost) {
                continue;
            }
            const cv::Scalar color = palette[static_cast<size_t>(qHash(track.id)) % 8];
            cv::Rect box(static_cast<int>(std::lround(track.box.x)),
                         static_cast<int>(std::lround(track.box.y)),
                         std::max(1, static_cast<int>(std::lround(track.box.width))),
                         std::max(1, static_cast<int>(std::lround(track.box.height))));
            box &= bounds;
            if (box.empty()) {
                continue;
            }
            cv::rectangle(canvas, box, color, 2);
            const std::string label = track.id.toStdString() + " " + std::to_string(static_cast<int>(track.score * 100)) + "%";
            int baseline = 0;
            const cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.55, 1, &baseline);
            cv::Point origin(box.x, std::max(textSize.height + 2, box.y - 4));
            const cv::Rect textBox(origin.x,
                                   origin.y - textSize.height - baseline,
                                   textSize.width,
                                   textSize.height + baseline * 2);
            if ((textBox & bounds).area() > 0) {
                cv::rectangle(canvas, textBox & bounds, color, cv::FILLED);
                cv::putText(canvas, label, origin, cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 255, 255), 1);
            }
        }
        return canvas;
    }

    void publish(QVariantMap results, cv::Mat image)
    {
        QMetaObject::invokeMethod(
            this,
            [this, results = std::move(results), image = std::move(image)]() mutable {
                if (m_cancelRequested.load() || !m_enabled) {
                    return;
                }
                m_outVariable = std::make_shared<VariableData>(results);
                Q_EMIT dataUpdated(1);
                if (!image.empty() && m_drawOverlay) {
                    pushMatToRingBuffer(m_outImage, m_outImageBuffer, m_lastPushedTimestamp, std::move(image));
                    Q_EMIT dataUpdated(0);
                }
            },
            Qt::QueuedConnection);
    }

    std::vector<TrackTarget> gallerySnapshot() const
    {
        QMutexLocker locker(&m_galleryMutex);
        return m_gallery;
    }

    void runTrack(cv::Mat frame, double confidence, bool draw, int hold)
    {
        QMutexLocker ortLock(&SharedOrt::instance().mutex);
        if (frame.empty()) {
            return;
        }
        PendingPick pick = takePending();
        if (m_cancelRequested.load()) {
            if (pick.active) {
                releasePreview(pick.hold);
            }
            return;
        }
        try {
            if (pick.active) {
                if (!ensureSam()) {
                    releasePreview(pick.hold);
                } else {
                    postStatus(QStringLiteral("正在分割 %1").arg(pick.id));
                    const cv::Mat mask = segmentClick(pick.frame, pick.click);
                    const cv::Mat objectImage = packObjectImage(pick.frame, mask);
                    const QString file = saveObjectPng(objectImage, pick.id);
                    if (objectImage.empty() || file.isEmpty()) {
                        postStatus(QStringLiteral("没有分割出可跟踪的物体"));
                        releasePreview(pick.hold);
                    } else if (commitSample(pick.id, objectImage, file)) {
                        presentSegment(pick.hold, mask);
                        refreshTargetListLater();
                        postStatus(QStringLiteral("已保存 %1，可继续选择或开始检测").arg(pick.id));
                    } else {
                        releasePreview(pick.hold);
                    }
                }
            }
            if (m_cancelRequested.load() || !m_enabled) {
                return;
            }
            const std::vector<TrackTarget> gallery = gallerySnapshot();
            const int gen = m_galleryGen.load();
            if (gallery.empty()) {
                publish(buildResult(frame.size(), gallery), cv::Mat());
                return;
            }
            if (!ensureDetect()) {
                return;
            }
            if (gen != m_encodedGen) {
                m_tracks.clear();
                if (!rebuildEncodings(gallery)) {
                    m_encodedGen = gen;
                    publish(buildResult(frame.size(), gallery), cv::Mat());
                    return;
                }
                m_encodedGen = gen;
            }
            if (m_encoded.empty() || m_embedDim <= 0 || ort().detectEmbedIndex < 0 || !ort().memory || !ort().detect.session) {
                publish(buildResult(frame.size(), gallery), cv::Mat());
                return;
            }
            LetterboxedInput prepared = makeLetterbox(frame, ort().inputSize);
            m_letterScale = prepared.scale;
            m_letterPadX = prepared.padX;
            m_letterPadY = prepared.padY;
            const int active = static_cast<int>(m_encoded.size());
            const int idSlots = ort().fixedSlots > 0 ? ort().fixedSlots : active;
            std::vector<float> embeddings(static_cast<size_t>(idSlots) * static_cast<size_t>(m_embedDim), 0.f);
            for (int i = 0; i < active && i < idSlots; ++i) {
                const std::vector<float>& source = m_encoded[static_cast<size_t>(i)].embedding;
                if (static_cast<int>(source.size()) != m_embedDim) {
                    continue;
                }
                std::copy(source.begin(),
                          source.end(),
                          embeddings.begin() + static_cast<size_t>(i) * static_cast<size_t>(m_embedDim));
            }
            const int64_t imageShape[4] = {1, 3, ort().inputSize, ort().inputSize};
            const int64_t embedShape[3] = {1, idSlots, m_embedDim};
            std::vector<Ort::Value> inputs;
            std::vector<const char*> names;
            inputs.push_back(Ort::Value::CreateTensor<float>(
                *ort().memory, prepared.imageBlob.ptr<float>(), prepared.imageBlob.total(), imageShape, 4));
            names.push_back(ort().detect.inputNames[static_cast<size_t>(ort().detectImageIndex)]);
            inputs.push_back(Ort::Value::CreateTensor<float>(
                *ort().memory, embeddings.data(), embeddings.size(), embedShape, 3));
            names.push_back(ort().detect.inputNames[static_cast<size_t>(ort().detectEmbedIndex)]);
            auto outputs = ort().detect.session->Run(
                Ort::RunOptions{nullptr}, names.data(), inputs.data(), names.size(),
                ort().detect.outputNames.data(), ort().detect.outputNames.size());
            if (m_cancelRequested.load() || outputs.empty()) {
                return;
            }
            float maxScore = 0.f;
            const std::vector<Detection> detections = decodeDetections(
                outputs[0], frame.size(), static_cast<float>(confidence), maxScore);
            updateTracks(detections, frame.size(), hold);
            if (m_tracks.empty()) {
                postStatus(QStringLiteral("未匹配，最高分 %1").arg(QString::number(maxScore, 'f', 2)));
            } else {
                postStatus(QStringLiteral("跟踪中 %1 个，最高分 %2")
                               .arg(m_tracks.size())
                               .arg(QString::number(maxScore, 'f', 2)));
            }
            publish(buildResult(frame.size(), gallery), draw ? drawTracks(frame) : cv::Mat());
        } catch (const Ort::Exception& error) {
            qDebug() << "Object Track ONNX 错误:" << error.what();
            if (pick.active) {
                releasePreview(pick.hold);
                postStatus(QStringLiteral("分割失败"));
                return;
            }
            if (ort().useCuda && !ort().cudaUnavailable) {
                ort().cudaUnavailable = true;
                ort().detectReady = false;
                releaseBundle(ort().prompt);
                releaseBundle(ort().detect);
                postStatus(QStringLiteral("CUDA 推理失败，改用 CPU"));
                runTrack(std::move(frame), confidence, draw, hold);
                return;
            }
            ort().detectReady = false;
            postStatus(QStringLiteral("跟踪推理失败"));
        } catch (const std::exception& error) {
            qDebug() << "Object Track 推理错误:" << error.what();
            if (pick.active) {
                releasePreview(pick.hold);
            }
            postStatus(QStringLiteral("跟踪推理失败"));
        }
    }

    ObjectTrackInterface* m_widget = nullptr;
    std::shared_ptr<ImageData> m_inImage;
    std::shared_ptr<VariableData> m_outVariable;
    std::shared_ptr<ImageData> m_outImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outImageBuffer;
    qint64 m_lastPushedTimestamp = -1;
    qint64 m_lastSeenInputTimestamp = -1;
    QThread* m_inferenceThread = nullptr;
    QElapsedTimer m_inferenceTimer;
    QElapsedTimer m_previewTimer;
    std::atomic<int> m_previewHold{0};

    mutable QMutex m_galleryMutex;
    std::vector<TrackTarget> m_gallery;
    std::atomic<int> m_galleryGen{0};
    std::atomic<bool> m_cancelRequested{false};
    bool m_hasPending = false;
    QString m_pendingId;
    cv::Mat m_pendingFrame;
    cv::Mat m_previewFrame;
    cv::Point m_pendingClick{-1, -1};
    int m_pendingHold = 0;

    double m_confThreshold = 0.25;
    double m_maxFps = 15.0;
    int m_holdFrames = 8;
    bool m_enabled = false;
    bool m_drawOverlay = true;
    std::atomic<bool> m_modelErrorLogged{false};

    std::vector<EncodedTarget> m_encoded;
    int m_encodedGen = -1;
    int m_embedDim = 0;
    std::vector<TrackState> m_tracks;
    QHash<QString, LastPosition> m_lastPosition;
    float m_letterScale = 1.f;
    float m_letterPadX = 0.f;
    float m_letterPadY = 0.f;
    QString m_workerStatus;
};
} // namespace Nodes
