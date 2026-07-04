#pragma once

/**
 * @file PoseEstimationMediaPipeDataModel.hpp
 * @brief MediaPipe 人体姿态估计节点（PersonDetector + PoseEstimator）
 *
 * ## 数据流（ImageData 环形缓存架构）
 *
 *   上游 ImageData ──► getLatestImageFrame ──► readInputMatForInference (GUI)
 *        │                                              │
 *        │                                              ▼
 *        │                                    QtConcurrent 后台 ONNX 推理
 *        │                                              │
 *        ├── RESULT 口 (VariableData) ◄── dataUpdated(1) │ buildDetectionResults
 *        └── IMAGE 口 (drawOverlay 时) ◄── pushMatToRingBuffer (GUI)
 *
 * ## 调度与帧率
 *
 * - 触发：TimestampGenerator::frameCountUpdated、推理完成、setInData、参数变更
 * - maxFps：QElapsedTimer 限制两次推理最小间隔；m_lastSeenInputTimestamp 输入去重
 * - 推理串行：同一时刻仅一个 QFutureWatcher 任务
 *
 * ## 线程约束
 *
 * - readInputMatForInference / pushMatToRingBuffer：须在 GUI 线程（OpenGL 读回/上传）
 * - runInferenceOnImage：工作线程；结果经 QMetaObject::invokeMethod 回 GUI
 */

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "MediaPipePoseEngine.hpp"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/ImageReadback.h"
#include "NodeDataList.hpp"
#include "PluginDefinition.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "PoseEstimationMediaPipeInterface.hpp"
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonObject>
#include <QMutex>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QObject>
#include <QtNodes/NodeDelegateModel>
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
/** BGRA / 灰度 / BGR → 三通道 BGR，供 OpenCV DNN 消费 */
inline cv::Mat ensureBgr(const cv::Mat& src)
{
    if (src.empty()) {
        return {};
    }
    if (src.channels() == 4) {
        cv::Mat bgr;
        cv::cvtColor(src, bgr, cv::COLOR_BGRA2BGR);
        return bgr;
    }
    if (src.channels() == 1) {
        cv::Mat bgr;
        cv::cvtColor(src, bgr, cv::COLOR_GRAY2BGR);
        return bgr;
    }
    if (src.channels() == 3) {
        return src;
    }
    return {};
}

/** 长边缩至 1280 以内，降低推理开销；返回独立 Mat 副本 */
inline cv::Mat scaleMatForInference(const cv::Mat& src)
{
    const cv::Mat bgr = ensureBgr(src);
    if (bgr.empty()) {
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

/** 轻量窥探：只读 ring buffer 最新帧 timestamp，不做 Mat 读回 */
inline bool peekLatestInputTimestamp(const std::shared_ptr<ImageData>& input, qint64& timestamp)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return false;
    }
    timestamp = frame.timestamp;
    return true;
}

/**
 * 从输入 ring buffer 取最新帧并转为推理用 BGR Mat。
 * @note 须在 GUI 线程：上游若为纯 GPU 纹理（如 Flip 输出），会经 ImageGpuUpload 读回。
 */
inline cv::Mat readInputMatForInference(const std::shared_ptr<ImageData>& input)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return {};
    }
    return scaleMatForInference(ImageReadback::matFromFrame(frame));
}

/** 推理结果 Mat 上传 GPU 并写入输出 ring buffer（含 CPU 副本） */
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

/** 断开连接或禁用时清空输出 buffer */
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
} // namespace

namespace Nodes
{
/**
 * @brief MediaPipe 人体姿态估计节点
 *
 * 端口：
 *   IN  0 IMAGE   - ImageData 共享 ring buffer
 *   IN  1 ENABLE  - VariableData 布尔使能
 *   OUT 0 IMAGE   - 叠加骨架图（drawOverlay=true 时有数据）
 *   OUT 1 RESULT  - VariableData 关键点 JSON
 */
class PoseEstimationMediaPipeDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double personScore READ getPersonScore WRITE setPersonScore NOTIFY personScoreChanged)
    Q_PROPERTY(double nmsThreshold READ getNmsThreshold WRITE setNmsThreshold NOTIFY nmsThresholdChanged)
    Q_PROPERTY(double confidence READ getConfidence WRITE setConfidence NOTIFY confidenceChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(double maxFps READ maxFps WRITE setMaxFps NOTIFY maxFpsChanged)
    Q_PROPERTY(bool drawOverlay READ drawOverlay WRITE setDrawOverlay NOTIFY drawOverlayChanged)
    Q_PROPERTY(bool useGpu READ useGpu WRITE setUseGpu NOTIFY useGpuChanged)

public:
    PoseEstimationMediaPipeDataModel()
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
        person_model_path_ = "./plugins/Models/person_detection_mediapipe_2023mar.onnx";
        pose_model_path_ = "./plugins/Models/pose_estimation_mediapipe_2023mar.onnx";

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "personScore";
            b.control = widget->PersonScoreFilter;
            AbstractDelegateModel::registerExternalBinding("/personScore", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "nmsThreshold";
            b.control = widget->NmsFilter;
            AbstractDelegateModel::registerExternalBinding("/nms", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "confidence";
            b.control = widget->ConfidenceFilter;
            AbstractDelegateModel::registerExternalBinding("/confidence", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "enabled";
            b.control = widget->EnableBtn;
            AbstractDelegateModel::registerExternalBinding("/enable", this, b);
        }

        connect(widget->PersonScoreFilter, &FloatDragValueWidget::valueChanged,
                this, &PoseEstimationMediaPipeDataModel::setPersonScore);
        connect(widget->NmsFilter, &FloatDragValueWidget::valueChanged,
                this, &PoseEstimationMediaPipeDataModel::setNmsThreshold);
        connect(widget->ConfidenceFilter, &FloatDragValueWidget::valueChanged,
                this, &PoseEstimationMediaPipeDataModel::setConfidence);
        connect(widget->EnableBtn, &QPushButton::clicked,
                this, &PoseEstimationMediaPipeDataModel::setEnabled);
        connect(widget->MaxFpsFilter, &FloatDragValueWidget::valueChanged,
                this, &PoseEstimationMediaPipeDataModel::setMaxFps);
        connect(widget->DrawOverlayCheck, &QCheckBox::toggled,
                this, &PoseEstimationMediaPipeDataModel::setDrawOverlay);
        connect(widget->UseGpuCheck, &QCheckBox::toggled,
                this, &PoseEstimationMediaPipeDataModel::setUseGpu);
    }

    ~PoseEstimationMediaPipeDataModel() override
    {
        cancelPendingInference();
        GlobalEventBus::instance()->unsubscribe(this);
    }

    /** 预创建离屏 GL；订阅 OSC；挂接系统 tick 驱动 tryScheduleInference */
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        ImageGpuUpload::instance().warmup();
        auto bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/personScore"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/nms"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/confidence"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64) { tryScheduleInference(); },
                Qt::QueuedConnection);
    }

    double getPersonScore() const { return m_personScore; }
    void setPersonScore(double value)
    {
        if (qFuzzyCompare(m_personScore, value)) {
            return;
        }
        m_personScore = value;
        if (widget && !qFuzzyCompare(widget->PersonScoreFilter->value(), value)) {
            QSignalBlocker blocker(widget->PersonScoreFilter);
            widget->PersonScoreFilter->setValue(value);
        }
        invalidateModels();
        emit personScoreChanged(value);
        requestInferenceRefresh();
    }

    double getNmsThreshold() const { return m_nmsThreshold; }
    void setNmsThreshold(double value)
    {
        if (qFuzzyCompare(m_nmsThreshold, value)) {
            return;
        }
        m_nmsThreshold = value;
        if (widget && !qFuzzyCompare(widget->NmsFilter->value(), value)) {
            QSignalBlocker blocker(widget->NmsFilter);
            widget->NmsFilter->setValue(value);
        }
        invalidateModels();
        emit nmsThresholdChanged(value);
        requestInferenceRefresh();
    }

    double getConfidence() const { return m_confThreshold; }
    void setConfidence(double value)
    {
        if (qFuzzyCompare(m_confThreshold, value)) {
            return;
        }
        m_confThreshold = value;
        if (widget && !qFuzzyCompare(widget->ConfidenceFilter->value(), value)) {
            QSignalBlocker blocker(widget->ConfidenceFilter);
            widget->ConfidenceFilter->setValue(value);
        }
        invalidateModels();
        emit confidenceChanged(value);
        requestInferenceRefresh();
    }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool value)
    {
        if (m_enabled == value) {
            return;
        }
        m_enabled = value;
        if (widget && widget->EnableBtn->isChecked() != value) {
            QSignalBlocker blocker(widget->EnableBtn);
            widget->EnableBtn->setChecked(value);
        }
        emit enabledChanged(value);
        if (!m_enabled) {
            cancelPendingInference();
            m_outVariable = std::make_shared<VariableData>();
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
        if (widget && !qFuzzyCompare(widget->MaxFpsFilter->value(), value)) {
            QSignalBlocker blocker(widget->MaxFpsFilter);
            widget->MaxFpsFilter->setValue(value);
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
        if (widget && widget->DrawOverlayCheck->isChecked() != value) {
            QSignalBlocker blocker(widget->DrawOverlayCheck);
            widget->DrawOverlayCheck->setChecked(value);
        }
        emit drawOverlayChanged(value);
    }

    bool useGpu() const { return m_useGpu; }
    void setUseGpu(bool value)
    {
        if (m_useGpu == value) {
            return;
        }
        m_useGpu = value;
        if (widget && widget->UseGpuCheck->isChecked() != value) {
            QSignalBlocker blocker(widget->UseGpuCheck);
            widget->UseGpuCheck->setChecked(value);
        }
        m_cudaUnavailable = false;
        invalidateModels();
        emit useGpuChanged(value);
        requestInferenceRefresh();
    }

    /** 重置输入去重并立即尝试调度（参数/模型变更后强制重推理） */
    void requestInferenceRefresh()
    {
        m_lastSeenInputTimestamp = -1;
        tryScheduleInference();
    }

Q_SIGNALS:
    void personScoreChanged(double value);
    void nmsThresholdChanged(double value);
    void confidenceChanged(double value);
    void enabledChanged(bool value);
    void maxFpsChanged(double value);
    void drawOverlayChanged(bool value);
    void useGpuChanged(bool value);

private Q_SLOTS:
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
        if (localPath == "personScore") {
            setPersonScore(ev.payload.toDouble());
        } else if (localPath == "nms") {
            setNmsThreshold(ev.payload.toDouble());
        } else if (localPath == "confidence") {
            setConfidence(ev.payload.toDouble());
        } else if (localPath == "enable") {
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
            return "RESULT";
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
        return m_outVariable;
    }

    /**
     * @brief 输入 IMAGE 连接变化
     * - 仅保存 m_inImage0 指针，不在此读像素
     * - 断开时取消推理并清空输出 ring buffer
     */
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
            setEnabled(enableData->value().toBool());
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
        values["personScore"] = m_personScore;
        values["nmsThreshold"] = m_nmsThreshold;
        values["confidence"] = m_confThreshold;
        values["enabled"] = m_enabled;
        values["maxFps"] = m_maxFps;
        values["drawOverlay"] = m_drawOverlay;
        values["useGpu"] = m_useGpu;
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        const QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            const QJsonObject values = v.toObject();
            if (values.contains("personScore")) {
                setPersonScore(values["personScore"].toDouble(0.5));
            }
            if (values.contains("nmsThreshold")) {
                setNmsThreshold(values["nmsThreshold"].toDouble(0.3));
            }
            if (values.contains("confidence")) {
                setConfidence(values["confidence"].toDouble(0.8));
            }
            if (values.contains("enabled")) {
                setEnabled(values["enabled"].toBool(false));
            }
            if (values.contains("maxFps")) {
                setMaxFps(values["maxFps"].toDouble(15.0));
            }
            if (values.contains("drawOverlay")) {
                setDrawOverlay(values["drawOverlay"].toBool(true));
            }
            if (values.contains("useGpu")) {
                setUseGpu(values["useGpu"].toBool(true));
            }
        }
    }

    /**
     * @brief 推理调度入口（GUI 线程）
     *
     * 门控顺序：enabled → 无在途任务 → 输入有新 timestamp → maxFps 间隔 → 读 Mat → 投递 QtConcurrent
     */
    void tryScheduleInference()
    {
        if (!m_enabled || !m_inImage0 || imageDataIsEmpty(m_inImage0)) {
            return;
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

        const qint64 intervalMs = inferenceIntervalMs();
        if (m_inferenceTimer.isValid() && m_inferenceTimer.elapsed() < intervalMs) {
            return;
        }

        cv::Mat frame = readInputMatForInference(m_inImage0);
        if (frame.empty()) {
            return;
        }

        m_lastSeenInputTimestamp = latestTs;
        m_inferenceTimer.start();
        m_cancelRequested.store(false);
        const double personScore = m_personScore;
        const double nms = m_nmsThreshold;
        const double conf = m_confThreshold;
        const bool draw = m_drawOverlay;
        const QString personPath = person_model_path_;
        const QString posePath = pose_model_path_;

        auto future = QtConcurrent::run([this, frame = std::move(frame), personScore, nms, conf, draw, personPath, posePath]() mutable {
            if (m_cancelRequested.load()) {
                return;
            }
            runInferenceOnImage(std::move(frame), personScore, nms, conf, draw, personPath, posePath);
        });
        m_inferenceWatcher->setFuture(future);
    }

    /** maxFps 对应的两次推理最小间隔（毫秒） */
    qint64 inferenceIntervalMs() const
    {
        return static_cast<qint64>(std::lround(1000.0 / std::max(1.0, m_maxFps)));
    }

    void invalidateModels()
    {
        QMutexLocker locker(&m_modelsMutex);
        m_modelsReady = false;
        m_personDetector.reset();
        m_poseEstimator.reset();
    }

    /** 按路径与阈值懒加载 DNN；参数未变则复用已加载实例 */
    bool ensureModelsUnlocked(const QString& personPath,
                              const QString& posePath,
                              float personScore,
                              float nmsThreshold,
                              float confThreshold)
    {
        if (m_modelsReady
            && m_cachedPersonPath == personPath
            && m_cachedPosePath == posePath
            && qFuzzyCompare(m_cachedPersonScore, static_cast<double>(personScore))
            && qFuzzyCompare(m_cachedNms, static_cast<double>(nmsThreshold))
            && qFuzzyCompare(m_cachedConf, static_cast<double>(confThreshold))
            && m_cachedUseGpu == m_useGpu
            && m_cachedCudaUnavailable == m_cudaUnavailable) {
            return m_personDetector && m_personDetector->isLoaded()
                && m_poseEstimator && m_poseEstimator->isLoaded();
        }

        if (!QFileInfo::exists(personPath)) {
            qDebug() << "人体检测模型不存在:" << personPath;
            return false;
        }
        if (!QFileInfo::exists(posePath)) {
            qDebug() << "姿态估计模型不存在:" << posePath;
            return false;
        }

        const bool preferCuda = m_useGpu && !m_cudaUnavailable;

        m_personDetector = std::make_unique<MediaPipePose::PersonDetector>(
            personPath.toStdString(), nmsThreshold, personScore, 5000, preferCuda);
        m_poseEstimator = std::make_unique<MediaPipePose::PoseEstimator>(
            posePath.toStdString(), confThreshold, preferCuda);

        if (!m_personDetector->isLoaded() || !m_poseEstimator->isLoaded()) {
            qDebug() << "MediaPipe 模型加载失败";
            m_personDetector.reset();
            m_poseEstimator.reset();
            return false;
        }

        m_cachedPersonPath = personPath;
        m_cachedPosePath = posePath;
        m_cachedPersonScore = personScore;
        m_cachedNms = nmsThreshold;
        m_cachedConf = confThreshold;
        m_cachedUseGpu = m_useGpu;
        m_cachedCudaUnavailable = m_cudaUnavailable;
        m_modelsReady = true;
        qDebug() << "MediaPipe DNN 初始化成功, CUDA:"
                 << (m_personDetector->usingCuda() && m_poseEstimator->usingCuda());
        return true;
    }

    void handleCudaFallbackUnlocked()
    {
        if (m_cudaUnavailable || !m_useGpu) {
            return;
        }
        m_cudaUnavailable = true;
        if (m_personDetector) {
            m_personDetector->disableCuda();
        }
        if (m_poseEstimator) {
            m_poseEstimator->disableCuda();
        }
        m_modelsReady = false;
        m_personDetector.reset();
        m_poseEstimator.reset();
        qDebug() << "MediaPipe CUDA 不可用，已回退 CPU";
    }

    /** 将 PoseResult 序列化为 VariableData 用的 QVariantMap */
    static QVariantMap buildDetectionResults(const std::vector<MediaPipePose::PoseResult>& poses,
                                             int width,
                                             int height)
    {
        QVariantMap results;
        results["count"] = static_cast<int>(poses.size());
        results["width"] = width;
        results["height"] = height;

        QVariantList detections;
        for (const auto& pose : poses) {
            if (!pose.valid) {
                continue;
            }

            QVariantMap item;
            item["confidence"] = static_cast<double>(pose.confidence);

            if (!pose.bbox.empty()) {
                cv::Mat box;
                pose.bbox.convertTo(box, CV_32S);
                QVariantMap bbox;
                bbox["x1"] = box.at<int>(0, 0);
                bbox["y1"] = box.at<int>(0, 1);
                bbox["x2"] = box.at<int>(1, 0);
                bbox["y2"] = box.at<int>(1, 1);
                item["bbox"] = bbox;
            }

            QVariantList keypoints;
            const cv::Mat landmarks = pose.landmarks.rowRange(0, pose.landmarks.rows - 6);
            for (int i = 0; i < landmarks.rows; ++i) {
                QVariantMap kp;
                kp["x"] = static_cast<double>(landmarks.at<float>(i, 0));
                kp["y"] = static_cast<double>(landmarks.at<float>(i, 1));
                kp["z"] = static_cast<double>(landmarks.at<float>(i, 2));
                kp["visibility"] = static_cast<double>(landmarks.at<float>(i, 3));
                kp["presence"] = static_cast<double>(landmarks.at<float>(i, 4));
                keypoints.push_back(kp);
            }
            item["keypoints"] = keypoints;

            if (!pose.worldLandmarks.empty()) {
                const cv::Mat world = pose.worldLandmarks.rowRange(0, pose.worldLandmarks.rows - 6);
                QVariantList worldPoints;
                for (int i = 0; i < world.rows; ++i) {
                    QVariantMap wp;
                    wp["x"] = static_cast<double>(world.at<float>(i, 0));
                    wp["y"] = static_cast<double>(world.at<float>(i, 1));
                    wp["z"] = static_cast<double>(world.at<float>(i, 2));
                    worldPoints.push_back(wp);
                }
                item["world_landmarks"] = worldPoints;
            }

            detections.push_back(item);
        }
        results["detections"] = detections;
        return results;
    }

    /**
     * @brief 工作线程执行 ONNX 推理
     * - 懒加载/缓存 PersonDetector + PoseEstimator（m_modelsMutex）
     * - CUDA 异常时 handleCudaFallbackUnlocked 并重试一次
     * - 结果 QueuedConnection 回 GUI 更新 Variable / 可选 push 图像
     */
    void runInferenceOnImage(cv::Mat inputImage,
                             double personScore,
                             double nmsThreshold,
                             double confidence,
                             bool drawOverlay,
                             const QString& personPath,
                             const QString& posePath)
    {
        if (inputImage.empty() || m_cancelRequested.load()) {
            return;
        }

        try {
            std::vector<MediaPipePose::PoseResult> poses;
            {
                QMutexLocker locker(&m_modelsMutex);
                if (!ensureModelsUnlocked(
                        personPath,
                        posePath,
                        static_cast<float>(personScore),
                        static_cast<float>(nmsThreshold),
                        static_cast<float>(confidence))) {
                    return;
                }
                if (m_cancelRequested.load()) {
                    return;
                }

                poses = MediaPipePose::detectPoses(*m_personDetector, *m_poseEstimator, inputImage);
            }

            cv::Mat resultImage = inputImage;
            if (drawOverlay) {
                resultImage = inputImage.clone();
                for (const auto& pose : poses) {
                    MediaPipePose::drawPoseOverlay(resultImage, pose);
                }
            }

            const QVariantMap detectionResults =
                buildDetectionResults(poses, inputImage.cols, inputImage.rows);

            QMetaObject::invokeMethod(
                this,
                [this, resultImage = std::move(resultImage), detectionResults, drawOverlay]() mutable {
                    const bool resultsChanged = (m_lastDetectionResults != detectionResults);
                    if (resultsChanged) {
                        m_lastDetectionResults = detectionResults;
                        m_outVariable = std::make_shared<VariableData>(detectionResults);
                        Q_EMIT dataUpdated(1);
                    }
                    if (drawOverlay) {
                        pushMatToRingBuffer(
                            m_outImage, m_outImageBuffer, m_lastPushedTimestamp, std::move(resultImage));
                    }
                },
                Qt::QueuedConnection);
        } catch (const cv::Exception& e) {
            if (m_useGpu && !m_cudaUnavailable) {
                bool retry = false;
                {
                    QMutexLocker locker(&m_modelsMutex);
                    handleCudaFallbackUnlocked();
                    retry = ensureModelsUnlocked(
                        personPath,
                        posePath,
                        static_cast<float>(personScore),
                        static_cast<float>(nmsThreshold),
                        static_cast<float>(confidence));
                }
                if (retry) {
                    runInferenceOnImage(
                        std::move(inputImage),
                        personScore,
                        nmsThreshold,
                        confidence,
                        drawOverlay,
                        personPath,
                        posePath);
                }
                return;
            }
            qDebug() << "OpenCV 推理错误:" << e.what();
            invalidateModels();
        } catch (const std::exception& e) {
            qDebug() << "推理错误:" << e.what();
        }
    }

    /** 置 cancel 标志并阻塞等待当前推理结束（析构/禁用时） */
    void cancelPendingInference()
    {
        m_cancelRequested.store(true);
        if (m_inferenceWatcher && m_inferenceWatcher->isRunning()) {
            m_inferenceWatcher->waitForFinished();
        }
    }

private:
    QFutureWatcher<void>* m_inferenceWatcher = nullptr;  ///< 串行化后台推理任务

    PoseEstimationMediaPipeInterface* widget = new PoseEstimationMediaPipeInterface();

    // --- 输入 / 输出 ImageData 句柄（像素在 ring buffer，不在 NodeData 内） ---
    std::shared_ptr<ImageData> m_inImage0;
    std::shared_ptr<VariableData> m_outVariable;
    std::shared_ptr<ImageData> m_outImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outImageBuffer;
    qint64 m_lastPushedTimestamp = -1;       ///< 输出 push 去重
    qint64 m_lastSeenInputTimestamp = -1;    ///< 已调度推理的输入 timestamp

    QString person_model_path_;
    QString pose_model_path_;

    // --- UI / 推理参数 ---
    double m_personScore = 0.5;
    double m_nmsThreshold = 0.3;
    double m_confThreshold = 0.8;
    bool m_enabled = false;
    double m_maxFps = 15.0;
    bool m_drawOverlay = true;
    bool m_useGpu = true;
    bool m_cudaUnavailable = false;
    QVariantMap m_lastDetectionResults;      ///< RESULT 口变更检测，避免重复 emit

    QMutex m_modelsMutex;
    QElapsedTimer m_inferenceTimer;           ///< maxFps 节流
    std::atomic<bool> m_cancelRequested{false};

    // --- DNN 实例与懒加载缓存键 ---
    bool m_modelsReady = false;
    QString m_cachedPersonPath;
    QString m_cachedPosePath;
    double m_cachedPersonScore = -1.0;
    double m_cachedNms = -1.0;
    double m_cachedConf = -1.0;
    bool m_cachedUseGpu = false;
    bool m_cachedCudaUnavailable = false;
    std::unique_ptr<MediaPipePose::PersonDetector> m_personDetector;
    std::unique_ptr<MediaPipePose::PoseEstimator> m_poseEstimator;
};
} // namespace Nodes
