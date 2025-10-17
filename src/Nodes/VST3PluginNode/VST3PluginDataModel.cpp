#include "VST3PluginDataModel.hpp"

#include "QtNodes/internal/NodeState.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Steinberg;
using namespace VST3;
using namespace Nodes;
using namespace NodeDataTypes;


VST3PluginDataModel::VST3PluginDataModel(const QString& path){

    CaptionVisible = true;
    QFileInfo fileinfo(path);
    Caption = fileinfo.baseName();
    WidgetEmbeddable = true;
    Resizable = false;

    widget = new VST3PluginInterface();

    // 初始化音频处理参数
    sampleRate_ = 48000;
    blockSize_ = sampleRate_/TimestampGenerator::getInstance()->getFrameRate();

    // 创建音频处理线程
    audioProcessingThread_ = std::make_unique<VST3AudioProcessingThread>(this);
    
    // 设置音频参数
    audioProcessingThread_->setAudioParameters(sampleRate_, blockSize_);
    // qDebug() << "Audio processing thread started";
    connect(widget->ShowController, &QPushButton::clicked, this, &VST3PluginDataModel::showController);
    loadPlugin(path);
}

/**
 * @brief 析构函数，清理VST3插件资源
 */
VST3PluginDataModel::~VST3PluginDataModel(){
    // qDebug() << "VST3PluginDataModel destructor started";
    
    // 1. 首先停止音频处理线程
    if (audioProcessingThread_) {
        if (audioProcessingThread_->isRunning()) {
            audioProcessingThread_->stopProcessing();
            
            // 等待线程正常退出
            if (!audioProcessingThread_->wait(1000)) {
                qWarning() << "Audio processing thread did not exit gracefully, terminating...";
                audioProcessingThread_->terminate();
                audioProcessingThread_->wait(1000);
            }
        }
        
        // 重置智能指针
        audioProcessingThread_.reset();
    }

    // 2. 关闭界面窗口
    if (window) {
        window->close();
        window.reset();
    }

    // 3. 安全清理VST3插件资源（线程停止后再清理）
    if (audioEffect_) {
        audioEffect_->setProcessing(false);
        audioEffect_ = nullptr;
    }

    if (vstPlug_) {
        vstPlug_->setActive(false);
        vstPlug_->terminate();
        vstPlug_ = nullptr;
    }

    if (editController_) {
        editController_->terminate();
        editController_ = nullptr;
    }

    if (view) {
        view = nullptr;
    }

    plugProvider_ = nullptr;
    module_ = nullptr;
    pluginContext_ = nullptr;
    
    // qDebug() << "VST3PluginDataModel destructor completed";
}

/**
 * @brief 显示VST3插件控制界面
 */
void VST3PluginDataModel::showController()
{
    if (window && window->isVisible()) {
        window->close();
        return;
    }

    if (editController_) {
        view = owned(editController_->createView(Vst::ViewType::kEditor));
        if (view) {
            view->addRef();
            window = std::make_shared<Container>(view);
            window->setWindowTitle(module_->getFactory().info().vendor().c_str());
            window->show();
        }
    }
}

/**
 * @brief 获取端口数据类型
 */
NodeDataType VST3PluginDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        return AudioData().type();
    case PortType::Out:

        return AudioData().type();

    case PortType::None:
        break;
    default:
        break;
    }
    return AudioData().type();
}

/**
 * @brief 获取输出数据
 */
std::shared_ptr<NodeData> VST3PluginDataModel::outData(PortIndex const portIndex)
{
    auto audioData = std::make_shared<AudioData>();
    audioData->setSharedAudioBuffer( audioProcessingThread_->getOutputAudioBuffers(portIndex));
    return audioData;
}

/**
 * @brief 设置输入数据并进行VST3音频处理
 */
void VST3PluginDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
{
    auto audioData = std::dynamic_pointer_cast<AudioData>(data);
    if (audioData && audioData->isConnectedToSharedBuffer())
    {
        // 获取输入音频缓冲区
        auto inputAudioBuffer_ = audioData->getSharedAudioBuffer();
        if (!inputAudioBuffer_) {
            return;
        }
        // 设置音频处理线程的缓冲区和VST3组件
        if (audioProcessingThread_) {

            audioProcessingThread_->setInputAudioBuffers(portIndex,inputAudioBuffer_);


        }
    }
    else
    {
        // 断开连接：停止音频处理线程
        if (audioProcessingThread_) {
            audioProcessingThread_->setInputAudioBuffers(portIndex,nullptr);
        }

    }
}


/**
 * @brief 保存节点状态（改进版）
 */
QJsonObject VST3PluginDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    
    // 保存插件路径和状态
    if (!pluginInfo_.isEmpty()) {
        values["PluginPath"] = pluginInfo_["Plugin Path"].toString();
        values["PluginUID"] = pluginUID_;
    }
    
    // 保存插件状态（Base64编码）
    if (vstPlug_ || !savedProcessorState_.isEmpty()) {
        values["ProcessorState"] = QString::fromUtf8(readProcessorState().toBase64());
    }
    
    if (editController_ || !savedControllerState_.isEmpty()) {
        values["ControllerState"] = QString::fromUtf8(readControllerState().toBase64());
    }
    
    // 保存当前参数值
    if (!currentParameterValues_.empty()) {
        QJsonObject params;
        for (const auto& [paramId, value] : currentParameterValues_) {
            params[QString::number(paramId)] = value;
        }
        values["Parameters"] = params;
    }
    
    modelJson["values"] = values;
    return modelJson;
}

/**
 * @brief 加载节点状态（改进版）
 */
void VST3PluginDataModel::load(const QJsonObject &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined() && v.isObject()) {
        QJsonObject values = v.toObject();
        
        // 恢复插件路径
        if (values.contains("PluginPath")) {
            QString pluginPath = values["PluginPath"].toString();
            pluginUID_ = values["PluginUID"].toString();
            
            // 恢复状态数据
            if (values.contains("ProcessorState")) {
                savedProcessorState_ = QByteArray::fromBase64(
                    values["ProcessorState"].toString().toUtf8());
            }
            
            if (values.contains("ControllerState")) {
                savedControllerState_ = QByteArray::fromBase64(
                    values["ControllerState"].toString().toUtf8());
            }
            
            // 恢复参数值
            if (values.contains("Parameters")) {
                QJsonObject params = values["Parameters"].toObject();
                for (auto it = params.begin(); it != params.end(); ++it) {
                    Vst::ParamID paramId = it.key().toUInt();
                    Vst::ParamValue value = it.value().toDouble();
                    currentParameterValues_[paramId] = value;
                    
                    // 应用到插件
                    if (editController_) {
                        editController_->setParamNormalized(paramId, value);
                    }
                }
            }
        }
    }
}
        
QWidget* VST3PluginDataModel::embeddedWidget()  { return widget; }


void VST3PluginDataModel::loadPlugin(const QString& pluginPath) {
    // 2. 关闭界面窗口
    if (window && window->isVisible()) {
        window->close();
    }

    // 3. 安全地清理现有资源 - 按正确顺序
    if (audioEffect_) {
        audioEffect_->setProcessing(false);
        audioEffect_ = nullptr;  // 立即置空
    }

    if (vstPlug_) {
        vstPlug_->setActive(false);
        vstPlug_ = nullptr;  // 立即置空
    }

    if (editController_) {
        editController_ = nullptr;
    }

    // 4. 清理视图和窗口
    view.reset();
    window.reset();

    // 5. 清理提供者和模块 - 最后清理
    plugProvider_.reset();
    module_.reset();

    // 6. 重新创建插件上下文（如果需要）
    if (!pluginContext_) {
        pluginContext_ = owned(NEW Steinberg::Vst::HostApplication());
        Vst::PluginContextFactory::instance().setPluginContext(pluginContext_);
    }

    // 7. 加载新插件
    std::string error;
    module_ = VST3::Hosting::Module::create(pluginPath.toStdString(), error);
    if (!module_) {
        qWarning() << "Failed to load module from path:" << QString::fromStdString(error);
        updateNodeState(QtNodes::NodeValidationState::State::Error ,"Failed to load module from path");
        return;
    }

    const auto& factory = module_->getFactory();

    // 8. 更新插件信息
    pluginInfo_["Plugin Path"] = pluginPath;
    auto factoryInfo = module_->getFactory().info();
    pluginInfo_["Vendor"] = QString::fromStdString(factoryInfo.get().vendor);
    pluginInfo_["Version"] = QString::fromStdString(factoryInfo.get().url);
    pluginInfo_["email"] = QString::fromStdString(factoryInfo.get().email);

    // 9. 创建插件提供者
    for (auto& classInfo : factory.classInfos()) {
        if (classInfo.category() == kVstAudioEffectClass) {
            plugProvider_ = owned(new Steinberg::Vst::PlugProvider(factory, classInfo, true));
            if (!plugProvider_->initialize()) {
                qWarning() << "Failed to initialize plugin provider";
                updateNodeState(QtNodes::NodeValidationState::State::Error ,"Failed to initialize plugin provider");
                plugProvider_ = nullptr;
                return;
            }
            break;
        }
    }

    if (!plugProvider_) {
        qWarning() << "Failed to create plugin provider";
        updateNodeState(QtNodes::NodeValidationState::State::Error ,"Failed to create plugin provider");
        return;
    }

    // 10. 获取接口
    editController_ = plugProvider_->getController();
    vstPlug_ = plugProvider_->getComponent();
    audioEffect_ = FUnknownPtr<Steinberg::Vst::IAudioProcessor>(vstPlug_);

    if (!audioEffect_) {
        qWarning() << "Failed to get audio processor interface";
        updateNodeState(QtNodes::NodeValidationState::State::Error ,"Failed to get audio processor interface");
        return;
    }
    // 验证接口
    if (!validateVST3Interfaces()) {
        qWarning() << "VST3 interface validation failed";
        updateNodeState(QtNodes::NodeValidationState::State::Error ,"VST3 interface validation failed");
        return;
    }
    updateNodeState();
    // 11. 初始化音频处理（重要！）
    initializeAudioProcessing();

    // 12. 更新UI状态
    if (widget) {
        widget->ShowController->setEnabled(editController_ != nullptr);
    }

    // 13. 重新启动音频处理定时器
    // if (audioProcessTimer_) {
    //     audioProcessTimer_->start();
    // }
    
    // 在插件加载完成后恢复状态
    if (validateVST3Interfaces()) {
        initializeAudioProcessing();
        
        // 恢复保存的状态
        writeState();
        
        // 更新UI状态
        if (widget) {
            widget->ShowController->setEnabled(editController_ != nullptr);
        }
    }
    audioProcessingThread_->setVST3Components(audioEffect_, &audioProcessingData_);

    // 启动音频处理线程
    if (!audioProcessingThread_->isRunning())
        audioProcessingThread_->startProcessing();
}
        
/**
 * @brief 改进的VST3音频处理初始化，支持多总线和双精度
 */
void VST3PluginDataModel::initializeAudioProcessing()
{
    if (!audioEffect_ || !vstPlug_) {
        qWarning() << "VST3 components not initialized";
        return;
    }

    // 1. 检查处理精度支持
    bool supportsDouble = (audioEffect_->canProcessSampleSize(Vst::kSample64) == kResultOk);
    bool supportsFloat = (audioEffect_->canProcessSampleSize(Vst::kSample32) == kResultOk);
    
    if (!supportsFloat && !supportsDouble) {
        qWarning() << "Plugin doesn't support any compatible sample format";
        return;
    }
    
    // 优先使用双精度以获得更好的音质
    audioProcessingData_.useDoubleProcessing = supportsDouble;
    // 2. 配置处理设置
    Vst::ProcessSetup processSetup;
    processSetup.processMode = Vst::kRealtime;
    processSetup.symbolicSampleSize = audioProcessingData_.useDoubleProcessing ? 
                                      Vst::kSample64 : Vst::kSample32;
    processSetup.maxSamplesPerBlock = blockSize_;
    processSetup.sampleRate = sampleRate_;

    tresult result = audioEffect_->setupProcessing(processSetup);
    if (result != kResultOk) {
        qWarning() << "Failed to setup processing, error code:" << QString::number(result, 16);
        return;
    }

    // 3. 获取并配置音频总线信息
    setupAudioBuses();
    
    // 4. 初始化音频缓冲区
    initializeAudioBuffers();
    
    // 5. 激活组件
    result = vstPlug_->setActive(true);
    if (result != kResultOk) {
        qWarning() << "Failed to activate component, error code:" << QString::number(result, 16);
        return;
    }

    // 6. 开始音频处理
    result = audioEffect_->setProcessing(true);
    if (result != kResultOk) {
        qWarning() << "Failed to start audio processing, error code:" << QString::number(result, 16);
        vstPlug_->setActive(false);
        return;
    }
}

/**
 * @brief 设置音频总线配置
 */
void VST3PluginDataModel::setupAudioBuses()
{
    // 获取总线数量
    int32 numInputBuses = vstPlug_->getBusCount(Vst::kAudio, Vst::kInput);
    int32 numOutputBuses = vstPlug_->getBusCount(Vst::kAudio, Vst::kOutput);
    
    // 重置总线信息
    audioProcessingData_.inputChannelCounts.clear();
    audioProcessingData_.outputChannelCounts.clear();
    audioProcessingData_.totalInputChannels = 0;
    audioProcessingData_.totalOutputChannels = 0;
    
    // 配置输入总线并计算实际端口数
    int actualInputPorts = 0;
    for (int32 i = 0; i < numInputBuses; ++i) {
        Vst::BusInfo busInfo;
        tresult result = vstPlug_->getBusInfo(Vst::kAudio, Vst::kInput, i, busInfo);
        if (result == kResultOk) {
            // 激活主总线
            if (busInfo.busType == Vst::kMain || busInfo.busType == Vst::kAux) {
                result = vstPlug_->activateBus(Vst::kAudio, Vst::kInput, i, true);
                if (result == kResultOk) {
                    audioProcessingData_.inputChannelCounts.push_back(busInfo.channelCount);
                    audioProcessingData_.totalInputChannels += busInfo.channelCount;
                    actualInputPorts += busInfo.channelCount; // 每个通道对应一个端口
                    // qDebug() << "Activated input bus" << i << "with" << busInfo.channelCount << "channels";
                }
            }
        }
    }
    
    // 配置输出总线并计算实际端口数
    int actualOutputPorts = 0;
    for (int32 i = 0; i < numOutputBuses; ++i) {
        Vst::BusInfo busInfo;
        tresult result = vstPlug_->getBusInfo(Vst::kAudio, Vst::kOutput, i, busInfo);
        if (result == kResultOk) {
            // 激活主总线
            if (busInfo.busType == Vst::kMain || busInfo.busType == Vst::kAux) {
                result = vstPlug_->activateBus(Vst::kAudio, Vst::kOutput, i, true);
                if (result == kResultOk) {
                    audioProcessingData_.outputChannelCounts.push_back(busInfo.channelCount);
                    audioProcessingData_.totalOutputChannels += busInfo.channelCount;
                    actualOutputPorts += busInfo.channelCount; // 每个通道对应一个端口
                    // qDebug() << "Activated output bus" << i << "with" << busInfo.channelCount << "channels";
                }
            }
        }
    }
    
    // 设置实际的端口数量（每个通道对应一个端口）
    InPortCount = actualInputPorts;
    OutPortCount = actualOutputPorts;
}

/**
 * @brief 初始化音频缓冲区
 */
void VST3PluginDataModel::initializeAudioBuffers()
{
    // 调整VST总线缓冲区大小
    audioProcessingData_.vstInput.resize(audioProcessingData_.inputChannelCounts.size());
    audioProcessingData_.vstOutput.resize(audioProcessingData_.outputChannelCounts.size());

    if (audioProcessingData_.useDoubleProcessing) {
        // 双精度缓冲区
        audioProcessingData_.doubleBuffers.resize(
            qMax(audioProcessingData_.totalInputChannels, audioProcessingData_.totalOutputChannels));
        for (auto& buffer : audioProcessingData_.doubleBuffers) {
            buffer.resize(blockSize_);
        }
    } else {
        // 单精度缓冲区
        audioProcessingData_.floatBuffers.resize(
            qMax(audioProcessingData_.totalInputChannels, audioProcessingData_.totalOutputChannels));
        for (auto& buffer : audioProcessingData_.floatBuffers) {
            buffer.resize(blockSize_);
        }
    }
    
    // 配置VST处理数据结构
    audioProcessingData_.vstData.processMode = Vst::kRealtime;
    audioProcessingData_.vstData.symbolicSampleSize = audioProcessingData_.useDoubleProcessing ? 
                                                      Vst::kSample64 : Vst::kSample32;
    audioProcessingData_.vstData.numInputs = audioProcessingData_.inputChannelCounts.size();
    audioProcessingData_.vstData.numOutputs = audioProcessingData_.outputChannelCounts.size();
    audioProcessingData_.vstData.inputs = audioProcessingData_.vstInput.data();
    audioProcessingData_.vstData.outputs = audioProcessingData_.vstOutput.data();
    audioProcessingData_.vstData.inputParameterChanges = &parameterManager_.inputChanges;
    audioProcessingData_.vstData.outputParameterChanges = &parameterManager_.outputChanges;
}
            

/**
 * @brief 验证VST3接口的完整性
 */
bool VST3PluginDataModel::validateVST3Interfaces() {
    if (!vstPlug_) {
        qWarning() << "IComponent interface is null";
        return false;
    }
    
    if (!audioEffect_) {
        qWarning() << "IAudioProcessor interface is null";
        return false;
    }
    
    // 检查基本接口方法
    // int32 inputBusCount = vstPlug_->getBusCount(Vst::kAudio, Vst::kInput);
    // int32 outputBusCount = vstPlug_->getBusCount(Vst::kAudio, Vst::kOutput);
    

    
    // 检查是否支持32位处理
    tresult canProcess32 = audioEffect_->canProcessSampleSize(Vst::kSample32);

    
    if (canProcess32 != kResultOk) {
        qWarning() << "Plugin does not support 32-bit processing";
        return false;
    }
    
    return true;
}

/**
 * @brief 读取VST3处理器状态
 */
QByteArray VST3PluginDataModel::readProcessorState() const
{
    if (vstPlug_) {
        QByteArray vstData;
        QDataStream stream(&vstData, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        // 使用已有的VST3数据流适配器
        Vst3DataStream vstStream(stream);
        vstPlug_->getState(&vstStream);
        return vstData;
    }
    return savedProcessorState_;
}

/**
 * @brief 读取VST3控制器状态
 */
QByteArray VST3PluginDataModel::readControllerState() const
{
    if (editController_) {
        QByteArray vstData;
        QDataStream stream(&vstData, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        Vst3DataStream vstStream(stream);
        editController_->getState(&vstStream);
        return vstData;
    }
    return savedControllerState_;
}

/**
 * @brief 将保存的状态写入VST3插件
 */
void VST3PluginDataModel::writeState()
{
    if (vstPlug_ && !savedProcessorState_.isEmpty()) {
        QDataStream stream(savedProcessorState_);
        stream.setByteOrder(QDataStream::LittleEndian);
        Vst3DataStream vstStream(stream);
        vstPlug_->setState(&vstStream);
        
        // 同步到控制器
        if (editController_) {
            QDataStream stream2(savedProcessorState_);
            stream2.setByteOrder(QDataStream::LittleEndian);
            Vst3DataStream vstStream2(stream2);
            editController_->setComponentState(&vstStream2);
        }
        
        savedProcessorState_.clear();
    }
    
    if (editController_ && !savedControllerState_.isEmpty()) {
        QDataStream stream(savedControllerState_);
        stream.setByteOrder(QDataStream::LittleEndian);
        Vst3DataStream vstStream(stream);
        editController_->setState(&vstStream);
        
        savedControllerState_.clear();
    }
}
