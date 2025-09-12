//
// Created by WuBin on 2025/8/26.
//
#include "AudioDeviceOutDataModel.hpp"
using namespace Nodes;
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

bool AudioDeviceOutDataModel::startAudioOutput() {
    if (isPlaying) return true;

    PaStreamParameters outputParameters;
    outputParameters.device = selectedDeviceIndex;
    outputParameters.channelCount = getDeviceMaxChannels(selectedDeviceIndex);
    outputParameters.sampleFormat = paInt16;
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
        return false;
    }

    err = Pa_StartStream(paStream);
    if (err != paNoError) {
        qDebug() << "无法启动音频流:" << Pa_GetErrorText(err);
        Pa_CloseStream(paStream);
        paStream = nullptr;
        return false;
    }

    isPlaying = true;
    return true;
}

void AudioDeviceOutDataModel::stopAudioOutput() {
    if (paStream && isPlaying) {
        Pa_StopStream(paStream);
        isPlaying = false;
    }
}

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