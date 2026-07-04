#pragma once

/**
 * @file HumanSegmentationPPHumanSegDataModel.hpp
 * @brief PPHumanSeg 人物分割节点（前景 / 背景双路 Image 输出）
 *
 * ## 数据流
 *
 *   上游 ImageData → readInputMatForInference (GUI) → QtConcurrent 分割
 *        → foreground / background Mat → pushMatToRingBuffer × 2 (GUI)
 *
 * ## 与 Pose/Hand 节点差异
 *
 * - 无双模型级联，单 ONNX PPHumanSegModel
 * - 无 VariableData 输出，OUT0=前景 OUT1=背景
 * - 无 drawOverlay 开关，每帧均 push 两路图像
 *
 * 调度、maxFps、线程约束同 Pose 节点。
 */

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/ImageReadback.h"
#include "HumanSegmentationPPHumanSegInterface.hpp"
#include "NodeDataList.hpp"
#include "PPHumanSegEngine.hpp"
#include "PluginDefinition.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
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
/** BGRA / 灰度 → BGR，供 PPHumanSeg 推理 */
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

/** 长边 ≤1280，控制分割网络输入尺寸 */
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

/** 调度门控：仅窥探输入 timestamp，不读 Mat */
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
 * 从 ring buffer 读最新帧为 BGR Mat（须 GUI 线程，见文件头说明）
 */
inline cv::Mat readInputMatForInference(const std::shared_ptr<ImageData>& input)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return {};
    }
    return scaleMatForInference(ImageReadback::matFromFrame(frame));
}

/** 分割 Mat push 到指定输出 buffer */
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

/** 断开/禁用：清空前景或背景 ring buffer */
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
 * @brief PPHumanSeg 人物分割节点
 *
 * 端口：IN IMAGE / ENABLE；OUT FOREGROUND / BACKGROUND（均为 ImageData ring buffer）
 */
class HumanSegmentationPPHumanSegDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(double maxFps READ maxFps WRITE setMaxFps NOTIFY maxFpsChanged)
    Q_PROPERTY(bool useGpu READ useGpu WRITE setUseGpu NOTIFY useGpuChanged)

public:
    HumanSegmentationPPHumanSegDataModel()
    {
        InPortCount = 2;
        OutPortCount = 2;
        CaptionVisible = true;
        Caption = PLUGIN_NAME;
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;
        ensureImageDataBuffer(m_outForeground, m_outForegroundBuffer);
        ensureImageDataBuffer(m_outBackground, m_outBackgroundBuffer);
        model_path_ = "./plugins/Models/human_segmentation_pphumanseg_2023mar.onnx";

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "enabled";
            b.control = widget->EnableBtn;
            AbstractDelegateModel::registerExternalBinding("/enable", this, b);
        }

        connect(widget->EnableBtn, &QPushButton::clicked,
                this, &HumanSegmentationPPHumanSegDataModel::setEnabled);
        connect(widget->MaxFpsFilter, &FloatDragValueWidget::valueChanged,
                this, &HumanSegmentationPPHumanSegDataModel::setMaxFps);
        connect(widget->UseGpuCheck, &QCheckBox::toggled,
                this, &HumanSegmentationPPHumanSegDataModel::setUseGpu);
    }

    ~HumanSegmentationPPHumanSegDataModel() override
    {
        cancelPendingInference();
        GlobalEventBus::instance()->unsubscribe(this);
    }

    /** GL warmup + tick 订阅 + OSC enable */
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        //确保GPU上下文已准备好
        ImageGpuUpload::instance().warmup();
        auto bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64) { tryScheduleInference(); },
                Qt::QueuedConnection);
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
            clearRingBufferOutput(m_outForeground, m_outForegroundBuffer, m_lastPushedForeground);
            clearRingBufferOutput(m_outBackground, m_outBackgroundBuffer, m_lastPushedBackground);
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
        invalidateModel();
        emit useGpuChanged(value);
        requestInferenceRefresh();
    }

    void requestInferenceRefresh()
    {
        m_lastSeenInputTimestamp = -1;
        tryScheduleInference();
    }

Q_SIGNALS:
    void enabledChanged(bool value);
    void maxFpsChanged(double value);
    void useGpuChanged(bool value);

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
                return "FOREGROUND";
            }
            return "BACKGROUND";
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
            return ImageData().type();
        default:
            return ImageData().type();
        }
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        if (port == 0) {
            return m_outForeground;
        }
        return m_outBackground;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        switch (portIndex) {
        case 0: {
            m_inImage0 = std::dynamic_pointer_cast<ImageData>(data);
            m_lastSeenInputTimestamp = -1;
            if (!m_inImage0) {
                cancelPendingInference();
                clearRingBufferOutput(m_outForeground, m_outForegroundBuffer, m_lastPushedForeground);
                clearRingBufferOutput(m_outBackground, m_outBackgroundBuffer, m_lastPushedBackground);
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
        values["enabled"] = m_enabled;
        values["maxFps"] = m_maxFps;
        values["useGpu"] = m_useGpu;
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        const QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            const QJsonObject values = v.toObject();
            if (values.contains("enabled")) {
                setEnabled(values["enabled"].toBool(false));
            }
            if (values.contains("maxFps")) {
                setMaxFps(values["maxFps"].toDouble(15.0));
            }
            if (values.contains("useGpu")) {
                setUseGpu(values["useGpu"].toBool(true));
            }
        }
    }

    /** 推理调度入口，逻辑同 Pose/Hand 节点 */
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
        const QString modelPath = model_path_;

        auto future = QtConcurrent::run([this, frame = std::move(frame), modelPath]() mutable {
            if (m_cancelRequested.load()) {
                return;
            }
            runInferenceOnImage(std::move(frame), modelPath);
        });
        m_inferenceWatcher->setFuture(future);
    }

    qint64 inferenceIntervalMs() const
    {
        return static_cast<qint64>(std::lround(1000.0 / std::max(1.0, m_maxFps)));
    }

    void invalidateModel()
    {
        QMutexLocker locker(&m_modelMutex);
        m_modelReady = false;
        m_segmentationModel.reset();
    }

    /** 懒加载 PPHumanSegModel，路径/GPU 选项变化时重建 */
    bool ensureModelUnlocked(const QString& modelPath)
    {
        if (m_modelReady
            && m_cachedModelPath == modelPath
            && m_cachedUseGpu == m_useGpu
            && m_cachedCudaUnavailable == m_cudaUnavailable) {
            return m_segmentationModel && m_segmentationModel->isLoaded();
        }

        if (!QFileInfo::exists(modelPath)) {
            qDebug() << "人物分割模型不存在:" << modelPath;
            return false;
        }

        const bool preferCuda = m_useGpu && !m_cudaUnavailable;
        m_segmentationModel = std::make_unique<PPHumanSeg::PPHumanSegModel>(
            modelPath.toStdString(), preferCuda);

        if (!m_segmentationModel->isLoaded()) {
            qDebug() << "PPHumanSeg 模型加载失败";
            m_segmentationModel.reset();
            return false;
        }

        m_cachedModelPath = modelPath;
        m_cachedUseGpu = m_useGpu;
        m_cachedCudaUnavailable = m_cudaUnavailable;
        m_modelReady = true;
        qDebug() << "PPHumanSeg DNN 初始化成功, CUDA:" << m_segmentationModel->usingCuda();
        return true;
    }

    void handleCudaFallbackUnlocked()
    {
        if (m_cudaUnavailable || !m_useGpu) {
            return;
        }
        m_cudaUnavailable = true;
        if (m_segmentationModel) {
            m_segmentationModel->disableCuda();
        }
        m_modelReady = false;
        m_segmentationModel.reset();
        qDebug() << "PPHumanSeg CUDA 不可用，已回退 CPU";
    }

    /** 工作线程分割；GUI 线程 push 前景/背景两路 ring buffer */
    void runInferenceOnImage(cv::Mat inputImage, const QString& modelPath)
    {
        if (inputImage.empty() || m_cancelRequested.load()) {
            return;
        }

        try {
            PPHumanSeg::SegmentationResult result;
            {
                QMutexLocker locker(&m_modelMutex);
                if (!ensureModelUnlocked(modelPath)) {
                    return;
                }
                if (m_cancelRequested.load()) {
                    return;
                }
                result = m_segmentationModel->infer(inputImage);
            }

            if (!result.valid || m_cancelRequested.load()) {
                return;
            }

            QMetaObject::invokeMethod(
                this,
                [this,
                 foreground = std::move(result.foreground),
                 background = std::move(result.background)]() mutable {
                    pushMatToRingBuffer(
                        m_outForeground, m_outForegroundBuffer, m_lastPushedForeground, std::move(foreground));
                    pushMatToRingBuffer(
                        m_outBackground, m_outBackgroundBuffer, m_lastPushedBackground, std::move(background));
                },
                Qt::QueuedConnection);
        } catch (const cv::Exception& e) {
            if (m_useGpu && !m_cudaUnavailable) {
                bool retry = false;
                {
                    QMutexLocker locker(&m_modelMutex);
                    handleCudaFallbackUnlocked();
                    retry = ensureModelUnlocked(modelPath);
                }
                if (retry) {
                    runInferenceOnImage(std::move(inputImage), modelPath);
                }
                return;
            }
            qDebug() << "OpenCV 推理错误:" << e.what();
            invalidateModel();
        } catch (const std::exception& e) {
            qDebug() << "推理错误:" << e.what();
        }
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
    HumanSegmentationPPHumanSegInterface* widget = new HumanSegmentationPPHumanSegInterface();

    std::shared_ptr<ImageData> m_inImage0;
    std::shared_ptr<ImageData> m_outForeground;
    std::shared_ptr<ImageData> m_outBackground;
    std::shared_ptr<ImageTimestampRingQueue> m_outForegroundBuffer;
    std::shared_ptr<ImageTimestampRingQueue> m_outBackgroundBuffer;
    qint64 m_lastPushedForeground = -1;
    qint64 m_lastPushedBackground = -1;
    qint64 m_lastSeenInputTimestamp = -1;

    QString model_path_;
    bool m_enabled = false;
    double m_maxFps = 15.0;
    bool m_useGpu = true;
    bool m_cudaUnavailable = false;

    QMutex m_modelMutex;
    QElapsedTimer m_inferenceTimer;
    std::atomic<bool> m_cancelRequested{false};

    bool m_modelReady = false;
    QString m_cachedModelPath;
    bool m_cachedUseGpu = false;
    bool m_cachedCudaUnavailable = false;
    std::unique_ptr<PPHumanSeg::PPHumanSegModel> m_segmentationModel;
};
} // namespace Nodes
