#pragma once

/**
 * @file ViosoDataModel.hpp
 * @brief Vioso 节点：用 `.vwf` 对输入图像做 Warp + Blend
 *
 * 加载 `.vwf` 后按通道数自动展开多路 IMAGE（IMAGE 0…N-1），一帧内渲染全部投影机。
 * 单机校准文件即为 1 路输出。
 *
 * V 轴：输入翻 V → VWB_render → 输出翻 V（翻转在 runGl 外侧，避免死锁）。
 */

#include "ViosoInterface.hpp"
#include "ViosoWarper.hpp"

#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuPass.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "NodeDataList.hpp"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSignalBlocker>
#include <QtNodes/NodeDelegateModel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

#include <memory>
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
namespace
{

constexpr int kViosoMaxChannels = 64;

qint64 currentSystemTimestamp()
{
    return TimestampGenerator::getInstance()->getCurrentFrameCount();
}

bool resolveInputGpuFrame(const std::shared_ptr<ImageData>& imageData,
                          qint64 targetTimestamp,
                          ImageFrame& frame,
                          qint64 searchBackRange = 64)
{
    frame = ImageFrame();
    if (!imageData || !imageData->isConnectedToSharedBuffer()) {
        return false;
    }
    const auto buffer = imageData->getSharedImageBuffer();
    if (!buffer) {
        return false;
    }

    const qint64 normalized = targetTimestamp >= 0
                                  ? targetTimestamp
                                  : currentSystemTimestamp();
    for (qint64 offset = 0; offset <= searchBackRange; ++offset) {
        const qint64 candidate = normalized - offset;
        if (candidate < 0) {
            break;
        }
        if (buffer->getFrameByTimestamp(candidate, frame) && !frame.empty()) {
            return frame.ensureGpuTexture();
        }
    }

    ImageFrame latest;
    if (buffer->getLatestFrame(latest) && !latest.empty()) {
        frame = latest;
        return frame.ensureGpuTexture();
    }
    return false;
}

void pushGpuResult(const std::shared_ptr<ImageTimestampRingQueue>& outBuffer,
                   GpuTextureHandle&& texture,
                   qint64& lastPushedTimestamp)
{
    if (!outBuffer || !texture.valid()) {
        return;
    }
    const qint64 outputTimestamp = currentSystemTimestamp();
    ImageFrame outFrame = ImageFrame::fromTexture(std::move(texture), outputTimestamp);
    if (outBuffer->pushFrame(std::move(outFrame))) {
        lastPushedTimestamp = outputTimestamp;
    }
}

QString resolveVwfAbsolutePath(const QString& mediaRelativeOrAbsolute)
{
    const QString trimmed = mediaRelativeOrAbsolute.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    const QFileInfo info(trimmed);
    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }
    return AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QStringLiteral("/") + trimmed;
}

GpuTextureHandle flipVerticalFull(const GpuTextureHandle& src)
{
    if (!src.valid()) {
        return {};
    }
    return ImageGpuPass::instance().resample(
        src, src.width, src.height, 0.f, 0.f, 1.f, 1.f, false, true);
}

} // namespace

class ViosoDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString configFile READ configFile WRITE setConfigFile NOTIFY configFileChanged DESIGNABLE false)
    Q_PROPERTY(QString passkey READ passkey WRITE setPasskey NOTIFY passkeyChanged DESIGNABLE false)
    Q_PROPERTY(bool reload READ reload WRITE setReload NOTIFY reloadChanged DESIGNABLE false)

public:
    ViosoDataModel()
    {
        InPortCount = 1;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Vioso";
        WidgetEmbeddable = true;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "configFile";
            binding.control = m_ui->fileSelectComboBox;
            AbstractDelegateModel::registerExternalBinding("/configFile", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "passkey";
            binding.control = m_ui->passkeyEdit;
            AbstractDelegateModel::registerExternalBinding("/passkey", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "reload";
            binding.control = m_ui->reloadButton;
            AbstractDelegateModel::registerExternalBinding("/reload", this, binding);
        }

        connect(m_ui->fileSelectComboBox, &SelectorComboBox::textChanged,
                this, &ViosoDataModel::setConfigFile);
        connect(this, &ViosoDataModel::configFileChanged, this, [this](const QString&) {
            QSignalBlocker blocker(m_ui->fileSelectComboBox);
            m_ui->fileSelectComboBox->setText(m_configFile);
        });

        connect(m_ui->passkeyEdit, &QLineEdit::editingFinished, this, [this]() {
            setPasskey(m_ui->passkeyEdit->text());
        });
        connect(this, &ViosoDataModel::passkeyChanged, this, [this](const QString&) {
            QSignalBlocker blocker(m_ui->passkeyEdit);
            if (m_ui->passkeyEdit->text() != m_passkey) {
                m_ui->passkeyEdit->setText(m_passkey);
            }
        });

        connect(m_ui->reloadButton, &QPushButton::clicked, this, [this]() {
            setReload(true);
        });

        ensureOutputSlots(1);

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64 frameCount) {
                    if (m_lastRequestedFrame == frameCount && !m_paramsDirty && !m_needsReinit) {
                        return;
                    }
                    m_lastRequestedFrame = frameCount;
                    requestProcess(frameCount);
                },
                Qt::QueuedConnection);
    }

    ~ViosoDataModel() override
    {
        GlobalEventBus::instance()->unsubscribe(this);
        m_warpers.clear();
    }

    QString configFile() const { return m_configFile; }
    QString passkey() const { return m_passkey; }
    bool reload() const { return m_reload; }

    QWidget* embeddedWidget() override { return m_ui; }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex);
        return ImageData().type();
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            return QStringLiteral("IMAGE");
        }
        if (OutPortCount > 1) {
            return QStringLiteral("IMAGE %1").arg(portIndex);
        }
        return QStringLiteral("IMAGE");
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        if (port < 0 || static_cast<size_t>(port) >= m_outputs.size()) {
            return {};
        }
        return m_outputs[static_cast<size_t>(port)].imageData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (portIndex != 0) {
            return;
        }
        m_inImage = std::dynamic_pointer_cast<ImageData>(data);
        ensureOutputSlots(OutPortCount);
        m_lastProcessedInputTimestamp = -1;
        m_paramsDirty = true;
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values[QStringLiteral("configFile")] = m_configFile;
        values[QStringLiteral("passkey")] = m_passkey;
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson[QStringLiteral("values")] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data[QStringLiteral("values")].toObject();
        m_configFile = values.value(QStringLiteral("configFile")).toString().trimmed();
        m_passkey = values.value(QStringLiteral("passkey")).toString();
        m_initFailed = false;
        m_needsReinit = true;
        m_paramsDirty = true;

        {
            QSignalBlocker blocker(m_ui->fileSelectComboBox);
            m_ui->fileSelectComboBox->setText(m_configFile);
        }
        {
            QSignalBlocker blocker(m_ui->passkeyEdit);
            m_ui->passkeyEdit->setText(m_passkey);
        }
        syncOutputPorts(/*notifyPorts=*/false);
    }

public slots:
    void setConfigFile(const QString& path)
    {
        const QString trimmed = path.trimmed();
        if (trimmed == m_configFile) {
            return;
        }
        m_configFile = trimmed;
        m_initFailed = false;
        m_needsReinit = true;
        m_paramsDirty = true;
        syncOutputPorts(true);
        Q_EMIT configFileChanged(m_configFile);
    }

    void setPasskey(const QString& key)
    {
        if (key == m_passkey) {
            return;
        }
        m_passkey = key;
        m_initFailed = false;
        m_needsReinit = true;
        m_paramsDirty = true;
        Q_EMIT passkeyChanged(m_passkey);
    }

    void setReload(bool value)
    {
        if (!value || m_reload) {
            return;
        }
        m_reload = true;
        Q_EMIT reloadChanged(true);

        // Reload 时同步输入框里尚未触发 editingFinished 的内容
        setPasskey(m_ui->passkeyEdit->text());

        m_initFailed = false;
        m_needsReinit = true;
        m_paramsDirty = true;
        syncOutputPorts(true);

        m_reload = false;
        Q_EMIT reloadChanged(false);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/configFile")) {
            setConfigFile(ev.payload.toString());
        } else if (ev.address == makeFullOscAddress("/passkey")) {
            setPasskey(ev.payload.toString());
        } else if (ev.address == makeFullOscAddress("/reload")) {
            setReload(ev.payload.toBool());
        }
    }

signals:
    void configFileChanged(const QString& path);
    void passkeyChanged(const QString& key);
    void reloadChanged(bool value);

private:
    struct OutputSlot {
        std::shared_ptr<ImageData> imageData;
        std::shared_ptr<ImageTimestampRingQueue> buffer;
        qint64 lastPushedTimestamp = -1;
    };

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/configFile"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/passkey"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/reload"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    void syncOutputPorts(bool notifyPorts)
    {
        unsigned int newCount = 1;
        const QString absVwf = resolveVwfAbsolutePath(m_configFile);
        int channels = 0;
        QString error;
        if (!absVwf.isEmpty()
            && ViosoWarper::queryChannelCount(absVwf, channels, &error)
            && channels > 0) {
            newCount = static_cast<unsigned int>(qBound(1, channels, kViosoMaxChannels));
        } else if (!error.isEmpty()) {
            updateNodeState(QtNodes::NodeValidationState::State::Error, error);
        }

        applyOutputPortCount(newCount, notifyPorts);
        ensureOutputSlots(newCount);
    }

    void applyOutputPortCount(unsigned int newCount, bool notifyPorts)
    {
        const unsigned int oldCount = OutPortCount;
        if (newCount == oldCount) {
            return;
        }

        if (notifyPorts) {
            if (newCount > oldCount) {
                Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, newCount - 1);
                OutPortCount = newCount;
                Q_EMIT portsInserted();
            } else {
                Q_EMIT portsAboutToBeDeleted(PortType::Out, newCount, oldCount - 1);
                OutPortCount = newCount;
                Q_EMIT portsDeleted();
            }
            Q_EMIT embeddedWidgetSizeUpdated();
        } else {
            OutPortCount = newCount;
        }
    }

    void ensureOutputSlots(unsigned int count)
    {
        if (count == 0) {
            count = 1;
        }

        if (m_outputs.size() > count) {
            m_outputs.resize(count);
        } else {
            while (m_outputs.size() < count) {
                OutputSlot slot;
                ensureImageDataBuffer(slot.imageData, slot.buffer);
                m_outputs.push_back(std::move(slot));
            }
        }

        for (auto& slot : m_outputs) {
            ensureImageDataBuffer(slot.imageData, slot.buffer);
        }

        if (m_warpers.size() > count) {
            m_warpers.resize(count);
        } else {
            while (m_warpers.size() < count) {
                m_warpers.push_back(std::make_unique<ViosoWarper>());
            }
        }
    }

    void clearOutput()
    {
        for (auto& slot : m_outputs) {
            if (slot.buffer) {
                slot.buffer->clear();
            }
            slot.lastPushedTimestamp = -1;
        }
        m_lastProcessedInputTimestamp = -1;
        m_paramsDirty = false;
    }

    void requestProcess(qint64 targetTimestamp = -1)
    {
        ensureOutputSlots(OutPortCount);

        if (!m_inImage || imageDataIsEmpty(m_inImage)) {
            clearOutput();
            return;
        }

        const QString absVwf = resolveVwfAbsolutePath(m_configFile);
        if (absVwf.isEmpty()) {
            clearOutput();
            return;
        }

        // Init 已失败且用户未改文件/点 Reload：禁止每帧再试（避免 VIOSO 泄漏导致内存暴涨）
        if (m_initFailed && !m_needsReinit) {
            return;
        }

        if (m_needsReinit) {
            syncOutputPorts(true);
        }

        ImageFrame inputFrame;
        if (!resolveInputGpuFrame(m_inImage, targetTimestamp, inputFrame)) {
            if (!m_inImage || !m_inImage->isConnectedToSharedBuffer()) {
                clearOutput();
            }
            return;
        }

        if (!m_paramsDirty && !m_needsReinit
            && inputFrame.timestamp == m_lastProcessedInputTimestamp) {
            return;
        }

        const bool needsReinit = m_needsReinit;
        const unsigned int outCount = OutPortCount;

        QByteArray cryptoKey16;
        QString keyParseError;
        if (!ViosoWarper::parseCryptoKey(m_passkey, cryptoKey16, &keyParseError)) {
            m_initFailed = true;
            m_needsReinit = false;
            m_paramsDirty = false;
            updateNodeState(QtNodes::NodeValidationState::State::Error, keyParseError);
            clearOutput();
            return;
        }

        GpuTextureHandle srcForWarp = flipVerticalFull(inputFrame.texture);
        if (!srcForWarp.valid()) {
            clearOutput();
            return;
        }

        std::vector<GpuTextureHandle> rawOuts(outCount);
        QString renderError;

        const bool ok = ImageGpuUpload::instance().runGl(
            [&](QOpenGLFunctions* f) -> bool {
                for (unsigned int i = 0; i < outCount; ++i) {
                    if (i >= m_warpers.size() || !m_warpers[i]) {
                        renderError = QStringLiteral("warper slot %1 missing").arg(i);
                        return false;
                    }

                    auto& warper = *m_warpers[i];
                    if (needsReinit || !warper.isReady()) {
                        if (!warper.ensureReady(f, absVwf, static_cast<int>(i), cryptoKey16)) {
                            renderError = warper.lastError();
                            return false;
                        }
                    }
                    rawOuts[i] = warper.render(f, srcForWarp);
                    if (!rawOuts[i].valid()) {
                        renderError = warper.lastError();
                        return false;
                    }
                }
                return true;
            });

        if (!ok) {
            // 锁定失败态，避免每帧 VWB_Create/Init（加密 .vwf 缺 passkey 时会打爆内存）
            m_initFailed = true;
            m_needsReinit = false;
            m_paramsDirty = false;
            if (!renderError.isEmpty()) {
                QString msg = renderError;
                if (msg.contains(QStringLiteral("VWB_Init"), Qt::CaseInsensitive)) {
                    msg += QStringLiteral(
                        " — if VIOSO log says \"Passkey missing\", enter the 16-byte AES key "
                        "(32 hex chars) in Passkey, or export an unencrypted .vwf.");
                }
                updateNodeState(QtNodes::NodeValidationState::State::Error, msg);
            }
            rawOuts.clear();
            clearOutput();
            return;
        }

        m_initFailed = false;
        m_needsReinit = false;
        updateNodeState(QtNodes::NodeValidationState::State::Valid);

        for (unsigned int i = 0; i < outCount; ++i) {
            GpuTextureHandle flipped = flipVerticalFull(rawOuts[i]);
            rawOuts[i] = {};
            if (!flipped.valid() || i >= m_outputs.size()) {
                clearOutput();
                return;
            }
            pushGpuResult(m_outputs[i].buffer, std::move(flipped), m_outputs[i].lastPushedTimestamp);
        }

        m_lastProcessedInputTimestamp = inputFrame.timestamp;
        m_paramsDirty = false;
    }

    ViosoInterface* m_ui = new ViosoInterface();

    std::shared_ptr<ImageData> m_inImage;
    std::vector<OutputSlot> m_outputs;
    std::vector<std::unique_ptr<ViosoWarper>> m_warpers;

    QString m_configFile;
    QString m_passkey;
    bool m_reload = false;

    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestamp = -1;
    bool m_paramsDirty = false;
    bool m_needsReinit = true;
    bool m_initFailed = false; ///< Init/渲染失败后锁定，直到换文件或 Reload
};

} // namespace Nodes
