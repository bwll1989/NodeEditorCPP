
#include "FmodDecoderWorker.h"
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include "TimestampGenerator/TimestampGenerator.hpp"

namespace Nodes {

FmodDecoderWorker::FmodDecoderWorker(QObject *parent)
    : QObject(parent)
{
}

FmodDecoderWorker::~FmodDecoderWorker()
{
    stopProcessing();
    
    // 清理 FMOD 资源
    if (captureDSP_) {
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (coreSystem_) {
            coreSystem_->getMasterChannelGroup(&masterGroup);
            if (masterGroup) {
                masterGroup->removeDSP(captureDSP_);
            }
        }
        captureDSP_->release();
    }
    
    if (eventInstance_) {
        eventInstance_->stop(FMOD_STUDIO_STOP_IMMEDIATE);
        eventInstance_->release();
    }
    
    for (auto* bank : loadedBanks_) {
        bank->unload();
    }
    loadedBanks_.clear();
    
    if (studioSystem_) {
        studioSystem_->release();
    }
}

void FmodDecoderWorker::initialize(std::vector<std::shared_ptr<AudioTimestampRingQueue>> buffers)
{
    outputBuffers_ = buffers;
    initFMOD();
}

void FmodDecoderWorker::startProcessing()
{
    // 启动定时器，用于周期性调用 studioSystem_->update()
    if (!updateTimer_) {
        updateTimer_ = new QTimer(this);
        connect(updateTimer_, &QTimer::timeout, this, [this]() {
            if (studioSystem_) {
                studioSystem_->update();
                
                // 检查播放状态，如果事件播放结束，自动重置 isPlaying_
                if (eventInstance_ && isPlaying_) {
                    FMOD_STUDIO_PLAYBACK_STATE state;
                    eventInstance_->getPlaybackState(&state);
                    if (state == FMOD_STUDIO_PLAYBACK_STOPPED) {
                        isPlaying_ = false;
                    }
                }
            }
        });
        updateTimer_->start(20); // 50Hz 更新频率
    }
}

void FmodDecoderWorker::stopProcessing()
{
    if (updateTimer_) {
        updateTimer_->stop();
        delete updateTimer_;
        updateTimer_ = nullptr;
    }
}

void FmodDecoderWorker::initFMOD()
{
    FMOD_RESULT result;
    
    // 1. 创建 Studio System
    result = FMOD::Studio::System::create(&studioSystem_);
    if (result != FMOD_OK) {
        emit errorOccurred(QString("FMOD Studio create failed: %1").arg(result));
        return;
    }
    
    // 2. 获取 Core System
    studioSystem_->getCoreSystem(&coreSystem_);
    
    // 3. 配置 Core System
    // 使用 NOSOUND 模式，避免 FMOD 直接输出声音到系统设备
    // 我们只需要通过 DSP 捕获音频数据
    coreSystem_->setOutput(FMOD_OUTPUTTYPE_NOSOUND); 

    // 设置软件格式为 48kHz RAW 12 声道（7.1.4），保证所有通道可被捕获
    coreSystem_->setSoftwareFormat(48000, FMOD_SPEAKERMODE_RAW, 12);

    // 设置 DSP 缓冲区大小为 2048 采样点，缓冲数量 8
    // 2048 对应我们的帧大小，有助于减少抖动
    coreSystem_->setDSPBufferSize(2048, 8);
    
    // 4. 初始化 Studio System
    // 启用 Live Update 以便 FMOD Studio 可以连接并实时调试
    result = studioSystem_->initialize(1024, FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        emit errorOccurred(QString("FMOD Studio initialize failed: %1").arg(result));
        return;
    }
    
    // 5. 创建并添加 Capture DSP
    FMOD_DSP_DESCRIPTION dspDesc = {};
    memset(&dspDesc, 0, sizeof(dspDesc));
    dspDesc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    strcpy_s(dspDesc.name, "CaptureDSP");
    dspDesc.version = 0x00010000;
    dspDesc.numinputbuffers = 1;
    dspDesc.numoutputbuffers = 1;
    dspDesc.read = captureDSPCallback; // 设置回调函数
    
    result = coreSystem_->createDSP(&dspDesc, &captureDSP_);
    if (result == FMOD_OK) {
        captureDSP_->setUserData(this); // 传递 this 指针以便在回调中访问成员
        
        FMOD::ChannelGroup* masterGroup = nullptr;
        coreSystem_->getMasterChannelGroup(&masterGroup);
        if (masterGroup) {
            // 将 DSP 添加到 Master Group 的末尾 (Index -1)
            masterGroup->addDSP(-1, captureDSP_);
            captureDSP_->setActive(true);
            captureDSP_->setBypass(false);
        }
    } else {
        emit errorOccurred(QString("Create DSP failed: %1").arg(result));
    }
}

void FmodDecoderWorker::loadBanks(const QString& path)
{
    if (!studioSystem_ || path.isEmpty()) return;
    
    // 卸载已加载的 Bank
    for (auto* bank : loadedBanks_) {
        bank->unload();
    }
    loadedBanks_.clear();
    
    QFileInfo info(path);
    QString dirPath = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    QDir dir(dirPath);
    
    QStringList filters;
    filters << "*.bank";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
    
    if (fileList.isEmpty()) {
        emit errorOccurred(QString("No .bank files found in: %1").arg(dirPath));
        return;
    }
    
    // 排序：优先加载 strings.bank，这是 FMOD 的要求
    std::sort(fileList.begin(), fileList.end(), [](const QFileInfo& a, const QFileInfo& b) {
        bool aIsStrings = a.fileName().contains("strings.bank");
        bool bIsStrings = b.fileName().contains("strings.bank");
        if (aIsStrings && !bIsStrings) return true;
        if (!aIsStrings && bIsStrings) return false;
        return a.fileName() < b.fileName();
    });
    
    // 加载 Bank 文件
    for (const QFileInfo& fileInfo : fileList) {
        FMOD::Studio::Bank* bank = nullptr;
        FMOD_RESULT result = studioSystem_->loadBankFile(fileInfo.absoluteFilePath().toUtf8().constData(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
        if (result == FMOD_OK && bank) {
            loadedBanks_.push_back(bank);
        } else {
            emit errorOccurred(QString("Failed to load bank: %1 Error: %2").arg(fileInfo.fileName()).arg(result));
        }
    }
    
    studioSystem_->update();
    updateEventList();
}

void FmodDecoderWorker::updateEventList()
{
    QStringList eventList;
    if (loadedBanks_.empty()) {
        emit eventListUpdated(eventList);
        return;
    }
    
    // 遍历 Bank 获取 Event 列表
    for (auto* bank : loadedBanks_) {
        int count = 0;
        bank->getEventCount(&count);
        
        if (count > 0) {
            std::vector<FMOD::Studio::EventDescription*> events(count);
            bank->getEventList(events.data(), count, &count);
            
            for (auto* event : events) {
                char path[256] = {0};
                int retrieved;
                FMOD_RESULT res = event->getPath(path, 256, &retrieved);
                
                if (res == FMOD_OK) {
                    eventList.append(QString::fromUtf8(path));
                } else {
                    // 如果获取不到路径，使用 GUID 作为标识
                    FMOD_GUID id;
                    event->getID(&id);
                    QString guidStr = QString("{%1-%2-%3-%4%5-%6%7%8%9%10%11}")
                        .arg(id.Data1, 8, 16, QChar('0'))
                        .arg(id.Data2, 4, 16, QChar('0'))
                        .arg(id.Data3, 4, 16, QChar('0'))
                        .arg(id.Data4[0], 2, 16, QChar('0'))
                        .arg(id.Data4[1], 2, 16, QChar('0'))
                        .arg(id.Data4[2], 2, 16, QChar('0'))
                        .arg(id.Data4[3], 2, 16, QChar('0'))
                        .arg(id.Data4[4], 2, 16, QChar('0'))
                        .arg(id.Data4[5], 2, 16, QChar('0'))
                        .arg(id.Data4[6], 2, 16, QChar('0'))
                        .arg(id.Data4[7], 2, 16, QChar('0'));
                    eventList.append("ID: " + guidStr.toUpper());
                }
            }
        }
    }
    emit eventListUpdated(eventList);
}

void FmodDecoderWorker::playEvent(const QString& eventPath)
{
    if (!studioSystem_ || eventPath.isEmpty()) return;
    
    // 处理 GUID 格式的路径
    QString cleanPath = eventPath;
    if (cleanPath.startsWith("ID: ")) {
        cleanPath = cleanPath.mid(4);
    }
    
    // 停止之前的 Event
    if (eventInstance_) {
        eventInstance_->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        eventInstance_->release();
        eventInstance_ = nullptr;
    }
    
    FMOD::Studio::EventDescription* eventDesc = nullptr;
    FMOD_RESULT result = studioSystem_->getEvent(cleanPath.toUtf8().constData(), &eventDesc);
    
    if (result != FMOD_OK) {
        emit errorOccurred(QString("FMOD getEvent failed for path: %1 Error: %2").arg(cleanPath).arg(result));
        return;
    }
    
    if (eventDesc) {
        result = eventDesc->createInstance(&eventInstance_);
        if (result == FMOD_OK && eventInstance_) {
            eventInstance_->start();
            isPlaying_ = true; // 标记开始播放
            timestampAligned_ = false; // 重置时间戳对齐标志
        } else {
             emit errorOccurred(QString("FMOD createInstance failed: %1").arg(result));
        }
    }
}

FMOD_RESULT F_CALLBACK FmodDecoderWorker::captureDSPCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels)
{
    void* userdata = nullptr;
    FMOD::DSP* dsp = (FMOD::DSP*)dsp_state->instance;
    dsp->getUserData(&userdata);
    
    FmodDecoderWorker* self = (FmodDecoderWorker*)userdata;
    
    // --- 1. 透传音频数据 ---
    // 即使在 NOSOUND 模式下，保持透传也是良好的 DSP 习惯，
    // 以防链中有后续 DSP 需要数据。
    if (inbuffer && outbuffer) {
        memcpy(outbuffer, inbuffer, length * inchannels * sizeof(float));
    }
    if (outchannels) {
        *outchannels = inchannels;
    }
    
    // 如果没有 Worker 实例或不在播放状态，直接返回
    if (self && !self->isPlaying_) {
        return FMOD_OK;
    }
    
    if (self && inbuffer) {
        // --- 2. 初始化/对齐缓冲区 ---
        // 第一次收到数据时，初始化暂存区并同步时间戳
        if (!self->timestampAligned_) {
            self->baseFrameCount_ = TimestampGenerator::getInstance()->getCurrentFrameCount();
            self->emittedFrameCount_ = 0;
            self->samplesPerFrame_ = 2048; // 强制 2048
            self->timestampAligned_ = true;
            
            // 初始化通道缓存
            self->pendingChannelData_.assign(inchannels, QVector<float>());
            self->pendingChannelSampleCount_.assign(inchannels, 0);
            for (int ch = 0; ch < inchannels; ++ch) {
                self->pendingChannelData_[ch].reserve(self->samplesPerFrame_ * 4);
            }
        } else {
            // 如果通道数发生变化（罕见），重新调整大小
            if ((int)self->pendingChannelData_.size() != inchannels) {
                self->pendingChannelData_.assign(inchannels, QVector<float>());
                self->pendingChannelSampleCount_.assign(inchannels, 0);
                for (int ch = 0; ch < inchannels; ++ch) {
                    self->pendingChannelData_[ch].reserve(self->samplesPerFrame_ * 4);
                }
            }
        }

        // --- 3. 分离通道数据并存入暂存区 ---
        // FMOD 的数据是交错的 (L, R, L, R...)，我们需要将其分离
        for (unsigned int i = 0; i < length; ++i) {
            for (int ch = 0; ch < inchannels; ++ch) {
                float sample = inbuffer[i * inchannels + ch];
                self->pendingChannelData_[ch].push_back(sample);
                self->pendingChannelSampleCount_[ch] += 1;
            }
        }

        // --- 4. 检查是否凑够一帧数据 ---
        bool canEmit = true;
        for (int ch = 0; ch < inchannels; ++ch) {
            if (self->pendingChannelSampleCount_[ch] < self->samplesPerFrame_) {
                canEmit = false;
                break;
            }
        }

        // --- 5. 循环输出帧 ---
        // 可能一次回调的数据足够输出多帧
        while (canEmit) {
            if (!self->isPlaying_) return FMOD_OK;

            // 计算时间戳：基准 + 已输出帧数 + 延迟补偿
            qint64 timestamp = self->baseFrameCount_ + self->emittedFrameCount_ + self->latencyOffsetFrames_;
            
            // 将每通道数据写入对应的 RingQueue
            for (int ch = 0; ch < inchannels; ++ch) {
                if (ch >= self->outputBuffers_.size()) break;
                auto& buffer = self->outputBuffers_[ch];
                if (!buffer) continue;

                AudioFrame frame;
                frame.sampleRate = self->sampleRate_;
                frame.channels = 1; // 单声道帧
                frame.bitsPerSample = 32;
                frame.timestamp = timestamp;

                frame.data.resize(self->samplesPerFrame_ * sizeof(float));
                // 从暂存区拷贝数据
                memcpy(frame.data.data(),
                       self->pendingChannelData_[ch].data(),
                       self->samplesPerFrame_ * sizeof(float));

                buffer->pushFrame(frame);

                // 移除已处理的数据
                self->pendingChannelData_[ch].erase(self->pendingChannelData_[ch].begin(),
                                                    self->pendingChannelData_[ch].begin() + self->samplesPerFrame_);
                self->pendingChannelSampleCount_[ch] -= self->samplesPerFrame_;
            }
            self->emittedFrameCount_ += 1;

            // 检查剩余数据是否还够一帧
            canEmit = true;
            for (int ch = 0; ch < inchannels; ++ch) {
                if (self->pendingChannelSampleCount_[ch] < self->samplesPerFrame_) {
                    canEmit = false;
                    break;
                }
            }
        }
    }
    
    return FMOD_OK;
}

} // namespace Nodes
