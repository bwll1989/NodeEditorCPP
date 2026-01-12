//
// Created by WuBin on 2025/8/26.
//
#include "AudioDeviceOutDataModel.hpp"
using namespace Nodes;
/**
 * 初始化 PortAudio 库并刷新设备列表
 * - 仅在节点构造后调用一次，失败时记录错误并停止后续流程
 * - 成功后调用 updateDeviceList() 以填充可用输出设备
 */
void AudioDeviceOutDataModel::initPortAudio() {
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            qWarning() << "PortAudio initialization failed:" << Pa_GetErrorText(err);
            return;
        }
        updateDeviceList();
    }
/**
 * @brief 简化的设备列表更新函数
 */
/**
 * 更新输出设备列表（按驱动类型筛选）
 * - 仅列出具备输出能力的设备（maxOutputChannels>0）
 * - 当驱动选择为“全部”时展示 host API 名称；否则按所选驱动过滤
 * - 若无设备可用，回退到系统默认输出设备
 */
void AudioDeviceOutDataModel::updateDeviceList() {
    widget->deviceSelector->clear();
    
    int selectedDriverType = widget->driverSelector->currentData().toInt();
    bool showAllDrivers = (selectedDriverType == -1);
    
    int numDevices = Pa_GetDeviceCount();
    
    // 直接遍历设备，减少中间处理步骤
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (!deviceInfo || deviceInfo->maxOutputChannels <= 0) {
            continue;
        }
        
        const PaHostApiInfo* hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
        QString hostApiName = hostApiInfo ? QString(hostApiInfo->name) : "Unknown";
        
        // 简化的驱动类型筛选
        if (!showAllDrivers) {
            QString apiLower = hostApiName.toLower();
            bool match = false;
            switch (selectedDriverType) {
                case 0: match = apiLower.contains("wasapi"); break;
                case 1: match = apiLower.contains("directsound"); break;
                case 2: match = apiLower.contains("asio"); break;
                case 3: match = apiLower.contains("mme"); break;
            }
            if (!match) continue;
        }
        
        // 直接使用设备名称，不进行复杂的清理和去重
        QString deviceName = QString(deviceInfo->name);
        QString displayName;
        
        if (showAllDrivers) {
            displayName = QString("%1: %2 (%3)")
                .arg(i)
                .arg(deviceName)
                .arg(hostApiName);
        } else {
            displayName = QString("%1: %2")
                .arg(i)
                .arg(deviceName);
        }
        
        widget->deviceSelector->addItem(displayName, i);
    }
    
    // 如果没有找到设备，添加默认设备
    if (widget->deviceSelector->count() == 0) {
        PaDeviceIndex defaultDevice = Pa_GetDefaultOutputDevice();
        if (defaultDevice != paNoDevice) {
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(defaultDevice);
            if (deviceInfo) {
                QString displayName = QString("%1: %2 (默认)")
                    .arg(defaultDevice)
                    .arg(deviceInfo->name);
                widget->deviceSelector->addItem(displayName, defaultDevice);
            }
        }
    }
}

/**
 * 打开并启动音频输出流
 * - 根据当前选择设备设置通道数、采样格式、建议延迟
 * - 使用 BUFFER_SIZE 作为帧块大小，来源于 SAMPLE_RATE 与全局帧率的比值
 * - 绑定静态回调 paCallback 执行实时输出
 * - 失败时更新节点状态并返回 false
 */
bool AudioDeviceOutDataModel::startAudioOutput() {
    if (isPlaying) return true;

    PaStreamParameters outputParameters;
    outputParameters.device = selectedDeviceIndex;
    outputParameters.channelCount = getDeviceMaxChannels(selectedDeviceIndex);
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(selectedDeviceIndex)->defaultLowOutputLatency;

    outputParameters.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &paStream,
        nullptr,
        &outputParameters,
        SAMPLE_RATE,
        BUFFER_SIZE,
        paClipOff | paDitherOff,  // 添加 paDitherOff 减少数字噪声
        paCallback,
        this
    );

    if (err != paNoError) {
        qDebug() << "无法打开音频流:" << Pa_GetErrorText(err);
        updateNodeState(QtNodes::NodeValidationState::State::Error,"无法打开音频流");
        return false;
    }

    err = Pa_StartStream(paStream);
    if (err != paNoError) {
        qDebug() << "无法启动音频流:" << Pa_GetErrorText(err);
        updateNodeState(QtNodes::NodeValidationState::State::Warning,"无法启动音频流");
        Pa_CloseStream(paStream);
        paStream = nullptr;
        return false;
    }
    updateNodeState(QtNodes::NodeValidationState::State::Valid,"");
    isPlaying = true;
    return true;
}

/**
 * 停止音频输出流
 * - 仅在流处于播放状态时调用 Pa_StopStream
 * - 不关闭流与设备，保持资源以便后续快速恢复
 */
void AudioDeviceOutDataModel::stopAudioOutput() {
    if (paStream && isPlaying) {
        Pa_StopStream(paStream);
        isPlaying = false;
    }
}

/**
 * 驱动类型改变回调
 * - 根据 UI 选择刷新设备列表
 * - 若当前正在播放，为确保配置生效，尝试选择第一个可用设备并重启音频流
 * @param index 驱动选择器索引（未直接使用）
 */
void AudioDeviceOutDataModel::onDriverChanged(int index) {
    Q_UNUSED(index)
    // 重新更新设备列表
    updateDeviceList();

    // 如果当前正在播放，重启音频流以应用新设备
    if (isPlaying && widget->deviceSelector->count() > 0) {
        // 选择第一个可用设备
        widget->deviceSelector->setCurrentIndex(0);
        onDeviceChanged(widget->deviceSelector->currentText());
    }
}

/**
 * 设备选择改变回调
 * - 从 UI 文本解析设备索引并应用到 selectedDeviceIndex
 * - 若正在播放则重启音频流以应用新设备
 * @param deviceText 形如 "<index>: <name>" 的显示文本
 */
void AudioDeviceOutDataModel::onDeviceChanged(const QString& deviceText) {
    QStringList parts = deviceText.split(":");
    if (!parts.isEmpty()) {
        bool ok;
        int deviceIndex = parts[0].toInt(&ok);
        if (ok) {
            selectedDeviceIndex = deviceIndex;

            // 重启音频流以应用新设备
            if (isPlaying) {
                Pa_StopStream(paStream);
                Pa_CloseStream(paStream);
                paStream = nullptr;
                isPlaying = false;
                startAudioOutput();
            }
        }
    }
}
