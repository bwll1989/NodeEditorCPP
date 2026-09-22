#pragma once

/**
 * @file ISFDataModel.hpp
 * @brief ISF 节点（Resolume Wire 风格）：按 .fs INPUTS 动态展开输入端口并逐帧渲染
 *
 * ---------------------------------------------------------------------------
 * 类型映射
 * ---------------------------------------------------------------------------
 * - image                         → ImageData
 * - float / bool / long / event
 *   / color / point2D             → VariableData
 * - audio / audioFFT              → AudioData（节点内分别打波形 / FFT 纹理）
 * - cube                          → 不建口（暂缓）
 *
 * ---------------------------------------------------------------------------
 * 每帧数据流
 * ---------------------------------------------------------------------------
 * TimestampGenerator::frameCountUpdated
 *   → requestProcess
 *   → 取输入图 + 取 AudioFrame → cook 波形/FFT 行
 *   → ImageGpuUpload::runGl {
 *         ensureReady(forceReload?)
 *         ISFRenderer::render（图 flipped；音频 setBufferForAudioInputKey）
 *     }
 *   → adoptTextureLifetime → 输出翻 V → IMAGE 出
 *
 * ---------------------------------------------------------------------------
 * 标量两套 map
 * ---------------------------------------------------------------------------
 * - m_scalarValues  ：当前生效值（面板 + 上游接线）
 * - m_scalarDefaults：断线回退 / 存盘基础；面板改参时同步更新
 *   （即断线后恢复的是「上次面板值」，不是原始文件 DEFAULT）
 */

#include "ISFAudioCook.hpp"
#include "ISFInterface.hpp"
#include "ISFRenderer.hpp"

#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/DataConvert.hpp"
#include "Common/DataTypes/ImageGpuPass.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "NodeDataList.hpp"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QSignalBlocker>
#include <QtNodes/NodeDelegateModel>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <memory>

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

/**
 * 从 ImageData 共享环取帧并确保 GPU 纹理就绪。
 * 先按 targetTimestamp 向前搜，失败再取最新帧。
 */
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

/** 媒体库相对路径或绝对路径 → 绝对路径 */
QString resolveFsAbsolutePath(const QString& mediaRelativeOrAbsolute)
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

/** 全幅垂直翻转（runGl 外侧调用，避免与 ImageGpuUpload 锁重入） */
GpuTextureHandle flipVerticalFull(const GpuTextureHandle& src)
{
    if (!src.valid()) {
        return {};
    }
    return ImageGpuPass::instance().resample(
        src, src.width, src.height, 0.f, 0.f, 1.f, 1.f, false, true);
}

/** 给 blit 出的裸纹理挂上共享 lifetime，供下游安全引用 */
void adoptTextureLifetime(GpuTextureHandle& handle)
{
    if (!handle.valid() || handle.lifetime) {
        return;
    }
    handle.lifetime = makeTextureLifetime(handle.textureId).lifetime;
}

QVariant defaultVariantForPort(const ISFPortDesc& desc)
{
    if (desc.defaultValue.isValid()) {
        return desc.defaultValue;
    }
    switch (desc.kind) {
    case ISFPortDesc::Kind::Float:
        return 0.0;
    case ISFPortDesc::Kind::Bool:
    case ISFPortDesc::Kind::Event:
        return false;
    case ISFPortDesc::Kind::Long:
        return 0;
    case ISFPortDesc::Kind::Point2D:
        return QVariantList{0.0, 0.0};
    case ISFPortDesc::Kind::Color:
        return QVariantList{1.0, 1.0, 1.0, 1.0};
    default:
        return {};
    }
}

/** VariableData → 写入 ISF 用的 QVariant（按 ATTR 类型规范化） */
QVariant variableDataToVariant(const std::shared_ptr<VariableData>& data, ISFPortDesc::Kind kind)
{
    if (!data) {
        return {};
    }
    switch (kind) {
    case ISFPortDesc::Kind::Float:
        return data->asNumber();
    case ISFPortDesc::Kind::Bool:
    case ISFPortDesc::Kind::Event:
        return data->asBool();
    case ISFPortDesc::Kind::Long:
        return data->asInt();
    case ISFPortDesc::Kind::Point2D: {
        const QVector<float> xy = data->asFloats(2);
        return QVariantList{
            xy.size() > 0 ? xy[0] : 0.f,
            xy.size() > 1 ? xy[1] : 0.f};
    }
    case ISFPortDesc::Kind::Color: {
        const QColor c = colorFromVariant(data->value());
        if (c.isValid()) {
            return colorToList(c);
        }
        const QVector<float> rgba = data->asFloats(4);
        return QVariantList{
            rgba.size() > 0 ? rgba[0] : 0.f,
            rgba.size() > 1 ? rgba[1] : 0.f,
            rgba.size() > 2 ? rgba[2] : 0.f,
            rgba.size() > 3 ? rgba[3] : 1.f};
    }
    default:
        return data->value();
    }
}

} // namespace

class ISFDataModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString shaderFile READ shaderFile WRITE setShaderFile NOTIFY shaderFileChanged DESIGNABLE false)
    Q_PROPERTY(bool reload READ reload WRITE setReload NOTIFY reloadChanged DESIGNABLE false)

public:
    ISFDataModel()
    {
        InPortCount = 0;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = QStringLiteral("ISF");
        WidgetEmbeddable = true;
        Resizable = true;
        PortEditable = false;

        ensureImageDataBuffer(m_outImageData, m_outBuffer);

        // OSC / 属性树绑定
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "shaderFile";
            binding.control = m_ui->fileSelectComboBox;
            AbstractDelegateModel::registerExternalBinding("/shaderFile", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "reload";
            binding.control = m_ui->reloadButton;
            AbstractDelegateModel::registerExternalBinding("/reload", this, binding);
        }

        connect(m_ui->fileSelectComboBox, &SelectorComboBox::textChanged,
                this, &ISFDataModel::setShaderFile, Qt::AutoConnection);
        connect(this, &ISFDataModel::shaderFileChanged, this, [this](const QString&) {
            if (m_destructing || !m_ui) {
                return;
            }
            QSignalBlocker blocker(m_ui->fileSelectComboBox);
            m_ui->fileSelectComboBox->setText(m_shaderFile);
        }, Qt::AutoConnection);
        connect(m_ui->reloadButton, &QPushButton::clicked, this, [this]() {
            setReload(true);
        }, Qt::AutoConnection);
        connect(m_ui, &ISFInterface::parameterChanged, this,
                [this](const QString& name, const QVariant& value) {
                    if (m_destructing) {
                        return;
                    }
                    m_scalarValues[name] = value;
                    m_scalarDefaults[name] = value; // 断线回退到「上次面板值」
                    m_paramsDirty = true;
                }, Qt::AutoConnection);

        m_clock.start();

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64 frameCount) {
                    if (m_destructing) {
                        return;
                    }
                    // 同帧且无参数/重载请求则跳过，减轻空转
                    if (m_lastRequestedFrame == frameCount && !m_paramsDirty && !m_needsReinit) {
                        return;
                    }
                    m_lastRequestedFrame = frameCount;
                    requestProcess(frameCount);
                },
                Qt::AutoConnection);
    }

    ~ISFDataModel() override
    {
        m_destructing = true;

        GlobalEventBus::instance()->unsubscribe(this);

        // —— 第一优先级：断开所有会回调到 this 的信号 ——
        // TimestampGenerator 是全局单例，可能晚于本节点销毁；Queued 也可能还在队列
        TimestampGenerator::getInstance()->disconnect(this);

        // UI 子控件 lambda + m_ui 自身信号
        if (m_ui) {
            m_ui->fileSelectComboBox->disconnect(this);
            m_ui->reloadButton->disconnect(this);
            m_ui->disconnect(this);
            this->disconnect(m_ui);
        }
        this->disconnect(SIGNAL(shaderFileChanged(QString)));
        this->disconnect(SIGNAL(reloadChanged()));

        // ISFRenderer 析构内部会 runGl(destroy)；必须在 TimestampGenerator 断连后再释放，
        // 防止 requestProcess 回调访问已处于销毁中的 renderer / widget
        m_renderer.reset();

        // —— 最后释放 UI 控件（无 parent，需手动 deleteLater）——
        if (m_ui) {
            m_ui->deleteLater();
            m_ui = nullptr;
        }
    }

    QString shaderFile() const { return m_shaderFile; }
    /** 脉冲属性：读始终 false；写 true 触发一次重载 */
    bool reload() const { return false; }

    QWidget* embeddedWidget() override { return m_ui; }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return ImageData().type();
        }
        if (portIndex < 0 || portIndex >= m_ports.size()) {
            return VariableData().type();
        }
        const ISFPortDesc& p = m_ports[portIndex];
        if (p.isImage()) {
            return ImageData().type();
        }
        if (p.isAudioPort()) {
            return AudioData().type();
        }
        return VariableData().type();
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return QStringLiteral("IMAGE");
        }
        if (portIndex < 0 || portIndex >= m_ports.size()) {
            return QStringLiteral("IN %1").arg(portIndex);
        }
        return m_ports[portIndex].caption;
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return m_outImageData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (m_destructing || !m_ui) {
            return;
        }
        if (portIndex < 0 || portIndex >= m_ports.size()) {
            return;
        }
        const ISFPortDesc& desc = m_ports[portIndex];

        if (desc.isImage()) {
            m_imageInputs[desc.name] = std::dynamic_pointer_cast<ImageData>(data);
            m_lastProcessedInputTimestamp = -1;
            m_paramsDirty = true;
            return;
        }

        if (desc.isAudioPort()) {
            m_audioInputs[desc.name] = std::dynamic_pointer_cast<AudioData>(data);
            m_lastProcessedAudioTimestamp = -1;
            m_paramsDirty = true;
            return;
        }

        if (desc.isVariable()) {
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (var) {
                const QVariant v = variableDataToVariant(var, desc.kind);
                m_scalarValues[desc.name] = v;
                m_ui->setParameterValue(desc.name, v);
            } else {
                // 断线：回退到 defaults
                const QVariant fallback = m_scalarDefaults.value(
                    desc.name, defaultVariantForPort(desc));
                m_scalarValues[desc.name] = fallback;
                m_ui->setParameterValue(desc.name, fallback);
            }
            m_paramsDirty = true;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values[QStringLiteral("shaderFile")] = m_shaderFile;

        // 只存当前仍存在的 ATTR，避免换文件后残留旧键
        QJsonObject scalars;
        for (const auto& p : m_ports) {
            if (!p.isVariable()) {
                continue;
            }
            const QVariant v = m_scalarValues.value(
                p.name, m_scalarDefaults.value(p.name, p.defaultValue));
            scalars.insert(p.name, QJsonValue::fromVariant(v));
        }
        values[QStringLiteral("scalars")] = scalars;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson[QStringLiteral("values")] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        if (m_destructing || !m_ui) {
            return;
        }
        const QJsonObject values = data[QStringLiteral("values")].toObject();
        m_shaderFile = values.value(QStringLiteral("shaderFile")).toString().trimmed();
        m_scalarDefaults.clear();
        m_scalarValues.clear();

        const QJsonObject scalars = values.value(QStringLiteral("scalars")).toObject();
        for (auto it = scalars.begin(); it != scalars.end(); ++it) {
            const QVariant v = it.value().toVariant();
            m_scalarDefaults.insert(it.key(), v);
            m_scalarValues.insert(it.key(), v);
        }

        m_needsReinit = true;
        m_paramsDirty = true;
        {
            QSignalBlocker blocker(m_ui->fileSelectComboBox);
            m_ui->fileSelectComboBox->setText(m_shaderFile);
        }
        // load 阶段不发 portsInserted，避免与场景反序列化抢连线
        rebuildPortsFromFile(/*notifyPorts=*/false);
    }

public slots:
    void setShaderFile(const QString& path)
    {
        if (m_destructing || !m_ui) {
            return;
        }
        const QString trimmed = path.trimmed();
        if (trimmed == m_shaderFile) {
            return;
        }
        m_shaderFile = trimmed;
        m_needsReinit = true;
        m_paramsDirty = true;
        rebuildPortsFromFile(true);
        Q_EMIT shaderFileChanged(m_shaderFile);
    }

    void setReload(bool value)
    {
        if (m_destructing || !m_ui) {
            return;
        }
        if (!value) {
            return;
        }
        // forceReload：同路径也重新编译 .fs（改文件后点 Reload）
        m_needsReinit = true;
        m_paramsDirty = true;
        rebuildPortsFromFile(true);
        Q_EMIT reloadChanged();
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (m_destructing) {
            return;
        }
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/shaderFile")) {
            setShaderFile(ev.payload.toString());
        } else if (ev.address == makeFullOscAddress("/reload")) {
            setReload(ev.payload.toBool());
        }
    }

signals:
    void shaderFileChanged(const QString& path);
    void reloadChanged();

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/shaderFile"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/reload"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    /**
     * CPU 侧解析 .fs INPUTS → 重建 In 口数量 / 类型 / 参数面板。
     * GL scene 的真正重载由下一帧 requestProcess + ensureReady(force) 完成。
     */
    void rebuildPortsFromFile(bool notifyPorts)
    {
        if (m_destructing || !m_ui) {
            return;
        }
        QVector<ISFPortDesc> newPorts;
        const QString absPath = resolveFsAbsolutePath(m_shaderFile);
        QString error;
        if (!absPath.isEmpty()) {
            if (!parseISFPortsFromFile(absPath, newPorts, &error)) {
                updateNodeState(QtNodes::NodeValidationState::State::Error, error);
                newPorts.clear();
            } else {
                updateNodeState(QtNodes::NodeValidationState::State::Valid);
            }
        }

        // 保留仍存在的同名图像 / 音频接线；标量为新 ATTR 填默认
        QHash<QString, std::shared_ptr<ImageData>> keepImages;
        QHash<QString, std::shared_ptr<AudioData>> keepAudios;
        QSet<QString> liveScalarKeys;
        QSet<QString> liveAudioKeys;
        for (const auto& p : newPorts) {
            if (p.isImage() && m_imageInputs.contains(p.name)) {
                keepImages.insert(p.name, m_imageInputs.value(p.name));
            }
            if (p.isAudioPort()) {
                liveAudioKeys.insert(p.name);
                if (m_audioInputs.contains(p.name)) {
                    keepAudios.insert(p.name, m_audioInputs.value(p.name));
                }
                // 为每个 audioFFT 口准备独立 Gist 状态；换文件时清滑动窗
                if (p.isAudioFft()) {
                    if (!m_fftCookers.contains(p.name) || !m_fftCookers.value(p.name)) {
                        m_fftCookers.insert(p.name, std::make_shared<IsfAudio::FftCooker>());
                    }
                    m_fftCookers[p.name]->ring.maxKeep =
                        static_cast<size_t>(IsfAudio::kFftFrameSize) * 4;
                }
                if (p.isAudioWave()) {
                    auto& ring = m_waveRings[p.name];
                    ring.maxKeep = static_cast<size_t>(std::max(p.audioMaxSamples * 4, 4096));
                }
            }
            if (p.isVariable()) {
                liveScalarKeys.insert(p.name);
                if (!m_scalarValues.contains(p.name)) {
                    const QVariant def = m_scalarDefaults.value(p.name, defaultVariantForPort(p));
                    m_scalarValues.insert(p.name, def);
                    m_scalarDefaults.insert(p.name, def);
                } else if (!m_scalarDefaults.contains(p.name)) {
                    m_scalarDefaults.insert(p.name, m_scalarValues.value(p.name));
                }
            }
        }
        m_imageInputs.swap(keepImages);
        m_audioInputs.swap(keepAudios);

        // 丢掉已删除 ATTR 的标量 / FFT 状态
        for (auto it = m_scalarValues.begin(); it != m_scalarValues.end();) {
            if (!liveScalarKeys.contains(it.key())) {
                it = m_scalarValues.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = m_scalarDefaults.begin(); it != m_scalarDefaults.end();) {
            if (!liveScalarKeys.contains(it.key())) {
                it = m_scalarDefaults.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = m_fftCookers.begin(); it != m_fftCookers.end();) {
            if (!liveAudioKeys.contains(it.key())) {
                it = m_fftCookers.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = m_waveRings.begin(); it != m_waveRings.end();) {
            if (!liveAudioKeys.contains(it.key())) {
                it = m_waveRings.erase(it);
            } else {
                ++it;
            }
        }

        // 先换端口表 + 重建面板（得到正确 sizeHint），再改 InPortCount。
        // 若先 portsInserted/Deleted → nodeUpdated → recomputeSize，会用到旧控件高度，
        // 导致换 .fs 后外框与面板错位（撑破或留白）。
        m_ports = std::move(newPorts);
        m_ui->rebuildParameters(m_ports, m_scalarValues);
        applyInputPortCount(static_cast<unsigned int>(m_ports.size()), notifyPorts);
        Q_EMIT embeddedWidgetSizeUpdated();
    }

    void applyInputPortCount(unsigned int newCount, bool notifyPorts)
    {
        const unsigned int oldCount = InPortCount;
        if (newCount == oldCount) {
            return;
        }
        if (notifyPorts) {
            if (newCount > oldCount) {
                Q_EMIT portsAboutToBeInserted(PortType::In, oldCount, newCount - 1);
                InPortCount = newCount;
                Q_EMIT portsInserted();
            } else {
                Q_EMIT portsAboutToBeDeleted(PortType::In, newCount, oldCount - 1);
                InPortCount = newCount;
                Q_EMIT portsDeleted();
            }
        } else {
            InPortCount = newCount;
        }
    }

    void clearOutput()
    {
        if (m_outBuffer) {
            m_outBuffer->clear();
        }
        m_lastPushedTimestamp = -1;
        m_lastProcessedInputTimestamp = -1;
        m_lastProcessedAudioTimestamp = -1;
        m_paramsDirty = false;
    }

    bool requiresImageInput() const
    {
        for (const auto& p : m_ports) {
            if (p.isImage()) {
                return true;
            }
        }
        return false;
    }

    /**
     * 按端口 cook 音频行：
     * - audio    ：滑动 PCM → 波形 [0,1]（0.5 中心）
     * - audioFFT ：滑动 PCM → Gist FFT → 峰值归一化频谱
     * 未接线 / 无帧时喂静音行，不阻断渲染（与缺图策略不同）。
     */
    QHash<QString, ISFAudioRow> cookAudioRows(qint64 targetTimestamp, qint64& latestAudioTs)
    {
        QHash<QString, ISFAudioRow> rows;
        latestAudioTs = -1;

        for (const auto& p : m_ports) {
            if (!p.isAudioPort()) {
                continue;
            }

            const int width = std::max(1, p.audioMaxSamples);
            ISFAudioRow row;

            auto audioData = m_audioInputs.value(p.name);
            AudioFrame frame;
            bool gotFrame = false;
            if (audioData && audioData->isConnectedToSharedBuffer()) {
                auto buffer = audioData->getSharedAudioBuffer();
                if (buffer && buffer->isActive()
                    && buffer->getFrameByTimestamp(targetTimestamp, frame)
                    && !frame.data.isEmpty()) {
                    gotFrame = true;
                    latestAudioTs = qMax(latestAudioTs, frame.timestamp);
                }
            }

            if (p.isAudioWave()) {
                auto& ring = m_waveRings[p.name];
                ring.maxKeep = static_cast<size_t>(std::max(width * 4, 4096));
                if (gotFrame) {
                    ring.append(IsfAudio::extractMonoSamples(frame));
                }
                row.values = IsfAudio::cookWaveformRow(ring, width);
            } else {
                // audioFFT
                auto cooker = m_fftCookers.value(p.name);
                if (!cooker) {
                    cooker = std::make_shared<IsfAudio::FftCooker>();
                    m_fftCookers.insert(p.name, cooker);
                }
                if (gotFrame) {
                    cooker->pushMono(IsfAudio::extractMonoSamples(frame), frame.sampleRate);
                }
                row.values = cooker->cookSpectrumRow(width);
                if (row.values.isEmpty()) {
                    row.values = IsfAudio::silenceSpectrum(width);
                }
            }

            rows.insert(p.name, row);
        }
        return rows;
    }

    /** 清掉所有 event 脉冲（渲染成功后调用） */
    void clearEventPulses()
    {
        if (m_destructing || !m_ui) {
            return;
        }
        for (const auto& p : m_ports) {
            if (p.kind != ISFPortDesc::Kind::Event) {
                continue;
            }
            if (!m_scalarValues.value(p.name).toBool()) {
                continue;
            }
            m_scalarValues[p.name] = false;
            m_ui->setParameterValue(p.name, false);
        }
    }

    void requestProcess(qint64 targetTimestamp = -1)
    {
        if (m_destructing) {
            return;
        }
        ensureImageDataBuffer(m_outImageData, m_outBuffer);

        const QString absPath = resolveFsAbsolutePath(m_shaderFile);
        if (absPath.isEmpty()) {
            clearOutput();
            return;
        }

        // —— 收集图像输入 ——
        QHash<QString, GpuTextureHandle> images;
        qint64 latestImageTs = -1;
        bool missingRequiredImage = false;
        for (const auto& p : m_ports) {
            if (!p.isImage()) {
                continue;
            }
            auto inData = m_imageInputs.value(p.name);
            ImageFrame frame;
            if (!resolveInputGpuFrame(inData, targetTimestamp, frame)) {
                missingRequiredImage = true;
                continue;
            }
            images.insert(p.name, frame.texture);
            latestImageTs = qMax(latestImageTs, frame.timestamp);
        }

        // Filter / 多图：任一必填图缺失则清空输出，避免半新半旧花屏
        if (requiresImageInput() && (missingRequiredImage || images.isEmpty())) {
            clearOutput();
            return;
        }

        // —— 音频 cook（CPU，可在 runGl 外）——
        qint64 latestAudioTs = -1;
        const QHash<QString, ISFAudioRow> audioRows = cookAudioRows(
            targetTimestamp >= 0 ? targetTimestamp : currentSystemTimestamp(),
            latestAudioTs);

        const bool hasAudioPorts = std::any_of(
            m_ports.begin(), m_ports.end(),
            [](const ISFPortDesc& p) { return p.isAudioPort(); });
        // 无 image、无 audio：纯 generator，靠 TIME 每 tick 都渲
        const bool generatorNoMedia = !requiresImageInput() && !hasAudioPorts;

        if (!m_paramsDirty && !m_needsReinit && !generatorNoMedia) {
            if (latestImageTs == m_lastProcessedInputTimestamp
                && latestAudioTs == m_lastProcessedAudioTimestamp
                && (latestImageTs >= 0 || latestAudioTs >= 0)) {
                return;
            }
            // 有音频口但从未收到帧：静音已喂过一次后跳过空转（哨兵 -2）
            if (latestImageTs < 0 && latestAudioTs < 0
                && m_lastProcessedAudioTimestamp == -2) {
                return;
            }
        }

        if (!m_renderer) {
            m_renderer = std::make_unique<ISFRenderer>();
        }

        const bool forceReload = m_needsReinit;
        GpuTextureHandle rawOut;
        QString renderError;
        const QVariantMap scalars = m_scalarValues;
        const double renderTime = m_clock.elapsed() / 1000.0;

        const bool ok = ImageGpuUpload::instance().runGl(
            [&](QOpenGLFunctions* f) -> bool {
                if (forceReload || !m_renderer->isReady()
                    || m_renderer->loadedPath() != absPath) {
                    if (!m_renderer->ensureReady(f, absPath, forceReload)) {
                        renderError = m_renderer->lastError();
                        return false;
                    }
                }
                rawOut = m_renderer->render(
                    f, images, audioRows, scalars, renderTime, 0, 0);
                if (!rawOut.valid()) {
                    renderError = m_renderer->lastError();
                    return false;
                }
                return true;
            });

        if (!ok || !rawOut.valid()) {
            if (!renderError.isEmpty()) {
                updateNodeState(QtNodes::NodeValidationState::State::Error, renderError);
            }
            rawOut = {};
            clearOutput();
            return;
        }

        adoptTextureLifetime(rawOut);
        m_needsReinit = false;
        updateNodeState(QtNodes::NodeValidationState::State::Valid);

        clearEventPulses();

        GpuTextureHandle flippedOut = flipVerticalFull(rawOut);
        rawOut = {};
        if (!flippedOut.valid()) {
            clearOutput();
            return;
        }

        pushGpuResult(m_outBuffer, std::move(flippedOut), m_lastPushedTimestamp);
        m_lastProcessedInputTimestamp = latestImageTs;
        // -2：有音频口但本帧无 PCM，表示静音已渲染过
        m_lastProcessedAudioTimestamp =
            (latestAudioTs >= 0) ? latestAudioTs : (hasAudioPorts ? -2 : -1);
        m_paramsDirty = false;
    }

    ISFInterface* m_ui = new ISFInterface();
    std::unique_ptr<ISFRenderer> m_renderer;

    QVector<ISFPortDesc> m_ports;
    QHash<QString, std::shared_ptr<ImageData>> m_imageInputs;
    QHash<QString, std::shared_ptr<AudioData>> m_audioInputs;
    QHash<QString, IsfAudio::SampleRing> m_waveRings; ///< audio 口滑动 PCM
    /** audioFFT 口 Gist 状态（shared_ptr：QHash 要求可拷贝） */
    QHash<QString, std::shared_ptr<IsfAudio::FftCooker>> m_fftCookers;
    QVariantMap m_scalarValues;                       ///< 当前生效
    QVariantMap m_scalarDefaults;                     ///< 断线回退 / 面板记忆

    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    qint64 m_lastPushedTimestamp = -1;

    QString m_shaderFile;
    QElapsedTimer m_clock; ///< 喂给 ISF TIME（秒）

    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestamp = -1;
    qint64 m_lastProcessedAudioTimestamp = -1;
    bool m_paramsDirty = false; ///< 参数或接线变化，需重渲
    bool m_needsReinit = true;  ///< 需 forceReload GL scene
    bool m_destructing = false; ///< 析构阶段标志：禁止所有回调访问成员
};

} // namespace Nodes
