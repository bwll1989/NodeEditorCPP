#pragma once

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "MediaPipePoseEngine.hpp"
#include "NodeDataList.hpp"
#include "PluginDefinition.hpp"
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

namespace Nodes
{
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
        m_outImage = std::make_shared<ImageData>();
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

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/personScore"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/nms"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/confidence"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

    void requestInferenceRefresh()
    {
        if (m_inImage0) {
            QMutexLocker locker(&m_pendingMutex);
            m_hasPendingFrame = true;
        }
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
        const double personScore = m_personScore;
        const double nms = m_nmsThreshold;
        const double conf = m_confThreshold;
        const bool draw = m_drawOverlay;
        const QString personPath = person_model_path_;
        const QString posePath = pose_model_path_;

        auto future = QtConcurrent::run([this, frame = std::move(frame), personScore, nms, conf, draw, personPath, posePath]() mutable {
            runInferenceOnImage(std::move(frame), personScore, nms, conf, draw, personPath, posePath);
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

    void invalidateModels()
    {
        QMutexLocker locker(&m_modelsMutex);
        m_modelsReady = false;
        m_personDetector.reset();
        m_poseEstimator.reset();
    }

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
                        m_outImage = std::make_shared<ImageData>(std::move(resultImage));
                        Q_EMIT dataUpdated(0);
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
    PoseEstimationMediaPipeInterface* widget = new PoseEstimationMediaPipeInterface();
    std::shared_ptr<ImageData> m_inImage0;
    std::shared_ptr<VariableData> m_outVariable;
    std::shared_ptr<ImageData> m_outImage;
    QString person_model_path_;
    QString pose_model_path_;
    double m_personScore = 0.5;
    double m_nmsThreshold = 0.3;
    double m_confThreshold = 0.8;
    bool m_enabled = false;
    double m_maxFps = 15.0;
    bool m_drawOverlay = true;
    bool m_useGpu = true;
    bool m_cudaUnavailable = false;
    QVariantMap m_lastDetectionResults;

    QMutex m_pendingMutex;
    QMutex m_modelsMutex;
    bool m_hasPendingFrame = false;
    QElapsedTimer m_inferenceTimer;
    std::atomic<bool> m_cancelRequested{false};

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
