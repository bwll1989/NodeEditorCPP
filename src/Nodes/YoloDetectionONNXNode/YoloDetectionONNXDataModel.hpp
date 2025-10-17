#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QtConcurrent/QtConcurrent>
#include <QAbstractScrollArea>
#include <opencv2/dnn.hpp>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "YoloDetectionONNXInterface.hpp"
// 添加ONNX Runtime头文件
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <algorithm>
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;

namespace Nodes
{
    class YoloDetectionONNXDataModel : public NodeDelegateModel
    {
        Q_OBJECT

        public:
        YoloDetectionONNXDataModel()
        {
            InPortCount =1;
            OutPortCount=2;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_outVariable=std::make_shared<VariableData>();
            m_outImage=std::make_shared<ImageData>();
            model_path="./plugins/Models/yolo11n-Detection.onnx";
            // model_path="./plugins/Models/AnimeGANv3_Hayao_36.onnx";

            NodeDelegateModel::registerOSCControl("/enable",widget->EnableBtn);
            NodeDelegateModel::registerOSCControl("/filter",widget->ClassSelectorComboBox);
            NodeDelegateModel::registerOSCControl("/confidence",widget->ConfidenceFilterSpinBox);

        }

        virtual ~YoloDetectionONNXDataModel() override{}

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                    return "IMAGE";
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
                return ImageData().type();
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
            case 0:
                m_inImage0=std::dynamic_pointer_cast<ImageData>(data);
                imageReasoning();
                break;
            case 1:
                m_inImage0=std::dynamic_pointer_cast<ImageData>(data);
                imageReasoning();
                break;
            }
        }
       /**
     * @brief 性能优化的ONNX Runtime图像推理函数
     * @details 使用缓存的会话和预分配内存，优化图像预处理流程，支持CUDA加速
     */
    void imageReasoning()
    {
        if (!m_inImage0 || !widget->EnableBtn->isChecked()) {
            m_outVariable = std::make_shared<VariableData>();
            emit dataUpdated(1);
            return;
        }

        try {
            // 获取输入图像
            cv::Mat inputImage = m_inImage0->imgMat();
            if (inputImage.empty()) {
                qDebug() << "输入图像为空";
                return;
            }

            // 初始化ONNX Runtime会话（仅在需要时）
            if (!initializeOnnxSession()) {
                return;
            }

            // 高效的图像预处理
            cv::resize(inputImage, m_resizedImage, m_modelInputSize, 0, 0, cv::INTER_LINEAR);

            // 优化的颜色空间转换和归一化
            cv::cvtColor(m_resizedImage, m_rgbImage, cv::COLOR_BGR2RGB);
            m_rgbImage.convertTo(m_rgbImage, CV_32F, 1.0 / 255.0);

            // 高效的HWC到CHW转换（使用OpenCV的split和merge）
            std::vector<cv::Mat> channels(3);
            cv::split(m_rgbImage, channels);

            // 直接复制到输入缓冲区
            const int channelSize = m_modelInputSize.height * m_modelInputSize.width;
            for (int c = 0; c < 3; ++c) {
                std::memcpy(m_inputBuffer.data() + c * channelSize,
                           channels[c].ptr<float>(),
                           channelSize * sizeof(float));
            }

            // 创建输入张量（复用内存）
            std::vector<int64_t> inputTensorShape = {1, 3, m_modelInputSize.height, m_modelInputSize.width};
            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                memoryInfo, m_inputBuffer.data(), m_inputBuffer.size(),
                inputTensorShape.data(), inputTensorShape.size());

            // 执行推理
            auto outputTensors = m_ortSession->Run(Ort::RunOptions{nullptr},
                                                 m_inputNames.data(), &inputTensor, 1,
                                                 m_outputNames.data(), m_outputNames.size());

            // 处理输出结果
            cv::Mat resultImage = postProcessOnnxResults(outputTensors, inputImage, m_modelInputSize);

            // 更新输出图像数据
            m_outImage = std::make_shared<ImageData>(resultImage);
            emit dataUpdated(0);

        } catch (const Ort::Exception& e) {
            qDebug() << "ONNX Runtime错误:" << e.what();
            m_isModelInitialized = false;  // 强制重新初始化
        } catch (const std::exception& e) {
            qDebug() << "推理错误:" << e.what();
        } catch (...) {
            qDebug() << "未知错误";
        }
    }


    /**
     * @brief 针对YOLOv11检测模型优化的输出结果处理函数，支持界面参数过滤
     * @param outputTensors ONNX Runtime的输出张量
     * @param originalImage 原始输入图像
     * @param inputSize 模型输入尺寸
     * @return 带有检测框标注的图像
     */
    cv::Mat postProcessOnnxResults(std::vector<Ort::Value>& outputTensors, const cv::Mat& originalImage, const cv::Size& inputSize)
    {
        cv::Mat resultImage = originalImage.clone();

        if (outputTensors.empty()) {
            qDebug() << "输出张量为空";
            return resultImage;
        }

        // 获取输出张量数据
        const float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

        // YOLOv11检测模型输出格式: [1, 84, 8400]
        // 84 = 4(bbox) + 80(classes)
        if (outputShape.size() != 3 || outputShape[0] != 1) {
            qDebug() << "不支持的输出张量形状";
            return resultImage;
        }

        const int numClasses = outputShape[1] - 4;  // 减去4个边界框坐标
        const int numBoxes = outputShape[2];

        // 从界面获取过滤参数
        const float confidenceThreshold = static_cast<float>(widget->ConfidenceFilterSpinBox->value());
        const int selectedClassId = widget->ClassSelectorComboBox->currentIndex();
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
                // 检查所有类别
                for (int c = 0; c < numClasses; ++c) {
                    const float classScore = classScoresPtr[c * numBoxes + i];
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
            // 构建空的检测结果
            QVariantMap detectionResults;
            QVariantList detectionsArray;
            detectionResults["default"] = detectionsArray;
            detectionResults["total_detections"] = 0;
            m_outVariable = std::make_shared<VariableData>(detectionResults);
            emit dataUpdated(1);
            return resultImage;
        }

        // NMS去重
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, scores, confidenceThreshold, nmsThreshold, indices);

        // 绘制检测结果
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
            const std::string className = (classId < classNames.size()) ? classNames[classId] : "Unknown";
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
            const std::string classText = "Class: " + classNames[selectedClassId];
            cv::putText(resultImage, classText, cv::Point(10, 90),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        }

        // 构建检测结果数据并输出到第二个端口
        QVariantMap detectionResults;
        QVariantList detectionsArray;

        for (const int idx : indices) {
            const cv::Rect& box = boxes[idx];
            const int classId = classIds[idx];
            const float confidence = scores[idx];
            const std::string className = (classId < classNames.size()) ? classNames[classId] : "Unknown";

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

        detectionResults["default"] = detectionsArray;
        // detectionResults["filter_confidence"] = confidenceThreshold;
        // detectionResults["filter_class_id"] = filterByClass ? selectedClassId : -1;
        // detectionResults["filter_class_name"] = filterByClass ? QString::fromStdString(classNames[selectedClassId]) : "All";
        detectionResults["total_detections"] = detectionCount;

        // 输出检测结果到第二个端口
        m_outVariable = std::make_shared<VariableData>(detectionResults);
        emit dataUpdated(1);

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
            modelJson1["confidence"] = widget->ConfidenceFilterSpinBox->value();
            
            // 保存类别选择索引
            modelJson1["classIndex"] = widget->ClassSelectorComboBox->currentIndex();
            
            // 保存启用状态
            modelJson1["enabled"] = widget->EnableBtn->isChecked();
            
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
                    double confidence = values["confidence"].toDouble(0.4);
                    widget->ConfidenceFilterSpinBox->setValue(confidence);
                }
                
                // 加载类别选择索引
                if (values.contains("classIndex")) {
                    int classIndex = values["classIndex"].toInt(0);
                    if (classIndex >= 0 && classIndex < widget->ClassSelectorComboBox->count()) {
                        widget->ClassSelectorComboBox->setCurrentIndex(classIndex);
                    }
                }
                
                // 加载启用状态
                if (values.contains("enabled")) {
                    bool enabled = values["enabled"].toBool(true);
                    widget->EnableBtn->setChecked(enabled);
                }
            }
        }

    /**
     * @brief 初始化ONNX Runtime会话
     * @details 创建并配置ONNX Runtime环境，支持CUDA加速，只在模型路径改变时重新初始化
     * @return 初始化是否成功
     */
    bool initializeOnnxSession()
    {
        try {
            // 检查是否需要重新初始化
            if (m_isModelInitialized && m_cachedModelPath == model_path) {
                return true;
            }

            // 验证模型文件
            QFileInfo modelFile(model_path);
            if (!modelFile.exists()) {
                qDebug() << "模型文件不存在:" << model_path;
                return false;
            }

            // 重置状态
            m_isModelInitialized = false;
            m_inputNames.clear();
            m_outputNames.clear();
            m_inputNamesPtr.clear();
            m_outputNamesPtr.clear();

            // 创建ONNX Runtime环境（只创建一次）
            if (!m_ortEnv) {
                m_ortEnv = std::make_unique<Ort::Env>(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "ImageONNX");
            }

            // 创建会话选项
            m_sessionOptions = std::make_unique<Ort::SessionOptions>();

            // 优化设置
            m_sessionOptions->SetInterOpNumThreads(std::thread::hardware_concurrency());
            m_sessionOptions->SetIntraOpNumThreads(std::thread::hardware_concurrency());
            m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            m_sessionOptions->SetExecutionMode(ExecutionMode::ORT_PARALLEL);

            // 配置CUDA（如果可用）
            m_useCuda = false;
            try {
                auto providers = Ort::GetAvailableProviders();
                for (const auto& provider : providers) {
                    if (provider == "CUDAExecutionProvider") {
                        OrtCUDAProviderOptions cuda_options{};
                        cuda_options.device_id = 0;
                        cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
                        cuda_options.arena_extend_strategy = 1;  // 启用内存池扩展
                        cuda_options.do_copy_in_default_stream = 1;
                        cuda_options.gpu_mem_limit = SIZE_MAX;  // 不限制GPU内存

                        m_sessionOptions->AppendExecutionProvider_CUDA(cuda_options);
                        m_useCuda = true;
                        qDebug() << "使用CUDA执行提供者";
                        break;
                    }
                }
            } catch (const std::exception& e) {
                qDebug() << "CUDA配置失败，使用CPU:" << e.what();
            }

            // 创建会话
            m_ortSession = std::make_unique<Ort::Session>(*m_ortEnv, model_path.toStdWString().c_str(), *m_sessionOptions);

            // 获取输入输出信息
            Ort::AllocatorWithDefaultOptions allocator;

            // 缓存输入信息
            size_t numInputNodes = m_ortSession->GetInputCount();
            for (size_t i = 0; i < numInputNodes; i++) {
                auto inputNamePtr = m_ortSession->GetInputNameAllocated(i, allocator);
                m_inputNames.push_back(inputNamePtr.get());
                m_inputNamesPtr.push_back(std::move(inputNamePtr));
            }

            // 缓存输出信息
            size_t numOutputNodes = m_ortSession->GetOutputCount();
            for (size_t i = 0; i < numOutputNodes; i++) {
                auto outputNamePtr = m_ortSession->GetOutputNameAllocated(i, allocator);
                m_outputNames.push_back(outputNamePtr.get());
                m_outputNamesPtr.push_back(std::move(outputNamePtr));
            }

            // 预分配输入缓冲区
            const size_t inputTensorSize = 1 * 3 * m_modelInputSize.height * m_modelInputSize.width;
            m_inputBuffer.resize(inputTensorSize);

            // 预分配图像缓冲区
            m_resizedImage = cv::Mat(m_modelInputSize, CV_8UC3);
            m_rgbImage = cv::Mat(m_modelInputSize, CV_32FC3);

            m_isModelInitialized = true;
            m_cachedModelPath = model_path;

            qDebug() << "ONNX Runtime会话初始化成功, CUDA:" << m_useCuda;
            return true;

        } catch (const Ort::Exception& e) {
            qDebug() << "ONNX Runtime初始化错误:" << e.what();
            return false;
        } catch (const std::exception& e) {
            qDebug() << "初始化错误:" << e.what();
            return false;
        }
    }

    

    private:
        QFutureWatcher<double>* m_watcher = nullptr;
        YoloDetectionONNXInterface *widget=new YoloDetectionONNXInterface();
        std::shared_ptr<ImageData> m_inImage0;
        // std::shared_ptr<ImageData> m_inImage1;
        std::shared_ptr<VariableData> m_outVariable;
        std::shared_ptr<ImageData> m_outImage;
        QString model_path;
         // ONNX Runtime缓存资源
        std::unique_ptr<Ort::Env> m_ortEnv;
        std::unique_ptr<Ort::Session> m_ortSession;
        std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
        std::vector<const char*> m_inputNames;
        std::vector<const char*> m_outputNames;
        std::vector<Ort::AllocatedStringPtr> m_inputNamesPtr;
        std::vector<Ort::AllocatedStringPtr> m_outputNamesPtr;
        
        // 预分配的内存缓冲区
        std::vector<float> m_inputBuffer;
        cv::Mat m_resizedImage;
        cv::Mat m_rgbImage;
        
        // 模型信息缓存
        cv::Size m_modelInputSize{640, 640};
        bool m_isModelInitialized = false;
        QString m_cachedModelPath;
        bool m_useCuda = false;
    };
}
