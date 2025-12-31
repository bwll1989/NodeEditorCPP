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
#include "StyleTransferONNXInterface.hpp"
// 添加ONNX Runtime头文件
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <algorithm>

#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;

namespace Nodes
{
    class YoloDetectionONNXDataModel : public AbstractDelegateModel
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
            model_path="./plugins/Models/AnimeGANv3_Hayao_36.onnx";

            AbstractDelegateModel::registerOSCControl("/enable",widget->EnableBtn);

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
            std::vector<int64_t> inputTensorShape = {1,m_modelInputSize.height, m_modelInputSize.width,3};
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
     * @brief 风格迁移模型的输出结果处理函数
     * @param outputTensors ONNX Runtime的输出张量
     * @param originalImage 原始输入图像
     * @param inputSize 模型输入尺寸
     * @return 风格迁移后的图像
     */
    cv::Mat postProcessOnnxResults(std::vector<Ort::Value>& outputTensors, const cv::Mat& originalImage, const cv::Size& inputSize)
    {
        if (outputTensors.empty()) {
            qDebug() << "输出张量为空";
            return originalImage;
        }

        try {
            // 获取输出张量数据
            const float* outputData = outputTensors[0].GetTensorMutableData<float>();
            auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

            // 风格迁移模型输出格式通常为: [1, 3, H, W] (CHW格式)
            if (outputShape.size() != 4 || outputShape[3] != 3 || outputShape[1]*outputShape[2] == 0) {
                qDebug() << "不支持的输出张量形状，期望 [1, 3, H, W]";
                return originalImage;
            }

            const int channels = static_cast<int>(outputShape[3]);
            const int height = static_cast<int>(outputShape[1]);
            const int width = static_cast<int>(outputShape[2]);
            const int channelSize = height * width;

            // 创建输出图像矩阵
            cv::Mat outputImage(height, width, CV_32FC3);
            
            // 从CHW格式转换为HWC格式
            std::vector<cv::Mat> channels_mat(3);
            for (int c = 0; c < 3; ++c) {
                channels_mat[c] = cv::Mat(height, width, CV_32F, 
                    const_cast<float*>(outputData + c * channelSize));
            }
            
            // 合并通道 (CHW -> HWC)
            cv::merge(channels_mat, outputImage);
            
            // 反归一化：从[0,1]范围转换到[0,255]
            outputImage *= 255.0f;
            
            // 确保像素值在有效范围内
            cv::threshold(outputImage, outputImage, 255.0, 255.0, cv::THRESH_TRUNC);
            cv::threshold(outputImage, outputImage, 0.0, 0.0, cv::THRESH_TOZERO);
            
            // 转换为8位图像
            cv::Mat result;
            outputImage.convertTo(result, CV_8UC3);
            
            // 如果输出尺寸与原图不同，调整到原图尺寸
            if (result.size() != originalImage.size()) {
                cv::resize(result, result, originalImage.size(), 0, 0, cv::INTER_LINEAR);
            }
            
            // 转换颜色空间 (RGB -> BGR，因为OpenCV使用BGR)
            cv::cvtColor(result, result, cv::COLOR_RGB2BGR);
            
            // 构建风格迁移结果信息并输出到第二个端口
            QVariantMap transferResults;
            transferResults["status"] = "success";
            transferResults["input_size"] = QString("%1x%2").arg(originalImage.cols).arg(originalImage.rows);
            transferResults["output_size"] = QString("%1x%2").arg(result.cols).arg(result.rows);
            transferResults["model_input_size"] = QString("%1x%2").arg(inputSize.width).arg(inputSize.height);
            transferResults["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            
            // 输出处理结果到第二个端口
            m_outVariable = std::make_shared<VariableData>(transferResults);
            emit dataUpdated(1);
            
            return result;
            
        } catch (const std::exception& e) {
            qDebug() << "风格迁移后处理错误:" << e.what();
            
            // 构建错误信息
            QVariantMap errorResults;
            errorResults["status"] = "error";
            errorResults["error_message"] = QString::fromStdString(e.what());
            errorResults["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            
            m_outVariable = std::make_shared<VariableData>(errorResults);
            emit dataUpdated(1);
            
            return originalImage;
        }
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
        cv::Size m_modelInputSize{720, 720};
        bool m_isModelInitialized = false;
        QString m_cachedModelPath;
        bool m_useCuda = false;
    };
}
