/**
 * @file FmodDecoderWorker.cpp
 * @brief FmodDecoderWorker 实现：Studio 初始化、Bank、多 Instance、参数、DSP 捕获
 */

#include "FmodDecoderWorker.h"

#include "TimestampGenerator/TimestampGenerator.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Nodes {

FmodDecoderWorker::FmodDecoderWorker(QObject* parent)
    : QObject(parent)
{
    samplesPerFrame_ = TimestampGenerator::getInstance()->getSamplesPerFrame(sampleRate_);
}

FmodDecoderWorker::~FmodDecoderWorker()
{
    stopProcessing();

    if (captureDSP_) {
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (coreSystem_) {
            coreSystem_->getMasterChannelGroup(&masterGroup);
            if (masterGroup) {
                masterGroup->removeDSP(captureDSP_);
            }
        }
        captureDSP_->release();
        captureDSP_ = nullptr;
    }

    stopAllInstances(FMOD_STUDIO_STOP_IMMEDIATE);

    for (auto* bank : loadedBanks_) {
        bank->unload();
    }
    loadedBanks_.clear();

    if (studioSystem_) {
        studioSystem_->release();
        studioSystem_ = nullptr;
    }
}

void FmodDecoderWorker::initialize(std::vector<std::shared_ptr<AudioTimestampRingQueue>> buffers)
{
    outputBuffers_ = std::move(buffers);
    initFMOD();
}

void FmodDecoderWorker::startProcessing()
{
    if (!realtimeGuard_) {
        realtimeGuard_ = std::make_unique<AudioThreadRealtimeGuard>(L"Pro Audio");
    }
    if (!updateTimer_) {
        updateTimer_ = new QTimer(this);
        connect(updateTimer_, &QTimer::timeout, this, [this]() {
            if (studioSystem_) {
                studioSystem_->update();
                pruneStoppedInstances();
            }
        });
        updateTimer_->start(20); // 50Hz
    }
}

void FmodDecoderWorker::stopProcessing()
{
    if (updateTimer_) {
        updateTimer_->stop();
        delete updateTimer_;
        updateTimer_ = nullptr;
    }
    realtimeGuard_.reset();
}

void FmodDecoderWorker::initFMOD()
{
    FMOD_RESULT result = FMOD::Studio::System::create(&studioSystem_);
    if (result != FMOD_OK) {
        emit errorOccurred(QString("FMOD Studio create failed: %1").arg(result));
        return;
    }

    studioSystem_->getCoreSystem(&coreSystem_);
    coreSystem_->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
    coreSystem_->setSoftwareFormat(48000, FMOD_SPEAKERMODE_RAW, 12);
    coreSystem_->setDSPBufferSize(static_cast<unsigned int>(samplesPerFrame_), 8);

    result = studioSystem_->initialize(1024, FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        emit errorOccurred(QString("FMOD Studio initialize failed: %1").arg(result));
        return;
    }

    FMOD_DSP_DESCRIPTION dspDesc = {};
    std::memset(&dspDesc, 0, sizeof(dspDesc));
    dspDesc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
    strcpy_s(dspDesc.name, "CaptureDSP");
    dspDesc.version = 0x00010000;
    dspDesc.numinputbuffers = 1;
    dspDesc.numoutputbuffers = 1;
    dspDesc.read = captureDSPCallback;

    result = coreSystem_->createDSP(&dspDesc, &captureDSP_);
    if (result == FMOD_OK) {
        captureDSP_->setUserData(this);
        FMOD::ChannelGroup* masterGroup = nullptr;
        coreSystem_->getMasterChannelGroup(&masterGroup);
        if (masterGroup) {
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
    if (!studioSystem_ || path.isEmpty()) {
        return;
    }

    stopAllInstances(FMOD_STUDIO_STOP_IMMEDIATE);

    for (auto* bank : loadedBanks_) {
        bank->unload();
    }
    loadedBanks_.clear();
    paramCache_.clear();

    QFileInfo info(path);
    const QString dirPath = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    QDir dir(dirPath);

    QStringList filters;
    filters << QStringLiteral("*.bank");
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    if (fileList.isEmpty()) {
        emit errorOccurred(QString("No .bank files found in: %1").arg(dirPath));
        emit eventCatalogUpdated(QStringList(), {});
        return;
    }

    std::sort(fileList.begin(), fileList.end(), [](const QFileInfo& a, const QFileInfo& b) {
        const bool aIsStrings = a.fileName().contains(QStringLiteral("strings.bank"));
        const bool bIsStrings = b.fileName().contains(QStringLiteral("strings.bank"));
        if (aIsStrings && !bIsStrings) {
            return true;
        }
        if (!aIsStrings && bIsStrings) {
            return false;
        }
        return a.fileName() < b.fileName();
    });

    for (const QFileInfo& fileInfo : fileList) {
        FMOD::Studio::Bank* bank = nullptr;
        const FMOD_RESULT result = studioSystem_->loadBankFile(
            fileInfo.absoluteFilePath().toUtf8().constData(),
            FMOD_STUDIO_LOAD_BANK_NORMAL,
            &bank);
        if (result == FMOD_OK && bank) {
            loadedBanks_.push_back(bank);
        } else {
            emit errorOccurred(
                QString("Failed to load bank: %1 Error: %2").arg(fileInfo.fileName()).arg(result));
        }
    }

    studioSystem_->update();
    updateEventCatalog();
}

void FmodDecoderWorker::updateEventCatalog()
{
    QStringList eventList;
    QVector<FmodParamDesc> params;

    if (loadedBanks_.empty()) {
        emit eventCatalogUpdated(eventList, params);
        return;
    }

    auto eventDisplayName = [](const QString& eventPath) {
        if (eventPath.startsWith(QStringLiteral("event:/"))) {
            return eventPath.mid(7);
        }
        return eventPath;
    };

    for (auto* bank : loadedBanks_) {
        int count = 0;
        bank->getEventCount(&count);
        if (count <= 0) {
            continue;
        }

        std::vector<FMOD::Studio::EventDescription*> events(static_cast<size_t>(count));
        bank->getEventList(events.data(), count, &count);

        for (auto* event : events) {
            if (!event) {
                continue;
            }

            char pathBuf[256] = {0};
            int retrieved = 0;
            QString eventPath;
            if (event->getPath(pathBuf, 256, &retrieved) == FMOD_OK) {
                eventPath = QString::fromUtf8(pathBuf);
            } else {
                FMOD_GUID id{};
                event->getID(&id);
                eventPath = QString("ID: {%1-%2-%3-%4%5-%6%7%8%9%10%11}")
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
                                .arg(id.Data4[7], 2, 16, QChar('0'))
                                .toUpper();
            }
            eventList.append(eventPath);

            int paramCount = 0;
            event->getParameterDescriptionCount(&paramCount);
            for (int pi = 0; pi < paramCount; ++pi) {
                FMOD_STUDIO_PARAMETER_DESCRIPTION desc{};
                if (event->getParameterDescriptionByIndex(pi, &desc) != FMOD_OK) {
                    continue;
                }
                if (desc.type != FMOD_STUDIO_PARAMETER_GAME_CONTROLLED) {
                    continue;
                }
                if (desc.flags & FMOD_STUDIO_PARAMETER_READONLY) {
                    continue;
                }
                if (desc.flags & FMOD_STUDIO_PARAMETER_AUTOMATIC) {
                    continue;
                }

                FmodParamDesc p;
                p.eventPath = eventPath;
                p.paramName = QString::fromUtf8(desc.name ? desc.name : "");
                if (p.paramName.isEmpty()) {
                    continue;
                }
                p.caption = eventDisplayName(eventPath) + QLatin1Char('/') + p.paramName;
                p.minimum = desc.minimum;
                p.maximum = desc.maximum;
                p.defaultValue = desc.defaultvalue;
                p.discrete = (desc.flags & FMOD_STUDIO_PARAMETER_DISCRETE) != 0;
                p.labeled = (desc.flags & FMOD_STUDIO_PARAMETER_LABELED) != 0;

                if (p.labeled) {
                    const int first = static_cast<int>(std::lround(desc.minimum));
                    const int last = static_cast<int>(std::lround(desc.maximum));
                    for (int li = first; li <= last; ++li) {
                        char labelBuf[128] = {0};
                        int labelRetrieved = 0;
                        if (event->getParameterLabelByIndex(
                                pi, li - first, labelBuf, 128, &labelRetrieved)
                            == FMOD_OK) {
                            p.labels.append(QString::fromUtf8(labelBuf));
                        } else {
                            p.labels.append(QString::number(li));
                        }
                    }
                }

                if (!paramCache_[eventPath].contains(p.paramName)) {
                    paramCache_[eventPath].insert(p.paramName, p.defaultValue);
                }

                params.append(p);
            }
        }
    }

    emit eventCatalogUpdated(eventList, params);
}

QString FmodDecoderWorker::cleanEventPath(const QString& eventPath)
{
    QString cleanPath = eventPath;
    if (cleanPath.startsWith(QStringLiteral("ID: "))) {
        cleanPath = cleanPath.mid(4);
    }
    return cleanPath;
}

void FmodDecoderWorker::applyCachedParameters(const QString& eventPath,
                                              FMOD::Studio::EventInstance* instance) const
{
    if (!instance) {
        return;
    }
    const auto it = paramCache_.constFind(eventPath);
    if (it == paramCache_.cend()) {
        return;
    }
    for (auto pit = it->cbegin(); pit != it->cend(); ++pit) {
        instance->setParameterByName(pit.key().toUtf8().constData(), pit.value());
    }
}

void FmodDecoderWorker::playEvent(const QString& eventPath)
{
    if (!studioSystem_ || eventPath.isEmpty()) {
        return;
    }

    const QString cleanPath = cleanEventPath(eventPath);

    FMOD::Studio::EventDescription* eventDesc = nullptr;
    FMOD_RESULT result = studioSystem_->getEvent(cleanPath.toUtf8().constData(), &eventDesc);
    if (result != FMOD_OK || !eventDesc) {
        emit errorOccurred(
            QString("FMOD getEvent failed for path: %1 Error: %2").arg(cleanPath).arg(result));
        return;
    }

    FMOD::Studio::EventInstance* instance = nullptr;
    result = eventDesc->createInstance(&instance);
    if (result != FMOD_OK || !instance) {
        emit errorOccurred(QString("FMOD createInstance failed: %1").arg(result));
        return;
    }

    applyCachedParameters(eventPath, instance);
    if (eventPath != cleanPath) {
        applyCachedParameters(cleanPath, instance);
    }

    result = instance->start();
    if (result != FMOD_OK) {
        instance->release();
        emit errorOccurred(QString("FMOD EventInstance::start failed: %1").arg(result));
        return;
    }

    eventInstances_[cleanPath].append(instance);

    const bool wasPlaying = isPlaying_.load();
    isPlaying_ = true;
    if (!wasPlaying) {
        timestampAligned_ = false;
    }
}

void FmodDecoderWorker::setEventParameter(const QString& eventPath,
                                          const QString& paramName,
                                          float value)
{
    if (eventPath.isEmpty() || paramName.isEmpty()) {
        return;
    }

    const QString cleanPath = cleanEventPath(eventPath);
    paramCache_[eventPath].insert(paramName, value);
    if (eventPath != cleanPath) {
        paramCache_[cleanPath].insert(paramName, value);
    }

    const auto applyToList = [&](const QString& key) {
        const auto it = eventInstances_.constFind(key);
        if (it == eventInstances_.cend()) {
            return;
        }
        for (FMOD::Studio::EventInstance* instance : it.value()) {
            if (instance) {
                instance->setParameterByName(paramName.toUtf8().constData(), value);
            }
        }
    };
    applyToList(cleanPath);
    applyToList(eventPath);
}

void FmodDecoderWorker::setEventParameterMap(const QString& eventPath, const QVariantMap& values)
{
    if (eventPath.isEmpty()) {
        return;
    }
    for (auto it = values.begin(); it != values.end(); ++it) {
        setEventParameter(eventPath, it.key(), static_cast<float>(it.value().toDouble()));
    }
}

void FmodDecoderWorker::stopAllInstances(FMOD_STUDIO_STOP_MODE mode)
{
    for (auto it = eventInstances_.begin(); it != eventInstances_.end(); ++it) {
        for (FMOD::Studio::EventInstance* instance : it.value()) {
            if (instance) {
                instance->stop(mode);
                instance->release();
            }
        }
    }
    eventInstances_.clear();
    isPlaying_ = false;
}

void FmodDecoderWorker::pruneStoppedInstances()
{
    if (eventInstances_.isEmpty()) {
        isPlaying_ = false;
        return;
    }

    for (auto it = eventInstances_.begin(); it != eventInstances_.end();) {
        QList<FMOD::Studio::EventInstance*>& list = it.value();
        for (int i = list.size() - 1; i >= 0; --i) {
            FMOD::Studio::EventInstance* instance = list.at(i);
            if (!instance) {
                list.removeAt(i);
                continue;
            }

            FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
            instance->getPlaybackState(&state);
            if (state == FMOD_STUDIO_PLAYBACK_STOPPED) {
                instance->release();
                list.removeAt(i);
            }
        }

        if (list.isEmpty()) {
            it = eventInstances_.erase(it);
        } else {
            ++it;
        }
    }

    isPlaying_ = !eventInstances_.isEmpty();
}

FMOD_RESULT F_CALLBACK FmodDecoderWorker::captureDSPCallback(
    FMOD_DSP_STATE* dsp_state,
    float* inbuffer,
    float* outbuffer,
    unsigned int length,
    int inchannels,
    int* outchannels)
{
    // FMOD mixer 线程：首次回调注册 MMCSS，线程退出时自动撤销
    thread_local AudioThreadRealtimeGuard s_mmcsGuard(L"Pro Audio");

    void* userdata = nullptr;
    auto* dsp = reinterpret_cast<FMOD::DSP*>(dsp_state->instance);
    dsp->getUserData(&userdata);

    auto* self = static_cast<FmodDecoderWorker*>(userdata);

    if (inbuffer && outbuffer) {
        std::memcpy(outbuffer, inbuffer, length * static_cast<size_t>(inchannels) * sizeof(float));
    }
    if (outchannels) {
        *outchannels = inchannels;
    }

    if (self && !self->isPlaying_) {
        return FMOD_OK;
    }

    if (self && inbuffer) {
        if (!self->timestampAligned_) {
            self->baseFrameCount_ = TimestampGenerator::getInstance()->getCurrentFrameCount();
            self->emittedFrameCount_ = 0;
            self->timestampAligned_ = true;

            self->pendingChannelData_.assign(static_cast<size_t>(inchannels), QVector<float>());
            self->pendingChannelSampleCount_.assign(static_cast<size_t>(inchannels), 0);
            for (int ch = 0; ch < inchannels; ++ch) {
                self->pendingChannelData_[static_cast<size_t>(ch)].reserve(self->samplesPerFrame_ * 4);
            }
        } else if (static_cast<int>(self->pendingChannelData_.size()) != inchannels) {
            self->pendingChannelData_.assign(static_cast<size_t>(inchannels), QVector<float>());
            self->pendingChannelSampleCount_.assign(static_cast<size_t>(inchannels), 0);
            for (int ch = 0; ch < inchannels; ++ch) {
                self->pendingChannelData_[static_cast<size_t>(ch)].reserve(self->samplesPerFrame_ * 4);
            }
        }

        for (unsigned int i = 0; i < length; ++i) {
            for (int ch = 0; ch < inchannels; ++ch) {
                const float sample = inbuffer[i * static_cast<unsigned int>(inchannels)
                                              + static_cast<unsigned int>(ch)];
                self->pendingChannelData_[static_cast<size_t>(ch)].push_back(sample);
                self->pendingChannelSampleCount_[static_cast<size_t>(ch)] += 1;
            }
        }

        bool canEmit = true;
        for (int ch = 0; ch < inchannels; ++ch) {
            if (self->pendingChannelSampleCount_[static_cast<size_t>(ch)] < self->samplesPerFrame_) {
                canEmit = false;
                break;
            }
        }

        while (canEmit) {
            if (!self->isPlaying_) {
                return FMOD_OK;
            }

            const qint64 timestamp =
                self->baseFrameCount_ + self->emittedFrameCount_ + self->latencyOffsetFrames_;

            for (int ch = 0; ch < inchannels; ++ch) {
                if (ch >= static_cast<int>(self->outputBuffers_.size())) {
                    break;
                }
                auto& buffer = self->outputBuffers_[static_cast<size_t>(ch)];
                if (!buffer) {
                    continue;
                }

                AudioFrame frame;
                frame.sampleRate = self->sampleRate_;
                frame.channels = 1;
                frame.bitsPerSample = 32;
                frame.timestamp = timestamp;
                frame.data.resize(self->samplesPerFrame_ * static_cast<int>(sizeof(float)));
                std::memcpy(frame.data.data(),
                            self->pendingChannelData_[static_cast<size_t>(ch)].data(),
                            static_cast<size_t>(self->samplesPerFrame_) * sizeof(float));

                buffer->pushFrame(frame);

                self->pendingChannelData_[static_cast<size_t>(ch)].erase(
                    self->pendingChannelData_[static_cast<size_t>(ch)].begin(),
                    self->pendingChannelData_[static_cast<size_t>(ch)].begin()
                        + self->samplesPerFrame_);
                self->pendingChannelSampleCount_[static_cast<size_t>(ch)] -= self->samplesPerFrame_;
            }
            self->emittedFrameCount_ += 1;

            canEmit = true;
            for (int ch = 0; ch < inchannels; ++ch) {
                if (self->pendingChannelSampleCount_[static_cast<size_t>(ch)]
                    < self->samplesPerFrame_) {
                    canEmit = false;
                    break;
                }
            }
        }
    }

    return FMOD_OK;
}

} // namespace Nodes
