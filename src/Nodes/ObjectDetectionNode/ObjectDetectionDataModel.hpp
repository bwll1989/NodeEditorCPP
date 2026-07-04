#pragma once

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QtConcurrent/QtConcurrent>
#include <QAbstractScrollArea>
#include <QFutureWatcher>
#include <QMutex>
#include <opencv2/dnn.hpp>
#include <vector>
#include <array>
#include <atomic>
#include <thread>
#include <QtCore/qglobal.h>
#include <QElapsedTimer>
#include <QTimer>
#include "PluginDefinition.hpp"
#include "ObjectDetectionInterface.hpp"
// 添加ONNX Runtime头文件
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
    struct ObjectDetectionProfile
    {
        QString modelPath = QStringLiteral("./plugins/Models/yolo11n-Detection.onnx");
        QString caption = QStringLiteral(PLUGIN_NAME);
        const std::vector<std::string>* classNames = &::classNames;
    };

    class ObjectDetectionDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(double confidence READ getConfidence WRITE setConfidence NOTIFY confidenceChanged)
        Q_PROPERTY(int filterClassIndex READ getFilterClassIndex WRITE setFilterClassIndex NOTIFY filterClassIndexChanged)
        Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(double maxFps READ maxFps WRITE setMaxFps NOTIFY maxFpsChanged)
        Q_PROPERTY(bool drawOverlay READ drawOverlay WRITE setDrawOverlay NOTIFY drawOverlayChanged)

        public:
        explicit ObjectDetectionDataModel(const ObjectDetectionProfile& profile = {})
        {
            InPortCount = 2;
            OutPortCount=2;
            CaptionVisible=true;
            Caption=profile.caption;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_outVariable=std::make_shared<VariableData>();
            ensureImageDataBuffer(m_outImage, m_outImageBuffer);
            model_path=profile.modelPath;
            m_classNames = profile.classNames ? profile.classNames : &::classNames;
            widget = new ObjectDetectionInterface(*m_classNames);
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
                b.member = "filterClassIndex";
                b.control = widget->ClassSelectorComboBox;
                AbstractDelegateModel::registerExternalBinding("/filter", this, b);
            }
            connect(widget->ConfidenceFilter, &FloatDragValueWidget::valueChanged,
                    this, &ObjectDetectionDataModel::setConfidence);
            connect(this, &ObjectDetectionDataModel::confidenceChanged, this, [this](double value) {
                if (!widget || qFuzzyCompare(widget->ConfidenceFilter->value(), value)) {
                    return;
                }
                QSignalBlocker blocker(widget->ConfidenceFilter);
                widget->ConfidenceFilter->setValue(value);
            });
            connect(widget->ClassSelectorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &ObjectDetectionDataModel::setFilterClassIndex);
            connect(widget->EnableBtn, &QPushButton::clicked,
                    this, &ObjectDetectionDataModel::setEnabled);
            connect(widget->MaxFpsFilter, &FloatDragValueWidget::valueChanged,
                    this, &ObjectDetectionDataModel::setMaxFps);
            connect(widget->DrawOverlayCheck, &QCheckBox::toggled,
                    this, &ObjectDetectionDataModel::setDrawOverlay);
        }
        /**
         * @brief 安全析构：取消异步推理并释放ONNX资源
         */
        ~ObjectDetectionDataModel() override
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
            bus->subscribe(makeFullOscAddress("/filter"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        int getFilterClassIndex() const { return m_selectedClassId; }
        void setFilterClassIndex(int value) {
            if (m_selectedClassId == value) return;
            m_selectedClassId = value;
            if (widget && widget->ClassSelectorComboBox->currentIndex() != value) {
                QSignalBlocker blocker(widget->ClassSelectorComboBox);
                widget->ClassSelectorComboBox->setCurrentIndex(value);
            }
            emit filterClassIndexChanged(value);
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
        void filterClassIndexChanged(int value);
        void enabledChanged(bool value);
        void maxFpsChanged(double value);
        void drawOverlayChanged(bool value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "confidence") {
                setConfidence(ev.payload.toDouble());
            } else if (localPath == "filter") {
                setFilterClassIndex(ev.payload.toInt());
            } else if (localPath == "enable") {
                setEnabled(ev.payload.toBool());
            }
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch (portIndex) {
                case 0:
                    return "IMAGE";
                case 1:
                    return "ENABLE";
                default:
                    return "";
                }
            case PortType::Out:
                    if (portIndex==0)
                        return "IMAGE "+QString::number(portIndex);
                    else
                        return "RESULT";
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            // Q_UNUSED(portIndex);
            // Q_UNUSED(portType);
            switch(portType){
            case PortType::In:
                if (portIndex == 0) {
                    return ImageData().type();
                }
                return VariableData().type();
            case PortType::Out:
                    if (portIndex==0)
                        return ImageData().type();
                    else
                        return VariableData().type();
            default:
                return ImageData().type();
            }
        }


        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            if (port ==0 )
                return m_outImage;
            else
                return m_outVariable;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            switch (portIndex)
            {
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

        /**
         * @brief 调度异步推理：推理进行中时合并为最新帧，避免积压与丢帧
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
            const double conf = m_confThreshold;
            const int clsId = m_selectedClassId;
            const bool draw = m_drawOverlay;

            auto future = QtConcurrent::run([this, frame = std::move(frame), conf, clsId, draw]() mutable {
                runInferenceOnImage(std::move(frame), conf, clsId, draw);
            });
            m_inferenceWatcher->setFuture(future);
        }

    qint64 inferenceIntervalMs() const
    {
        return static_cast<qint64>(std::lround(1000.0 / std::max(1.0, m_maxFps)));
    }

    void runInferenceOnImage(cv::Mat inputImage, double confidence, int classId, bool drawOverlay)
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
                static_cast<float>(confidence), classId, detectionResults, drawOverlay);

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

    /**
     * @brief 取消正在进行的异步推理，避免析构时阻塞或崩溃
     */
    void cancelPendingInference()
    {
        m_cancelRequested.store(true);
        if (m_inferenceWatcher) {
            if (m_inferenceWatcher->isRunning()) {
                m_inferenceWatcher->waitForFinished();
            }
        }
    }


    /**
     * @brief 针对YOLOv11检测模型优化的输出结果处理函数，支持界面参数过滤
     * @param outputTensors ONNX Runtime的输出张量
     * @param originalImage 原始输入图像
     * @param inputSize 模型输入尺寸
     * @param confidence 置信度阈值
     * @param classId 筛选的类别ID
     * @param outDetectionResults 输出的检测结果数据
     * @return 带有检测框标注的图像
     */
    cv::Mat postProcessOnnxResults(
        std::vector<Ort::Value>& outputTensors,
        const cv::Mat& originalImage,
        const cv::Size& inputSize,
        float confidence,
        int classId,
        QVariantMap& outDetectionResults,
        bool drawOverlay)
    {
        if (outputTensors.empty()) {
            qDebug() << "输出张量为空";
            outDetectionResults["result"] = QVariantList{};
            outDetectionResults["default"] = 0;
            return originalImage;
        }

        // 获取输出张量数据
        const float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

        // YOLOv11检测模型输出格式: [1, 84, 8400]
        // 84 = 4(bbox) + 80(classes)
        if (outputShape.size() != 3 || outputShape[0] != 1) {
            qDebug() << "不支持的输出张量形状";
            outDetectionResults["result"] = QVariantList{};
            outDetectionResults["default"] = 0;
            return originalImage;
        }

        const int numClasses = outputShape[1] - 4;  // 减去4个边界框坐标
        const int numBoxes = outputShape[2];

        // 过滤参数
        const float confidenceThreshold = confidence;
        const int selectedClassId = classId;

        const bool filterByClass = (selectedClassId >= 0 && selectedClassId < numClasses);
        
        // 预计算缩放比例
        const float scaleX = static_cast<float>(originalImage.cols) / inputSize.width;
        const float scaleY = static_cast<float>(originalImage.rows) / inputSize.height;
        const float imgWidth = static_cast<float>(originalImage.cols);
        const float imgHeight = static_cast<float>(originalImage.rows);

        // 固定NMS阈值
        constexpr float nmsThreshold = 0.45f;

        // 预分配容器
        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        std::vector<int> classIds;

        boxes.reserve(numBoxes / 20);  // 预估5%的框会通过置信度筛选
        scores.reserve(numBoxes / 20);
        classIds.reserve(numBoxes / 20);

        // 预计算数据指针偏移（YOLOv11输出格式：[cx, cy, w, h, class0_conf, class1_conf, ...]）
        const float* centerXPtr = outputData;
        const float* centerYPtr = outputData + numBoxes;
        const float* widthPtr = outputData + 2 * numBoxes;
        const float* heightPtr = outputData + 3 * numBoxes;
        const float* classScoresPtr = outputData + 4 * numBoxes;

        // 主循环：解析检测结果
        for (int i = 0; i < numBoxes; ++i) {
            // 找到最高置信度的类别
            float maxClassScore = 0.0f;
            int bestClassId = -1;

            if (filterByClass) {
                // 只检查选定的类别
                const float classScore = classScoresPtr[selectedClassId * numBoxes + i];
                if (classScore > confidenceThreshold) {
                    maxClassScore = classScore;
                    bestClassId = selectedClassId;
                }
            } else {
                const float* classRow = classScoresPtr + i;
                for (int c = 0; c < numClasses; ++c) {
                    const float classScore = classRow[c * numBoxes];
                    if (classScore > maxClassScore) {
                        maxClassScore = classScore;
                        bestClassId = c;
                    }
                }
            }

            // 置信度筛选
            if (maxClassScore <= confidenceThreshold || bestClassId == -1) continue;

            // 获取边界框坐标（中心点格式转换为左上角格式）
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

            // 边界检查
            x1 = std::clamp(x1, 0.0f, imgWidth);
            y1 = std::clamp(y1, 0.0f, imgHeight);
            x2 = std::clamp(x2, 0.0f, imgWidth);
            y2 = std::clamp(y2, 0.0f, imgHeight);

            if (x2 <= x1 || y2 <= y1) continue;

            // 保存有效检测
            boxes.emplace_back(cv::Point(x1, y1), cv::Point(x2, y2));
            scores.push_back(maxClassScore);
            classIds.push_back(bestClassId);
        }

        if (boxes.empty()) {
            outDetectionResults["result"] = QVariantList{};
            outDetectionResults["default"] = 0;
            outDetectionResults["default"] = 0;
            return originalImage;
        }

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, scores, confidenceThreshold, nmsThreshold, indices);
        if (indices.empty()) {
            outDetectionResults["result"] = QVariantList{};
            outDetectionResults["default"] = 0;
            return originalImage;
        }

        if (!drawOverlay) {
            QVariantList detectionsArray;
            for (const int idx : indices) {
                const int detClassId = classIds[idx];
                const float detConfidence = scores[idx];
                const std::string className =
                    (detClassId < static_cast<int>(m_classNames->size()))
                        ? (*m_classNames)[detClassId]
                        : "Unknown";
                QVariantMap detection;
                detection["class_id"] = detClassId;
                detection["class_name"] = QString::fromStdString(className);
                detection["confidence"] = static_cast<double>(detConfidence);
                detectionsArray.append(detection);
            }
            outDetectionResults["result"] = detectionsArray;
            outDetectionResults["default"] = detectionsArray.size();
            return originalImage;
        }

        cv::Mat resultImage = originalImage.clone();
        int detectionCount = 0;
        for (const int idx : indices) {
            const cv::Rect& box = boxes[idx];
            const int classId = classIds[idx];
            const float confidence = scores[idx];

            // 获取类别颜色（循环使用预定义颜色）
            const cv::Scalar color = classColors[classId % classColors.size()];

            // 绘制边界框
            cv::rectangle(resultImage, box, color, 2);

            // 绘制类别标签和置信度
            const std::string className = (classId < static_cast<int>(m_classNames->size())) ? (*m_classNames)[classId] : "Unknown";
            const std::string label = className + " " + std::to_string(static_cast<int>(confidence * 100)) + "%";

            // 计算文本尺寸
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &baseline);

            // 绘制文本背景
            cv::Point textOrg(box.x, box.y - 10);
            if (textOrg.y < textSize.height) {
                textOrg.y = box.y + textSize.height + 10;
            }

            cv::rectangle(resultImage,
                         cv::Point(textOrg.x, textOrg.y - textSize.height - baseline),
                         cv::Point(textOrg.x + textSize.width, textOrg.y + baseline),
                         color, -1);

            // 绘制文本
            cv::putText(resultImage, label, textOrg,
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);

            detectionCount++;
        }

        // 在图像左上角显示检测统计信息和过滤参数
        const std::string statsText = "Detections: " + std::to_string(detectionCount);
        cv::putText(resultImage, statsText, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
        
        // 显示当前过滤参数
        const std::string filterText = "Conf: " + std::to_string(static_cast<int>(confidenceThreshold * 100)) + "%";
        cv::putText(resultImage, filterText, cv::Point(10, 60),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        
        if (filterByClass) {
            const std::string classText = "Class: " + (*m_classNames)[selectedClassId];
            cv::putText(resultImage, classText, cv::Point(10, 90),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        }

        // 构建检测结果数据
        QVariantList detectionsArray;

        for (const int idx : indices) {
            const cv::Rect& box = boxes[idx];
            const int classId = classIds[idx];
            const float confidence = scores[idx];
            const std::string className = (classId < static_cast<int>(m_classNames->size())) ? (*m_classNames)[classId] : "Unknown";

            QVariantMap detection;
            detection["class_id"] = classId;
            detection["class_name"] = QString::fromStdString(className);
            detection["confidence"] = static_cast<double>(confidence);
            // detection["bbox_x"] = box.x;
            // detection["bbox_y"] = box.y;
            // detection["bbox_width"] = box.width;
            // detection["bbox_height"] = box.height;

            detectionsArray.append(detection);
        }

        outDetectionResults["result"] = detectionsArray;
        outDetectionResults["default"] = detectionCount;

        return resultImage;
    }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        /**
         * @brief 保存节点配置
         * @details 保存置信度阈值、类别选择和启用状态到JSON对象
         * @return 包含节点配置的JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson = NodeDelegateModel::save();
            
            // 保存置信度阈值
            modelJson1["confidence"] = m_confThreshold;
            
            // 保存类别选择索引
            modelJson1["classIndex"] = m_selectedClassId;
            
            // 保存启用状态
            modelJson1["enabled"] = m_enabled;
            modelJson1["maxFps"] = m_maxFps;
            modelJson1["drawOverlay"] = m_drawOverlay;
            
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        /**
         * @brief 加载节点配置
         * @details 从JSON对象加载置信度阈值、类别选择和启用状态
         * @param p 包含节点配置的JSON对象
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject values = v.toObject();
                
                // 加载置信度阈值
                if (values.contains("confidence")) {
                    setConfidence(values["confidence"].toDouble(0.4));
                }
                
                // 加载类别选择索引
                if (values.contains("classIndex")) {
                    setFilterClassIndex(values["classIndex"].toInt(0));
                }
                
                // 加载启用状态
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

    /**
     * @brief 配置 SessionOptions（可选 CUDA EP）
     */
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

    /**
     * @brief 创建 Session 并缓存输入输出名
     */
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

    /**
     * @brief 初始化ONNX Runtime会话
     * @details CUDA EP 可能在列表中但实际无 GPU；创建 Session 失败时自动回退 CPU
     * @return 初始化是否成功
     */
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
            m_ortEnv = std::make_unique<Ort::Env>(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "ImageONNX");
        }

        const bool tryCuda = !m_cudaUnavailable;
        configureOrtSessionOptions(tryCuda);

        try {
            createOrtSessionFromOptions();
            if (m_useCuda) {
                qDebug() << "ONNX Runtime 使用 CUDA 执行提供者";
            } else {
                qDebug() << "ONNX Runtime 使用 CPU 执行提供者";
            }
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
                qDebug() << "ONNX Runtime 使用 CPU 执行提供者";
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
        ObjectDetectionInterface* widget = nullptr;
        const std::vector<std::string>* m_classNames = &::classNames;
        std::shared_ptr<ImageData> m_inImage0;
        std::shared_ptr<VariableData> m_outVariable;
        std::shared_ptr<ImageData> m_outImage;
        std::shared_ptr<ImageTimestampRingQueue> m_outImageBuffer;
        qint64 m_lastPushedTimestamp = -1;
        qint64 m_lastSeenInputTimestamp = -1;
        QString model_path;
        double m_confThreshold = 0.4;
        int m_selectedClassId = 0;
        bool m_enabled = false;
        double m_maxFps = 15.0;
        bool m_drawOverlay = true;
        QVariantMap m_lastDetectionResults;

        QElapsedTimer m_inferenceTimer;

        // ONNX Runtime缓存资源
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
