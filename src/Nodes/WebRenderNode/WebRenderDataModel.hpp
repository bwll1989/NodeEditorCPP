#pragma once

/**
 * @file WebRenderDataModel.hpp
 * @brief Web Render 节点 — CEF OSR 后端（CefRefactorPlan P1）
 *
 * CefOsrBrowser 无窗口渲染（GPU 共享纹理 / OnPaint BGRA 兜底）→ ImageFrame
 * → ImageTimestampRingQueue → ImageData。下游按 TimestampGenerator tick 取帧。
 *
 * 契约：端口 3进/2出、Info 键名不变（debug_port 退役=0，新增 engine/
 * frame_path）、save/load 10 键不变。
 *
 * 【架构（CefRefactorPlan P0-P2 已落地）】
 * - 无外部浏览器进程：CEF 多进程由 libcef 托管（FlowCefHelper 子进程宿主），
 *   重启死循环/"自动重启中"病灶消失；
 * - 分辨率变更走 WasResized，不再重启任何进程；
 * - CEF 运行时 init-once：全进程 CefInitialize 一次，多节点共享；
 * - 帧路径：GPU 共享纹理（OnAcceleratedPaint→D3D11→WGL interop）优先，
 *   探测/运行失败自动降级 CPU OnPaint（Info frame_path=cpu/gpu 可观测）；
 * - 退出路径：节点 dtor 仅 close 浏览器（零阻塞），qAddPostRoutine 统一
 *   排空 OnBeforeClose 后 CefShutdown → Flow 关闭秒退。
 */

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/CefBox/CefOsrBrowser.h"
#include "Common/CefBox/CefRuntime.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "CefD3dTextureBridge.h"
#include "WebRenderInterface.hpp"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QMutex>
#include <QPointer>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QUrl>

#include <atomic>
#include <memory>
#include <opencv2/core/mat.hpp>

using QtNodes::NodeData;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
class WebRenderDataModel final : public AbstractDelegateModel
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(double maxFps READ maxFps WRITE setMaxFps NOTIFY maxFpsChanged)

    enum InputPort : PortIndex {
        ActivePort = 0,
        UrlPort = 1,
        ReloadPort = 2
    };

    enum OutputPort : PortIndex {
        ImagePort = 0,
        InfoPort = 1
    };

public:
    WebRenderDataModel()
        : m_widget(new WebRenderInterface())
    {
        InPortCount = 3;
        OutPortCount = 2;
        CaptionVisible = true;
        Caption = QStringLiteral("Web Render");
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        ensureImageDataBuffer(m_outImage, m_outputBuffer);
        m_outInfo = std::make_shared<VariableData>();
        publishInfo();

        ImageGpuUpload::instance().warmup();

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "active";
            b.control = m_widget->m_activeCheck;
            registerExternalBinding(QStringLiteral("/active"), this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "url";
            b.control = m_widget->m_urlEdit;
            registerExternalBinding(QStringLiteral("/url"), this, b);
        }

        connect(m_widget, &WebRenderInterface::activeChanged, this, &WebRenderDataModel::setActive);
        connect(m_widget, &WebRenderInterface::sourceModeChanged, this, &WebRenderDataModel::setSourceMode);
        connect(m_widget, &WebRenderInterface::urlChanged, this, &WebRenderDataModel::setUrl);
        connect(m_widget, &WebRenderInterface::htmlChanged, this, &WebRenderDataModel::setHtml);
        connect(m_widget, &WebRenderInterface::widthChanged, this, &WebRenderDataModel::setWidth);
        connect(m_widget, &WebRenderInterface::heightChanged, this, &WebRenderDataModel::setHeight);
        connect(m_widget, &WebRenderInterface::maxFpsChanged, this, &WebRenderDataModel::setMaxFps);
        connect(m_widget, &WebRenderInterface::dpiScaleChanged, this, &WebRenderDataModel::setDpiScale);
        connect(m_widget, &WebRenderInterface::updateWhenLoadedChanged, this, &WebRenderDataModel::setUpdateWhenLoaded);
        connect(m_widget, &WebRenderInterface::autoRestartChanged, this, [this](bool v) { m_autoRestart = v; });
        connect(m_widget, &WebRenderInterface::reloadRequested, this, &WebRenderDataModel::reloadNow);

        m_fpsTimer.start();
        m_acceptClock.start();
    }

    ~WebRenderDataModel() override
    {
        m_shuttingDown.store(true);
        // 零阻塞退出：摘回调（保证此后 OnPaint 不触达已亡节点）+ 优雅关闭，
        // OnBeforeClose 由退出钩子（qAddPostRoutine）的排空泵收尾。
        // CefRuntime 为进程级单例，此处不 shutdown。
        if (m_cefBrowser) {
            m_cefBrowser->clearCallbacks();
            m_cefBrowser->close();
            m_cefBrowser = nullptr;
        }
        // 显式释放 GPU 桥（GL interop 资源须在 GL 上下文存活期内注销）
        m_bridge.reset();
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return portIndex == ImagePort ? ImageData().type() : VariableData().type();
        }
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        if (port == InfoPort) {
            return m_outInfo;
        }
        return m_outImage ? m_outImage : std::make_shared<ImageData>();
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (!data || m_shuttingDown.load()) {
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
        case UrlPort:
            setUrl(v->asString());
            break;
        case ReloadPort:
            if (v->asBool()) {
                reloadNow();
            }
            break;
        default:
            break;
        }
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case ActivePort: return QStringLiteral("Active");
            case UrlPort: return QStringLiteral("URL");
            case ReloadPort: return QStringLiteral("Reload");
            default: break;
            }
        } else if (portType == PortType::Out) {
            switch (portIndex) {
            case ImagePort: return QStringLiteral("IMAGE");
            case InfoPort: return QStringLiteral("Info");
            default: break;
            }
        }
        return {};
    }

    QWidget* embeddedWidget() override
    {
        return m_widget.data();
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        QJsonObject values;
        values.insert(QStringLiteral("active"), m_active);
        values.insert(QStringLiteral("sourceMode"), m_sourceMode);
        values.insert(QStringLiteral("url"), m_url);
        values.insert(QStringLiteral("html"), m_html);
        values.insert(QStringLiteral("width"), m_width);
        values.insert(QStringLiteral("height"), m_height);
        values.insert(QStringLiteral("maxFps"), m_maxFps);
        values.insert(QStringLiteral("dpiScale"), m_dpiScale);
        values.insert(QStringLiteral("updateWhenLoaded"), m_updateWhenLoaded);
        values.insert(QStringLiteral("autoRestart"), m_autoRestart);
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

        if (values.contains(QStringLiteral("sourceMode"))) {
            setSourceMode(values.value(QStringLiteral("sourceMode")).toString());
        }
        if (values.contains(QStringLiteral("url"))) {
            setUrl(values.value(QStringLiteral("url")).toString());
        }
        if (values.contains(QStringLiteral("html"))) {
            setHtml(values.value(QStringLiteral("html")).toString());
        }
        if (values.contains(QStringLiteral("width"))) {
            setWidth(values.value(QStringLiteral("width")).toInt(m_width));
        }
        if (values.contains(QStringLiteral("height"))) {
            setHeight(values.value(QStringLiteral("height")).toInt(m_height));
        }
        if (values.contains(QStringLiteral("maxFps"))) {
            setMaxFps(values.value(QStringLiteral("maxFps")).toDouble(m_maxFps));
        }
        if (values.contains(QStringLiteral("dpiScale"))) {
            setDpiScale(values.value(QStringLiteral("dpiScale")).toDouble(m_dpiScale));
        }
        if (values.contains(QStringLiteral("updateWhenLoaded"))) {
            setUpdateWhenLoaded(values.value(QStringLiteral("updateWhenLoaded")).toBool(true));
        }
        if (values.contains(QStringLiteral("autoRestart"))) {
            m_autoRestart = values.value(QStringLiteral("autoRestart")).toBool(true);
            if (m_widget) {
                QSignalBlocker b(m_widget->m_autoRestartCheck);
                m_widget->m_autoRestartCheck->setChecked(m_autoRestart);
            }
        }

        syncWidgetFromState();

        if (values.value(QStringLiteral("active")).toBool()) {
            QTimer::singleShot(600, this, [self = QPointer<WebRenderDataModel>(this)]() {
                if (self && !self->m_shuttingDown.load()) {
                    self->setActive(true);
                }
            });
        }
    }

    bool active() const { return m_active; }
    QString url() const { return m_url; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    double maxFps() const { return m_maxFps; }

    void setActive(bool value)
    {
        if (m_shuttingDown.load()) {
            return;
        }
        if (m_active == value) {
            return;
        }
        m_active = value;
        if (m_widget) {
            m_widget->setActiveChecked(m_active);
        }

        if (m_active) {
            startPipeline();
        } else {
            stopPipeline();
        }
        Q_EMIT activeChanged(m_active);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void setSourceMode(const QString& mode)
    {
        const QString m = (mode == QLatin1String("html")) ? QStringLiteral("html") : QStringLiteral("url");
        if (m_sourceMode == m) {
            return;
        }
        m_sourceMode = m;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_sourceCombo);
            const int idx = m_widget->m_sourceCombo->findData(m_sourceMode);
            if (idx >= 0) {
                m_widget->m_sourceCombo->setCurrentIndex(idx);
            }
            const bool htmlMode = m_sourceMode == QLatin1String("html");
            m_widget->m_urlEdit->setVisible(!htmlMode);
            m_widget->m_htmlEdit->setVisible(htmlMode);
        }
        if (m_active && m_cefBrowser && !m_cefBrowser->isClosed()) {
            m_cefBrowser->loadUrl(currentSourceUrl().toStdString());
        }
    }

    void setUrl(const QString& value)
    {
        const QString trimmed = value.trimmed();
        if (m_url == trimmed) {
            return;
        }
        m_url = trimmed;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_urlEdit);
            m_widget->m_urlEdit->setText(m_url);
        }
        Q_EMIT urlChanged(m_url);
        if (m_active && m_sourceMode == QLatin1String("url") && m_cefBrowser && !m_cefBrowser->isClosed()) {
            m_cefBrowser->loadUrl(currentSourceUrl().toStdString());
        }
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void setHtml(const QString& value)
    {
        if (m_html == value) {
            return;
        }
        m_html = value;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_htmlEdit);
            if (m_widget->m_htmlEdit->toPlainText() != m_html) {
                m_widget->m_htmlEdit->setPlainText(m_html);
            }
        }
        // HTML 编辑不自动导航，需点 Reload Source（避免每键重载）
    }

    void setWidth(int value)
    {
        value = qBound(16, value, 7680);
        if (m_width == value) {
            return;
        }
        m_width = value;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_widthSpin);
            m_widget->m_widthSpin->setValue(m_width);
        }
        Q_EMIT widthChanged(m_width);
        // 分辨率变更不再重启进程：WasResized 直达（CEF 后端核心改进）
        applyViewportToBrowser();
    }

    void setHeight(int value)
    {
        value = qBound(16, value, 4320);
        if (m_height == value) {
            return;
        }
        m_height = value;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_heightSpin);
            m_widget->m_heightSpin->setValue(m_height);
        }
        Q_EMIT heightChanged(m_height);
        applyViewportToBrowser();
    }

    void setMaxFps(double value)
    {
        value = qBound(0.5, value, 60.0);
        if (qFuzzyCompare(m_maxFps, value)) {
            return;
        }
        m_maxFps = value;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_fpsSpin);
            m_widget->m_fpsSpin->setValue(m_maxFps);
        }
        m_lastFrameAcceptMs = -1;  // 新节流窗立即可接收一帧
        Q_EMIT maxFpsChanged(m_maxFps);
    }

    void setDpiScale(double value)
    {
        value = qBound(0.25, value, 4.0);
        if (qFuzzyCompare(m_dpiScale, value)) {
            return;
        }
        m_dpiScale = value;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_dpiSpin);
            m_widget->m_dpiSpin->setValue(m_dpiScale);
        }
        applyViewportToBrowser();
    }

    void setUpdateWhenLoaded(bool value)
    {
        m_updateWhenLoaded = value;
        if (m_widget) {
            QSignalBlocker b(m_widget->m_updateWhenLoadedCheck);
            m_widget->m_updateWhenLoadedCheck->setChecked(value);
        }
    }

signals:
    void activeChanged(bool active);
    void urlChanged(const QString& url);
    void widthChanged(int width);
    void heightChanged(int height);
    void maxFpsChanged(double fps);

private Q_SLOTS:
    void publishPendingFrame()
    {
        m_uploadScheduled.store(false);
        if (m_shuttingDown.load() || !m_active) {
            return;
        }

        cv::Mat mat;
        {
            QMutexLocker locker(&m_pendingMutex);
            mat = std::move(m_pendingFrame);
        }
        if (!m_outputBuffer || mat.empty()) {
            return;
        }

        ensureImageDataBuffer(m_outImage, m_outputBuffer);
        const qint64 timestamp = mediaOutputTimestamp(
            TimestampGenerator::getInstance()->getCurrentFrameCount());
        // BGRA8 快路径：跳过色彩转换直传 GPU（CPU 副本同步保留）
        ImageFrame imageFrame = ImageFrame::fromBgra8Mat(std::move(mat), timestamp);
        if (!imageFrame.texture.valid()) {
            qWarning() << "Web Render: GPU upload failed";
            return;
        }
        ++m_totalFrames;
        pushFrameToImageBufferDedup(m_outputBuffer, std::move(imageFrame), m_lastPushedTimestamp);

        bool hasPending = false;
        {
            QMutexLocker locker(&m_pendingMutex);
            hasPending = !m_pendingFrame.empty();
        }
        if (hasPending) {
            scheduleUploadIfNeeded();
        }
    }

private:
    // ---- CEF 帧回调（全部在 GUI 主线程：external_message_pump 契约）----

    /// updateWhenLoaded 已在外层判断；此处仅 maxFps 节流（通过则记录接受时刻）
    bool acceptFrame()
    {
        const qint64 nowMs = m_acceptClock.elapsed();
        const qint64 minIntervalMs = qRound64(1000.0 / m_maxFps);
        if (m_lastFrameAcceptMs >= 0 && nowMs - m_lastFrameAcceptMs < minIntervalMs) {
            return false;
        }
        m_lastFrameAcceptMs = nowMs;
        return true;
    }

    void countFrameTick()
    {
        ++m_framesSinceFpsSample;
        if (m_fpsTimer.elapsed() >= 1000) {
            m_updatesPerSecond = m_framesSinceFpsSample * 1000.0 / double(m_fpsTimer.elapsed());
            m_framesSinceFpsSample = 0;
            m_fpsTimer.restart();
            publishInfo();
            Q_EMIT dataUpdated(InfoPort);
        }
    }

    void onCefFrame(const void* bgra, int width, int height)
    {
        if (m_shuttingDown.load() || !m_active || !bgra || width < 1 || height < 1) {
            return;
        }
        // updateWhenLoaded：页面加载完成前不出帧（对齐旧 CDP 语义）
        if (m_updateWhenLoaded && !m_loaded) {
            return;
        }
        if (!acceptFrame()) {
            return;
        }

        // OnPaint buffer 仅回调内有效，必须拷出（BGRA8 预乘 alpha）
        cv::Mat wrapped(height, width, CV_8UC4, const_cast<void*>(bgra));
        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingFrame = wrapped.clone();
        }
        scheduleUploadIfNeeded();
        if (m_totalFrames == 0 && m_framesSinceFpsSample == 0) {
            qDebug() << "WebRender: first CPU frame" << width << "x" << height;
        }

        countFrameTick();
    }

    /// P2 GPU 路径：OnAcceleratedPaint → 桥内 D3D 拷贝 + interop 拷入 GL 纹理，
    /// 同步产出普通 GL 纹理句柄，直接推 buffer（无 CPU 拷出链）。
    void onCefAccelFrame(const cefbox::SharedFrameInfo& info)
    {
        if (m_shuttingDown.load() || !m_active) {
            return;
        }
        if (info.colorBgra != 1) {
            return;  // GPU 链路仅支持 BGRA8（防御 RGBA 情形；CEF 默认即 BGRA）
        }
        if (m_updateWhenLoaded && !m_loaded) {
            return;
        }
        if (!acceptFrame()) {
            return;
        }
        if (!m_bridge) {
            return;  // 无桥却收到 accel 帧（异常状态），丢弃
        }

        auto handle = m_bridge->pushFrame(info);
        if (!handle.textureId) {
            // GPU 拷贝失败：连续 5 帧 → 禁用 GPU 路径，延迟到事件循环重建浏览器
            // 降级 CPU OnPaint（不可在 CEF 泵回调内同步重入）
            if (++m_accelFailCount >= 5 && !m_gpuDisabled) {
                qWarning() << "WebRender: gpu push failed x5, degrading to CPU path";
                m_gpuDisabled = true;
                m_bridge.reset();
                m_framePathGpu = false;
                m_lastFrameAcceptMs = -1;
                publishInfo();
                Q_EMIT dataUpdated(InfoPort);
                QMetaObject::invokeMethod(
                    this, [this]() { recreateBrowser(); }, Qt::QueuedConnection);
            }
            return;
        }
        m_accelFailCount = 0;
        if (m_totalFrames == 0) {
            qDebug() << "WebRender: first GPU frame" << handle.width << "x" << handle.height
                     << "tex=" << handle.textureId;
        }

        const qint64 timestamp = mediaOutputTimestamp(
            TimestampGenerator::getInstance()->getCurrentFrameCount());
        ImageFrame imageFrame = ImageFrame::fromTexture(std::move(handle), timestamp);
        if (!imageFrame.texture.valid()) {
            qWarning() << "Web Render: GPU frame invalid";
            return;
        }
        ++m_totalFrames;
        pushFrameToImageBufferDedup(m_outputBuffer, std::move(imageFrame), m_lastPushedTimestamp);
        countFrameTick();
    }

    void onCefLoad(bool ok)
    {
        if (m_shuttingDown.load()) {
            return;
        }
        qDebug() << "WebRender: onLoad ok=" << ok;
        if (ok) {
            m_loaded = true;
            setStatus(QStringLiteral("状态: 已加载 / 渲染中"), true);
        } else {
            m_lastError = QStringLiteral("页面加载失败");
            setStatus(QStringLiteral("状态: 页面加载失败"), false);
        }
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void scheduleUploadIfNeeded()
    {
        if (m_shuttingDown.load() || !m_active) {
            return;
        }
        if (!m_uploadScheduled.exchange(true)) {
            QMetaObject::invokeMethod(this, "publishPendingFrame", Qt::QueuedConnection);
        }
    }

    // ---- 生命周期 ----

    void startPipeline()
    {
        m_lastError.clear();
        setStatus(QStringLiteral("状态: 初始化 CEF…"), false);
        sweepOrphanCache();  // 首节点启动时清扫孤儿缓存（init 后为 no-op）

        cefbox::CefRuntime::Settings rs;
        rs.cacheDir = cefCacheDir().toStdString();  // 全进程共享一份 CEF profile
        if (!cefbox::CefRuntime::instance().initialize(rs)) {
            m_active = false;
            if (m_widget) {
                m_widget->setActiveChecked(false);
            }
            m_lastError = QStringLiteral("CEF 运行时初始化失败");
            setStatus(m_lastError, false);
            publishInfo();
            Q_EMIT dataUpdated(InfoPort);
            return;
        }
        ensureRuntimeExitHook();

        setStatus(QStringLiteral("状态: 创建离屏浏览器…"), false);
        m_loaded = false;
        m_accelFailCount = 0;
        const int vw = qMax(1, qRound(m_width * m_dpiScale));
        const int vh = qMax(1, qRound(m_height * m_dpiScale));
        const std::string target = currentSourceUrl().toStdString();

        // P2 GPU 共享纹理路径：进程级探测通过则建桥（D3D 设备跨浏览器复用），
        // init 失败 → 自动走 CPU OnPaint 兜底。
        if (!m_gpuDisabled && !m_bridge && CefD3dTextureBridge::isSupported()) {
            auto bridge = std::make_unique<CefD3dTextureBridge>();
            if (bridge->init()) {
                m_bridge = std::move(bridge);
            }
        }
        qDebug() << "WebRender: runtime ready, bridge=" << (m_bridge != nullptr)
                 << "gpuDisabled=" << m_gpuDisabled << "viewport=" << vw << "x" << vh;

        auto onLoad = [this](bool ok) { onCefLoad(ok); };
        CefRefPtr<cefbox::CefOsrBrowser> browser;
        if (m_bridge) {
            browser = cefbox::CefOsrBrowser::createAccelerated(
                vw, vh, target,
                [this](const cefbox::SharedFrameInfo& info) { onCefAccelFrame(info); },
                onLoad);
            if (!browser) {
                m_bridge.reset();  // 加速浏览器创建失败 → CPU 兜底重试
            }
        }
        if (!browser) {
            browser = cefbox::CefOsrBrowser::create(
                vw, vh, target,
                [this](const void* bgra, int w, int h) { onCefFrame(bgra, w, h); },
                onLoad);
        }
        m_framePathGpu = (m_bridge != nullptr);
        if (!browser) {
            m_active = false;
            if (m_widget) {
                m_widget->setActiveChecked(false);
            }
            m_lastError = QStringLiteral("离屏浏览器创建失败");
            setStatus(m_lastError, false);
            publishInfo();
            Q_EMIT dataUpdated(InfoPort);
            return;
        }
        m_cefBrowser = browser;
        qDebug() << "WebRender: browser created, frame_path="
                 << (m_framePathGpu ? "gpu" : "cpu");

        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void stopPipeline()
    {
        if (m_cefBrowser) {
            m_cefBrowser->clearCallbacks();
            m_cefBrowser->close();  // 优雅关闭；OnBeforeClose 由排空泵送达
            m_cefBrowser = nullptr;
        }
        m_loaded = false;

        m_uploadScheduled.store(false);
        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingFrame.release();
        }
        if (m_outputBuffer) {
            m_outputBuffer->clear();
        }
        m_lastPushedTimestamp = -1;
        setStatus(QStringLiteral("状态: 未启动"), false);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    /// Shift+点击强制重建浏览器（旧「Restart」语义）
    void recreateBrowser()
    {
        if (!m_active) {
            return;
        }
        stopPipeline();
        m_active = true;
        if (m_widget) {
            m_widget->setActiveChecked(true);
        }
        startPipeline();
    }

    /// 一键重载：Shift+点击=重建浏览器；否则按当前源导航/重载
    void reloadNow()
    {
        if (m_shuttingDown.load()) {
            return;
        }
        if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
            recreateBrowser();
            return;
        }
        if (!m_active) {
            return;
        }
        if (!m_cefBrowser || m_cefBrowser->isClosed()) {
            startPipeline();
            return;
        }
        const std::string target = currentSourceUrl().toStdString();
        if (m_cefBrowser->currentUrl() == target) {
            m_cefBrowser->reloadPage();
        } else {
            m_cefBrowser->loadUrl(target);
        }
    }

    void applyViewportToBrowser()
    {
        if (m_active && m_cefBrowser && !m_cefBrowser->isClosed()) {
            const int vw = qMax(1, qRound(m_width * m_dpiScale));
            const int vh = qMax(1, qRound(m_height * m_dpiScale));
            m_cefBrowser->resize(vw, vh);
        }
    }

    // ---- 源定位 ----

    /// 当前应加载的 URL：html 模式=临时文件 file:///；url 模式=QUrl 归一化
    /// （裸路径/无 scheme 域名统一补全——顺带解决旧后端裸路径问题）
    QString currentSourceUrl()
    {
        if (m_sourceMode == QLatin1String("html")) {
            return writeHtmlTemp();
        }
        const QString raw = m_url.trimmed().isEmpty() ? QStringLiteral("https://example.com") : m_url.trimmed();
        return QUrl::fromUserInput(raw).toString();
    }

    /// 把 m_html 写进本节点临时文件，返回 file:/// URL（reloadNow 时重写）
    QString writeHtmlTemp()
    {
        const QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/NodeEditorWebRender");
        QDir().mkpath(base);
        if (m_htmlPath.isEmpty()) {
            m_htmlPath = base + QLatin1String("/html_")
                + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".html");
        }
        QFile f(m_htmlPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(m_html.toUtf8());
        }
        return QUrl::fromLocalFile(m_htmlPath).toString();
    }

    /// 进程级共享 CEF profile 目录（首次 initialize 前清扫孤儿）
    static QString cefCacheDir()
    {
        return QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/NodeEditorWebRender/cef");
    }

    /// 首次初始化前清扫孤儿缓存（单实例互斥保证无并发 Flow；仅首节点执行）
    static void sweepOrphanCache()
    {
        if (cefbox::CefRuntime::instance().isInitialized()) {
            return;  // 已有节点初始化过 CEF，目录正被使用
        }
        // 整目录清扫：覆盖 CEF 缓存、html 临时文件、以及旧 Edge 后端遗留的
        // UUID profile 目录（孤儿 Edge profile 只有这里能收掉）
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)
             + QStringLiteral("/NodeEditorWebRender"))
            .removeRecursively();
    }

    /// 进程退出钩子（仅注册一次）：排空 OnBeforeClose → CefShutdown → 秒退
    static void ensureRuntimeExitHook()
    {
        static const bool registered = [] {
            qAddPostRoutine([]() {
                cefbox::CefRuntime::instance().shutdown();
            });
            return true;
        }();
        Q_UNUSED(registered);
    }

    // ---- Info / 状态 ----

    void publishInfo()
    {
        if (!m_outInfo) {
            m_outInfo = std::make_shared<VariableData>();
        }
        const QString currentUrl = (m_cefBrowser && !m_cefBrowser->isClosed())
            ? QString::fromStdString(m_cefBrowser->currentUrl()) : QString();
        m_outInfo->insert(QStringLiteral("loaded"), m_loaded);
        m_outInfo->insert(QStringLiteral("num_handle_updates"), qint64(m_totalFrames));
        m_outInfo->insert(QStringLiteral("url"), currentUrl.isEmpty() ? m_url : currentUrl);
        m_outInfo->insert(QStringLiteral("title"),
                          m_cefBrowser ? QString::fromStdString(m_cefBrowser->title()) : QString());
        m_outInfo->insert(QStringLiteral("error"), m_lastError);
        m_outInfo->insert(QStringLiteral("process_id"), QCoreApplication::applicationPid());
        m_outInfo->insert(QStringLiteral("debug_port"), 0);  // CEF 后端无 CDP 端口（兼容保留）
        m_outInfo->insert(QStringLiteral("updates_per_second"), m_updatesPerSecond);
        m_outInfo->insert(QStringLiteral("active"), m_active);
        m_outInfo->insert(QStringLiteral("browser"),
                          QStringLiteral("FlowCefHelper (libcef 156.0.8078.100)"));
        m_outInfo->insert(QStringLiteral("engine"), QStringLiteral("cef"));
        m_outInfo->insert(QStringLiteral("frame_path"),
                          m_framePathGpu ? QStringLiteral("gpu") : QStringLiteral("cpu"));
    }

    void setStatus(const QString& text, bool ok)
    {
        if (m_widget) {
            m_widget->setStatus(text, ok);
        }
    }

    void syncWidgetFromState()
    {
        if (!m_widget) {
            return;
        }
        {
            QSignalBlocker b(m_widget->m_urlEdit);
            m_widget->m_urlEdit->setText(m_url);
        }
        {
            QSignalBlocker b(m_widget->m_htmlEdit);
            m_widget->m_htmlEdit->setPlainText(m_html);
        }
        {
            QSignalBlocker b(m_widget->m_widthSpin);
            m_widget->m_widthSpin->setValue(m_width);
        }
        {
            QSignalBlocker b(m_widget->m_heightSpin);
            m_widget->m_heightSpin->setValue(m_height);
        }
        {
            QSignalBlocker b(m_widget->m_fpsSpin);
            m_widget->m_fpsSpin->setValue(m_maxFps);
        }
        {
            QSignalBlocker b(m_widget->m_dpiSpin);
            m_widget->m_dpiSpin->setValue(m_dpiScale);
        }
        {
            QSignalBlocker b(m_widget->m_updateWhenLoadedCheck);
            m_widget->m_updateWhenLoadedCheck->setChecked(m_updateWhenLoaded);
        }
        {
            QSignalBlocker b(m_widget->m_sourceCombo);
            const int idx = m_widget->m_sourceCombo->findData(m_sourceMode);
            if (idx >= 0) {
                m_widget->m_sourceCombo->setCurrentIndex(idx);
            }
        }
        const bool htmlMode = m_sourceMode == QLatin1String("html");
        m_widget->m_urlEdit->setVisible(!htmlMode);
        m_widget->m_htmlEdit->setVisible(htmlMode);
    }

    QPointer<WebRenderInterface> m_widget;
    CefRefPtr<cefbox::CefOsrBrowser> m_cefBrowser;

    // P2 GPU 共享纹理桥（跨浏览器复用；m_gpuDisabled 后永久走 CPU 兜底）
    std::unique_ptr<CefD3dTextureBridge> m_bridge;
    bool m_framePathGpu = false;   ///< 当前浏览器是否为 createAccelerated 路径
    int m_accelFailCount = 0;      ///< 连续 GPU 拷贝失败计数（成功清零；≥5 降级）
    bool m_gpuDisabled = false;    ///< GPU 路径熔断标志（本次会话不再尝试）

    std::shared_ptr<ImageData> m_outImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;
    std::shared_ptr<VariableData> m_outInfo;
    qint64 m_lastPushedTimestamp = -1;

    QMutex m_pendingMutex;
    cv::Mat m_pendingFrame;
    std::atomic<bool> m_uploadScheduled{false};
    std::atomic<bool> m_shuttingDown{false};

    bool m_active = false;
    bool m_loaded = false;
    QString m_sourceMode = QStringLiteral("url");
    QString m_url = QStringLiteral("https://example.com");
    QString m_html = QStringLiteral("<html><body style='font-family:sans-serif;background:#111;color:#eee;'>"
                                    "<h1>Web Render</h1><p>HTML source mode</p></body></html>");
    int m_width = 1280;
    int m_height = 720;
    double m_maxFps = 15.0;
    double m_dpiScale = 1.0;
    bool m_updateWhenLoaded = true;
    bool m_autoRestart = true;  // CEF 后端渲染进程自愈，此开关仅存档兼容
    QString m_htmlPath;
    QString m_lastError;

    QElapsedTimer m_fpsTimer;
    QElapsedTimer m_acceptClock;
    qint64 m_lastFrameAcceptMs = -1;
    int m_framesSinceFpsSample = 0;
    qint64 m_totalFrames = 0;
    double m_updatesPerSecond = 0.0;
};
} // namespace Nodes
