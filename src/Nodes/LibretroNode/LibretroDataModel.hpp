#pragma once

/**
 * @file LibretroDataModel.hpp
 * @brief Libretro 模拟器节点（GBA 专用精简版）— core DLL 固定加载 → 图像 / 音频 / 手柄三通道
 *
 * 架构：
 * - Core 固定为随主程序分发的 bin/cores/mgba_libretro.dll（不可更换，强制 GBA）。
 * - ROM 从媒体库 Game 分类选择（文件名）；启动时解析出全路径并拷贝到
 *   Documents/Flow/Cache/GBARoms（同大小复用），从缓存副本加载，
 *   演出中媒体库文件变动不影响运行中的模拟器。
 * - 模拟线程按 core 报告的刷新率（GBA=59.7275Hz）驱动 retro_run：
 *   video 回调把 RGB565 转 cv::Mat(BGR) 暂存最新帧；
 *   audio 回调把 int16 立体声（mGBA=65536Hz）追加进 FIFO。
 * - 模型线程订阅全局时钟（50Hz tick）：
 *   IMAGE = 取最新帧 → GPU 上传 → ImageTimestampRingQueue；
 *   AUDIO = FIFO 线性重采样到 48kHz → 恰好 getSamplesPerFrame 采样 → pushFrame
 *   （timestamp = clock + FIXED_DELAY_FRAMES，与全软件音频体系对齐）。
 * - 手柄三路掩码合并：面板按键 + OSC（/up /a ...）→ m_panelMask；
 *   12 个输入端口的电平语义 → m_portMask；
 *   每帧喂给 retro_run 的 = m_panelMask | m_portMask。
 * - Save/Load State：命令化转发到模拟线程执行（libretro 单线程约束）。
 *
 * In 端口（12，均 VariableData，电平语义）：
 *   0 Active / 1 Reset / 2 Up / 3 Down / 4 Left / 5 Right /
 *   6 A / 7 B / 8 L / 9 R / 10 Start / 11 Select
 * Out 端口：0 IMAGE / 1 AUDIO / 2 Info（core 信息/计数/错误）
 *
 * 许可：libretro API 为 MIT；mGBA core=MPL-2.0 可商用。
 */

#include "LibretroCore.h"
#include "LibretroInterface.hpp"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/AppConfig/ConstantDefines.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "MediaLibrary/MediaLibrary.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QPointer>
#include <QSignalBlocker>
#include <QThread>
#include <QTimer>

#include <atomic>
#include <deque>
#include <thread>
#include <chrono>
#include <opencv2/core.hpp>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

using QtNodes::NodeData;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
class LibretroDataModel final : public AbstractDelegateModel
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString romName READ romName WRITE setRomName NOTIFY romNameChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    // 动作型属性：OSC / 属性面板写 true 即触发一次
    Q_PROPERTY(bool reset READ reset WRITE setReset NOTIFY resetChanged)
    Q_PROPERTY(bool saveState READ saveState WRITE setSaveState NOTIFY saveStateChanged)
    Q_PROPERTY(bool loadState READ loadState WRITE setLoadState NOTIFY loadStateChanged)
    // 手柄 10 键（GBA 实机键位；位定义 = RETRO_DEVICE_ID_JOYPAD_*）
    Q_PROPERTY(bool btnUp READ btnUp WRITE setBtnUp NOTIFY btnUpChanged)
    Q_PROPERTY(bool btnDown READ btnDown WRITE setBtnDown NOTIFY btnDownChanged)
    Q_PROPERTY(bool btnLeft READ btnLeft WRITE setBtnLeft NOTIFY btnLeftChanged)
    Q_PROPERTY(bool btnRight READ btnRight WRITE setBtnRight NOTIFY btnRightChanged)
    Q_PROPERTY(bool btnA READ btnA WRITE setBtnA NOTIFY btnAChanged)
    Q_PROPERTY(bool btnB READ btnB WRITE setBtnB NOTIFY btnBChanged)
    Q_PROPERTY(bool btnL READ btnL WRITE setBtnL NOTIFY btnLChanged)
    Q_PROPERTY(bool btnR READ btnR WRITE setBtnR NOTIFY btnRChanged)
    Q_PROPERTY(bool btnStart READ btnStart WRITE setBtnStart NOTIFY btnStartChanged)
    Q_PROPERTY(bool btnSelect READ btnSelect WRITE setBtnSelect NOTIFY btnSelectChanged)

    enum InputPort : PortIndex {
        ActivePort = 0,
        ResetPort = 1,
        UpPort = 2,
        DownPort = 3,
        LeftPort = 4,
        RightPort = 5,
        APort = 6,
        BPort = 7,
        LPort = 8,
        RPort = 9,
        StartPort = 10,
        SelectPort = 11
    };

    enum OutputPort : PortIndex {
        ImagePort = 0,
        AudioPort = 1,
        InfoPort = 2
    };

    static constexpr int kSampleRate = 48000;
    static constexpr int kChannels = 2;

public:
    LibretroDataModel()
        : m_widget(new LibretroInterface())
    {
        InPortCount = 12;
        OutPortCount = 3;
        CaptionVisible = true;
        Caption = QStringLiteral("Libretro");
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        qRegisterMetaType<cv::Mat>("cv::Mat");
        ensureImageDataBuffer(m_outImage, m_outputBuffer);

        m_outAudio = std::make_shared<AudioData>();
        m_audioBuffer = std::make_shared<AudioTimestampRingQueue>(64);
        m_outAudio->setSharedAudioBuffer(m_audioBuffer);

        m_outInfo = std::make_shared<VariableData>();
        publishInfo();

        ImageGpuUpload::instance().warmup();

        registerOscBindings();

        connect(m_widget, &LibretroInterface::activeChanged, this, &LibretroDataModel::setActive);
        connect(m_widget, &LibretroInterface::romFileChanged, this, &LibretroDataModel::setRomName);
        connect(m_widget, &LibretroInterface::volumeChanged, this, &LibretroDataModel::setVolume);
        // 面板按键 + OSC 合并掩码 → m_panelMask（端口掩码独立走 m_portMask）
        connect(m_widget, &LibretroInterface::joypadMaskChanged, this,
                [this](quint16 mask) { m_panelMask.store(mask); });
        connect(m_widget, &LibretroInterface::resetRequested, this, &LibretroDataModel::resetNow);
        connect(m_widget, &LibretroInterface::saveStateRequested, this, &LibretroDataModel::saveStateNow);
        connect(m_widget, &LibretroInterface::loadStateRequested, this, &LibretroDataModel::loadStateNow);

        // Info 周期刷新（计数器/状态）
        m_infoTimer.setInterval(1000);
        connect(&m_infoTimer, &QTimer::timeout, this, [this]() {
            publishInfo();
            Q_EMIT dataUpdated(InfoPort);
        });
    }

    ~LibretroDataModel() override
    {
        m_infoTimer.stop();
        // 用缓存的 QPointer 断连：程序关闭时 TimestampGenerator 单例可能先于本节点销毁，
        // 析构里再调 getInstance() 有重建/悬空风险
        if (m_clockSrc) {
            disconnect(m_clockSrc, nullptr, this, nullptr);
        }
        // 顺序铁律：先静默停止模拟线程（join），最后才置退出标志。
        // 若先置 m_shuttingDown，setActive(false) 会被短路 → 模拟线程残留 →
        // std::thread 在 joinable 状态析构（std::terminate）/ 成员 LibretroCore
        // 析构在主线程 FreeLibrary 而模拟线程仍在执行 DLL 代码（悬空指针崩溃）。
        // 析构路径不走 setActive（避免半析构期 emit 触发框架重入），直接静默停机。
        m_active = false;
        stopEmulation();
        // 双保险：任何路径漏停都在这里兜底
        if (m_emuThread.joinable()) {
            m_emuRunning.store(false);
            stopThreadWithTimeout();
        }
        m_shuttingDown.store(true);
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            switch (portIndex) {
            case ImagePort: return ImageData().type();
            case AudioPort: return AudioData().type();
            default: return VariableData().type();
            }
        }
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        switch (port) {
        case ImagePort:
            return m_outImage ? m_outImage : std::make_shared<ImageData>();
        case AudioPort:
            return m_outAudio ? m_outAudio : std::make_shared<AudioData>();
        default:
            return m_outInfo;
        }
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (m_shuttingDown.load()) {
            return;
        }
        // 断连（data == null）保持现状：演出中拔线不产生按键状态漂移
        if (!data) {
            return;
        }
        auto v = std::dynamic_pointer_cast<VariableData>(data);
        if (!v) {
            return;
        }
        switch (portIndex) {
        case ActivePort:
            setActive(v->asBool());
            break;
        case ResetPort:
            if (v->asBool() && !m_lastResetState) {
                resetNow();
            }
            m_lastResetState = v->asBool();
            break;
        case UpPort:      setPortButton(4, v->asBool());  break; // RETRO_DEVICE_ID_JOYPAD_UP
        case DownPort:    setPortButton(5, v->asBool());  break; // DOWN
        case LeftPort:    setPortButton(6, v->asBool());  break; // LEFT
        case RightPort:   setPortButton(7, v->asBool());  break; // RIGHT
        case APort:       setPortButton(8, v->asBool());  break; // A
        case BPort:       setPortButton(0, v->asBool());  break; // B
        case LPort:       setPortButton(10, v->asBool()); break; // L
        case RPort:       setPortButton(11, v->asBool()); break; // R
        case StartPort:   setPortButton(3, v->asBool());  break; // START
        case SelectPort:  setPortButton(2, v->asBool());  break; // SELECT
        default:
            break;
        }
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case ActivePort: return QStringLiteral("Active");
            case ResetPort: return QStringLiteral("Reset");
            case UpPort: return QStringLiteral("Up");
            case DownPort: return QStringLiteral("Down");
            case LeftPort: return QStringLiteral("Left");
            case RightPort: return QStringLiteral("Right");
            case APort: return QStringLiteral("A");
            case BPort: return QStringLiteral("B");
            case LPort: return QStringLiteral("L");
            case RPort: return QStringLiteral("R");
            case StartPort: return QStringLiteral("Start");
            case SelectPort: return QStringLiteral("Select");
            default: break;
            }
        } else if (portType == PortType::Out) {
            switch (portIndex) {
            case ImagePort: return QStringLiteral("IMAGE");
            case AudioPort: return QStringLiteral("AUDIO");
            case InfoPort: return QStringLiteral("Info");
            default: break;
            }
        }
        return {};
    }

    QWidget* embeddedWidget() override { return m_widget.data(); }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        QJsonObject values;
        values.insert(QStringLiteral("active"), m_active);
        values.insert(QStringLiteral("romName"), m_romName);
        values.insert(QStringLiteral("volume"), m_volume);
        modelJson.insert(QStringLiteral("values"), values);
        return modelJson;
    }

    void load(const QJsonObject& jsonObject) override
    {
        NodeDelegateModel::load(jsonObject);
        const QJsonObject values = jsonObject.value(QStringLiteral("values")).toObject();
        if (values.isEmpty()) {
            return;
        }
        // 兼容旧版（romPath 绝对路径）的逻辑已按需求移除，只认 romName
        const QString romName = values.value(QStringLiteral("romName")).toString();
        if (!romName.isEmpty()) {
            setRomName(romName);
        }
        if (values.contains(QStringLiteral("volume"))) {
            setVolume(values.value(QStringLiteral("volume")).toDouble(m_volume));
        }
        if (m_widget) {
            m_widget->setRomFile(m_romName);
            QSignalBlocker b(m_widget->m_volumeSlider);
            m_widget->m_volumeSlider->setValue(int(m_volume * 100.0 + 0.5));
        }
        if (values.value(QStringLiteral("active")).toBool()) {
            QTimer::singleShot(600, this, [self = QPointer<LibretroDataModel>(this)]() {
                if (self && !self->m_shuttingDown.load()) {
                    self->setActive(true);
                }
            });
        }
    }

    // ---------------- 属性 ----------------

    bool active() const { return m_active; }
    QString romName() const { return m_romName; }
    double volume() const { return m_volume; }
    bool reset() const { return false; }
    bool saveState() const { return false; }
    bool loadState() const { return false; }

    bool btnUp() const { return false; }
    bool btnDown() const { return false; }
    bool btnLeft() const { return false; }
    bool btnRight() const { return false; }
    bool btnA() const { return false; }
    bool btnB() const { return false; }
    bool btnL() const { return false; }
    bool btnR() const { return false; }
    bool btnStart() const { return false; }
    bool btnSelect() const { return false; }

    void setReset(bool value) { if (value) resetNow(); Q_EMIT resetChanged(false); }
    void setSaveState(bool value) { if (value) saveStateNow(); Q_EMIT saveStateChanged(false); }
    void setLoadState(bool value) { if (value) loadStateNow(); Q_EMIT loadStateChanged(false); }

    // 手柄属性（OSC / 属性面板）：按住为 true（电平语义），松开为 false
    void setBtnUp(bool v) { applyPadButton(4, v); }
    void setBtnDown(bool v) { applyPadButton(5, v); }
    void setBtnLeft(bool v) { applyPadButton(6, v); }
    void setBtnRight(bool v) { applyPadButton(7, v); }
    void setBtnA(bool v) { applyPadButton(8, v); }
    void setBtnB(bool v) { applyPadButton(0, v); }
    void setBtnL(bool v) { applyPadButton(10, v); }
    void setBtnR(bool v) { applyPadButton(11, v); }
    void setBtnStart(bool v) { applyPadButton(3, v); }
    void setBtnSelect(bool v) { applyPadButton(2, v); }

    void setActive(bool value)
    {
        if (m_active == value) {
            return;
        }
        if (m_shuttingDown.load() && value) {
            return; // 析构中只允许停止，不允许再启动
        }
        m_active = value;
        if (m_widget) {
            m_widget->setActiveChecked(m_active);
        }
        if (m_active) {
            startEmulation();
        } else {
            stopEmulation();
        }
        Q_EMIT activeChanged(m_active);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void setRomName(const QString& value)
    {
        const QString trimmed = value.trimmed();
        if (m_romName == trimmed) {
            return;
        }
        m_romName = trimmed;
        if (m_widget) {
            m_widget->setRomFile(m_romName);
        }
        // 运行中换 ROM：热重启（停旧线程 → 用新 ROM 重新拉起）
        if (m_active) {
            stopEmulation();
            startEmulation();
        }
        Q_EMIT romNameChanged(m_romName);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void setVolume(double value)
    {
        value = qBound(0.0, value, 1.0);
        if (qFuzzyCompare(m_volume, value)) {
            return;
        }
        m_volume = value;
        if (m_widget) {
            m_widget->setVolumeScaled(int(m_volume * 100.0 + 0.5));
        }
        Q_EMIT volumeChanged(m_volume);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

Q_SIGNALS:
    void activeChanged(bool active);
    void romNameChanged(const QString& name);
    void volumeChanged(double volume);
    void resetChanged(bool value);
    void saveStateChanged(bool value);
    void loadStateChanged(bool value);
    void btnUpChanged(bool value);
    void btnDownChanged(bool value);
    void btnLeftChanged(bool value);
    void btnRightChanged(bool value);
    void btnAChanged(bool value);
    void btnBChanged(bool value);
    void btnLChanged(bool value);
    void btnRChanged(bool value);
    void btnStartChanged(bool value);
    void btnSelectChanged(bool value);

protected:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        ImageGpuUpload::instance().warmup();

        // 缓存时钟源 QPointer：程序关闭时 TimestampGenerator 单例可能先于本节点析构，
        // 析构里若再调 getInstance() 会重建/悬空访问
        m_clockSrc = TimestampGenerator::getInstance();

        // 全局时钟 50Hz：图像发布 + 音频重采样推送（QueuedConnection 归拢到模型线程）
        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [self = QPointer<LibretroDataModel>(this)](qint64 clock) {
                    if (!self || self->m_shuttingDown.load()) {
                        return;
                    }
                    self->onClockTick(clock);
                },
                Qt::QueuedConnection);

        if (m_active || !m_romName.isEmpty()) {
            m_infoTimer.start();
        }
    }

private Q_SLOTS:
    void resetNow()
    {
        submitCommand(Cmd::Reset);
    }

    void saveStateNow()
    {
        if (submitCommand(Cmd::Save)) {
            setStatus(QStringLiteral("状态: 存档点已保存"), true);
        } else {
            setStatus(QStringLiteral("状态: 存档点保存失败"), false);
        }
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void loadStateNow()
    {
        if (submitCommand(Cmd::Load)) {
            setStatus(QStringLiteral("状态: 存档点已恢复"), true);
        } else {
            setStatus(QStringLiteral("状态: 存档点恢复失败（先 Save）"), false);
        }
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

private:
    enum class Cmd { None, Save, Load, Reset };

    void applyPadButton(int bit, bool pressed)
    {
        if (m_widget) {
            m_widget->applyExternalButton(bit, pressed);
        }
    }

    /** @brief 输入端口按键电平 → 端口位掩码（与面板/OSC 掩码独立，每帧合并） */
    void setPortButton(int bit, bool on)
    {
        if (bit < 0 || bit > 15) {
            return;
        }
        if (on) {
            m_portMask.fetch_or(uint16_t(1u << bit), std::memory_order_relaxed);
        } else {
            m_portMask.fetch_and(uint16_t(~(1u << bit)), std::memory_order_relaxed);
        }
    }

    void registerOscBindings()
    {
        const std::initializer_list<std::pair<const char*, QWidget*>> padControls = {
            {"btnUp", m_widget->m_upBtn}, {"btnDown", m_widget->m_downBtn},
            {"btnLeft", m_widget->m_leftBtn}, {"btnRight", m_widget->m_rightBtn},
            {"btnA", m_widget->m_aBtn}, {"btnB", m_widget->m_bBtn},
            {"btnL", m_widget->m_lBtn}, {"btnR", m_widget->m_rBtn},
            {"btnStart", m_widget->m_startBtn}, {"btnSelect", m_widget->m_selectBtn},
        };
        for (const auto& [name, ctrl] : padControls) {
            NodeDelegateModel::ExternalBinding b;
            b.member = name;
            b.control = ctrl;
            QString path = QString::fromLatin1(name);
            path = path.remove(0, 3).toLower(); // btnUp -> /up
            registerExternalBinding(QStringLiteral("/%1").arg(path), this, b);
        }

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "active";
            b.control = m_widget->m_activeCheck;
            registerExternalBinding(QStringLiteral("/active"), this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "volume";
            b.control = m_widget->m_volumeSlider;
            registerExternalBinding(QStringLiteral("/volume"), this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "reset";
            b.control = m_widget->m_resetBtn;
            registerExternalBinding(QStringLiteral("/reset"), this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "saveState";
            b.control = m_widget->m_saveBtn;
            registerExternalBinding(QStringLiteral("/savestate"), this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "loadState";
            b.control = m_widget->m_loadBtn;
            registerExternalBinding(QStringLiteral("/loadstate"), this, b);
        }
    }

    // ---------------- ROM 解析与缓存 ----------------

    /**
     * @brief 媒体库文件名 → 全路径
     *        优先在媒体库 allFiles() 中按文件名反查；兜底拼接媒体库存储目录
     */
    QString resolveRomSource() const
    {
        if (m_romName.isEmpty()) {
            return {};
        }
        const QStringList files = MediaLibrary::instance()->allFiles();
        for (const QString& p : files) {
            if (QFileInfo(p).fileName().compare(m_romName, Qt::CaseInsensitive) == 0) {
                return p;
            }
        }
        const QString direct =
            AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QLatin1Char('/') + m_romName;
        if (QFileInfo::exists(direct)) {
            return direct;
        }
        return {};
    }

    /**
     * @brief 把媒体库源 ROM 拷贝到运行缓存目录（同大小复用），返回缓存路径
     *        拷贝失败时回退直接用源路径（可用性优先）
     */
    QString ensureRomCache(const QString& sourcePath)
    {
        if (sourcePath.isEmpty()) {
            return {};
        }
        QDir().mkpath(AppConstants::GAME_ROM_CACHE_DIR);
        const QString target = AppConstants::GAME_ROM_CACHE_DIR + QLatin1Char('/')
            + QFileInfo(sourcePath).fileName();

        QFileInfo srcInfo(sourcePath);
        QFileInfo dstInfo(target);
        if (dstInfo.exists() && dstInfo.size() == srcInfo.size()) {
            return target; // 缓存命中
        }
        if (dstInfo.exists()) {
            QFile::remove(target);
        }
        if (QFile::copy(sourcePath, target)) {
            return target;
        }
        qWarning() << "LibretroNode: ROM 缓存拷贝失败，直接使用媒体库源文件" << sourcePath;
        return sourcePath;
    }

    // ---------------- 模拟管线 ----------------

    void startEmulation()
    {
        m_lastError.clear();

        // Core 固定随主程序分发（强制 GBA / mGBA，不可更换），直接位于 bin/ 下
        const QString core = QCoreApplication::applicationDirPath()
            + QStringLiteral("/mgba_libretro.dll");
        if (!QFileInfo::exists(core)) {
            failStart(QStringLiteral("Core DLL 不存在: %1").arg(core));
            return;
        }

        const QString source = resolveRomSource();
        if (source.isEmpty()) {
            failStart(QStringLiteral("媒体库中未找到 ROM: %1").arg(m_romName));
            return;
        }
        const QString rom = ensureRomCache(source);
        if (rom.isEmpty() || !QFileInfo::exists(rom)) {
            failStart(QStringLiteral("ROM 缓存失败: %1").arg(source));
            return;
        }
        m_romCachePath = rom;

        m_romData.clear();
        {
            QFile f(rom);
            if (!f.open(QIODevice::ReadOnly)) {
                failStart(QStringLiteral("ROM 读取失败"));
                return;
            }
            m_romData = f.readAll();
        }

        // system/save 目录（SRAM/即时存档落盘位置）
        const QString coreStem = QFileInfo(core).completeBaseName();
        const QString sysDir = QCoreApplication::applicationDirPath()
            + QStringLiteral("/saves/") + coreStem;
        QDir().mkpath(sysDir);

        std::string err;
        if (!m_core.load(core.toStdWString(), err)) {
            failStart(QStringLiteral("Core 加载失败: %1").arg(QString::fromStdString(err)));
            return;
        }
        m_core.setSystemDirectory(sysDir.toUtf8().constData());

        // 回调槽：全部在模拟线程触发，只触碰带锁成员，绝不碰 GUI
        m_core.onVideo = [this](const void* pixels, unsigned w, unsigned h,
                                unsigned pitch, int fmt) {
            onCoreVideo(pixels, w, h, pitch, fmt);
        };
        m_core.onAudio = [this](const int16_t* samples, size_t frames) {
            onCoreAudio(samples, frames);
        };
        m_core.onErrorLog = [this](const std::string& msg) {
            QMutexLocker l(&m_errorMutex);
            m_lastError = QString::fromStdString(msg);
        };

        if (!m_core.loadGame(rom.toUtf8().constData(), m_romData.constData(),
                             size_t(m_romData.size()), err)) {
            m_core.unload();
            failStart(QStringLiteral("ROM 加载失败: %1").arg(QString::fromStdString(err)));
            return;
        }

        m_srcRate = m_core.sampleRate();
        if (m_srcRate <= 1000.0) {
            m_srcRate = 65536.0;
        }

        // 复位管线状态
        {
            QMutexLocker l(&m_audioMutex);
            m_audioFifo.clear();
            m_readPos = 0.0;
            m_fifoPrimed = false;
        }
        {
            QMutexLocker l(&m_frameMutex);
            m_latestFrame.release();
            m_frameSeq = 0;
            m_publishedSeq = 0;
        }
        m_videoFrames.store(0);
        m_audioFramesTotal.store(0);

        m_emuRunning.store(true);
        m_emuThread = std::thread(&LibretroDataModel::emulationLoop, this);

        const QString info = QStringLiteral("%1 %2 — %3x%4 @ %5Hz / %6Hz")
            .arg(QString::fromStdString(m_core.libraryName()),
                 QString::fromStdString(m_core.libraryVersion()))
            .arg(m_core.baseWidth()).arg(m_core.baseHeight())
            .arg(m_core.fps(), 0, 'f', 2).arg(m_srcRate, 0, 'f', 0);
        setStatus(QStringLiteral("状态: 模拟运行中（%1）").arg(info), true);
        m_infoTimer.start();
    }

    void stopEmulation()
    {
        m_emuRunning.store(false);
        // 模拟线程每帧都检查退出标志，正常一帧内退出；带超时兜底防止
        // retro_run 被 core 内部卡死时拖死 GUI 线程（关闭时鼠标忙碌）
        stopThreadWithTimeout();
        m_core.unload();
        m_romData.clear();

        // 清三路按键状态：面板+OSC 掩码（含 widget 侧）与端口掩码，
        // 防止停止/换 ROM 重启后旧按键残留"卡住"
        m_panelMask.store(0);
        m_portMask.store(0);
        m_lastResetState = false;
        if (m_widget) {
            m_widget->clearPadState();
        }

        if (m_outputBuffer) {
            m_outputBuffer->clear();
        }
        if (m_audioBuffer) {
            m_audioBuffer->clear();
        }
        m_lastPushedTimestamp = -1;
        {
            QMutexLocker l(&m_frameMutex);
            m_latestFrame.release();
        }
        {
            QMutexLocker l(&m_audioMutex);
            m_audioFifo.clear();
            m_fifoPrimed = false;
            m_readPos = 0.0;
        }
        m_hasState.store(false, std::memory_order_relaxed);
        setStatus(QStringLiteral("状态: 未启动"), false);
        m_infoTimer.stop();
    }

    void failStart(const QString& msg)
    {
        m_lastError = msg;
        setStatus(msg, false);
        m_active = false;
        if (m_widget) {
            m_widget->setActiveChecked(false);
        }
        Q_EMIT activeChanged(false);
    }

    /**
     * @brief 停止模拟线程，等待最多 500ms。
     *        正常路径模拟线程每帧检查退出标志，一帧内退出；
     *        超时说明 retro_run 被 core 内部卡死——强杀线程兜底，
     *        绝不允许 GUI 线程（析构/程序关闭）被 join 永久拖死。
     */
    void stopThreadWithTimeout()
    {
        if (!m_emuThread.joinable()) {
            return;
        }
#ifdef Q_OS_WIN
        const HANDLE th = static_cast<HANDLE>(m_emuThread.native_handle());
        if (WaitForSingleObject(th, 500) == WAIT_OBJECT_0) {
            m_emuThread.join(); // 已退出，立即返回
        } else {
            // 强杀兜底：进程即将退出时的最后手段；放弃 FreeLibrary，交给 OS 回收
            TerminateThread(th, 1);
            m_emuThread.detach();
        }
#else
        m_emuThread.join();
#endif
    }

    void emulationLoop()
    {
        const double f = m_core.fps();
        if (f <= 1.0) {
            return;
        }
        const auto frameDur = std::chrono::microseconds(qint64(1e6 / f));
        auto next = std::chrono::steady_clock::now();
        while (m_emuRunning.load() && !m_shuttingDown.load()) {
            processCommand();
            // 三路掩码合并：面板(+OSC) | 输入端口
            m_core.runFrame(uint16_t(m_panelMask.load() | m_portMask.load()));
            next += frameDur;
            const auto now = std::chrono::steady_clock::now();
            if (next < now) {
                // 落后于实时（卡顿/断点）：不对齐历史，直接重同步，避免追帧风暴
                next = now + frameDur;
            }
            std::this_thread::sleep_until(next);
        }
    }

    bool submitCommand(Cmd cmd)
    {
        if (!m_emuRunning.load() || !m_core.isGameLoaded()) {
            return false;
        }
        // m_stateBlob 由模拟线程写，主线程只看 m_hasState 原子标志，严禁跨线程读 blob
        if (cmd == Cmd::Load && !m_hasState.load(std::memory_order_relaxed)) {
            return false;
        }
        QMutexLocker l(&m_cmdMutex);
        m_cmd = cmd;
        m_cmdDone = false;
        // 等模拟线程在帧间执行（最多 ~250ms）
        for (int i = 0; i < 250 && !m_cmdDone && m_emuRunning.load(); ++i) {
            l.unlock();
            QThread::msleep(1);
            l.relock();
        }
        const bool ok = m_cmdDone && m_cmdResult;
        m_cmd = Cmd::None;
        return ok;
    }

    /** @brief 模拟线程帧间执行存档/读档/复位命令 */
    void processCommand()
    {
        QMutexLocker l(&m_cmdMutex);
        if (m_cmd == Cmd::None) {
            return;
        }
        const Cmd c = m_cmd;
        m_cmd = Cmd::None;
        l.unlock();

        bool ok = false;
        switch (c) {
        case Cmd::Save:
            ok = m_core.serialize(m_stateBlob);
            if (ok) {
                m_hasState.store(true, std::memory_order_relaxed);
            }
            break;
        case Cmd::Load:
            ok = m_core.unserialize(m_stateBlob);
            break;
        case Cmd::Reset:
            m_core.resetGame();
            ok = true;
            break;
        default:
            break;
        }

        l.relock();
        m_cmdResult = ok;
        m_cmdDone = true;
    }

    // ---------------- core 回调（模拟线程） ----------------

    void onCoreVideo(const void* pixels, unsigned w, unsigned h, unsigned pitch, int fmt)
    {
        if (!pixels || w == 0 || h == 0) {
            return;
        }
        cv::Mat bgr(int(h), int(w), CV_8UC3);
        const uint8_t* src = static_cast<const uint8_t*>(pixels);

        if (fmt == RETRO_PIXEL_FORMAT_RGB565) {
            // 5/6/5 位扩展查找表（C++11 函数局部 static 初始化线程安全）
            static const struct Lut {
                uint8_t r[32];
                uint8_t g[64];
                uint8_t b[32];
            } lut = [] {
                Lut t{};
                for (int i = 0; i < 32; ++i) {
                    t.r[i] = uint8_t((i << 3) | (i >> 2));
                    t.b[i] = uint8_t((i << 3) | (i >> 2));
                }
                for (int i = 0; i < 64; ++i) {
                    t.g[i] = uint8_t((i << 2) | (i >> 4));
                }
                return t;
            }();
            for (unsigned y = 0; y < h; ++y) {
                const uint16_t* line = reinterpret_cast<const uint16_t*>(src + size_t(y) * pitch);
                uint8_t* dst = bgr.ptr(int(y));
                for (unsigned x = 0; x < w; ++x) {
                    const uint16_t px = line[x];
                    dst[x * 3 + 0] = lut.b[px & 0x1F];
                    dst[x * 3 + 1] = lut.g[(px >> 5) & 0x3F];
                    dst[x * 3 + 2] = lut.r[(px >> 11) & 0x1F];
                }
            }
        } else { // XRGB8888：内存序 B G R X，直接取前三字节
            for (unsigned y = 0; y < h; ++y) {
                const uint8_t* line = src + size_t(y) * pitch;
                uint8_t* dst = bgr.ptr(int(y));
                for (unsigned x = 0; x < w; ++x) {
                    dst[x * 3 + 0] = line[x * 4 + 0];
                    dst[x * 3 + 1] = line[x * 4 + 1];
                    dst[x * 3 + 2] = line[x * 4 + 2];
                }
            }
        }

        QMutexLocker l(&m_frameMutex);
        m_latestFrame = std::move(bgr);
        ++m_frameSeq;
        m_videoFrames.fetch_add(1);
    }

    void onCoreAudio(const int16_t* samples, size_t frames)
    {
        if (!samples || frames == 0) {
            return;
        }
        QMutexLocker l(&m_audioMutex);
        const size_t kMaxFifoFrames = size_t(m_srcRate) * 2; // 最多缓冲 2 秒
        if (m_audioFifo.size() / 2 + frames > kMaxFifoFrames) {
            // 防爆：丢最旧的
            const size_t excess = (m_audioFifo.size() / 2 + frames) - kMaxFifoFrames;
            m_audioFifo.erase(m_audioFifo.begin(),
                              m_audioFifo.begin() + std::min(excess * 2, m_audioFifo.size()));
        }
        m_audioFifo.insert(m_audioFifo.end(), samples, samples + frames * 2);
        m_audioFramesTotal.fetch_add(frames);
    }

    // ---------------- 全局时钟 tick（模型线程） ----------------

    void onClockTick(qint64 clock)
    {
        if (!m_active || !m_emuRunning.load()) {
            return;
        }
        publishVideoFrame();
        publishAudioFrame(clock);
    }

    void publishVideoFrame()
    {
        cv::Mat mat;
        {
            QMutexLocker l(&m_frameMutex);
            if (m_latestFrame.empty() || m_frameSeq == m_publishedSeq) {
                return; // 没有新帧
            }
            mat = m_latestFrame.clone(); // 模拟线程会覆写，必须拷贝
            m_publishedSeq = m_frameSeq;
        }
        if (!m_outputBuffer || mat.empty()) {
            return;
        }
        ensureImageDataBuffer(m_outImage, m_outputBuffer);
        const qint64 timestamp = mediaOutputTimestamp(
            TimestampGenerator::getInstance()->getCurrentFrameCount());
        ImageFrame imageFrame = ImageFrame::fromMat(std::move(mat), timestamp);
        if (!imageFrame.texture.valid()) {
            qWarning() << "LibretroNode: GPU upload failed";
            return;
        }
        pushFrameToImageBufferDedup(m_outputBuffer, std::move(imageFrame), m_lastPushedTimestamp);
    }

    void publishAudioFrame(qint64 clock)
    {
        if (m_volume <= 0.0) {
            return;
        }
        const int samplesPerFrame =
            TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate);
        if (samplesPerFrame <= 0) {
            return;
        }

        std::vector<float> mix(size_t(samplesPerFrame) * kChannels, 0.0f);
        {
            QMutexLocker l(&m_audioMutex);
            const double step = m_srcRate / double(kSampleRate);
            const size_t availFrames = m_audioFifo.size() / 2;

            if (!m_fifoPrimed) {
                // 预积 ~50ms 再开始消费，避免开头频繁欠载
                if (availFrames >= size_t(m_srcRate * 0.05)) {
                    m_fifoPrimed = true;
                } else {
                    return;
                }
            }

            for (int i = 0; i < samplesPerFrame; ++i) {
                const size_t idx = size_t(m_readPos);
                if (idx + 1 >= availFrames) {
                    // 欠载：残尾丢弃，回到预热态
                    m_audioFifo.clear();
                    m_readPos = 0.0;
                    m_fifoPrimed = false;
                    return;
                }
                const double frac = m_readPos - double(idx);
                const int16_t* f0 = &m_audioFifo[idx * 2];
                const int16_t* f1 = f0 + 2;
                mix[size_t(i) * kChannels + 0] =
                    (float(f0[0]) + (float(f1[0]) - float(f0[0])) * float(frac))
                    / 32768.0f * float(m_volume);
                mix[size_t(i) * kChannels + 1] =
                    (float(f0[1]) + (float(f1[1]) - float(f0[1])) * float(frac))
                    / 32768.0f * float(m_volume);
                m_readPos += step;
            }

            // 消费 readPos 之前的源帧
            const size_t drop = size_t(m_readPos);
            if (drop > 0) {
                m_audioFifo.erase(m_audioFifo.begin(),
                                  m_audioFifo.begin() + std::min(drop * 2, m_audioFifo.size()));
                m_readPos -= double(drop);
            }
        }

        AudioFrame frame;
        frame.data = QByteArray(reinterpret_cast<const char*>(mix.data()),
                                int(mix.size() * sizeof(float)));
        frame.sampleRate = kSampleRate;
        frame.channels = kChannels;
        frame.bitsPerSample = 32; // float
        frame.timestamp = clock + AppConfigs::FIXED_DELAY_FRAMES;
        m_audioBuffer->pushFrame(frame);
    }

    // ---------------- 杂项 ----------------

    void setStatus(const QString& text, bool ok)
    {
        if (m_widget) {
            m_widget->setStatus(text, ok);
        }
    }

    void publishInfo()
    {
        if (!m_outInfo) {
            m_outInfo = std::make_shared<VariableData>();
        }
        QMutexLocker el(&m_errorMutex);
        m_outInfo->insert(QStringLiteral("core"), QString::fromStdString(m_core.libraryName()));
        m_outInfo->insert(QStringLiteral("core_version"),
                          QString::fromStdString(m_core.libraryVersion()));
        m_outInfo->insert(QStringLiteral("loaded"), m_core.isGameLoaded());
        m_outInfo->insert(QStringLiteral("fps"), m_core.fps());
        m_outInfo->insert(QStringLiteral("sample_rate"), m_core.sampleRate());
        m_outInfo->insert(QStringLiteral("width"), int(m_core.baseWidth()));
        m_outInfo->insert(QStringLiteral("height"), int(m_core.baseHeight()));
        m_outInfo->insert(QStringLiteral("video_frames"), int(m_videoFrames.load()));
        m_outInfo->insert(QStringLiteral("audio_frames"), int(m_audioFramesTotal.load()));
        m_outInfo->insert(QStringLiteral("rom"), m_romName);
        m_outInfo->insert(QStringLiteral("rom_path"), m_romCachePath);
        m_outInfo->insert(QStringLiteral("active"), m_active);
        m_outInfo->insert(QStringLiteral("error"), m_lastError);
    }

    QPointer<LibretroInterface> m_widget;
    QPointer<QObject> m_clockSrc; // TimestampGenerator 缓存引用（析构期安全 disconnect）
    LibretroCore m_core;

    // 模拟线程
    std::thread m_emuThread;
    std::atomic<bool> m_emuRunning{false};
    std::atomic<bool> m_shuttingDown{false};
    // 手柄三路掩码：面板+OSC → m_panelMask；输入端口 → m_portMask；每帧合并喂 core
    std::atomic<uint16_t> m_panelMask{0};
    std::atomic<uint16_t> m_portMask{0};
    QByteArray m_romData; // core 引用的 ROM 镜像，unload 前保持有效

    // 视频共享（m_frameMutex 保护）
    QMutex m_frameMutex;
    cv::Mat m_latestFrame;
    quint64 m_frameSeq = 0;
    quint64 m_publishedSeq = 0;
    std::atomic<quint64> m_videoFrames{0};

    // 音频共享（m_audioMutex 保护）
    QMutex m_audioMutex;
    std::deque<int16_t> m_audioFifo; // 交织立体声 int16
    double m_readPos = 0.0;          // 源帧坐标（相对 FIFO 头）
    bool m_fifoPrimed = false;
    double m_srcRate = 65536.0;
    std::atomic<quint64> m_audioFramesTotal{0};

    // 命令（m_cmdMutex 保护，模拟线程帧间执行）
    QMutex m_cmdMutex;
    Cmd m_cmd = Cmd::None;
    bool m_cmdDone = false;
    bool m_cmdResult = false;
    std::vector<uint8_t> m_stateBlob;                       // 模拟线程读写
    std::atomic<bool> m_hasState{false};                    // 主线程只读此标志，避免跨线程访问 blob

    QMutex m_errorMutex;
    QString m_lastError;

    // 输出
    std::shared_ptr<ImageData> m_outImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;
    std::shared_ptr<AudioData> m_outAudio;
    std::shared_ptr<AudioTimestampRingQueue> m_audioBuffer;
    std::shared_ptr<VariableData> m_outInfo;
    qint64 m_lastPushedTimestamp = -1;

    // 状态
    bool m_active = false;
    bool m_lastResetState = false;
    QString m_romName;      // 媒体库 Game 分类中的文件名
    QString m_romCachePath; // 最近一次启动用的缓存副本路径（Info 输出）
    double m_volume = 0.8;

    QTimer m_infoTimer;
};

} // namespace Nodes
