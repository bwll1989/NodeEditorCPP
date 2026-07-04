#pragma once

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QtConcurrent/QtConcurrent>
#include <QAbstractScrollArea>
#include <QFutureWatcher>
#include <opencv2/dnn.hpp>
#include <vector>
#include <array>
#include <atomic>
#include <thread>
#include <QtCore/qglobal.h>
#include <QElapsedTimer>
#include "PluginDefinition.hpp"
#include "FaceDetectionInterface.hpp"
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <algorithm>
#include <cmath>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/ImageReadback.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;

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
} // namespace

namespace Nodes
{
    class FaceDetectionDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(double confidence READ getConfidence WRITE setConfidence NOTIFY confidenceChanged)
        Q_PROPERTY(double nmsThreshold READ getNmsThreshold WRITE setNmsThreshold NOTIFY nmsThresholdChanged)
        Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(double maxFps READ maxFps WRITE setMaxFps NOTIFY maxFpsChanged)
        Q_PROPERTY(bool drawOverlay READ drawOverlay WRITE setDrawOverlay NOTIFY drawOverlayChanged)

        public:
        FaceDetectionDataModel()
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
            model_path = "./plugins/Models/yolo11n-face-detection.onnx";

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
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "nmsThreshold";
                b.control = widget->NmsFilter;
                AbstractDelegateModel::registerExternalBinding("/nms", this, b);
            }

            connect(widget->ConfidenceFilter, &FloatDragValueWidget::valueChanged,
                    this, &FaceDetectionDataModel::setConfidence);
            connect(this, &FaceDetectionDataModel::confidenceChanged, this, [this](double value) {
                if (!widget || qFuzzyCompare(widget->ConfidenceFilter->value(), value)) {
                    return;
                }
                QSignalBlocker blocker(widget->ConfidenceFilter);
                widget->ConfidenceFilter->setValue(value);
            });
            connect(widget->NmsFilter, &FloatDragValueWidget::valueChanged,
                    this, &FaceDetectionDataModel::setNmsThreshold);
            connect(this, &FaceDetectionDataModel::nmsThresholdChanged, this, [this](double value) {
                if (!widget || qFuzzyCompare(widget->NmsFilter->value(), value)) {
                    return;
                }
                QSignalBlocker blocker(widget->NmsFilter);
                widget->NmsFilter->setValue(value);
            });
            connect(widget->EnableBtn, &QPushButton::clicked,
                    this, &FaceDetectionDataModel::setEnabled);
            connect(widget->MaxFpsFilter, &FloatDragValueWidget::valueChanged,
                    this, &FaceDetectionDataModel::setMaxFps);
            connect(widget->DrawOverlayCheck, &QCheckBox::toggled,
                    this, &FaceDetectionDataModel::setDrawOverlay);
        }

        ~FaceDetectionDataModel() override
        {
            cancelPendingInference();
            disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
            GlobalEventBus::instance()->unsubscribe(this);
            m_ortSession.reset();
            m_sessionOptions.reset();
            m_ortEnv.reset();
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            ImageGpuUpload::instance().warmup();
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/confidence"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/nms"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));

            connect(TimestampGenerator::getInstance(),
                    &TimestampGenerator::frameCountUpdated,
                    this,
                    [this](qint64) { tryScheduleInference(); },
                    Qt::QueuedConnection);
        }

    public:
        double getConfidence() const { return m_confThreshold; }
        void setConfidence(double value) {
            if (qFuzzyCompare(m_confThreshold, value)) return;
            m_confThreshold = value;
            if (widget && !qFuzzyCompare(widget->ConfidenceFilter->value(), value)) {
                QSignalBlocker blocker(widget->ConfidenceFilter);
                widget->ConfidenceFilter->setValue(value);
            }
            emit confidenceChanged(value);
            requestInferenceRefresh();
        }

        double getNmsThreshold() const { return m_nmsThreshold; }
        void setNmsThreshold(double value) {
            if (qFuzzyCompare(m_nmsThreshold, value)) return;
            m_nmsThreshold = value;
            if (widget && !qFuzzyCompare(widget->NmsFilter->value(), value)) {
                QSignalBlocker blocker(widget->NmsFilter);
                widget->NmsFilter->setValue(value);
            }
            emit nmsThresholdChanged(value);
            requestInferenceRefresh();
        }

        bool isEnabled() const { return m_enabled; }

        double maxFps() const { return m_maxFps; }
        void setMaxFps(double value) {
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
        void setDrawOverlay(bool value) {
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

        void requestInferenceRefresh()
        {
            m_lastSeenInputTimestamp = -1;
            tryScheduleInference();
        }

        void setEnabled(bool value) {
            if (m_enabled == value) return;
            m_enabled = value;
            if (widget && widget->EnableBtn->isChecked() != value) {
                QSignalBlocker blocker(widget->EnableBtn);
                widget->EnableBtn->setChecked(value);
            }
            emit enabledChanged(value);
            if (!m_enabled) {
                cancelPendingInference();
                clearRingBufferOutput(m_outImage, m_outImageBuffer, m_lastPushedTimestamp);
                m_outVariable = std::make_shared<VariableData>();
                Q_EMIT dataUpdated(1);
                return;
            }
            requestInferenceRefresh();
        }

    Q_SIGNALS:
        void confidenceChanged(double value);
        void nmsThresholdChanged(double value);
        void enabledChanged(bool value);
        void maxFpsChanged(double value);
        void drawOverlayChanged(bool value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "confidence") {
                setConfidence(ev.payload.toDouble());
            } else if (localPath == "nms") {
                setNmsThreshold(ev.payload.toDouble());
            } else if (localPath == "enable") {
                setEnabled(ev.payload.toBool());
            }
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
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
                    return "IMAGE " + QString::number(portIndex);
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
            const double conf = m_confThreshold;
            const double nms = m_nmsThreshold;
            const bool draw = m_drawOverlay;

            auto future = QtConcurrent::run([this, frame = std::move(frame), conf, nms, draw]() mutable {
                runInferenceOnImage(std::move(frame), conf, nms, draw);
            });
            m_inferenceWatcher->setFuture(future);
        }

        qint64 inferenceIntervalMs() const
        {
            return static_cast<qint64>(std::lround(1000.0 / std::max(1.0, m_maxFps)));
        }

        void runInferenceOnImage(cv::Mat inputImage, double confidence, double nmsThreshold, bool drawOverlay)
        {
            if (inputImage.empty()) {
                return;
            }
            if (m_cancelRequested.load()) {
                return;
            }
            try {
                if (!initializeOnnxSession()) {
                    return;
                }
                if (m_cancelRequested.load()) {
                    return;
                }

                cv::Mat blob = cv::dnn::blobFromImage(
                    inputImage,
                    1.0f / 255.0f,
                    m_modelInputSize,
                    cv::Scalar(),
                    true,
                    false,
                    CV_32F);

                if (m_cancelRequested.load()) {
                    return;
                }

                std::array<int64_t, 4> inputTensorShape{
                    1, 3, m_modelInputSize.height, m_modelInputSize.width};
                Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                    m_memoryInfo, blob.ptr<float>(), blob.total(),
                    inputTensorShape.data(), inputTensorShape.size());
                auto outputTensors = m_ortSession->Run(
                    m_runOptions,
                    m_inputNames.data(), &inputTensor, 1,
                    m_outputNames.data(), m_outputNames.size());

                if (m_cancelRequested.load()) {
                    return;
                }

                QVariantMap detectionResults;
                cv::Mat resultImage = postProcessOnnxResults(
                    outputTensors, inputImage, m_modelInputSize,
                    static_cast<float>(confidence), static_cast<float>(nmsThreshold),
                    detectionResults, drawOverlay);

                QMetaObject::invokeMethod(this, [this, resultImage = std::move(resultImage), detectionResults]() mutable {
                    const bool resultsChanged = (m_lastDetectionResults != detectionResults);
                    if (resultsChanged) {
                        m_lastDetectionResults = detectionResults;
                        m_outVariable = std::make_shared<VariableData>(detectionResults);
                        Q_EMIT dataUpdated(1);
                    }
                    if (m_drawOverlay) {
                        pushMatToRingBuffer(
                            m_outImage, m_outImageBuffer, m_lastPushedTimestamp, std::move(resultImage));
                    }
                }, Qt::QueuedConnection);
            } catch (const Ort::Exception& e) {
                qDebug() << "ONNX Runtime错误:" << e.what();
                m_isModelInitialized = false;
            } catch (const std::exception& e) {
                qDebug() << "推理错误:" << e.what();
            } catch (...) {
                qDebug() << "未知错误";
            }
        }

        void cancelPendingInference()
        {
            m_cancelRequested.store(true);
            if (m_inferenceWatcher) {
                if (m_inferenceWatcher->isRunning()) {
                    m_inferenceWatcher->waitForFinished();
                }
            }
        }

        cv::Mat postProcessOnnxResults(
            std::vector<Ort::Value>& outputTensors,
            const cv::Mat& originalImage,
            const cv::Size& inputSize,
            float confidenceThreshold,
            float nmsThreshold,
            QVariantMap& outDetectionResults,
            bool drawOverlay)
        {
            if (outputTensors.empty()) {
                outDetectionResults["default"] = 0;
                outDetectionResults["detections"] = QVariantList{};
                outDetectionResults["width"] = originalImage.cols;
                outDetectionResults["height"] = originalImage.rows;
                return originalImage;
            }

            const float* outputData = outputTensors[0].GetTensorMutableData<float>();
            auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

            if (outputShape.size() < 2) {
                outDetectionResults["default"] = 0;
                outDetectionResults["detections"] = QVariantList{};
                outDetectionResults["width"] = originalImage.cols;
                outDetectionResults["height"] = originalImage.rows;
                return originalImage;
            }

            const int numBoxes = static_cast<int>(outputShape[2]);
            const int numValues = static_cast<int>(outputShape[1]);

            const float scaleX = static_cast<float>(originalImage.cols) / inputSize.width;
            const float scaleY = static_cast<float>(originalImage.rows) / inputSize.height;
            const float imgWidth = static_cast<float>(originalImage.cols);
            const float imgHeight = static_cast<float>(originalImage.rows);

            std::vector<cv::Rect> boxes;
            std::vector<float> scores;
            std::vector<int> classIds;
            boxes.reserve(numBoxes / 10);
            scores.reserve(numBoxes / 10);
            classIds.reserve(numBoxes / 10);

            const float* centerXPtr = outputData;
            const float* centerYPtr = outputData + numBoxes;
            const float* widthPtr = outputData + 2 * numBoxes;
            const float* heightPtr = outputData + 3 * numBoxes;
            const float* confidencePtr = outputData + 4 * numBoxes;
            const int classesPerBox = std::max(0, numValues - 5);
            const float* classesPtr = classesPerBox > 0 ? (outputData + 5 * numBoxes) : nullptr;

            for (int i = 0; i < numBoxes; ++i) {
                const float confidence = confidencePtr[i];
                if (confidence <= confidenceThreshold) {
                    continue;
                }

                const float centerX = centerXPtr[i];
                const float centerY = centerYPtr[i];
                const float width = widthPtr[i];
                const float height = heightPtr[i];
                const float halfWidth = width * 0.5f;
                const float halfHeight = height * 0.5f;

                float x1 = (centerX - halfWidth) * scaleX;
                float y1 = (centerY - halfHeight) * scaleY;
                float x2 = (centerX + halfWidth) * scaleX;
                float y2 = (centerY + halfHeight) * scaleY;

                x1 = std::clamp(x1, 0.0f, imgWidth);
                y1 = std::clamp(y1, 0.0f, imgHeight);
                x2 = std::clamp(x2, 0.0f, imgWidth);
                y2 = std::clamp(y2, 0.0f, imgHeight);

                if (x2 <= x1 || y2 <= y1) {
                    continue;
                }

                int classId = 0;
                if (classesPerBox > 0 && classesPtr) {
                    const float* clsBase = classesPtr + i;
                    float classScore = -1.0f;
                    for (int c = 0; c < classesPerBox; ++c) {
                        const float s = clsBase[c * numBoxes];
                        if (s > classScore) {
                            classScore = s;
                            classId = c;
                        }
                    }
                }

                boxes.emplace_back(cv::Point(x1, y1), cv::Point(x2, y2));
                scores.push_back(confidence);
                classIds.push_back(classId);
            }

            QVariantList detections;
            if (boxes.empty()) {
                outDetectionResults["default"] = 0;
                outDetectionResults["detections"] = detections;
                outDetectionResults["width"] = originalImage.cols;
                outDetectionResults["height"] = originalImage.rows;
                return originalImage;
            }

            std::vector<int> indices;
            cv::dnn::NMSBoxes(boxes, scores, confidenceThreshold, nmsThreshold, indices);

            for (const int idx : indices) {
                const cv::Rect& box = boxes[idx];
                QVariantList boxList;
                boxList << box.x << box.y << (box.x + box.width) << (box.y + box.height);
                QVariantMap det;
                det["score"] = scores[idx];
                det["box"] = boxList;
                det["classId"] = (classIds.size() > static_cast<size_t>(idx) ? classIds[idx] : 0);
                det["className"] = "face";
                detections << det;
            }

            outDetectionResults["default"] = detections.size();
            outDetectionResults["detections"] = detections;
            outDetectionResults["width"] = originalImage.cols;
            outDetectionResults["height"] = originalImage.rows;

            if (!drawOverlay || indices.empty()) {
                return originalImage;
            }

            cv::Mat resultImage = originalImage.clone();
            for (const int idx : indices) {
                const cv::Rect& box = boxes[idx];
                cv::rectangle(resultImage, box, cv::Scalar(0, 255, 0), 2);
                const std::string confText =
                    "Face " + std::to_string(static_cast<int>(scores[idx] * 100)) + "%";
                cv::putText(resultImage, confText, cv::Point(box.x, box.y - 10),
                           cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
            }
            return resultImage;
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson1["confidence"] = m_confThreshold;
            modelJson1["nmsThreshold"] = m_nmsThreshold;
            modelJson1["enabled"] = m_enabled;
            modelJson1["maxFps"] = m_maxFps;
            modelJson1["drawOverlay"] = m_drawOverlay;
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject values = v.toObject();
                if (values.contains("confidence")) {
                    setConfidence(values["confidence"].toDouble(0.25));
                }
                if (values.contains("nmsThreshold")) {
                    setNmsThreshold(values["nmsThreshold"].toDouble(0.45));
                }
                if (values.contains("enabled")) {
                    setEnabled(values["enabled"].toBool(true));
                }
                if (values.contains("maxFps")) {
                    setMaxFps(values["maxFps"].toDouble(15.0));
                }
                if (values.contains("drawOverlay")) {
                    setDrawOverlay(values["drawOverlay"].toBool(true));
                }
            }
        }

        void configureOrtSessionOptions(bool tryCuda)
        {
            const unsigned int cores = std::max(1u, std::thread::hardware_concurrency());
            m_sessionOptions = std::make_unique<Ort::SessionOptions>();
            m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            m_sessionOptions->SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);

            m_useCuda = false;
            if (tryCuda) {
                try {
                    const auto providers = Ort::GetAvailableProviders();
                    for (const auto& provider : providers) {
                        if (provider == "CUDAExecutionProvider") {
                            OrtCUDAProviderOptions cuda_options{};
                            cuda_options.device_id = 0;
                            cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchHeuristic;
                            cuda_options.arena_extend_strategy = 1;
                            cuda_options.do_copy_in_default_stream = 1;
                            cuda_options.gpu_mem_limit = SIZE_MAX;
                            m_sessionOptions->AppendExecutionProvider_CUDA(cuda_options);
                            m_useCuda = true;
                            break;
                        }
                    }
                } catch (const std::exception& e) {
                    qDebug() << "CUDA EP 注册失败，将使用 CPU:" << e.what();
                }
            }

            if (m_useCuda) {
                m_sessionOptions->SetIntraOpNumThreads(1);
                m_sessionOptions->SetInterOpNumThreads(1);
            } else {
                const int cpuThreads = static_cast<int>(
                    std::min(2u, std::max(1u, cores / 2)));
                m_sessionOptions->SetIntraOpNumThreads(cpuThreads);
                m_sessionOptions->SetInterOpNumThreads(1);
            }
        }

        bool createOrtSessionFromOptions()
        {
            m_ortSession = std::make_unique<Ort::Session>(
                *m_ortEnv, model_path.toStdWString().c_str(), *m_sessionOptions);

            Ort::AllocatorWithDefaultOptions allocator;
            m_inputNames.clear();
            m_outputNames.clear();
            m_inputNamesPtr.clear();
            m_outputNamesPtr.clear();

            const size_t numInputNodes = m_ortSession->GetInputCount();
            for (size_t i = 0; i < numInputNodes; i++) {
                auto inputNamePtr = m_ortSession->GetInputNameAllocated(i, allocator);
                m_inputNames.push_back(inputNamePtr.get());
                m_inputNamesPtr.push_back(std::move(inputNamePtr));
            }

            const size_t numOutputNodes = m_ortSession->GetOutputCount();
            for (size_t i = 0; i < numOutputNodes; i++) {
                auto outputNamePtr = m_ortSession->GetOutputNameAllocated(i, allocator);
                m_outputNames.push_back(outputNamePtr.get());
                m_outputNamesPtr.push_back(std::move(outputNamePtr));
            }
            return true;
        }

        bool initializeOnnxSession()
        {
            if (m_isModelInitialized && m_cachedModelPath == model_path) {
                return true;
            }

            QFileInfo modelFile(model_path);
            if (!modelFile.exists()) {
                qDebug() << "模型文件不存在:" << model_path;
                return false;
            }

            m_isModelInitialized = false;
            m_inputNames.clear();
            m_outputNames.clear();
            m_inputNamesPtr.clear();
            m_outputNamesPtr.clear();
            m_ortSession.reset();

            if (!m_ortEnv) {
                m_ortEnv = std::make_unique<Ort::Env>(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "FaceONNX");
            }

            const bool tryCuda = !m_cudaUnavailable;
            configureOrtSessionOptions(tryCuda);

            try {
                createOrtSessionFromOptions();
            } catch (const Ort::Exception& e) {
                if (!m_useCuda) {
                    qDebug() << "ONNX Runtime 初始化错误:" << e.what();
                    return false;
                }
                qDebug() << "CUDA 不可用，回退 CPU:" << e.what();
                m_cudaUnavailable = true;
                m_ortSession.reset();
                configureOrtSessionOptions(false);
                try {
                    createOrtSessionFromOptions();
                } catch (const Ort::Exception& cpuErr) {
                    qDebug() << "ONNX Runtime 初始化错误:" << cpuErr.what();
                    return false;
                } catch (const std::exception& cpuErr) {
                    qDebug() << "初始化错误:" << cpuErr.what();
                    return false;
                }
            } catch (const std::exception& e) {
                qDebug() << "初始化错误:" << e.what();
                return false;
            }

            m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            m_runOptions = Ort::RunOptions{nullptr};
            m_isModelInitialized = true;
            m_cachedModelPath = model_path;
            qDebug() << "ONNX Runtime 会话初始化成功, CUDA:" << m_useCuda;
            return true;
        }

    private:
        QFutureWatcher<void>* m_inferenceWatcher = nullptr;
        FaceDetectionInterface* widget = new FaceDetectionInterface();
        std::shared_ptr<ImageData> m_inImage0;
        std::shared_ptr<VariableData> m_outVariable;
        std::shared_ptr<ImageData> m_outImage;
        std::shared_ptr<ImageTimestampRingQueue> m_outImageBuffer;
        qint64 m_lastPushedTimestamp = -1;
        qint64 m_lastSeenInputTimestamp = -1;
        QString model_path;
        double m_confThreshold = 0.25;
        double m_nmsThreshold = 0.45;
        bool m_enabled = false;
        double m_maxFps = 15.0;
        bool m_drawOverlay = true;
        QVariantMap m_lastDetectionResults;

        QElapsedTimer m_inferenceTimer;

        std::unique_ptr<Ort::Env> m_ortEnv;
        std::unique_ptr<Ort::Session> m_ortSession;
        std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
        std::vector<const char*> m_inputNames;
        std::vector<const char*> m_outputNames;
        std::vector<Ort::AllocatedStringPtr> m_inputNamesPtr;
        std::vector<Ort::AllocatedStringPtr> m_outputNamesPtr;
        Ort::MemoryInfo m_memoryInfo{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};
        Ort::RunOptions m_runOptions{nullptr};

        cv::Size m_modelInputSize{640, 640};
        bool m_isModelInitialized = false;
        QString m_cachedModelPath;
        bool m_useCuda = false;
        bool m_cudaUnavailable = false;
        std::atomic<bool> m_cancelRequested{false};
    };
}
