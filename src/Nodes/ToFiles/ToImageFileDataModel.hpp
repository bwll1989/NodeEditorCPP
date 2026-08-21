#pragma once

#include <QDir>
#include <QElapsedTimer>

#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageReadback.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "FfmpegWriters.hpp"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "ToFilePath.hpp"
#include "ToImageFileInterface.hpp"

#include <QPointer>
#include <atomic>

using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace ToImageFileDetail
{
inline cv::Mat readLatestMatForWrite(const std::shared_ptr<ImageData>& input)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return {};
    }
    return ImageReadback::matFromFrame(frame);
}
} // namespace

namespace Nodes
{
class ToImageFileDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)

public:
    ToImageFileDataModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        m_outRecording = std::make_shared<VariableData>(false);
        CaptionVisible = true;
        Caption = QStringLiteral("To Image File");
        WidgetEmbeddable = true;
        Resizable = false;

        m_file = QStringLiteral("capture.png");
        _widget = new ToImageFileInterface();

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

        connect(_widget, &ToImageFileInterface::fileTextChanged, this, &ToImageFileDataModel::setFile);
        connect(_widget, &ToImageFileInterface::outputDirChanged, this, &ToImageFileDataModel::setOutputDir);
        connect(_widget, &ToImageFileInterface::recordingToggled, this, &ToImageFileDataModel::setRecording);
    }

    ~ToImageFileDataModel() override
    {
        m_shuttingDown.store(true);
        disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
        GlobalEventBus::instance()->unsubscribe(this);
        if (_widget) {
            disconnect(_widget.data(), nullptr, this, nullptr);
        }
        stopRecording();
        m_inImage.reset();
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return VariableData().type();
        }
        switch (portIndex) {
        case 0:
            return ImageData().type();
        case 1:
        case 2:
            return VariableData().type();
        default:
            return VariableData().type();
        }
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return portIndex == 0 ? QStringLiteral("RECORDING") : QString();
        }
        switch (portIndex) {
        case 0:
            return QStringLiteral("IMAGE");
        case 1:
            return QStringLiteral("START");
        case 2:
            return QStringLiteral("STOP");
        default:
            return {};
        }
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, PortIndex portIndex) override
    {
        switch (portIndex) {
        case 0: {
            m_inImage = std::dynamic_pointer_cast<ImageData>(nodeData);
            m_lastSeenInputTimestamp = -1;
            break;
        }
        case 1: {
            const auto trigger = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (trigger && trigger->asBool()) {
                startRecording();
            }
            break;
        }
        case 2: {
            const auto trigger = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (trigger && trigger->asBool()) {
                stopRecording();
            }
            break;
        }
        default:
            break;
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
        if (ToImageFileInterface* widget = _widget.data()) {
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
        if (ToImageFileInterface* widget = _widget.data()) {
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
            if (ToImageFileInterface* widget = _widget.data()) {
                widget->setRecording(false);
            }
            return;
        }

        m_recording = true;
        m_lastSeenInputTimestamp = -1;
        m_saveTimer.invalidate();
        if (ToImageFileInterface* widget = _widget.data()) {
            widget->setRecording(true);
        }
        emit recordingChanged(m_recording);
        AbstractDelegateModel::stateFeedBack("/recording", m_recording);
        updateRecordingOutput();
        trySaveFrame();
    }

    void stopRecording()
    {
        if (!m_recording) {
            return;
        }

        m_recording = false;
        if (m_shuttingDown.load()) {
            return;
        }
        if (ToImageFileInterface* widget = _widget.data()) {
            widget->setRecording(false);
        }
        emit recordingChanged(m_recording);
        AbstractDelegateModel::stateFeedBack("/recording", m_recording);
        updateRecordingOutput();
    }

signals:
    void fileChanged(const QString& file);
    void outputDirChanged(const QString& dir);
    void recordingChanged(bool recording);

protected:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/file"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/outputDir"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/recording"), this, SLOT(onGlobalEvent(GlobalEvent)));

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [self = QPointer<ToImageFileDataModel>(this)](qint64) {
                    if (!self || self->m_shuttingDown.load()) {
                        return;
                    }
                    self->trySaveFrame();
                },
                Qt::QueuedConnection);
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

    /** 系统 tick：录制中按帧率间隔写文件 */
    void trySaveFrame()
    {
        if (m_shuttingDown.load() || !m_recording || m_saving || !m_inImage
            || imageDataIsEmpty(m_inImage)) {
            return;
        }

        ImageFrame peek;
        if (!getLatestImageFrame(m_inImage, peek) || peek.empty()) {
            return;
        }
        if (peek.timestamp >= 0 && peek.timestamp <= m_lastSeenInputTimestamp) {
            return;
        }

        const int intervalMs = qMax(
            1,
            static_cast<int>(1000.0 / TimestampGenerator::getInstance()->getFrameRate()));
        if (m_saveTimer.isValid() && m_saveTimer.elapsed() < intervalMs) {
            return;
        }

        if (m_file.isEmpty()) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, QStringLiteral("请设置输出文件名"));
            stopRecording();
            return;
        }

        cv::Mat mat = ToImageFileDetail::readLatestMatForWrite(m_inImage);
        if (mat.empty()) {
            return;
        }

        m_lastSeenInputTimestamp = peek.timestamp;
        m_saveTimer.start();

        m_saving = true;
        const QString outputPath = resolveOutputPath(m_outputDir, m_file);

        QString error;
        const bool ok = FfmpegImageWriter::saveImage(mat, outputPath, &error);
        m_saving = false;

        if (!ok) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, error);
            stopRecording();
            return;
        }

        updateNodeState(QtNodes::NodeValidationState::State::Valid);
    }

private:
    QPointer<ToImageFileInterface> _widget;
    std::shared_ptr<VariableData> m_outRecording;
    std::shared_ptr<ImageData> m_inImage;
    qint64 m_lastSeenInputTimestamp = -1;
    QElapsedTimer m_saveTimer;
    QString m_file;
    QString m_outputDir;
    bool m_recording = false;
    bool m_saving = false;
    std::atomic<bool> m_shuttingDown{false};
};
} // namespace Nodes
