#pragma once
#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/qglobal.h>
#include <cstdio>
#include <iostream>
#if WIN32
#include "windows.h"
#include <wtypes.h>
#endif
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "pluginterfaces/gui/iplugview.h"
#include <pluginterfaces/vst/ivstcomponent.h>
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "base/source/fdebug.h"

#include "VST3PluginInterface.hpp"
#include "Container.h"
#include "VST3AudioProcessingThread.hpp"  // 添加多线程处理类
#include <memory>
#include "ParameterChanges.hpp"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "Vst3DataStream.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Steinberg;
using namespace VST3;
using namespace NodeDataTypes;

// 改进的音频处理数据结构
struct AudioProcessingData {
    Steinberg::Vst::ProcessData vstData;
    std::vector<Steinberg::Vst::AudioBusBuffers> vstInput;
    std::vector<Steinberg::Vst::AudioBusBuffers> vstOutput;

    // 支持双精度和单精度
    std::vector<std::vector<float>> floatBuffers;
    std::vector<std::vector<double>> doubleBuffers;

    // 音频总线信息
    std::vector<int> inputChannelCounts;
    std::vector<int> outputChannelCounts;
    int totalInputChannels = 0;
    int totalOutputChannels = 0;

    // 处理精度标志
    bool useDoubleProcessing = false;
};


// 改进的参数管理
struct ParameterManager {
    std::map<Vst::ParamID, Vst::ParamValue> currentValues;
    std::map<Vst::ParamID, std::unique_ptr<ParamValueQueue>> paramQueues;
    ParameterChanges inputChanges;
    ParameterChanges outputChanges;
};

namespace Nodes
{
    class VST3PluginDataModel : public NodeDelegateModel
    {
        Q_OBJECT
    public:
        /**
         * @brief 构造函数，初始化VST3插件节点
         */
        VST3PluginDataModel(const QString& path);
        
        /**
         * @brief 析构函数，清理VST3插件资源
         */
        ~VST3PluginDataModel();
        
    public slots:
        /**
         * @brief 显示VST3插件控制界面
         */
        void showController();

    public:
        /**
         * @brief 获取端口数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

        /**
         * @brief 获取输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override;
        
        /**
         * @brief 设置输入数据并进行VST3音频处理
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override;


        /**
         * @brief 保存节点状态
         */
        QJsonObject save() const override;
        
        /**
         * @brief 加载节点状态
         */
        void load(const QJsonObject &p) override;
        
        QWidget *embeddedWidget() override ;

    private:
        /**
         * @brief 加载VST3插件
         * @param pluginPath 插件文件路径
         */
        void loadPlugin(const QString& pluginPath) ;
        
        /**
         * @brief 初始化VST3音频处理
         */
        void initializeAudioProcessing();
            

        /**
         * @brief 验证VST3接口的完整性
         */
        bool validateVST3Interfaces() ;

        void processAudioData();
        /**
         * @brief 设置音频总线配置
         */
        void setupAudioBuses();

        /**
         * @brief 初始化音频缓冲区
         */
        void initializeAudioBuffers();

        /**
         * @brief 双精度音频处理
         */
        void processAudioDouble(const AudioFrame& inputFrame);

        /**
         * @brief 单精度音频处理
         */
        void processAudioFloat(const AudioFrame& inputFrame);
        /**
         * @brief 读取VST3处理器状态
         * @return 序列化的处理器状态数据
         */
        QByteArray readProcessorState() const;

        /**
         * @brief 读取VST3控制器状态
         * @return 序列化的控制器状态数据
         */
        QByteArray readControllerState() const;

        /**
         * @brief 将保存的状态写入VST3插件
         */
        void writeState();
    private:
        VST3::Hosting::Module::Ptr module_ = nullptr;
        Steinberg::IPtr<Steinberg::Vst::PlugProvider> plugProvider_ = nullptr;
        Steinberg::IPtr<Steinberg::Vst::IComponent> vstPlug_ = nullptr;
        Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioEffect_ = nullptr;
        Steinberg::IPtr<Steinberg::Vst::IEditController> editController_ = nullptr;
        Steinberg::IPtr<Steinberg::IPlugView> view = nullptr;
        VST3PluginInterface* widget;
        Steinberg::IPtr<Steinberg::Vst::HostApplication> pluginContext_ = nullptr;
        std::shared_ptr<Container> window;
        QVariantMap pluginInfo_;
        // 音频处理相关
        // 移除原有的定时器相关成员
        // QTimer* audioProcessTimer_;  // 删除这行
        
        // 添加多线程处理成员
        std::unique_ptr<VST3AudioProcessingThread> audioProcessingThread_;
        
        std::shared_ptr<AudioTimestampRingQueue> outputAudioBuffer_;
        std::shared_ptr<AudioTimestampRingQueue> inputAudioBuffer_;
        int inputConsumerId_ = -1;
        double sampleRate_;
        int blockSize_;
        ParameterChanges inputParameterChanges_;
        ParameterChanges outputParameterChanges_;
        std::map<Vst::ParamID, Vst::ParamValue> currentParameterValues_;
        AudioProcessingData audioProcessingData_;
        ParameterManager parameterManager_;
        QByteArray savedProcessorState_;     // 保存的处理器状态
        QByteArray savedControllerState_;    // 保存的控制器状态
        QString pluginUID_;                  // 插件唯一标识
    };
}