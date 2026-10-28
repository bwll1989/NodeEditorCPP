#pragma once

/**
 * @file MpcdiDataModel.hpp
 * @brief MPCDI 节点：用 `.mpcdi` 对输入图像做 Warp + Blend
 *
 * 选定 buffer 后按 region 展开多路 IMAGE。单投影机文件即为 1 路。
 * V 轴与 Vioso 相同：输入翻 V → warp → 输出翻 V，翻转在 runGl 外侧。
 */

#include "MpcdiInterface.hpp"
#include "MpcdiWarper.hpp"

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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QWidget>

#include <algorithm>
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

    const qint64 normalized = targetTimestamp >= 0 ? targetTimestamp : currentSystemTimestamp();
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

QString resolveMpcdiAbsolutePath(const QString& mediaRelativeOrAbsolute)
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

bool jsonBool(const QJsonObject& values, const QString& key, bool fallback)
{
    return values.contains(key) ? values.value(key).toBool() : fallback;
}

void syncCheck(QCheckBox* box, bool value)
{
    QSignalBlocker blocker(box);
    box->setChecked(value);
}

} // namespace

class MpcdiDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString configFile READ configFile WRITE setConfigFile NOTIFY configFileChanged DESIGNABLE false)
    Q_PROPERTY(QString bufferId READ bufferId WRITE setBufferId NOTIFY bufferIdChanged DESIGNABLE false)
    Q_PROPERTY(int coordMode READ coordMode WRITE setCoordMode NOTIFY coordModeChanged DESIGNABLE false)
    Q_PROPERTY(bool flipY READ flipY WRITE setFlipY NOTIFY flipYChanged DESIGNABLE false)
    Q_PROPERTY(bool flipWarpY READ flipWarpY WRITE setFlipWarpY NOTIFY flipWarpYChanged DESIGNABLE false)
    Q_PROPERTY(bool regionLocal READ regionLocal WRITE setRegionLocal NOTIFY regionLocalChanged DESIGNABLE false)
    Q_PROPERTY(bool blend READ blend WRITE setBlend NOTIFY blendChanged DESIGNABLE false)
    Q_PROPERTY(bool linearizeAlpha READ linearizeAlpha WRITE setLinearizeAlpha NOTIFY linearizeAlphaChanged DESIGNABLE false)
    Q_PROPERTY(bool blackLevel READ blackLevel WRITE setBlackLevel NOTIFY blackLevelChanged DESIGNABLE false)
    Q_PROPERTY(bool bounds READ bounds WRITE setBounds NOTIFY boundsChanged DESIGNABLE false)

public:
    MpcdiDataModel()
    {
        InPortCount = 1;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "MPCDI";
        WidgetEmbeddable = true;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "configFile";
            binding.control = m_ui->fileSelectComboBox;
            AbstractDelegateModel::registerExternalBinding("/configFile", this, binding);
        }
        bindOption("coordMode", "/coordMode", m_ui->coordModeCombo);
        bindOption("flipY", "/flipY", m_ui->flipYCheck);
        bindOption("flipWarpY", "/flipWarpY", m_ui->flipWarpYCheck);
        bindOption("regionLocal", "/regionLocal", m_ui->regionLocalCheck);
        bindOption("blend", "/blend", m_ui->blendCheck);
        bindOption("linearizeAlpha", "/linearizeAlpha", m_ui->linearizeAlphaCheck);
        bindOption("blackLevel", "/blackLevel", m_ui->blackLevelCheck);
        bindOption("bounds", "/bounds", m_ui->boundsCheck);

        connect(m_ui->fileSelectComboBox, &SelectorComboBox::textChanged,
                this, &MpcdiDataModel::setConfigFile);
        connect(this, &MpcdiDataModel::configFileChanged, this, [this](const QString&) {
            QSignalBlocker blocker(m_ui->fileSelectComboBox);
            m_ui->fileSelectComboBox->setText(m_configFile);
        });

        connect(m_ui->coordModeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
                this, &MpcdiDataModel::setCoordMode);
        connect(m_ui->flipYCheck, &QCheckBox::toggled, this, &MpcdiDataModel::setFlipY);
        connect(m_ui->flipWarpYCheck, &QCheckBox::toggled, this, &MpcdiDataModel::setFlipWarpY);
        connect(m_ui->regionLocalCheck, &QCheckBox::toggled, this, &MpcdiDataModel::setRegionLocal);
        connect(m_ui->blendCheck, &QCheckBox::toggled, this, &MpcdiDataModel::setBlend);
        connect(m_ui->linearizeAlphaCheck, &QCheckBox::toggled, this, &MpcdiDataModel::setLinearizeAlpha);
        connect(m_ui->blackLevelCheck, &QCheckBox::toggled, this, &MpcdiDataModel::setBlackLevel);
        connect(m_ui->boundsCheck, &QCheckBox::toggled, this, &MpcdiDataModel::setBounds);

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

    ~MpcdiDataModel() override
    {
        GlobalEventBus::instance()->unsubscribe(this);
        m_warp.releaseGpu();
    }

    QString configFile() const { return m_configFile; }
    QString bufferId() const { return m_bufferId; }
    int coordMode() const { return m_coordMode; }
    bool flipY() const { return m_flipY; }
    bool flipWarpY() const { return m_flipWarpY; }
    bool regionLocal() const { return m_regionLocal; }
    bool blend() const { return m_blend; }
    bool linearizeAlpha() const { return m_linearizeAlpha; }
    bool blackLevel() const { return m_blackLevel; }
    bool bounds() const { return m_bounds; }

    QWidget* embeddedWidget() override { return m_ui; }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portType);
        Q_UNUSED(portIndex);
        return ImageData().type();
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            return QStringLiteral("IMAGE");
        }
        if (OutPortCount > 1 && portIndex >= 0 && portIndex < m_warp.regionCount()) {
            return m_warp.regionId(portIndex);
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
        values[QStringLiteral("bufferId")] = m_bufferId;
        values[QStringLiteral("coordMode")] = m_coordMode;
        values[QStringLiteral("flipY")] = m_flipY;
        values[QStringLiteral("flipWarpY")] = m_flipWarpY;
        values[QStringLiteral("regionLocal")] = m_regionLocal;
        values[QStringLiteral("blend")] = m_blend;
        values[QStringLiteral("linearizeAlpha")] = m_linearizeAlpha;
        values[QStringLiteral("blackLevel")] = m_blackLevel;
        values[QStringLiteral("bounds")] = m_bounds;
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson[QStringLiteral("values")] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data[QStringLiteral("values")].toObject();
        m_configFile = values.value(QStringLiteral("configFile")).toString().trimmed();
        m_bufferId = values.value(QStringLiteral("bufferId")).toString().trimmed();
        m_coordMode = values.contains(QStringLiteral("coordMode"))
                          ? values.value(QStringLiteral("coordMode")).toInt()
                          : 0;
        if (m_coordMode < 0 || m_coordMode > 2) {
            m_coordMode = 0;
        }
        m_flipY = jsonBool(values, QStringLiteral("flipY"), true);
        m_flipWarpY = jsonBool(values, QStringLiteral("flipWarpY"), false);
        m_regionLocal = jsonBool(values, QStringLiteral("regionLocal"), false);
        m_blend = jsonBool(values, QStringLiteral("blend"), true);
        m_linearizeAlpha = jsonBool(values, QStringLiteral("linearizeAlpha"), true);
        m_blackLevel = jsonBool(values, QStringLiteral("blackLevel"), true);
        m_bounds = jsonBool(values, QStringLiteral("bounds"), false);
        m_initFailed = false;
        m_needsReinit = true;
        m_paramsDirty = true;

        {
            QSignalBlocker blocker(m_ui->fileSelectComboBox);
            m_ui->fileSelectComboBox->setText(m_configFile);
        }
        {
            QSignalBlocker blocker(m_ui->coordModeCombo);
            m_ui->coordModeCombo->setCurrentIndex(m_coordMode);
        }
        syncCheck(m_ui->flipYCheck, m_flipY);
        syncCheck(m_ui->flipWarpYCheck, m_flipWarpY);
        syncCheck(m_ui->regionLocalCheck, m_regionLocal);
        syncCheck(m_ui->blendCheck, m_blend);
        syncCheck(m_ui->linearizeAlphaCheck, m_linearizeAlpha);
        syncCheck(m_ui->blackLevelCheck, m_blackLevel);
        syncCheck(m_ui->boundsCheck, m_bounds);
        syncOutputPorts(false);
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

    void setBufferId(const QString& id)
    {
        const QString trimmed = id.trimmed();
        if (trimmed == m_bufferId) {
            return;
        }
        m_bufferId = trimmed;
        m_initFailed = false;
        m_needsReinit = true;
        m_paramsDirty = true;
        syncOutputPorts(true);
        Q_EMIT bufferIdChanged(m_bufferId);
    }

    void setFlipY(bool value) { assignBool(m_flipY, value, m_ui->flipYCheck, &MpcdiDataModel::flipYChanged); }
    void setFlipWarpY(bool value) { assignBool(m_flipWarpY, value, m_ui->flipWarpYCheck, &MpcdiDataModel::flipWarpYChanged); }
    void setRegionLocal(bool value) { assignBool(m_regionLocal, value, m_ui->regionLocalCheck, &MpcdiDataModel::regionLocalChanged); }
    void setBlend(bool value) { assignBool(m_blend, value, m_ui->blendCheck, &MpcdiDataModel::blendChanged); }
    void setLinearizeAlpha(bool value) { assignBool(m_linearizeAlpha, value, m_ui->linearizeAlphaCheck, &MpcdiDataModel::linearizeAlphaChanged); }
    void setBlackLevel(bool value) { assignBool(m_blackLevel, value, m_ui->blackLevelCheck, &MpcdiDataModel::blackLevelChanged); }
    void setBounds(bool value) { assignBool(m_bounds, value, m_ui->boundsCheck, &MpcdiDataModel::boundsChanged); }

    void setCoordMode(int value)
    {
        if (value < 0 || value > 2) {
            value = 0;
        }
        if (value == m_coordMode) {
            return;
        }
        m_coordMode = value;
        m_paramsDirty = true;
        if (m_ui->coordModeCombo->currentIndex() != value) {
            QSignalBlocker blocker(m_ui->coordModeCombo);
            m_ui->coordModeCombo->setCurrentIndex(value);
        }
        Q_EMIT coordModeChanged(value);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/configFile")) {
            setConfigFile(ev.payload.toString());
        } else if (ev.address == makeFullOscAddress("/bufferId")) {
            setBufferId(ev.payload.toString());
        } else if (ev.address == makeFullOscAddress("/coordMode")) {
            setCoordMode(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/flipY")) {
            setFlipY(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/flipWarpY")) {
            setFlipWarpY(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/regionLocal")) {
            setRegionLocal(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/blend")) {
            setBlend(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/linearizeAlpha")) {
            setLinearizeAlpha(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/blackLevel")) {
            setBlackLevel(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/bounds")) {
            setBounds(ev.payload.toBool());
        }
    }

signals:
    void configFileChanged(const QString& path);
    void bufferIdChanged(const QString& id);
    void coordModeChanged(int value);
    void flipYChanged(bool value);
    void flipWarpYChanged(bool value);
    void regionLocalChanged(bool value);
    void blendChanged(bool value);
    void linearizeAlphaChanged(bool value);
    void blackLevelChanged(bool value);
    void boundsChanged(bool value);

private:
    struct OutputSlot
    {
        std::shared_ptr<ImageData> imageData;
        std::shared_ptr<ImageTimestampRingQueue> buffer;
        qint64 lastPushedTimestamp = -1;
    };

    void bindOption(const char* member, const char* osc, QWidget* control)
    {
        NodeDelegateModel::ExternalBinding binding;
        binding.member = member;
        binding.control = control;
        AbstractDelegateModel::registerExternalBinding(osc, this, binding);
    }

    void assignBool(bool& slot, bool value, QCheckBox* box, void (MpcdiDataModel::*signal)(bool))
    {
        if (value == slot) {
            return;
        }
        slot = value;
        m_paramsDirty = true;
        if (box->isChecked() != value) {
            QSignalBlocker blocker(box);
            box->setChecked(value);
        }
        (this->*signal)(value);
    }

    MpcdiWarpSet::RenderOptions currentOptions() const
    {
        MpcdiWarpSet::RenderOptions options;
        options.flipY = m_flipY;
        options.flipWarpY = m_flipWarpY;
        options.regionLocal = m_regionLocal;
        options.blend = m_blend;
        options.linearizeAlpha = m_linearizeAlpha;
        options.blackLevel = m_blackLevel;
        options.bounds = m_bounds;
        options.coordMode = m_coordMode;
        return options;
    }

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/configFile"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/bufferId"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/coordMode"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/flipY"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/flipWarpY"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/regionLocal"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/blend"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/linearizeAlpha"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/blackLevel"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/bounds"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    void syncOutputPorts(bool notifyPorts)
    {
        unsigned int newCount = 1;
        const QString absPath = resolveMpcdiAbsolutePath(m_configFile);
        if (absPath.isEmpty()) {
            m_warp.invalidate();
            applyOutputPortCount(1, notifyPorts);
            ensureOutputSlots(1);
            updateNodeState(QtNodes::NodeValidationState::State::Valid);
            return;
        }

        QString error;
        if (!m_warp.load(absPath, m_bufferId, &error, false)) {
            m_initFailed = true;
            m_needsReinit = false;
            updateNodeState(QtNodes::NodeValidationState::State::Error,
                            error.isEmpty() ? m_warp.lastError() : error);
            applyOutputPortCount(1, notifyPorts);
            ensureOutputSlots(1);
            return;
        }

        if (m_bufferId != m_warp.loadedBufferId()) {
            m_bufferId = m_warp.loadedBufferId();
            Q_EMIT bufferIdChanged(m_bufferId);
        }
        updateNodeState(QtNodes::NodeValidationState::State::Valid);

        newCount = static_cast<unsigned int>(std::max(1, m_warp.regionCount()));
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

        const QString absPath = resolveMpcdiAbsolutePath(m_configFile);
        if (absPath.isEmpty()) {
            clearOutput();
            return;
        }

        if (m_initFailed && !m_needsReinit) {
            return;
        }

        if (m_needsReinit || !m_warp.matches(absPath, m_bufferId)) {
            syncOutputPorts(true);
        }
        if (!m_warp.isCpuReady()) {
            m_initFailed = true;
            m_needsReinit = false;
            m_paramsDirty = false;
            clearOutput();
            return;
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

        const unsigned int outCount = OutPortCount;
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
                    rawOuts[i] = m_warp.render(f, static_cast<int>(i), srcForWarp, currentOptions());
                    if (!rawOuts[i].valid()) {
                        renderError = m_warp.lastError();
                        return false;
                    }
                }
                return true;
            });

        if (!ok) {
            m_initFailed = true;
            m_needsReinit = false;
            m_paramsDirty = false;
            if (!renderError.isEmpty()) {
                updateNodeState(QtNodes::NodeValidationState::State::Error, renderError);
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

    MpcdiInterface* m_ui = new MpcdiInterface();
    MpcdiWarpSet m_warp;

    std::shared_ptr<ImageData> m_inImage;
    std::vector<OutputSlot> m_outputs;

    QString m_configFile;
    QString m_bufferId;
    int m_coordMode = 0;
    bool m_flipY = true;
    bool m_flipWarpY = false;
    bool m_regionLocal = false;
    bool m_blend = true;
    bool m_linearizeAlpha = true;
    bool m_blackLevel = true;
    bool m_bounds = false;

    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestamp = -1;
    bool m_paramsDirty = false;
    bool m_needsReinit = true;
    bool m_initFailed = false;
};

} // namespace Nodes
