#pragma once

#include <QFile>
#include <QTimer>

#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/VariableData.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "ToFilePath.hpp"
#include "ToTextFileInterface.hpp"

using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
class ToTextFileDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)

public:
    ToTextFileDataModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        m_outRecording = std::make_shared<VariableData>(false);
        CaptionVisible = true;
        Caption = QStringLiteral("To Text File");
        WidgetEmbeddable = true;
        Resizable = false;

        m_file = QStringLiteral("output.json");
        _widget = new ToTextFileInterface();

        _saveTimer = new QTimer(this);
        _saveTimer->setSingleShot(true);
        connect(_saveTimer, &QTimer::timeout, this, &ToTextFileDataModel::saveCurrentData);

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

        connect(_widget, &ToTextFileInterface::fileTextChanged, this, &ToTextFileDataModel::setFile);
        connect(_widget, &ToTextFileInterface::outputDirChanged, this, &ToTextFileDataModel::setOutputDir);
        connect(_widget, &ToTextFileInterface::recordingToggled, this, &ToTextFileDataModel::setRecording);
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return VariableData().type();
        }
        switch (portIndex) {
        case 0:
            return VariableData().type();
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
            return QStringLiteral("DATA");
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
            const auto variable = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (variable) {
                m_inVariable = std::make_shared<VariableData>(variable->asMap());
            } else {
                m_inVariable.reset();
            }
            if (m_recording) {
                scheduleSave();
            }
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
        return _widget;
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
        {
            QSignalBlocker blocker(_widget->fileEdit());
            _widget->fileEdit()->setText(m_file);
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
        _widget->setOutputDir(m_outputDir);
        emit outputDirChanged(m_outputDir);
    }

public slots:
    void setRecording(bool recording)
    {
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
        if (m_recording) {
            return;
        }
        if (m_file.isEmpty()) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, QStringLiteral("请设置输出文件名"));
            _widget->setRecording(false);
            return;
        }

        m_recording = true;
        _widget->setRecording(true);
        emit recordingChanged(m_recording);
        updateRecordingOutput();
        scheduleSave();
    }

    void stopRecording()
    {
        if (!m_recording) {
            return;
        }

        m_recording = false;
        _saveTimer->stop();
        _widget->setRecording(false);
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
        if (ev.kind != GlobalEventKind::Command) {
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
        m_outRecording = std::make_shared<VariableData>(m_recording);
        emit dataUpdated(0);
    }

    void scheduleSave()
    {
        if (!m_recording || m_saving || _saveTimer->isActive()) {
            return;
        }
        const int intervalMs = qMax(1, static_cast<int>(1000.0 / TimestampGenerator::getInstance()->getFrameRate()));
        _saveTimer->start(intervalMs);
    }

    void saveCurrentData()
    {
        if (!m_recording || m_saving) {
            return;
        }

        if (!m_inVariable) {
            scheduleSave();
            return;
        }
        if (m_file.isEmpty()) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, QStringLiteral("请设置输出文件名"));
            stopRecording();
            return;
        }

        m_saving = true;
        const QString outputPath = resolveOutputPath(m_outputDir, m_file);

        QFile file(outputPath);
        QString error;
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            error = QStringLiteral("无法写入文件: %1").arg(outputPath);
        } else {
            const qint64 written = file.write(m_inVariable->toJsonString().toUtf8());
            file.close();
            if (written < 0) {
                error = QStringLiteral("写入文件失败: %1").arg(outputPath);
            }
        }
        m_saving = false;

        if (!error.isEmpty()) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, error);
            stopRecording();
            return;
        }

        updateNodeState(QtNodes::NodeValidationState::State::Valid);
        scheduleSave();
    }

private:
    ToTextFileInterface* _widget = nullptr;
    std::shared_ptr<VariableData> m_outRecording;
    QTimer* _saveTimer = nullptr;
    std::shared_ptr<VariableData> m_inVariable;
    QString m_file;
    QString m_outputDir;
    bool m_recording = false;
    bool m_saving = false;
};
} // namespace Nodes
