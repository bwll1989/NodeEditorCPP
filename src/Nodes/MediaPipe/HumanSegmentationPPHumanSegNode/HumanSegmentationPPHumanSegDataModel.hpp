#pragma once

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "HumanSegmentationPPHumanSegInterface.hpp"
#include "NodeDataList.hpp"
#include "PPHumanSegEngine.hpp"
#include "PluginDefinition.hpp"
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

namespace Nodes
{
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
        m_outForeground = std::make_shared<ImageData>();
        m_outBackground = std::make_shared<ImageData>();
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

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
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
            m_outForeground = std::make_shared<ImageData>();
            m_outBackground = std::make_shared<ImageData>();
            Q_EMIT dataUpdated(0);
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
        if (m_inImage0) {
            QMutexLocker locker(&m_pendingMutex);
            m_hasPendingFrame = true;
        }
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
        if (data == nullptr) {
            return;
        }
        switch (portIndex) {
        case 0: {
            auto imageData = std::dynamic_pointer_cast<ImageData>(data);
            if (!imageData) {
                return;
            }
            m_inImage0 = imageData;
            {
                QMutexLocker locker(&m_pendingMutex);
                m_hasPendingFrame = true;
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

    void tryScheduleInference()
    {
        if (!m_enabled || (!m_hasPendingFrame && !m_inImage0)) {
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

        const qint64 intervalMs = inferenceIntervalMs();
        if (m_inferenceTimer.isValid()) {
            const qint64 elapsed = m_inferenceTimer.elapsed();
            if (elapsed < intervalMs) {
                QTimer::singleShot(
                    static_cast<int>(intervalMs - elapsed),
                    this,
                    [this]() { tryScheduleInference(); });
                return;
            }
        }

        cv::Mat frame;
        {
            QMutexLocker locker(&m_pendingMutex);
            if (!m_hasPendingFrame || !m_inImage0) {
                return;
            }
            frame = cloneFrameForInference(m_inImage0->mat());
            m_hasPendingFrame = false;
        }

        if (frame.empty()) {
            return;
        }

        m_inferenceTimer.start();
        m_cancelRequested.store(false);
        const QString modelPath = model_path_;

        auto future = QtConcurrent::run([this, frame = std::move(frame), modelPath]() mutable {
            runInferenceOnImage(std::move(frame), modelPath);
        });
        m_inferenceWatcher->setFuture(future);
    }

    static cv::Mat cloneFrameForInference(const cv::Mat& src)
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
                    m_outForeground = std::make_shared<ImageData>(std::move(foreground));
                    m_outBackground = std::make_shared<ImageData>(std::move(background));
                    Q_EMIT dataUpdated(0);
                    Q_EMIT dataUpdated(1);
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
        QMutexLocker locker(&m_pendingMutex);
        m_hasPendingFrame = false;
    }

private:
    QFutureWatcher<void>* m_inferenceWatcher = nullptr;
    HumanSegmentationPPHumanSegInterface* widget = new HumanSegmentationPPHumanSegInterface();
    std::shared_ptr<ImageData> m_inImage0;
    std::shared_ptr<ImageData> m_outForeground;
    std::shared_ptr<ImageData> m_outBackground;
    QString model_path_;
    bool m_enabled = false;
    double m_maxFps = 15.0;
    bool m_useGpu = true;
    bool m_cudaUnavailable = false;

    QMutex m_pendingMutex;
    QMutex m_modelMutex;
    bool m_hasPendingFrame = false;
    QElapsedTimer m_inferenceTimer;
    std::atomic<bool> m_cancelRequested{false};

    bool m_modelReady = false;
    QString m_cachedModelPath;
    bool m_cachedUseGpu = false;
    bool m_cachedCudaUnavailable = false;
    std::unique_ptr<PPHumanSeg::PPHumanSegModel> m_segmentationModel;
};
} // namespace Nodes
