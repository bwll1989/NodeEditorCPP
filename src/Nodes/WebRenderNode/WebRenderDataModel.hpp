#pragma once

/**
 * @file WebRenderDataModel.hpp
 * @brief Web Render 节点 — 对齐 TouchDesigner Web Render TOP 模式
 *
 * 独立 Edge/Chrome 进程离屏渲染 → CDP 截图 → ImageTimestampRingQueue → ImageData。
 * 下游按 TimestampGenerator tick 取帧，不每帧 dataUpdated。
 */

#include "WebRenderBrowserProcess.h"
#include "WebRenderCdpClient.h"
#include "WebRenderInterface.hpp"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QMutex>
#include <QPointer>
#include <QProcess>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QThreadPool>

#include <atomic>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

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
        , m_browser(new WebRenderBrowserProcess(this))
        , m_cdp(new WebRenderCdpClient(this))
    {
        InPortCount = 3;
        OutPortCount = 2;
        CaptionVisible = true;
        Caption = QStringLiteral("Web Render");
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        qRegisterMetaType<cv::Mat>("cv::Mat");
        ensureImageDataBuffer(m_outImage, m_outputBuffer);
        m_outInfo = std::make_shared<VariableData>();
        publishInfo();

        m_userDataDir = makeUniqueUserDir();

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

        connect(m_browser, &WebRenderBrowserProcess::ready, this, &WebRenderDataModel::onBrowserReady);
        connect(m_browser, &WebRenderBrowserProcess::processDied, this, &WebRenderDataModel::onBrowserDied);
        connect(m_browser, &WebRenderBrowserProcess::errorOccurred, this, &WebRenderDataModel::onBrowserError);
        connect(m_browser, &WebRenderBrowserProcess::startFailed, this, [this]() {
            if (m_shuttingDown.load()) {
                return;
            }
            // 异步启动失败：回滚 Active，与同步参数校验失败行为一致
            m_active = false;
            if (m_widget) {
                m_widget->setActiveChecked(false);
            }
            publishInfo();
            Q_EMIT dataUpdated(InfoPort);
        });

        // 分辨率变更防抖：停止拖动 500ms 后才重启浏览器进程，避免连环重启卡顿
        m_restartDebounce.setSingleShot(true);
        m_restartDebounce.setInterval(500);
        connect(&m_restartDebounce, &QTimer::timeout, this, [this]() {
            if (m_active && !m_shuttingDown.load()) {
                restartBrowser();
            }
        });

        connect(m_cdp, &WebRenderCdpClient::connected, this, &WebRenderDataModel::onCdpConnected);
        connect(m_cdp, &WebRenderCdpClient::disconnected, this, &WebRenderDataModel::onCdpDisconnected);
        connect(m_cdp, &WebRenderCdpClient::loadedChanged, this, &WebRenderDataModel::onLoadedChanged);
        connect(m_cdp, &WebRenderCdpClient::pageInfoChanged, this, &WebRenderDataModel::onPageInfoChanged);
        connect(m_cdp, &WebRenderCdpClient::frameCaptured, this, &WebRenderDataModel::onFrameCaptured, Qt::QueuedConnection);
        connect(m_cdp, &WebRenderCdpClient::errorOccurred, this, &WebRenderDataModel::onCdpError);

        m_cdp->setMaxRenderRate(m_maxFps);
        m_cdp->setUpdateWhenLoaded(m_updateWhenLoaded);
        m_cdp->setViewport(m_width, m_height, m_dpiScale);

        m_fpsTimer.start();
    }

    ~WebRenderDataModel() override
    {
        m_shuttingDown.store(true);
        setActive(false);
        m_cdp->disconnectFromDebugger();
        m_browser->stop();

        // 清理本实例缓存目录
        if (!m_userDataDir.isEmpty()) {
            QDir(m_userDataDir).removeRecursively();
        }
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
            m_restartDebounce.stop();
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
        if (m_active && m_cdp->isConnected()) {
            reloadSource();
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
        if (m_active && m_sourceMode == QLatin1String("url") && m_cdp->isConnected()) {
            m_cdp->navigate(m_url);
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
        m_cdp->setViewport(m_width, m_height, m_dpiScale);
        Q_EMIT widthChanged(m_width);
        // 分辨率变化需重启浏览器窗口尺寸才稳定（防抖后执行，拖动不卡顿）
        if (m_active) {
            m_restartDebounce.start();
        }
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
        m_cdp->setViewport(m_width, m_height, m_dpiScale);
        Q_EMIT heightChanged(m_height);
        if (m_active) {
            m_restartDebounce.start();
        }
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
        m_cdp->setMaxRenderRate(m_maxFps);
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
        m_cdp->setViewport(m_width, m_height, m_dpiScale);
    }

    void setUpdateWhenLoaded(bool value)
    {
        m_updateWhenLoaded = value;
        m_cdp->setUpdateWhenLoaded(value);
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

protected:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        ImageGpuUpload::instance().warmup();
    }

private Q_SLOTS:
    void onBrowserReady(int debugPort)
    {
        if (m_shuttingDown.load() || !m_active) {
            return;
        }
        setStatus(QStringLiteral("状态: CDP 连接中…"), false);
        m_cdp->connectToDebugger(debugPort);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void onBrowserDied(int, QProcess::ExitStatus)
    {
        if (m_shuttingDown.load()) {
            return;
        }
        m_cdp->disconnectFromDebugger();
        setStatus(QStringLiteral("状态: 浏览器进程已退出"), false);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);

        if (m_active && m_autoRestart) {
            setStatus(QStringLiteral("状态: 正在自动重启…"), false);
            QTimer::singleShot(800, this, [self = QPointer<WebRenderDataModel>(this)]() {
                if (self && self->m_active && !self->m_shuttingDown.load()) {
                    self->startPipeline();
                }
            });
        }
    }

    void onBrowserError(const QString& message)
    {
        setStatus(QStringLiteral("状态: %1").arg(message), false);
        m_lastError = message;
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void onCdpConnected()
    {
        setStatus(QStringLiteral("状态: 已连接，加载页面…"), true);
        m_cdp->setViewport(m_width, m_height, m_dpiScale);
        m_cdp->setMaxRenderRate(m_maxFps);
        m_cdp->setUpdateWhenLoaded(m_updateWhenLoaded);
        reloadSource();
        m_cdp->setCapturing(true);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void onCdpDisconnected()
    {
        if (!m_shuttingDown.load()) {
            setStatus(QStringLiteral("状态: CDP 已断开"), false);
        }
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void onLoadedChanged(bool loaded)
    {
        if (loaded) {
            setStatus(QStringLiteral("状态: 已加载 / 渲染中"), true);
        }
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void onPageInfoChanged(const QString&, const QString&)
    {
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void onFrameCaptured(const QByteArray& imageBytes)
    {
        if (m_shuttingDown.load() || !m_active || imageBytes.isEmpty()) {
            return;
        }

        // 解码积压时直接丢帧，避免线程池排队越来越长
        if (m_decodeInFlight.load() >= 2) {
            return;
        }
        m_decodeInFlight.fetch_add(1);

        const QByteArray bytes = imageBytes;
        QThreadPool::globalInstance()->start([self = QPointer<WebRenderDataModel>(this), bytes]() {
            std::vector<uchar> buf(bytes.begin(), bytes.end());
            cv::Mat decoded = cv::imdecode(buf, cv::IMREAD_COLOR);
            if (!self) {
                return;
            }
            if (decoded.empty()) {
                self->m_decodeInFlight.fetch_sub(1);
                return;
            }
            // QueuedConnection 把 Mat 投递回 GUI 线程上传 GPU
            QMetaObject::invokeMethod(self.data(), "onFrameDecoded", Qt::QueuedConnection,
                                      Q_ARG(cv::Mat, decoded));
        });
    }

    void onFrameDecoded(cv::Mat mat)
    {
        m_decodeInFlight.fetch_sub(1);
        if (m_shuttingDown.load() || !m_active || mat.empty()) {
            return;
        }

        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingFrame = std::move(mat);
        }
        scheduleUploadIfNeeded();

        ++m_framesSinceFpsSample;
        if (m_fpsTimer.elapsed() >= 1000) {
            m_updatesPerSecond = m_framesSinceFpsSample * 1000.0 / double(m_fpsTimer.elapsed());
            m_framesSinceFpsSample = 0;
            m_fpsTimer.restart();
            publishInfo();
            Q_EMIT dataUpdated(InfoPort);
        }
    }

    void onCdpError(const QString& message)
    {
        m_lastError = message;
        setStatus(QStringLiteral("状态: %1").arg(message), false);
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

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
        ImageFrame imageFrame = ImageFrame::fromMat(std::move(mat), timestamp);
        if (!imageFrame.texture.valid()) {
            qWarning() << "Web Render: GPU upload failed";
            return;
        }
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
    void scheduleUploadIfNeeded()
    {
        if (m_shuttingDown.load() || !m_active) {
            return;
        }
        if (!m_uploadScheduled.exchange(true)) {
            QMetaObject::invokeMethod(this, "publishPendingFrame", Qt::QueuedConnection);
        }
    }

    void startPipeline()
    {
        m_lastError.clear();
        if (m_userDataDir.isEmpty()) {
            m_userDataDir = makeUniqueUserDir();
        }
        QDir().mkpath(m_userDataDir);

        setStatus(QStringLiteral("状态: 启动浏览器…"), false);
        if (!m_browser->start(m_userDataDir, m_width, m_height)) {
            m_active = false;
            if (m_widget) {
                m_widget->setActiveChecked(false);
            }
        }
        publishInfo();
        Q_EMIT dataUpdated(InfoPort);
    }

    void stopPipeline()
    {
        m_cdp->setCapturing(false);
        m_cdp->disconnectFromDebugger();
        m_browser->stop();

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

    void reloadSource()
    {
        if (!m_cdp->isConnected()) {
            return;
        }
        if (m_sourceMode == QLatin1String("html")) {
            m_cdp->loadHtml(m_html, m_userDataDir);
        } else {
            m_cdp->navigate(m_url);
        }
    }

    /**
     * @brief 一键重载（原 Reload Source / Reload Page / Restart 三按钮合并）
     *
     * 按当前状态自动选择最轻量的恢复路径，GUI 线程零阻塞：
     * - Shift+点击：强制重启浏览器进程
     * - 进程未运行：拉起浏览器
     * - CDP 未连接：重连调试端口；端口未就绪则重启进程
     * - CDP 就绪：HTML 模式重写临时文件并加载；
     *   URL 模式导航回配置地址（页面仍停在配置地址上时用轻量的 Page.reload）
     */
    void reloadNow()
    {
        if (m_shuttingDown.load()) {
            return;
        }
        if (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) {
            restartBrowser();
            return;
        }
        if (!m_active) {
            return;
        }
        if (!m_browser->isRunning()) {
            startPipeline();
            return;
        }
        if (!m_cdp->isConnected()) {
            const int port = m_browser->debugPort();
            if (port > 0) {
                setStatus(QStringLiteral("状态: 重新连接 CDP…"), false);
                m_cdp->connectToDebugger(port);
            } else {
                startPipeline();
            }
            return;
        }
        if (m_sourceMode == QLatin1String("html")) {
            m_cdp->loadHtml(m_html, m_userDataDir);
        } else if (m_cdp->currentUrl().trimmed() == m_url) {
            m_cdp->reload();
        } else {
            m_cdp->navigate(m_url);
        }
    }

    void restartBrowser()
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

    void publishInfo()
    {
        if (!m_outInfo) {
            m_outInfo = std::make_shared<VariableData>();
        }
        m_outInfo->insert(QStringLiteral("loaded"), m_cdp->isLoaded());
        m_outInfo->insert(QStringLiteral("num_handle_updates"), qint64(m_cdp->handleUpdates()));
        m_outInfo->insert(QStringLiteral("url"), m_cdp->currentUrl().isEmpty() ? m_url : m_cdp->currentUrl());
        m_outInfo->insert(QStringLiteral("title"), m_cdp->pageTitle());
        m_outInfo->insert(QStringLiteral("error"), m_lastError.isEmpty() ? m_cdp->lastError() : m_lastError);
        m_outInfo->insert(QStringLiteral("process_id"), m_browser->processId());
        m_outInfo->insert(QStringLiteral("debug_port"), m_browser->debugPort());
        m_outInfo->insert(QStringLiteral("updates_per_second"), m_updatesPerSecond);
        m_outInfo->insert(QStringLiteral("active"), m_active);
        m_outInfo->insert(QStringLiteral("browser"), m_browser->browserPath());
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

    static QString makeUniqueUserDir()
    {
        const QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/NodeEditorWebRender");
        QDir().mkpath(base);
        return base + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    QPointer<WebRenderInterface> m_widget;
    WebRenderBrowserProcess* m_browser = nullptr;
    WebRenderCdpClient* m_cdp = nullptr;

    std::shared_ptr<ImageData> m_outImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;
    std::shared_ptr<VariableData> m_outInfo;
    qint64 m_lastPushedTimestamp = -1;

    QMutex m_pendingMutex;
    cv::Mat m_pendingFrame;
    std::atomic<bool> m_uploadScheduled{false};
    std::atomic<bool> m_shuttingDown{false};
    std::atomic<int> m_decodeInFlight{0};

    bool m_active = false;
    QString m_sourceMode = QStringLiteral("url");
    QString m_url = QStringLiteral("https://example.com");
    QString m_html = QStringLiteral("<html><body style='font-family:sans-serif;background:#111;color:#eee;'>"
                                    "<h1>Web Render</h1><p>HTML source mode</p></body></html>");
    int m_width = 1280;
    int m_height = 720;
    double m_maxFps = 15.0;
    double m_dpiScale = 1.0;
    bool m_updateWhenLoaded = true;
    bool m_autoRestart = true;
    QString m_userDataDir;
    QString m_lastError;

    QElapsedTimer m_fpsTimer;
    int m_framesSinceFpsSample = 0;
    double m_updatesPerSecond = 0.0;

    QTimer m_restartDebounce; // 分辨率变更防抖重启
};
} // namespace Nodes
