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
#include "ToVideoFileInterface.hpp"

#include <QPointer>
#include <atomic>

using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace ToVideoFileDetail
{
inline cv::Mat readLatestMatForWrite(const std::shared_ptr<ImageData>& input,
                                     qint64* outSourceTimestamp = nullptr)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return {};
    }
    if (outSourceTimestamp) {
        *outSourceTimestamp = frame.timestamp;
    }
    return ImageReadback::matFromFrame(frame);
}
} // namespace

namespace Nodes
{
class ToVideoFileDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(double fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(bool recording READ recording WRITE setRecording NOTIFY recordingChanged)

public:
    ToVideoFileDataModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        m_outRecording = std::make_shared<VariableData>(false);
        CaptionVisible = true;
        Caption = QStringLiteral("To Video File");
        WidgetEmbeddable = true;
        Resizable = false;

        m_file = QStringLiteral("output.mp4");
        _widget = new ToVideoFileInterface();
        _widget->setFps(m_fps);

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "file";
            binding.control = _widget->fileEdit();
            AbstractDelegateModel::registerExternalBinding("/file", this, binding);
        }
        if (_widget->fpsWidget()) {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "fps";
            binding.control = _widget->fpsWidget();
            AbstractDelegateModel::registerExternalBinding("/fps", this, binding);
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

        connect(_widget, &ToVideoFileInterface::fileTextChanged, this, &ToVideoFileDataModel::setFile);
        connect(_widget, &ToVideoFileInterface::outputDirChanged, this, &ToVideoFileDataModel::setOutputDir);
        connect(_widget, &ToVideoFileInterface::fpsChanged, this, &ToVideoFileDataModel::setFps);
        connect(_widget, &ToVideoFileInterface::recordingToggled, this, &ToVideoFileDataModel::setRecording);
    }

    ~ToVideoFileDataModel() override
    {
        m_shuttingDown.store(true);
        disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
        GlobalEventBus::instance()->unsubscribe(this);
        if (_widget) {
            disconnect(_widget.data(), nullptr, this, nullptr);
        }
        stopRecording();
        m_inImage.reset();
        m_cachedWriteMat.release();
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
            break;
        }
        case 1: {
            const auto trigger = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (trigger && trigger->value().toBool()) {
                startRecording();
            }
            break;
        }
        case 2: {
            const auto trigger = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (trigger && trigger->value().toBool()) {
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
        modelJson["fps"] = m_fps;
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
        if (jsonObj.contains("fps")) {
            setFps(jsonObj["fps"].toDouble(25.0));
        }
    }

    QString file() const { return m_file; }
    QString outputDir() const { return m_outputDir; }
    double fps() const { return m_fps; }
    bool recording() const { return m_recording; }

    void setFile(const QString& fileName)
    {
        const QString trimmed = fileName.trimmed();
        if (trimmed == m_file) {
            return;
        }
        m_file = trimmed;
        if (ToVideoFileInterface* widget = _widget.data()) {
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
        if (ToVideoFileInterface* widget = _widget.data()) {
            widget->setOutputDir(m_outputDir);
        }
        emit outputDirChanged(m_outputDir);
    }

    void setFps(double value)
    {
        const double clamped = qBound(1.0, value, 120.0);
        if (qFuzzyCompare(m_fps, clamped)) {
            return;
        }
        if (m_recording) {
            return;
        }
        m_fps = clamped;
        if (ToVideoFileInterface* widget = _widget.data()) {
            widget->setFps(m_fps);
        }
        emit fpsChanged(m_fps);
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
            if (ToVideoFileInterface* widget = _widget.data()) {
                widget->setRecording(false);
            }
            return;
        }

        m_recording = true;
        m_writtenFrameCount = 0;
        m_cachedSourceTimestamp = -1;
        m_cachedWriteMat.release();
        m_recordingClock.start();
        if (ToVideoFileInterface* widget = _widget.data()) {
            widget->setRecording(true);
        }
        emit recordingChanged(m_recording);
        updateRecordingOutput();
        updateNodeState(QtNodes::NodeValidationState::State::Valid);
        tryCaptureFrame();
    }

    void stopRecording()
    {
        if (!m_recording && !m_encoder.isOpen()) {
            return;
        }

        m_recording = false;
        m_encoder.close();
        if (m_shuttingDown.load()) {
            return;
        }
        if (ToVideoFileInterface* widget = _widget.data()) {
            widget->setRecording(false);
        }
        emit recordingChanged(m_recording);
        updateRecordingOutput();
    }

signals:
    void fileChanged(const QString& file);
    void outputDirChanged(const QString& dir);
    void fpsChanged(double fps);
    void recordingChanged(bool recording);

protected:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/file"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/outputDir"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/fps"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/recording"), this, SLOT(onGlobalEvent(GlobalEvent)));

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [self = QPointer<ToVideoFileDataModel>(this)](qint64) {
                    if (!self || self->m_shuttingDown.load()) {
                        return;
                    }
                    self->tryCaptureFrame();
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
        } else if (localPath == "fps") {
            setFps(ev.payload.toDouble());
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

    /** 按 wall-clock + m_fps 采样；上游帧率低于录制 fps 时重复写入同一帧 */
    void tryCaptureFrame()
    {
        if (m_shuttingDown.load() || !m_recording || !m_inImage || imageDataIsEmpty(m_inImage)) {
            return;
        }

        if (!m_recordingClock.isValid()) {
            return;
        }

        refreshWriteCache();
        if (m_cachedWriteMat.empty()) {
            return;
        }

        const qint64 elapsedMs = m_recordingClock.elapsed();
        const qint64 targetFrameCount =
            static_cast<qint64>(elapsedMs * m_fps / 1000.0);

        while (m_writtenFrameCount < targetFrameCount) {
            appendFrame(m_cachedWriteMat);
            ++m_writtenFrameCount;
        }
    }

    void refreshWriteCache()
    {
        qint64 sourceTimestamp = -1;
        const cv::Mat mat =
            ToVideoFileDetail::readLatestMatForWrite(m_inImage, &sourceTimestamp);
        if (mat.empty()) {
            return;
        }
        if (sourceTimestamp == m_cachedSourceTimestamp && !m_cachedWriteMat.empty()) {
            return;
        }
        m_cachedSourceTimestamp = sourceTimestamp;
        m_cachedWriteMat = mat;
    }

    void appendFrame(const cv::Mat& frame)
    {
        if (m_shuttingDown.load()) {
            return;
        }
        if (!m_encoder.isOpen()) {
            const QString outputPath = resolveOutputPath(m_outputDir, m_file);
            QString error;
            if (!m_encoder.open(outputPath, frame.cols, frame.rows, m_fps, &error)) {
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

private:
    QPointer<ToVideoFileInterface> _widget;
    std::shared_ptr<VariableData> m_outRecording;
    std::shared_ptr<ImageData> m_inImage;
    qint64 m_writtenFrameCount = 0;
    qint64 m_cachedSourceTimestamp = -1;
    cv::Mat m_cachedWriteMat;
    FfmpegVideoEncoder m_encoder;
    QElapsedTimer m_recordingClock;
    QString m_file;
    QString m_outputDir;
    double m_fps = 25.0;
    bool m_recording = false;
    std::atomic<bool> m_shuttingDown{false};
};
} // namespace Nodes
