#pragma once

#include <QDir>
#include <QTimer>
#include <map>

#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "FfmpegWriters.hpp"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "ToFilePath.hpp"
#include "ToAudioFileInterface.hpp"

#include <QPointer>
#include <atomic>

using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
class ToAudioFileDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)

    static constexpr int kControlPortCount = 2;

public:
    ToAudioFileDataModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        PortEditable = true;
        m_outRecording = std::make_shared<VariableData>(false);
        CaptionVisible = true;
        Caption = QStringLiteral("To Audio File");
        WidgetEmbeddable = true;
        Resizable = false;

        m_file = QStringLiteral("output.wav");
        _widget = new ToAudioFileInterface();

        _pollTimer = new QTimer(this);
        _pollTimer->setTimerType(Qt::PreciseTimer);
        connect(_pollTimer, &QTimer::timeout, this, &ToAudioFileDataModel::pollAudioFrame);

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "file";
            binding.control = _widget->fileEdit();
            AbstractDelegateModel::registerExternalBinding("/file", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "outputDir";
            binding.control = _widget->_folderButton;
            AbstractDelegateModel::registerExternalBinding("/outputDir", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "recording";
            binding.control = _widget->_recordToggle;
            AbstractDelegateModel::registerExternalBinding("/recording", this, binding);
        }
        connect(_widget, &ToAudioFileInterface::fileTextChanged, this, &ToAudioFileDataModel::setFile);
        connect(_widget, &ToAudioFileInterface::outputDirChanged, this, &ToAudioFileDataModel::setOutputDir);
        connect(_widget, &ToAudioFileInterface::recordingToggled, this, &ToAudioFileDataModel::setRecording);
    }

    ~ToAudioFileDataModel() override
    {
        m_shuttingDown.store(true);
        GlobalEventBus::instance()->unsubscribe(this);
        if (_pollTimer) {
            _pollTimer->stop();
            disconnect(_pollTimer, nullptr, this, nullptr);
        }
        if (_widget) {
            disconnect(_widget.data(), nullptr, this, nullptr);
        }
        stopRecording();
        m_audioQueues.clear();
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return VariableData().type();
        }
        if (portIndex == InPortCount - 1 || portIndex == InPortCount - 2) {
            return VariableData().type();
        }
        return AudioData().type();
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return portIndex == 0 ? QStringLiteral("RECORDING") : QString();
        }
        if (portIndex == InPortCount - 1) {
            return QStringLiteral("STOP");
        }
        if (portIndex == InPortCount - 2) {
            return QStringLiteral("START");
        }
        return QStringLiteral("AUDIO %1").arg(portIndex);
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, PortIndex portIndex) override
    {
        if (portIndex == InPortCount - 1) {
            const auto trigger = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (trigger && trigger->value().toBool()) {
                stopRecording();
            }
            return;
        }
        if (portIndex == InPortCount - 2) {
            const auto trigger = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (trigger && trigger->value().toBool()) {
                startRecording();
            }
            return;
        }

        const auto audio = std::dynamic_pointer_cast<AudioData>(nodeData);
        if (audio && audio->isConnectedToSharedBuffer()) {
            m_audioQueues[portIndex] = audio->getSharedAudioBuffer();
        } else {
            m_audioQueues.erase(portIndex);
        }
    }

    std::shared_ptr<QtNodes::NodeData> outData(PortIndex port) override
    {
        if (port == 0) {
            return m_outRecording;
        }
        return nullptr;
    }

    QWidget* embeddedWidget() override
    {
        return _widget.data();
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["file"] = m_file;
        modelJson["outputDir"] = m_outputDir;
        return modelJson;
    }

    void load(const QJsonObject& jsonObj) override
    {
        if (jsonObj.contains("file")) {
            setFile(jsonObj["file"].toString());
        }
        if (jsonObj.contains("outputDir")) {
            setOutputDir(jsonObj["outputDir"].toString());
        }
    }

    QString file() const { return m_file; }
    QString outputDir() const { return m_outputDir; }
    bool recording() const { return m_recording; }

    void setFile(const QString& fileName)
    {
        const QString trimmed = fileName.trimmed();
        if (trimmed == m_file) {
            return;
        }
        m_file = trimmed;
        if (ToAudioFileInterface* widget = _widget.data()) {
            QSignalBlocker blocker(widget->fileEdit());
            widget->fileEdit()->setText(m_file);
        }
        emit fileChanged(m_file);
    }

    void setOutputDir(const QString& dir)
    {
        const QString trimmed = dir.trimmed();
        if (trimmed == m_outputDir) {
            return;
        }
        m_outputDir = trimmed;
        if (ToAudioFileInterface* widget = _widget.data()) {
            widget->setOutputDir(m_outputDir);
        }
        emit outputDirChanged(m_outputDir);
    }

public slots:
    void setRecording(bool recording)
    {
        if (m_shuttingDown.load()) {
            return;
        }
        if (recording == m_recording) {
            return;
        }
        if (recording) {
            startRecording();
        } else {
            stopRecording();
        }
    }

    void startRecording()
    {
        if (m_shuttingDown.load() || m_recording) {
            return;
        }
        if (m_file.isEmpty()) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, QStringLiteral("请设置输出文件名"));
            if (ToAudioFileInterface* widget = _widget.data()) {
                widget->setRecording(false);
            }
            return;
        }

        m_recording = true;
        m_recordChannelCount = audioPortCount();
        m_nextTimestamp = TimestampGenerator::getInstance()->getCurrentFrameCount();
        const int intervalMs = qMax(1, static_cast<int>(1000.0 / TimestampGenerator::getInstance()->getFrameRate()));
        if (_pollTimer) {
            _pollTimer->start(intervalMs);
        }
        if (ToAudioFileInterface* widget = _widget.data()) {
            widget->setRecording(true);
        }
        emit recordingChanged(m_recording);
        updateRecordingOutput();
        updateNodeState(QtNodes::NodeValidationState::State::Valid);
    }

    void stopRecording()
    {
        if (!m_recording && !m_encoder.isOpen()) {
            return;
        }

        m_recording = false;
        m_recordChannelCount = 0;
        if (_pollTimer) {
            _pollTimer->stop();
        }
        m_encoder.close();
        if (m_shuttingDown.load()) {
            return;
        }
        if (ToAudioFileInterface* widget = _widget.data()) {
            widget->setRecording(false);
        }
        emit recordingChanged(m_recording);
        updateRecordingOutput();
    }

signals:
    void fileChanged(const QString& file);
    void outputDirChanged(const QString& dir);
    void recordingChanged(bool recording);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/file"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/outputDir"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/recording"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

private Q_SLOTS:
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (m_shuttingDown.load() || ev.kind != GlobalEventKind::Command) {
            return;
        }
        const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
        if (localPath == "file") {
            setFile(ev.payload.toString());
        } else if (localPath == "outputDir") {
            setOutputDir(ev.payload.toString());
        } else if (localPath == "recording") {
            setRecording(ev.payload.toBool());
        }
    }

    void updateRecordingOutput()
    {
        if (m_shuttingDown.load()) {
            return;
        }
        m_outRecording = std::make_shared<VariableData>(m_recording);
        emit dataUpdated(0);
    }

    void pollAudioFrame()
    {
        if (m_shuttingDown.load() || !m_recording || m_recordChannelCount <= 0 || m_audioQueues.empty()) {
            return;
        }

        AudioFrame frame;
        if (!tryFetchMergedFrame(frame)) {
            return;
        }
        ++m_nextTimestamp;

        if (!m_encoder.isOpen()) {
            const QString outputPath = resolveOutputPath(m_outputDir, m_file);
            QString error;
            if (!m_encoder.open(outputPath, frame.sampleRate, m_recordChannelCount, &error)) {
                updateNodeState(QtNodes::NodeValidationState::State::Error, error);
                stopRecording();
                return;
            }
        }

        QString error;
        if (!m_encoder.writeFrame(frame, &error)) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, error);
            stopRecording();
        }
    }

    int audioPortCount() const
    {
        return qMax(0, static_cast<int>(InPortCount) - kControlPortCount);
    }

    static int frameSamplesPerChannel(const AudioFrame& frame)
    {
        const int bytesPerSample = qMax(1, frame.bitsPerSample / 8);
        const int channels = qMax(1, frame.channels);
        return frame.data.size() / (bytesPerSample * channels);
    }

    bool tryFetchMergedFrame(AudioFrame& out) const
    {
        std::map<int, AudioFrame> portFrames;
        for (const auto& [portIndex, queue] : m_audioQueues) {
            if (portIndex < 0 || portIndex >= m_recordChannelCount || !queue) {
                continue;
            }

            AudioFrame frame;
            if (queue->getFrameByTimestamp(m_nextTimestamp, frame) && !frame.data.isEmpty()) {
                portFrames[portIndex] = frame;
            }
        }

        if (portFrames.empty()) {
            return false;
        }

        const AudioFrame& ref = portFrames.begin()->second;
        const int bytesPerSample = qMax(1, ref.bitsPerSample / 8);
        int samplesPerChannel = frameSamplesPerChannel(ref);
        for (const auto& [portIndex, frame] : portFrames) {
            Q_UNUSED(portIndex)
            samplesPerChannel = qMin(samplesPerChannel, frameSamplesPerChannel(frame));
        }
        if (samplesPerChannel <= 0) {
            return false;
        }

        QByteArray merged(samplesPerChannel * m_recordChannelCount * bytesPerSample, Qt::Uninitialized);
        merged.fill(0);

        for (const auto& [portIndex, frame] : portFrames) {
            const int srcChannels = qMax(1, frame.channels);
            for (int sample = 0; sample < samplesPerChannel; ++sample) {
                const int dstOffset = (sample * m_recordChannelCount + portIndex) * bytesPerSample;
                const int srcOffset = (sample * srcChannels) * bytesPerSample;
                if (srcOffset + bytesPerSample <= frame.data.size()) {
                    memcpy(merged.data() + dstOffset, frame.data.constData() + srcOffset, bytesPerSample);
                }
            }
        }

        out.data = merged;
        out.sampleRate = ref.sampleRate;
        out.channels = m_recordChannelCount;
        out.bitsPerSample = ref.bitsPerSample;
        out.timestamp = m_nextTimestamp;
        return true;
    }

private:
    QPointer<ToAudioFileInterface> _widget;
    std::shared_ptr<VariableData> m_outRecording;
    QTimer* _pollTimer = nullptr;
    FfmpegAudioEncoder m_encoder;
    std::map<int, std::shared_ptr<AudioTimestampRingQueue>> m_audioQueues;
    QString m_file;
    QString m_outputDir;
    qint64 m_nextTimestamp = 0;
    int m_recordChannelCount = 0;
    bool m_recording = false;
    std::atomic<bool> m_shuttingDown{false};
};
} // namespace Nodes
