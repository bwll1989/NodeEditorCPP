#pragma once

#include "NodeDataList.hpp"
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
#include "FaceDetectionInterface.hpp"
// 添加ONNX Runtime头文件
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <algorithm>
#include "Common/BaseClass/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;
namespace Nodes
{
    class FaceDetectionDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        public:
        FaceDetectionDataModel()
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
            model_path="./plugins/Models/yolo11n-face-detection.onnx";

        }

        /**
         * @brief 安全析构：取消异步推理并释放ONNX资源
         */
        ~FaceDetectionDataModel() override{
            cancelPendingInference();
            m_ortSession.reset();
            m_sessionOptions.reset();
            m_ortEnv.reset();
        }

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
                if (portIndex==0)
                    return ImageData().type();
                else
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
            case 0:
                m_inImage0=std::dynamic_pointer_cast<ImageData>(data);
                {
                    if (!m_inferenceWatcher) {
                        m_inferenceWatcher = new QFutureWatcher<void>(this);
                    }
                    if (m_inferenceWatcher->isRunning()) {
                        return;
                    }
                    cv::Mat matCopy = m_inImage0 ? m_inImage0->imgMat() : cv::Mat();
                    auto future = QtConcurrent::run([this, matCopy](){
                        runInferenceOnImage(matCopy);
                    });
                    m_inferenceWatcher->setFuture(future);
                }
                break;
            case 1:

                break;
            }
        }
       /**
     * @brief 性能优化的ONNX Runtime图像推理函数
     * @details 使用缓存的会话和预分配内存，优化图像预处理流程，支持CUDA加速
     */
    /**
     * @brief 异步推理入口（保持兼容的同步接口，不在UI线程直接调用）
     */
    void imageReasoning()
    {
        if (!m_inImage0) return;
        if (!m_inferenceWatcher) {
            m_inferenceWatcher = new QFutureWatcher<void>(this);
        }
        if (m_inferenceWatcher->isRunning()) {
            return;
        }
        cv::Mat matCopy = m_inImage0->imgMat();
        auto future = QtConcurrent::run([this, matCopy](){
            runInferenceOnImage(matCopy);
        });
        m_inferenceWatcher->setFuture(future);
    }

    /**
     * @brief 在工作线程中执行推理与后处理，并在主线程更新输出
     * @param inputImage 输入图像副本
     */
    void runInferenceOnImage(cv::Mat inputImage)
    {
        if (inputImage.empty()) return;
        if (m_cancelRequested.load()) return;
        try {
            if (!initializeOnnxSession()) {
                return;
            }
            if (m_cancelRequested.load()) return;
            cv::resize(inputImage, m_resizedImage, m_modelInputSize, 0, 0, cv::INTER_LINEAR);
            cv::cvtColor(m_resizedImage, m_rgbImage, cv::COLOR_BGR2RGB);
            m_rgbImage.convertTo(m_rgbImage, CV_32F, 1.0 / 255.0);
            std::vector<cv::Mat> channels(3);
            cv::split(m_rgbImage, channels);
            const int channelSize = m_modelInputSize.height * m_modelInputSize.width;
            for (int c = 0; c < 3; ++c) {
                std::memcpy(m_inputBuffer.data() + c * channelSize,
                            channels[c].ptr<float>(),
                            channelSize * sizeof(float));
            }
            if (m_cancelRequested.load()) return;
            std::vector<int64_t> inputTensorShape = {1, 3, m_modelInputSize.height, m_modelInputSize.width};
            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                memoryInfo, m_inputBuffer.data(), m_inputBuffer.size(),
                inputTensorShape.data(), inputTensorShape.size());
            auto outputTensors = m_ortSession->Run(Ort::RunOptions{nullptr},
                                                   m_inputNames.data(), &inputTensor, 1,
                                                   m_outputNames.data(), m_outputNames.size());
            if (widget) {
                m_confThreshold = widget->confSpin->value();
                m_nmsThreshold = widget->nmsSpin->value();
            }
            if (m_cancelRequested.load()) return;
            cv::Mat resultImage = postProcessOnnxResults(outputTensors, inputImage, m_modelInputSize);
            QMetaObject::invokeMethod(this, [this, resultImage](){
                m_outImage = std::make_shared<ImageData>(resultImage);
                emit dataUpdated(0);
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
    void cancelPendingInference(){
        m_cancelRequested.store(true);
        if (m_inferenceWatcher) {
            auto f = m_inferenceWatcher->future();
            if (f.isRunning()) {
                f.cancel();
                f.waitForFinished();
            }
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

        // 预计算缩放比例和图像边界
        const float scaleX = static_cast<float>(originalImage.cols) / inputSize.width;
        const float scaleY = static_cast<float>(originalImage.rows) / inputSize.height;
        const float imgWidth = static_cast<float>(originalImage.cols);
        const float imgHeight = static_cast<float>(originalImage.rows);

        const float confidenceThreshold = static_cast<float>(m_confThreshold);
        const float nmsThreshold = static_cast<float>(m_nmsThreshold);

        if (outputShape.size() < 2) {
            return resultImage;
        }

        const int numBoxes = outputShape[2];
        const int numValues = outputShape[1];
        const int totalElements = numValues * numBoxes;

        // 预分配容器，避免动态扩容
        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        std::vector<int> classIds;

        boxes.reserve(numBoxes / 10);  // 预估10%的框会通过置信度筛选
        scores.reserve(numBoxes / 10);
        classIds.reserve(numBoxes / 10);

        // 预计算数据指针偏移，避免重复计算
        const float* centerXPtr = outputData;
        const float* centerYPtr = outputData + numBoxes;
        const float* widthPtr = outputData + 2 * numBoxes;
        const float* heightPtr = outputData + 3 * numBoxes;
        const float* confidencePtr = outputData + 4 * numBoxes;
        const int classesPerBox = std::max(0, numValues - 5);
        const float* classesPtr = classesPerBox > 0 ? (outputData + 5 * numBoxes) : nullptr;

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

            int classId = 0;
            float classScore = confidence;
            if (classesPerBox > 0 && classesPtr) {
                // 取最大类别
                const float* clsBase = classesPtr + i;
                classScore = -1.0f;
                for (int c = 0; c < classesPerBox; ++c) {
                    float s = clsBase[c * numBoxes];
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

            // 绘制边界框
            cv::rectangle(resultImage, box, cv::Scalar(0, 255, 0), 2);

            // 优化文本绘制：减少格式化开销
            const std::string confText = "Face " + std::to_string(static_cast<int>(scores[idx] * 100)) + "%";
            cv::putText(resultImage, confText, cv::Point(box.x, box.y - 10),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        }

        // 构建检测结果并输出到第二端口
        QVariantList detections;
        for (const int idx : indices) {
            const cv::Rect& box = boxes[idx];

            QVariantList boxList;
            boxList << box.x << box.y << (box.x + box.width) << (box.y + box.height);

            QVariantMap det;
            det["score"] = scores[idx];
            det["box"] = boxList;
            det["classId"] = (classIds.size() > idx ? classIds[idx] : 0);
            det["className"] = "face";
            detections << det;
        }
        QVariantMap outMap;
        outMap["count"] = static_cast<int>(indices.size());
        outMap["detections"] = detections;
        outMap["width"] = originalImage.cols;
        outMap["height"] = originalImage.rows;
        m_outVariable = std::make_shared<VariableData>(outMap);
        emit dataUpdated(1);
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
        QFutureWatcher<void>* m_inferenceWatcher = nullptr;
        FaceDetectionInterface *widget=new FaceDetectionInterface();
        std::shared_ptr<ImageData> m_inImage0;
        // std::shared_ptr<ImageData> m_inImage1;
        std::shared_ptr<VariableData> m_outVariable;
        std::shared_ptr<ImageData> m_outImage;
        QString model_path;
        double m_confThreshold = 0.25;
        double m_nmsThreshold = 0.45;
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
        std::atomic<bool> m_cancelRequested{false};
    };
}
