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
#include "YoloPoseONNXInterface.hpp"
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
    class YoloPoseONNXDataModel : public NodeDelegateModel
    {
        Q_OBJECT

        public:
        YoloPoseONNXDataModel()
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
            model_path="./plugins/Models/yolo11n-pose.onnx";
            // model_path="./plugins/Models/AnimeGANv3_Hayao_36.onnx";

            // NodeDelegateModel::registerOSCControl("/method",widget->methodEdit);

        }

        virtual ~YoloPoseONNXDataModel() override{}

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
        if (!m_inImage0) {
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
     * @brief 性能优化的ONNX Runtime输出结果处理函数
     * @param outputTensors ONNX Runtime的输出张量
     * @param originalImage 原始输入图像
     * @param inputSize 模型输入尺寸
     * @return 带有姿态标注的图像
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

        // 预定义常量，避免重复计算
        static const std::vector<std::pair<int, int>> skeleton = {
            {16, 14}, {14, 12}, {17, 15}, {15, 13}, {12, 13},
            {6, 12}, {7, 13}, {6, 7}, {6, 8}, {7, 9},
            {8, 10}, {9, 11}, {2, 3}, {1, 2}, {1, 3},
            {2, 4}, {3, 5}, {4, 6}, {5, 7}
        };

        static const std::vector<cv::Scalar> keypointColors = {
            cv::Scalar(255, 0, 0), cv::Scalar(255, 85, 0), cv::Scalar(255, 170, 0),
            cv::Scalar(255, 255, 0), cv::Scalar(170, 255, 0), cv::Scalar(85, 255, 0),
            cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 85), cv::Scalar(0, 255, 170),
            cv::Scalar(0, 255, 255), cv::Scalar(0, 170, 255), cv::Scalar(0, 85, 255),
            cv::Scalar(0, 0, 255), cv::Scalar(85, 0, 255), cv::Scalar(170, 0, 255),
            cv::Scalar(255, 0, 255), cv::Scalar(255, 0, 170)
        };

        // 预计算缩放比例和图像边界
        const float scaleX = static_cast<float>(originalImage.cols) / inputSize.width;
        const float scaleY = static_cast<float>(originalImage.rows) / inputSize.height;
        const float imgWidth = static_cast<float>(originalImage.cols);
        const float imgHeight = static_cast<float>(originalImage.rows);

        // 常量阈值
        constexpr float confidenceThreshold = 0.25f;
        constexpr float nmsThreshold = 0.45f;
        constexpr float keypointThreshold = 0.2f;
        constexpr int numKeypoints = 17;

        if (outputShape.size() < 2) {
            return resultImage;
        }

        const int numBoxes = outputShape[2];
        const int numValues = outputShape[1];
        const int totalElements = numValues * numBoxes;

        // 预分配容器，避免动态扩容
        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        std::vector<std::array<cv::Point2f, numKeypoints>> allKeypoints;
        std::vector<std::array<float, numKeypoints>> allKeypointConfs;
        
        boxes.reserve(numBoxes / 10);  // 预估10%的框会通过置信度筛选
        scores.reserve(numBoxes / 10);
        allKeypoints.reserve(numBoxes / 10);
        allKeypointConfs.reserve(numBoxes / 10);

        // 预计算数据指针偏移，避免重复计算
        const float* centerXPtr = outputData;
        const float* centerYPtr = outputData + numBoxes;
        const float* widthPtr = outputData + 2 * numBoxes;
        const float* heightPtr = outputData + 3 * numBoxes;
        const float* confidencePtr = outputData + 4 * numBoxes;
        const float* keypointsPtr = outputData + 5 * numBoxes;

        // 主循环优化：减少分支预测失败
        for (int i = 0; i < numBoxes; ++i) {
            const float confidence = confidencePtr[i];
            
            if (confidence <= confidenceThreshold) continue;

            // 批量读取边界框数据
            const float centerX = centerXPtr[i];
            const float centerY = centerYPtr[i];
            const float width = widthPtr[i];
            const float height = heightPtr[i];

            // 计算边界框坐标
            const float halfWidth = width * 0.5f;
            const float halfHeight = height * 0.5f;
            
            float x1 = (centerX - halfWidth) * scaleX;
            float y1 = (centerY - halfHeight) * scaleY;
            float x2 = (centerX + halfWidth) * scaleX;
            float y2 = (centerY + halfHeight) * scaleY;

            // 优化的边界检查
            x1 = std::clamp(x1, 0.0f, imgWidth);
            y1 = std::clamp(y1, 0.0f, imgHeight);
            x2 = std::clamp(x2, 0.0f, imgWidth);
            y2 = std::clamp(y2, 0.0f, imgHeight);

            if (x2 <= x1 || y2 <= y1) continue;

            // 使用数组而非vector，减少内存分配
            std::array<cv::Point2f, numKeypoints> keypoints;
            std::array<float, numKeypoints> keypointConfs;
            
            // 批量处理关键点
            bool hasValidKeypoints = false;
            for (int k = 0; k < numKeypoints; ++k) {
                const int baseIdx = (k * 3) * numBoxes + i;
                
                if (baseIdx + 2 * numBoxes < totalElements) {
                    const float kpX = keypointsPtr[baseIdx] * scaleX;
                    const float kpY = keypointsPtr[baseIdx + numBoxes] * scaleY;
                    const float kpConf = keypointsPtr[baseIdx + 2 * numBoxes];

                    if (kpConf > keypointThreshold) {
                        keypoints[k] = cv::Point2f(
                            std::clamp(kpX, 0.0f, imgWidth),
                            std::clamp(kpY, 0.0f, imgHeight)
                        );
                        keypointConfs[k] = kpConf;
                        hasValidKeypoints = true;
                    } else {
                        keypoints[k] = cv::Point2f(-1, -1);
                        keypointConfs[k] = 0.0f;
                    }
                } else {
                    keypoints[k] = cv::Point2f(-1, -1);
                    keypointConfs[k] = 0.0f;
                }
            }

            // 只保存有效检测
            if (hasValidKeypoints) {
                boxes.emplace_back(cv::Point(x1, y1), cv::Point(x2, y2));
                scores.push_back(confidence);
                allKeypoints.push_back(std::move(keypoints));
                allKeypointConfs.push_back(std::move(keypointConfs));
            }
        }

        if (boxes.empty()) {
            return resultImage;
        }

        // NMS去重
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, scores, confidenceThreshold, nmsThreshold, indices);

        // 优化绘制：减少字符串操作
        const int numDetections = indices.size();
        
        for (const int idx : indices) {
            const cv::Rect& box = boxes[idx];
            const auto& keypoints = allKeypoints[idx];
            const auto& keypointConfs = allKeypointConfs[idx];

            // 绘制边界框
            cv::rectangle(resultImage, box, cv::Scalar(0, 255, 0), 2);

            // 优化文本绘制：减少格式化开销
            const std::string confText = "Person " + std::to_string(static_cast<int>(scores[idx] * 100)) + "%";
            cv::putText(resultImage, confText, cv::Point(box.x, box.y - 10),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);

            // 绘制关键点（只绘制高置信度的）
            for (int k = 0; k < numKeypoints; ++k) {
                const cv::Point2f& kp = keypoints[k];
                if (kp.x > 0 && kp.y > 0 && keypointConfs[k] > keypointThreshold) {
                    cv::circle(resultImage, cv::Point(kp.x, kp.y), 4, keypointColors[k], -1);
                }
            }

            // // 绘制骨架（批量处理）
            // for (const auto& bone : skeleton) {
            //     const int kp1 = bone.first - 1;
            //     const int kp2 = bone.second - 1;

            //     const cv::Point2f& pt1 = keypoints[kp1];
            //     const cv::Point2f& pt2 = keypoints[kp2];

            //     if (pt1.x > 0 && pt1.y > 0 && pt2.x > 0 && pt2.y > 0 &&
            //         keypointConfs[kp1] > keypointThreshold && keypointConfs[kp2] > keypointThreshold) {
            //         cv::line(resultImage, cv::Point(pt1.x, pt1.y), cv::Point(pt2.x, pt2.y),
            //                 cv::Scalar(0, 255, 255), 2);
            //     }
            // }
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
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
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
        YoloPoseONNXInterface *widget=new YoloPoseONNXInterface();
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