//
// Created by 吴斌 on 2024/1/18.
//

#include <QMessageBox>
#include "MainWindow.hpp"
#include "FramelessConfirmDialog.hpp"
#include "Common/AppConfig/ConstantDefines.h"
#include "Common/ProcessGuard/SpawnNoInherit.hpp"  // bInheritHandles=FALSE 派生（幽灵 socket 修复）
#include "Common/AppConfig/LicenseManager.h"
#include "Nodes/NodeEditorStyle.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "QFile"
#include"Widget/ConsoleWidget/LogWidget.hpp"
#include <QDropEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMimeData>
#include <QStatusBar> // statusBar()->showMessage（添加屏幕/属性写回提示）

#include <QWKWidgets/widgetwindowagent.h>
#include <widgetframe/windowbar.h>
#include <widgetframe/windowbutton.h>
// #include "Widget/PropertyWidget/PropertyWidget.hpp"
#include <QSettings>
#include <exception>
#include "../../Common/AppConfig/AutosaveManager.h"
#include "../../Common/AppConfig/ConfigManager.h"
#include "../../Common/AppConfig/ProjectPersistence.h"
#include "ProjectSnapshotBuilder.hpp"
#include <QMetaObject>
#include <QTimer>
// ---- MediaServer 编辑器组件（2026-11-12 第一步：UI 挂载）--------------------
#include "Common/TimeLineBase/BaseTimeLineWidget.h"  // TimeLineBase（原 ms_qttimeline）：时间轴视图 DLL
#include "../TimeLineWidget/QtTimelineModelAdapter.hpp"    // Scene↔帧模型适配器
#include "SceneCanvasWidget.hpp"         // 预览画布
#include "ScreenPropertyDialog.hpp"     // 屏幕属性对话框（2026-11-13 接通）
#include "SceneTreeWidget.hpp"           // 场景树
#include "PreviewRenderer.hpp"           // 本地预览渲染器
// ---- MediaServer 动作联动（2026-11-12 第二步）--------------------------------
#include "model/SceneModel.hpp"          // ms::Asset/AssetKind（钩子签名；
                                         // 纯结构无 C++20 依赖）
#include "standalone/SceneAssembler.hpp" // SceneAssembler::probeFile 静态
                                         // 探测（声明头纯 C++ 可用）
                                         // 【探测体在 MediaCore 侧】
                                         // FfmpegMediaSource.hpp 含
                                         // atomic<shared_ptr>（C++20），
                                         // 本 TU 为 C++17 不能 include——
                                         // 元数据探测统一走
                                         // SceneAssembler::probeFile 静态
                                         // 成员（C++20 TU 内编译）。
// ---- MediaServer 下发运行时（2026-11-12 第三步）----------------------------
#include "Widget/TimeLineWidget/MsRuntimeBridge.hpp"   // EditorRuntime 桥接壳（PIMPL：
                                        // EditorRuntime/ClockProvider 住
                                        // C++20 TU，本 TU 只见 Qt 类型）
// ---- MediaServer 时间轴 OSC 集成（2026-11-12 第四步）------------------------
#include "Common/Devices/StatusContainer/StatusContainer.h"  // 状态镜像单例
                                        // （GlobalEventBus 随其带入）
#include "Common/BaseClass/TimeLineDefines.h"  // usToFrames/framesToUs
                                        // （PAL 25fps 显示层换算）
#include <QDebug>
#include <QFileInfo>
using namespace ads;

// ============================================================================
// MediaServer 时间轴 OSC 地址（2026-11-12 第四步）
// 前缀与旧 TimeLineWidget 面板级 alias "timeline/master" 一致——旧面板已
// 退役，地址空间复用，外部控制脚本（OSC/WS 按地址交互）零迁移。
// ============================================================================
static QString msTimelineAddress(const QString& rel) {
    const QString norm = rel.startsWith('/') ? rel : ('/' + rel);
    return QStringLiteral("/timeline/master") + norm;
}

/**
 * @brief 启动 HTTP 服务器并在端口占用的短窗口期内重试
 * @param server HTTP 服务器对象
 * @param port 监听端口
 * @param waitMs 最大等待时间（毫秒）
 * @return true 启动成功；false 启动失败
 * 函数级注释：用于解决“重启切换项目后端口尚未完全释放”导致网页不可访问的问题。
 */
static bool startHttpServerWithRetry(Flow::NodeHttpServer* server, int port, int waitMs)
{
    if (!server) return false;
    const int step = 200;
    int elapsed = 0;
    while (elapsed <= waitMs) {
        if (server->start(port)) return true;
        QThread::msleep(static_cast<unsigned long>(step));
        elapsed += step;
    }
    return false;
}

/** @brief 按授权状态取应用图标：未授权用红色警示，授权后为品牌绿 */
static QIcon licensedAppIcon()
{
    return LicenseManager::isAuthorized()
        ? QIcon(QStringLiteral(":/icons/icons/Flow.png"))
        : QIcon(QStringLiteral(":/icons/icons/FlowRed.png"));
}

/** @brief 获取应用图标（优先 QApplication，回退到 Flow 资源） */
static QIcon applicationIcon()
{
    if (!QApplication::windowIcon().isNull()) {
        return QApplication::windowIcon();
    }
    return licensedAppIcon();
}

// ---------------------------------------------------------------------------
// MediaServer 素材导入（import_media 钩子体，2026-11-12 动作联动）
// 扩展名推断 + 轨道类型校验在本侧（纯 enum 比较，C++17 可用）；元数据
// 探测委托 MediaCore 的 SceneAssembler::probeFile 静态成员（FFmpeg 容器
// 头/音频静态探测/文本读取+测量——口径与 standalone 装配同源单一真相；
// 该头带 atomic<shared_ptr> C++20 特性，C++17 宿主 TU 不可直接 include）。
// \param kind 要求的素材种类（= 轨道类型约束；推断类型不符即拒绝）
// \param out 探测成功时填充（id 不在此生成——调用方 media-N 线性探测后入表）
// \return false = 失败并填 err
// ---------------------------------------------------------------------------
static bool msProbeMediaFile(const QString& file, ms::AssetKind kind,
                             ms::Asset& out, std::string& err)
{
    const QFileInfo fi(file);
    if (!fi.exists() || !fi.isFile()) {
        err = "文件不存在: " + file.toStdString();
        return false;
    }
    // 扩展名 → 素材种类（与 standalone SceneAssembler::kindFromExt 同表，
    // 另收 .txt → Text：媒体库文档分类的纯文本可作字幕/标题内容）
    const QString ext = fi.suffix().toLower();
    static const char* kVideo[] = {"mp4", "mov", "mkv", "avi",
                                   "webm", "m4v", "ts", "flv"};
    static const char* kImage[] = {"png", "jpg", "jpeg", "bmp",
                                   "gif", "webp"};
    static const char* kAudio[] = {"wav", "mp3", "aac", "flac",
                                   "ogg", "m4a", "wma"};
    ms::AssetKind ext_kind = ms::AssetKind::Unknown;
    for (const char* e : kVideo) if (ext == QLatin1String(e)) { ext_kind = ms::AssetKind::Video; break; }
    for (const char* e : kImage) if (ext == QLatin1String(e)) { ext_kind = ms::AssetKind::Image; break; }
    for (const char* e : kAudio) if (ext == QLatin1String(e)) { ext_kind = ms::AssetKind::Audio; break; }
    if (ext == QLatin1String("txt")) ext_kind = ms::AssetKind::Text;
    if (ext_kind == ms::AssetKind::Unknown) {
        err = "不识别的文件类型: ." + ext.toStdString();
        return false;
    }
    if (ext_kind != kind) { // 类型校验（钩子语义：须匹配轨道 kind）
        auto name = [](ms::AssetKind k) -> const char* {
            switch (k) {
                case ms::AssetKind::Video:  return "video";
                case ms::AssetKind::Image:  return "image";
                case ms::AssetKind::Text:   return "text";
                case ms::AssetKind::Audio:  return "audio";
                case ms::AssetKind::Live:   return "live";
                default:                    return "unknown";
            }
        };
        err = std::string("文件类型与轨道不匹配: .") + ext.toStdString()
              + "（期望 " + name(kind) + "）";
        return false;
    }

    // 元数据探测（MediaCore C++20 侧；path=绝对路径、尺寸/时长/文本测量）
    return ms::standalone::SceneAssembler::probeFile(
        fi.absoluteFilePath().toStdString(), ext_kind, out, err);
}

/**
 * @brief 手动模拟控件的鼠标离开事件，清除 hover 高亮状态
 *
 * 背景：点击标题栏「最大化/还原」按钮时，会先触发 QPushButton::clicked，再改变窗口
 *       状态（showMaximized / showNormal）。Qt 在此场景下有时不会向按钮发送
 *       QEvent::Leave / QHoverEvent::HoverLeave，导致按钮仍显示 :hover 样式，
 *       直到用户再次移动鼠标。该问题在无边框自定义标题栏中较常见。
 *
 * 做法：在窗口状态切换后的下一事件循环（singleShot(0)）检查光标是否已不在控件
 *       区域内；若是，则向控件投递 Leave 与 HoverLeave 事件，强制结束 hover 态。
 *
 * @param widget 需要清除 hover 状态的控件（当前用于最大化按钮 maxButton）
 */
static void emulateLeaveEvent(QWidget *widget)
{
    if (!widget) {
        return;
    }
    // 延迟到下一事件循环：等窗口几何/状态更新完成后再判断光标位置
    QTimer::singleShot(0, widget, [widget]() {
        const QScreen *screen = widget->screen();
        const QPoint globalPos = QCursor::pos(screen);
        // 光标已离开控件区域时，才需要补发离开事件
        if (!QRect(widget->mapToGlobal(QPoint{0, 0}), widget->size()).contains(globalPos)) {
            QCoreApplication::postEvent(widget, new QEvent(QEvent::Leave));
            if (widget->testAttribute(Qt::WA_Hover)) {
                const QPoint scenePos = widget->window()->mapFromGlobal(globalPos);
                static constexpr const auto oldPos = QPoint{};
                const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
                const auto event =
                    new QHoverEvent(QEvent::HoverLeave, scenePos, globalPos, oldPos, modifiers);
                QCoreApplication::postEvent(widget, event);
            }
        }
    });
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    // QWindowKit：尽早 setup，以便接管无边框窗口的尺寸与原生行为
    windowAgent = new QWK::WidgetWindowAgent(this);
    windowAgent->setup(this);
    setWindowTitle(AppConstants::PRODUCT_NAME);
    // 同步应用/窗口图标，供标题栏左上角与任务栏使用
    const QIcon appIcon = applicationIcon();
    QApplication::setWindowIcon(appIcon);
    setWindowIcon(appIcon);

    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    //    聚焦高亮
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, true);
    //    每个区域都有独立的关闭按钮
    ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, false);
    //
    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);

    ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);

    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasTabsMenuButton, true);
}

void MainWindow::refreshAppIcon()
{
    // 授权状态变化（设置页导入/清除激活）后调用：任务栏、窗口、标题栏图标同步换色
    const QIcon appIcon = licensedAppIcon();
    QApplication::setWindowIcon(appIcon);
    setWindowIcon(appIcon);
    if (windowBar) {
        if (auto *iconBtn = qobject_cast<QWK::WindowButton *>(windowBar->iconButton())) {
            iconBtn->setIconNormal(appIcon);
        }
    }
}

MainWindow::~MainWindow()
{
    if (httpServer) {
        if (httpServer->running()) {
            httpServer->stop();
        }
        delete httpServer;
        httpServer = nullptr;
    }
    delete controller;
    // 下发运行时先于预览/场景回收：桥析构内部 stop EditorRuntime
    // （停 recv/看门狗/发送线程），避免半析构期仍在下发/回调。
    delete msRuntime_;
    msRuntime_ = nullptr;
    // MediaServer 组件回收（dock widget 经 Qt 父子链随 m_DockManager 销毁；
    // preview_ 与 scene_ 是裸借用宿主——须在 dock children 之前删除，
    // SceneCanvasWidget 析构不反查这两个指针，顺序安全）
    delete msPreview_;
    msPreview_ = nullptr;
    delete msScene_;
    msScene_ = nullptr;
}

void MainWindow::init()
{
    m_DockManager = new ads::CDockManager(this);
    // 注册到 DockHub，供外部模块统一挂载
    //DockHub::instance().setDockManager(m_DockManager);

    m_DockManager->setDockWidgetToolBarStyle(Qt::ToolButtonStyle::ToolButtonIconOnly,CDockWidget::eState::StateDocked);
    this->setCentralWidget(m_DockManager);
    emit initStatus("Initialization ADS success");
     // 菜单条控件
    menuBar=new MenuBarWidget(this);
    connect(menuBar, &MenuBarWidget::recentFileTriggered,
             this, &MainWindow::openRecentFile);
    // 设置对话框关闭后检查授权状态变化，刷新应用图标（未授权红色/授权绿色）
    connect(menuBar, &MenuBarWidget::licenseStateChanged,
            this, &MainWindow::refreshAppIcon);
     // 初始化时刷新最近文件菜单
    menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());
    setupFramelessWindow();
    emit initStatus("Initialization MenuBar success");
    // 首先实例化终端显示控件，保证日志输出正常
    auto *logDockViewer= m_DockManager->createDockWidget("终端显示");
    logTable=new LogWidget();
    log=new LogHandler(logTable);
    logDockViewer->setWidget(logTable);
    logDockViewer->setIcon(QIcon(":/icons/icons/bug.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, logDockViewer);
    //添加菜单项
    QMenu* OptionsMenu = makeOptionsMenu(logTable, logTable->getActions());
    logDockViewer->setTitleBarActions({OptionsMenu->menuAction()});
    emit initStatus("Initialization console widget");
   
    // 按配置应用主题（重启后生效）
    applyTheme(ConfigManager::instance().isDefaultDarkTheme());
    // 节点编辑：由 DataflowViewsManger 创建单画布 Dock
    emit initStatus("load node editor style success");
    dataflowViewsManger=new DataflowViewsManger(m_DockManager,this);
    // 插件需先注册再创建根图（否则 Container/内置节点不在 registry）
    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization pluginsManager success");
    dataflowViewsManger->resetDataflow(QStringLiteral("dataflow"));
    emit initStatus("Init node editor success");
    // 节点库控件
    // nodeDockLibraryWidget = m_DockManager->createDockWidget("节点库");
    // nodeDockLibraryWidget->setIcon(QIcon(":/icons/icons/library.png"));
    // m_DockManager->addDockWidget(ads::BottomDockWidgetArea, nodeDockLibraryWidget);
    // emit initStatus("Initialization nodeLibrary success");
    // ---- MediaServer 编辑器组件（2026-11-12 第二步：UI 挂载 + 动作联动）----
    // 场景树 / 预览画布 / 时间轴三 dock 挂入 ADS；装配序列与 media_editor
    // 的 EditorWindow 同构（attachWorkScene → hooks 注入 → loadSceneToModel
    // → 控件）。本步接入动作联动（原先仅 UI 挂载）：
    //   * import_media 钩子：片段属性面板媒体选择/时间轴拖入的素材导入
    //     落点（探测 + 入表 + 清解码缓存）——未注入时 makeClipPropertyPanel
    //     返回 nullptr，双击片段无自定义面板（此前属性面板不出现的根因）；
    //   * 三信号：seekRequestedUs→预览 seekTo；sceneEdited→场景树重建 +
    //     预览失效；mediaImportFailed→日志可见提示；
    //   * 播放联动：工具栏 play/pause/stop → PreviewRenderer 状态机
    //     （本地独立时钟，语义对齐原 EditorRuntime::cmdPlay）+ 33ms tick
    //     驱动播放头回显时间轴/场景树活跃标记（画布内部 pull 泵自动跟进）。
    msScene_ = new ms::Scene();                            // 默认空场景
    // 【2026-11-13 修复】Scene 默认构造 canvas_w/h=0（SceneModel.hpp），而
    // 画布全链（wheelEvent 缩放 / resetView fit / paint 画布矩形）都以
    // "canvas>0" 为有效性闸——0 画布 = 滚轮缩放无反应（wheelEvent 早退
    // 穿透）+ 视图无界。注释原称"画布默认 1920×1080"但从未赋值，补上。
    msScene_->canvas_w = 1920.0;
    msScene_->canvas_h = 1080.0;
    // 【交付模式 GpuDeliver（2026-11-12 视频测试实证修复）】预览画布 3D 视口
    // 只从 GpuDeliver 逐项交付拉取贴图（SceneCanvasWidget paintGL view3d_
    // 分支），SoftCompose 模式下 items 恒空 → 片段永远棋盘格/描边占位、
    // 视频纹理永不显示。3D 视口 setPreviewRenderer 即翻 gpu_delivery_ok=false
    //（CUDA 0 拷贝不进 QPainter），解码端自动回退下载路径——GpuDeliver 在
    // 此处纯粹作为"逐项交付"语义使用，硬件协商机制照常自愈。
    msPreview_ = new ms::PreviewRenderer(
        std::string(), {},                                 // 空素材根：素材按绝对路径寻址
        ms::PreviewRenderer::DeliverMode::GpuDeliver);
    msTimelineAdapter_ = new ms::QtTimelineModelAdapter(this);
    msTimelineAdapter_->attachWorkScene(msScene_);
    // 素材导入钩子（构造后、首载前注入一次）：元数据探测在纯函数
    // msProbeMediaFile；此处补 id 生成（media-N 线性探测，live-N/text-N/
    // clip-N 同款惯例）+ 入表 + 清解码缓存（素材表变化）。
    {
        ms::ClipPanelHooks hooks; // 命名空间级结构（非适配器嵌套类型）
        hooks.import_media =
            [this](const QString& file, ms::AssetKind kind,
                   std::string& err) -> ms::Asset {
            ms::Asset a;
            if (!msProbeMediaFile(file, kind, a, err)) return ms::Asset{};
            int  next  = 1;
            bool clash = true;
            while (clash) {
                clash = false;
                a.id = "media-" + std::to_string(next++);
                for (const auto& x : msScene_->assets)
                    if (x.id == a.id) { clash = true; break; }
            }
            msScene_->assets.push_back(a);
            if (msPreview_) msPreview_->clearMediaCache();
            std::fprintf(stderr,
                         "[ms-main] 导入素材 %s ← %s（时长 %lld us %dx%d）\n",
                         a.id.c_str(), file.toUtf8().constData(),
                         (long long)a.duration_us, a.width, a.height);
            std::fflush(stderr);
            return a;
        };
        msTimelineAdapter_->setClipPanelHooks(std::move(hooks));
    }
    msTimelineAdapter_->loadSceneToModel(*msScene_);
    msTimeline_ = new BaseTimelineWidget(msTimelineAdapter_, this);
    msCanvas_ = new ms::SceneCanvasWidget(this);
    msCanvas_->setScene(msScene_);
    msCanvas_->setPreviewRenderer(msPreview_);             // 所见即所得（占位画法兜底）
    msPreview_->setScene(msScene_);
    // 【解码线程化配套】worker 出帧回调（工作线程调用！）——只排队重绘事件
    msPreview_->setFrameReadyCallback([this] {
        if (msCanvas_)
            QMetaObject::invokeMethod(msCanvas_,
                                      qOverload<>(&QWidget::update),
                                      Qt::QueuedConnection);
    });
    msSceneTree_ = new ms::SceneTreeWidget(this);
    msSceneTree_->rebuild(msScene_);
    // 预览画布 dock（右）
    auto *previewDockWidget = m_DockManager->createDockWidget(QStringLiteral("预览画布"));
    previewDockWidget->setObjectName(QStringLiteral("ms_preview"));
    previewDockWidget->setIcon(QIcon(":/icons/icons/timeline.png"));
    previewDockWidget->setWidget(msCanvas_);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, previewDockWidget);
    // 场景树 dock（左）
    auto *sceneTreeDockWidget = m_DockManager->createDockWidget(QStringLiteral("场景树"));
    sceneTreeDockWidget->setObjectName(QStringLiteral("ms_scene_tree"));
    sceneTreeDockWidget->setIcon(QIcon(":/icons/icons/timeline.png"));
    sceneTreeDockWidget->setWidget(msSceneTree_);
    m_DockManager->addDockWidget(ads::LeftDockWidgetArea, sceneTreeDockWidget);
    // 时间轴 dock（底部；objectName 沿用 "timeline"）
    auto *timelineDockWidget = m_DockManager->createDockWidget(QStringLiteral("时间轴"));
    timelineDockWidget->setObjectName(QStringLiteral("timeline"));
    timelineDockWidget->setIcon(QIcon(":/icons/icons/timeline.png"));
    timelineDockWidget->setWidget(msTimeline_);
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, timelineDockWidget);

    // ---- 动作联动接线（2026-11-12 第二步；收口原"第一步无动作联动"缺口）----
    // 视图拖播放头（适配器 80ms 节流后）→ 会话内跳转广播（第三步；Idle 时
    // 仅记录定位点）+ 预览 seekTo；画布内部 pull 泵检测播放头变化自动提交
    // 快照/重绘，此处无须手动刷新画布。
    connect(msTimelineAdapter_, &ms::QtTimelineModelAdapter::seekRequestedUs,
            this, [this](std::int64_t us) {
                if (msRuntime_ && msRuntime_->running())
                    msRuntime_->cmdSeekToUs(us);
                if (msPreview_) msPreview_->seekTo(us);
            });
    // 时间轴有本地修改（拖拽/面板写回/增删片段轨道）→ 场景树全量重建
    //（展开态/滚动位置树内部保持）+ 预览内容失效进下一帧提交。
    connect(msTimelineAdapter_, &ms::QtTimelineModelAdapter::sceneEdited,
            this, [this] {
                if (msSceneTree_) msSceneTree_->rebuild(msScene_);
                if (msPreview_)   msPreview_->invalidate();
                requestMsAutoPublish();
            });
    // 拖入导入失败 → 日志表可见提示（qWarning 经 LogHandler 进终端 dock）；
    // [E1] 语义：适配器已保证本次拖入不留 1 帧空占位，此处仅提示。
    connect(msTimelineAdapter_, &ms::QtTimelineModelAdapter::mediaImportFailed,
            this, [](const QString& file, const QString& reason) {
                qWarning() << "[MediaServer] 素材导入失败:"
                           << QFileInfo(file).fileName() << "-" << reason;
            });
    // ---- 播放联动：时间轴工具栏 → PreviewRenderer 状态机 + tick 驱动回显 ----
    // 33ms tick 定时器仅播放期运行（play 启动 / pause·stop 停止，非播放期
    // 零开销）；tick 推进播放头后：回显时间轴（setPlayheadFromRuntimeUs
    // 静态调基类，绕开 onSetPlayheadPos 虚派发不发 seek——防回环）+ 场景树
    // 活跃片段标记（▶ 前缀，幂等）。画布不出场（pull 泵自动跟进）。
    // 【2026-11-12 第四步】tick 定时器成员化（msPlayTimer_）+ 播放三动作
    // 提取为方法（msHandlePlay/Pause/Stop）——工具栏点击与外部 OSC 命令
    //（GlobalEventBus /timeline/master/*）同源复用；tick 内发布播放头帧号
    // 到 /currentFrame（StatusContainer 镜像 → OSC/MQTT/WS 出站反馈）。
    msPlayTimer_ = new QTimer(this);
    msPlayTimer_->setTimerType(Qt::PreciseTimer);
    connect(msPlayTimer_, &QTimer::timeout, this, [this] {
        // 播放权威时钟（第三步升级）：桥会话播放中 → 以对齐时钟为唯一
        // 权威（预览 seekTo 跟随，下发画面与本地预览同拍）；否则退回本地
        // PreviewRenderer 状态机自驱 tick（第二步行为，未配显示端时主路径）。
        std::int64_t us = -1;
        if (msRuntime_ && msRuntime_->running() && msRuntime_->playing()) {
            us = msRuntime_->sessionTimeUs();
            if (us < 0) return;
            if (msPreview_) msPreview_->seekTo(us); // 预览跟随会话播放头
        } else {
            if (!msPreview_ || !msPreview_->playing()) return;
            if (!msPreview_->tick()) return; // 非播放态/播放头未推进
            us = msPreview_->playheadUs();
        }
        msTimelineAdapter_->setPlayheadFromRuntimeUs(us);
        if (msSceneTree_) msSceneTree_->markPlayhead(us);
        // 播放头帧号反馈（PAL 25fps 四舍五入；旧 TimeLineToolBar::
        // publishCurrentFrame 同语义——StatusContainer 镜像后经
        // statusUpdated 出站到 OSC/MQTT/WS 反馈端口）
        if (msTimelineAdapter_) {
            GlobalEventBus::instance()->publishState(
                msTimelineAddress(QStringLiteral("/currentFrame")),
                static_cast<int>(QtTimeline::usToFrames(us)));
        }
    });
    connect(msTimeline_->toolbar, &DefaultTimeLineToolBar::playClicked, this,
            [this] { msHandlePlay(); });
    connect(msTimeline_->toolbar, &DefaultTimeLineToolBar::pauseClicked, this,
            [this] { msHandlePause(); });
    connect(msTimeline_->toolbar, &DefaultTimeLineToolBar::stopClicked, this,
            [this] { msHandleStop(); });
    // 【第四步】OSC 地址树绑定：/timeline/master/{play,pause,stop,
    // currentFrame} 订阅命令 + 播放控件注册 + 初始状态推送（详见方法注释）。
    msBindTimelineOsc();

    // ---- 下发到显示端（2026-11-12 第三步：EditorRuntime 桥接壳）----
    // 桥内部启动 EditorRuntime（UDP 心跳 17001 / 素材 HTTP 分发 17002 /
    // 显示端连接看门狗 17100）。素材根落在 Documents/Flow/Cache/MsAssets
    //（素材 CRC 缓存同址）；素材表以绝对路径直通（编辑器侧无 assets_root
    // 语义，EditorRuntime::assetFullPath 已兼容）。启动失败不阻断 GUI——
    // 仅无下发能力（qWarning 留痕），本地预览/时间轴编辑照常。
    msRuntime_ = new ms::MsRuntimeBridge(this);
    if (!msRuntime_->start()) {
        qWarning() << "[MediaServer] 下发运行时启动失败（端口占用？）——"
                      "本地编辑可用，显示端下发不可用";
    }
    // ---- 场景自动追平（2026-11-13；收编恢复链路）------------------------
    // sceneEdited（时间轴/画布/属性面板三路）→ requestMsAutoPublish()
    // （200ms 节流）→ 自动 syncModelToWorkScene + publishScene 全量广播。
    // 修复"每次动作都要手动下发场景才生效"——display 之前冻结在最后一次
    // 手动下发的快照，编辑脱节后播放头落在旧 clip 域外即黑屏（2026-11-13
    // display 有声无画排查结论）。播放中下发不打断播放（display 换场景
    // 不重置会话时钟，实测）。
    // 【2026-11-13 节流化（用户定案）】防抖"停手才发"体感不连续——改为
    // 节流：编辑后每隔 200ms 发一拍（首发立即、挂起拍兜底），编辑过程
    // display 持续跟手。display 渲染端实测 50Hz 发布满速合成、发送线程
    // 零节流——200ms 速率零压力。
    // 【2026-11-13 节流化（用户定案）】防抖"停手才发"体感不连续——改为
    // 节流：编辑后每隔 N ms 发一拍（首发立即、挂起拍兜底），编辑过程
    // display 持续跟手。display 渲染端实测 50Hz 发布满速合成、发送线程
    // 零节流——200ms 速率零压力。间隔可配：Settings.ini
    // [Network] SceneAutoPublishMs（默认 200，钳制 20-2000，重启生效）。
    msAutoPublishIntervalMs_ = ConfigManager::instance().getSceneAutoPublishMs();
    msAutoPublishTimer_ = new QTimer(this);
    msAutoPublishTimer_->setSingleShot(true);
    msAutoPublishTimer_->setInterval(msAutoPublishIntervalMs_);
    connect(msAutoPublishTimer_, &QTimer::timeout, this, [this] {
        publishMsSceneNow();
    });
    // 场景树右键动作 → 桥命令（只入队，GUI 零阻塞）
    // 下发场景：模型→工作场景兜底同步（防面板改动未写回）后全量广播。
    connect(msSceneTree_, &ms::SceneTreeWidget::publishSceneRequested, this,
            [this] {
                if (!msRuntime_ || !msRuntime_->running() || !msScene_) return;
                msTimelineAdapter_->syncModelToWorkScene();
                msRuntime_->publishScene(*msScene_);
            });
    // 同步媒体：注册素材 + 推清单（在线端比对拉取，幂等）。
    // 【2026-11-13 补反馈】对齐合并前 EditorWindow::onSyncMedia——此前接线
    // 静默执行（display 离线时清单未发出也零提示），用户观感"同步无效"。
    // 在线数口径 = connected && enabled（与清单广播的目标过滤同源）。
    connect(msSceneTree_, &ms::SceneTreeWidget::syncMediaRequested, this,
            [this] {
                if (!msScene_) return;
                if (!msRuntime_ || !msRuntime_->running()) {
                    statusBar()->showMessage(
                        QStringLiteral("同步媒体：下发运行时未启动（端口占用？）"
                                       "——素材无法分发"),
                        5000);
                    return;
                }
                msTimelineAdapter_->syncModelToWorkScene();
                const std::size_t n = msRuntime_->syncMedia(*msScene_);
                std::size_t online = 0;
                const auto rows = msRuntime_->displayRows();
                for (const auto& r : rows)
                    if (r.connected && r.enabled) ++online;
                statusBar()->showMessage(
                    online == 0
                        ? QStringLiteral("已注册 %1 个素材——当前无在线显示端，"
                                         "清单未送达（显示端上线后请再次同步）")
                              .arg(static_cast<int>(n))
                        : (n == 0
                               ? QStringLiteral("已推送素材清单到 %1 个在线显示端"
                                                "（无新增文件，全部已就位）")
                                     .arg(static_cast<int>(online))
                               : QStringLiteral("已注册 %1 个素材并推送清单到 %2 "
                                                "个在线显示端（缺失文件由显示"
                                                "端自动拉取）")
                                     .arg(static_cast<int>(n))
                                     .arg(static_cast<int>(online))),
                    5000);
            });
    // 重载场景：显示端权威快照（最后一次下发）→ 写回工作副本 → 全链
    // 刷新（时间轴模型重建/场景树重建/预览失效）。无发布记录 = 忽略。
    connect(msSceneTree_, &ms::SceneTreeWidget::reloadSceneRequested, this,
            [this] {
                if (!msRuntime_ || !msScene_) return;
                ms::Scene snap;
                if (!msRuntime_->authoritySnapshot(snap)) return;
                *msScene_ = std::move(snap);
                // 【2026-11-13】canvas 钳制（口径同工程回灌）：0 画布会让
                // 画布全链（滚轮缩放/fit/绘制）失效。
                if (msScene_->canvas_w <= 0.0) msScene_->canvas_w = 1920.0;
                if (msScene_->canvas_h <= 0.0) msScene_->canvas_h = 1080.0;
                msTimelineAdapter_->loadSceneToModel(*msScene_);
                msSceneTree_->rebuild(msScene_);
                msPreview_->invalidate();
            });
    // 视图复位：场景树"适配窗口"动作 → 画布 resetView。
    connect(msSceneTree_, &ms::SceneTreeWidget::fitViewRequested, this, [this] {
        if (msCanvas_) msCanvas_->resetView();
    });
    // 【单入口约定（对齐合并前 EditorWindow）】添加显示端的独立入口已退役
    //（2026-11-13 用户要求场景树不提供添加 display 功能）——屏幕与显示端
    // 一一对应，画布右键「添加屏幕」内自动连带登记本机显示端
    //（display_id = screen.player_id）；改绑走屏幕属性对话框「绑定」页
    //（UDP 扫描选设备 + 手填兜底）。
    // 连接/断开 = 槽位启停（停用立即断开；启用交看门狗重连）。
    connect(msSceneTree_, &ms::SceneTreeWidget::displayConnectRequested, this,
            [this](const QString& display_id, bool connect_) {
                if (msRuntime_)
                    msRuntime_->setDisplayEnabled(display_id, connect_);
            });
    // ---- 画布信号接线（2026-11-13 修复）--------------------------------------
    // 收编时原 EditorWindow 随迁丢失，msCanvas_ 全部信号悬空：
    //   * 右键"添加屏幕"零反应（screenAddRequested 无人接）——屏幕/场景树
    //     均不出现新银幕；
    //   * 拖拽/键盘编辑屏幕已直接写入工作副本，但场景树/预览不刷新
    //    （sceneEdited 无人接）。
    // 此处补齐屏幕管理最小闭环（添加/删除/编辑落点刷新）；双击属性面板
    //（ScreenPropertyDialog 已删，ScreenWarpPanel 存留）与画布删片段链路
    //（需经时间轴适配器同步）后续单独立项。
    // 添加屏幕（画布右键空白 = 单入口；语义对齐合并前 EditorWindow::onAddScreen）：
    // id = "scr-N" 线性探测（防删屏后重名）；默认几何 960×540 按屏序阶梯
    // 排布（避免完全重叠）；player_id = "player-N" 与屏幕编号对齐
    //（scr-1 ↔ player-1）并【自动连带登记】本机显示端（单入口约定；
    // 已登记同名则沿用——幂等，删屏重加不产生重复登记）。改绑/远程显示端
    // 走屏幕属性对话框「绑定」页。
    connect(msCanvas_, &ms::SceneCanvasWidget::screenAddRequested, this, [this] {
        if (!msScene_) return;
        ms::Screen scr;
        // id 自动化：scr-N 线性探测（防删屏后重名；对齐原版命名）
        int n = static_cast<int>(msScene_->screens.size()) + 1;
        auto idClash = [this](const std::string& v) {
            for (const auto& s : msScene_->screens)
                if (s.id == v) return true;
            return false;
        };
        scr.id = "scr-" + std::to_string(n);
        while (idClash(scr.id)) scr.id = "scr-" + std::to_string(++n);
        // 默认几何：960×540，阶梯排布（避免完全重叠）
        scr.x = 60.0 * ((n - 1) % 4) + 20.0;
        scr.y = 60.0 * (((n - 1) / 4) % 4) + 20.0;
        scr.w = 960;
        scr.h = 540;
        // 显示端自动连带登记：player-N（编号与屏幕对齐）；已登记同名沿用
        //（幂等——addDisplay 返回 false 属正常态，静默忽略）
        scr.player_id = "player-" + std::to_string(n);
        if (msRuntime_ && msRuntime_->running())
            msRuntime_->addDisplay(QString::fromStdString(scr.player_id),
                                   QStringLiteral("127.0.0.1"), 17100);

        msScene_->screens.push_back(scr);
        if (msSceneTree_) msSceneTree_->rebuild(msScene_);
        if (msPreview_)   msPreview_->invalidate();
        if (msCanvas_) {
            msCanvas_->update();
            msCanvas_->highlight(1, static_cast<int>(msScene_->screens.size()) - 1);
        }
        statusBar()->showMessage(
            QStringLiteral("屏幕 %1 已加入（显示端 %2 已登记 @127.0.0.1:17100）——"
                           "显示端程序以 --id=%2 启动后自动接入")
                .arg(QString::fromStdString(scr.id),
                     QString::fromStdString(scr.player_id)),
            8000);
        std::fprintf(stderr, "[ms-main] 添加屏幕 %s（player=%s 960x540@%.0f,%.0f）\n",
                     scr.id.c_str(), scr.player_id.c_str(), scr.x, scr.y);
        std::fflush(stderr);
    });
    // 删除屏幕（画布右键菜单）：从工作副本移除 + 全链刷新。显示端登记不
    // 连带删除（登记无害可复用，管理入口在场景树右键）。
    connect(msCanvas_, &ms::SceneCanvasWidget::screenDeleteRequested, this,
            [this](int screen_idx) {
                if (!msScene_ || screen_idx < 0 ||
                    screen_idx >= static_cast<int>(msScene_->screens.size()))
                    return;
                const std::string id = msScene_->screens[screen_idx].id;
                msScene_->screens.erase(msScene_->screens.begin() + screen_idx);
                if (msSceneTree_) msSceneTree_->rebuild(msScene_);
                if (msPreview_)   msPreview_->invalidate();
                if (msCanvas_)    msCanvas_->update();
                std::fprintf(stderr, "[ms-main] 删除屏幕 %s\n", id.c_str());
                std::fflush(stderr);
            });
    // 画布编辑落点（拖拽移动/旋转/缩放/键盘微调已直接写入工作副本）→
    // 场景树重建 + 预览失效（对齐适配器 sceneEdited 联动；画布自绘由
    // 内部 update 覆盖）。
    connect(msCanvas_, &ms::SceneCanvasWidget::sceneEdited, this, [this] {
        if (msSceneTree_) msSceneTree_->rebuild(msScene_);
        if (msPreview_)   msPreview_->invalidate();
        requestMsAutoPublish();
    });
    // 【live 编辑→display 节流下发 2026-11-13】拖动/旋转/缩放会话中
    // mouse-move 频率发出（工作副本已写）——节流 200ms 一拍，编辑过程
    // display 持续跟手（此前 live 中零信号，松手 sceneEdited 才下发 =
    // "停手才动"的不连续感）。场景树/预览失效不由本信号承担（画布内部
    // update + preview invalidate 已在会话中自理；松手 sceneEdited 走
    // 场景树重建全链）。
    connect(msCanvas_, &ms::SceneCanvasWidget::sceneEditedLive, this,
            [this] { requestMsAutoPublish(); });
    // ---- 屏幕属性面板（2026-11-13 收编恢复；用户要求以合并前为准）------
    // 两路入口汇聚同一路径（语义对齐合并前 EditorWindow::onEditScreen）：
    //   * 画布 screenDoubleClicked(idx)（双击屏幕 / 右键「屏幕属性…」）；
    //   * 场景树 propertiesRequested(kind,a,b)（屏幕行双击/右键，kind=1、
    //     a=screen_idx；kind=clip 按用户定案不弹——属性面板唯一入口 =
    //     timeline 双击，此处只接屏幕）。
    // 对话框 = 合并前原版：模态 exec + WATCHOUT 式三页（通用/几何结构/
    // 绑定）；「绑定」页 UDP 扫描选设备 + 手填兜底（场景树不再提供添加
    // 显示端入口——单入口约定，见上）。实时预览两条链（warp + 几何六字段）
    // 编辑中即时写工作副本并刷新视图，取消 = 还原打开时备份；OK = applyTo
    // + 显示端登记同步（改绑注销旧条目；同 id 地址/配对码变化重登记）。
    auto refreshMsViews = [this] {
        if (msSceneTree_) msSceneTree_->rebuild(msScene_);
        if (msPreview_)   msPreview_->invalidate();
        if (msCanvas_)    msCanvas_->update();
        requestMsAutoPublish();
    };
    auto openScreenProperty = [this, refreshMsViews](int screen_idx) {
        if (!msScene_ || screen_idx < 0 ||
            screen_idx >= static_cast<int>(msScene_->screens.size()))
            return;
        ms::Screen& scr = msScene_->screens[screen_idx];
        // 当前显示端登记（属性对话框绑定区预填；未登记给本机默认地址）
        std::string cur_host   = "127.0.0.1";
        std::uint16_t cur_port = 17100;
        std::string cur_pairing;
        if (msRuntime_) {
            const auto rows = msRuntime_->displayRows();
            for (const auto& r : rows) {
                if (r.id.toStdString() == scr.player_id) {
                    cur_host    = r.host.toStdString();
                    cur_port    = static_cast<std::uint16_t>(r.port);
                    cur_pairing = r.pairingCode.toStdString();
                    break;
                }
            }
        }
        const std::string old_pid = scr.player_id;
        // warp 取消还原基准（实时预览：编辑中会先写工作副本）
        const ms::ScreenWarp warp_backup = scr.warp;
        // 几何取消还原基准（同 warp 语义——取消时还原六字段）
        const double geo_backup[6] = {
            scr.x, scr.y, scr.w, scr.h, scr.rotation,
            static_cast<double>(scr.z)};

        ms::ScreenPropertyDialog dlg(scr, cur_host, cur_port, cur_pairing,
                                     this);
        dlg.setWindowTitle(QStringLiteral("屏幕属性：%1")
                               .arg(QString::fromStdString(scr.id)));
        const std::string sid = scr.id;
        // 【实时预览·变形】编辑中每次变化 → 写工作副本 + 视图刷新
        dlg.setWarpLiveApply([this, sid, refreshMsViews](
                                 const ms::ScreenWarp& w) {
            for (auto& s : msScene_->screens)
                if (s.id == sid) { s.warp = w; break; }
            refreshMsViews();
        });
        // 【实时预览·几何六字段】同链——编辑器画布即时可见
        dlg.setGeometryLiveApply([this, sid, refreshMsViews](
                                     const ms::Screen& g) {
            for (auto& s : msScene_->screens)
                if (s.id == sid) {
                    s.x = g.x; s.y = g.y; s.w = g.w; s.h = g.h;
                    s.rotation = g.rotation; s.z = g.z;
                    break;
                }
            refreshMsViews();
        });
        if (dlg.exec() != QDialog::Accepted) {
            // 取消 = 还原打开时 warp + 几何六字段（两条实时链路可能已把
            // 中间态写进工作副本；未动过时为幂等写）。绑定字段只在 OK 时
            // applyTo 写回，无需还原。
            for (auto& s : msScene_->screens) {
                if (s.id != sid) continue;
                const bool warp_changed =
                    s.warp.mode != warp_backup.mode ||
                    s.warp.cols != warp_backup.cols ||
                    s.warp.rows != warp_backup.rows ||
                    s.warp.pts  != warp_backup.pts;
                const bool geo_changed =
                    s.x != geo_backup[0] || s.y != geo_backup[1] ||
                    s.w != geo_backup[2] || s.h != geo_backup[3] ||
                    s.rotation != geo_backup[4] ||
                    s.z != static_cast<int>(geo_backup[5]);
                if (warp_changed) s.warp = warp_backup;
                if (geo_changed) {
                    s.x = geo_backup[0]; s.y = geo_backup[1];
                    s.w = geo_backup[2]; s.h = geo_backup[3];
                    s.rotation = geo_backup[4];
                    s.z = static_cast<int>(geo_backup[5]);
                }
                if (warp_changed || geo_changed) refreshMsViews();
                break;
            }
            return;
        }
        dlg.applyTo(scr);

        // ---- 显示端登记同步（合并前语义）--------------------------------
        // 绑定页可改 player_id + host + port + 配对码：改绑注销旧条目；
        // 同 id 登记三要素任一变化（含配对码）触发重登记（先 remove 再
        // add，看门狗按新地址重连）；无变化不动（避免播放中连接闪断）。
        const std::string new_pid     = scr.player_id;
        const std::string new_host    = dlg.displayHost();
        const std::uint16_t new_port  = dlg.displayPort();
        const std::string new_pairing = dlg.displayPairingCode();
        if (msRuntime_ && msRuntime_->running()) {
            if (!old_pid.empty() && old_pid != new_pid)
                msRuntime_->removeDisplay(
                    QString::fromStdString(old_pid)); // 幂等：未登记忽略
            if (!new_pid.empty()) {
                bool same = false;
                const auto rows = msRuntime_->displayRows();
                for (const auto& r : rows) {
                    if (r.id.toStdString() == new_pid) {
                        same = r.host.toStdString() == new_host &&
                               r.port == static_cast<int>(new_port) &&
                               r.pairingCode.toStdString() == new_pairing;
                        break;
                    }
                }
                if (!same) {
                    msRuntime_->removeDisplay(
                        QString::fromStdString(new_pid));
                    msRuntime_->addDisplay(
                        QString::fromStdString(new_pid),
                        QString::fromStdString(new_host),
                        static_cast<int>(new_port),
                        QString::fromStdString(new_pairing));
                }
            }
        }
        refreshMsViews();
        statusBar()->showMessage(
            QStringLiteral("屏幕 %1 已修改（绑定 %2@%3:%4；场景树右键"
                           "「下发场景」后生效）")
                .arg(QString::fromStdString(scr.id),
                     QString::fromStdString(new_pid),
                     QString::fromStdString(new_host))
                .arg(new_port),
            4000);
    };
    connect(msCanvas_, &ms::SceneCanvasWidget::screenDoubleClicked, this,
            openScreenProperty);
    connect(msSceneTree_, &ms::SceneTreeWidget::propertiesRequested, this,
            [openScreenProperty](int kind, int a, int b) {
                if (kind == 1) openScreenProperty(a); // 1=screen（b 未用）
            });
    // 200ms UI 拍：显示端在线状态 + 素材同步进度 → 场景树轻量刷新
    //（refreshDisplayStatuses/updateSyncProgress 幂等：状态不变零重绘；
    // 编辑触发的 rebuild 未带状态表，本拍 200ms 内补回标注）。
    auto* msUiTimer = new QTimer(this);
    msUiTimer->setTimerType(Qt::CoarseTimer);
    connect(msUiTimer, &QTimer::timeout, this, [this] {
        if (!msRuntime_ || !msRuntime_->running() || !msSceneTree_) return;
        ms::SceneTreeWidget::DisplayStates states;
        const auto rows = msRuntime_->displayRows();
        for (const auto& r : rows) {
            ms::SceneTreeWidget::DisplayState s;
            s.online    = r.connected && r.registered;
            s.enabled   = r.enabled;
            s.connected = r.connected;
            states.emplace(r.id.toStdString(), s);
        }
        msSceneTree_->refreshDisplayStatuses(states);
        // 【display 上线追平 2026-11-13】对比上一拍在线集合，新上线端自动
        // 发布当前场景（幂等广播）——覆盖"display 晚于项目打开上线"路径：
        // 注册应答只在有权威快照时单发，且此前编辑改动不会因 display 上线
        // 而自动补发。广播对老端无害（同内容幂等）。
        std::set<std::string> cur_conn;
        for (const auto& r : rows)
            if (r.connected && r.enabled)
                cur_conn.insert(r.id.toStdString());
        bool newcomer = false;
        for (const auto& id : cur_conn)
            if (!msLastConnected_.count(id)) { newcomer = true; break; }
        if (newcomer && msScene_) {
            msTimelineAdapter_->syncModelToWorkScene();
            msRuntime_->publishScene(*msScene_);
            std::fprintf(stderr, "[ms-main] 显示端上线 → 场景已自动追平下发\n");
            std::fflush(stderr);
        }
        msLastConnected_ = std::move(cur_conn);
        std::map<QString, QString> syncTexts;
        const auto texts = msRuntime_->assetSyncTexts(); // QMap
        for (auto it = texts.constBegin(); it != texts.constEnd(); ++it)
            syncTexts.emplace(it.key(), it.value());
        msSceneTree_->updateSyncProgress(syncTexts);
    });
    msUiTimer->start(200);

    emit initStatus("Initialization MediaServer editor docks success");
    auto *calendarDockWidget = m_DockManager->createDockWidget("计划任务");
    calendarDockWidget->setObjectName("scheduled");
    calendarDockWidget->setIcon(QIcon(":/icons/icons/scheduled.png"));
    scheduledTaskWidget=new ScheduledTaskWidget();
    calendarDockWidget->setWidget(scheduledTaskWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, calendarDockWidget);
    // QMenu* Options = makeOptionsMenu(scheduledTaskWidget, scheduledTaskWidget->getActions());
    calendarDockWidget->setTitleBarActions({makeOptionsMenu(scheduledTaskWidget, scheduledTaskWidget->getActions())->menuAction()});
    emit initStatus("Initialization Scheduled Task success");
    // 媒体库控件
    auto *mediaLibraryDockWidget = m_DockManager->createDockWidget("媒体库");
    mediaLibraryDockWidget->setObjectName("mediaLibrary");
    mediaLibraryDockWidget->setIcon(QIcon(":/icons/icons/media_mange.png"));
    mediaLibraryWidget = new MediaLibraryWidget();
    mediaLibraryDockWidget->setWidget(mediaLibraryWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, mediaLibraryDockWidget);
    // menuBar->views->addAction(mediaLibraryDockWidget->toggleViewAction());
    mediaLibraryDockWidget->setTitleBarActions({makeOptionsMenu(mediaLibraryWidget, mediaLibraryWidget->getActions())->menuAction()});
    emit initStatus("Initialization Media Library Widget success");
    // propertyDockWidget = m_DockManager->createDockWidget("节点属性");
    // propertyDockWidget->setObjectName("property");
    // propertyDockWidget->setIcon(QIcon(":/icons/icons/property.png"));
    // propertyWidget = new PropertyWidget(nullptr, propertyDockWidget);
    // propertyDockWidget->setWidget(propertyWidget);
    // m_DockManager->addDockWidget(ads::RightDockWidgetArea, propertyDockWidget);
    // emit initStatus("Initialization Property Widget success");

    // connect(dataflowViewsManger, &DataflowViewsManger::sceneIsActive, this, [this](const QString& title) {
    //     if (!propertyWidget || !dataflowViewsManger) return;
    //
    //     if (propertySelectionConn) {
    //         QObject::disconnect(propertySelectionConn);
    //         propertySelectionConn = QMetaObject::Connection();
    //     }
    //
    //     auto* model = dataflowViewsManger->modelByTitle(title);
    //     propertyWidget->setModel(model);
    //
    //     auto* scene = dataflowViewsManger->sceneByTitle(title);
    //     if (!scene) {
    //         propertyWidget->update(QtNodes::InvalidNodeId);
    //         return;
    //     }
    //
    //     propertySelectionConn = QObject::connect(scene, &QGraphicsScene::selectionChanged, this, [this, title]() {
        //     if (!propertyWidget || !dataflowViewsManger) return;
        //     auto* s = dataflowViewsManger->sceneByTitle(title);
        //     if (!s) return;
        //     const auto nodes = s->selectedNodes();
        //     const QtNodes::NodeId id = nodes.empty() ? QtNodes::InvalidNodeId : nodes.front();
        //     propertyWidget->update(id);
        // });

    //     const auto nodes = scene->selectedNodes();
    //     const QtNodes::NodeId id = nodes.empty() ? QtNodes::InvalidNodeId : nodes.front();
    //     propertyWidget->update(id);
    // });
    // 外部控制器
    controller=new ExternalControler();

    emit initStatus("Initialization external controler success");
	 // http 服务器
    httpServer=new Flow::NodeHttpServer();
    //http服务器文件上传后，直接打开（若已加载过项目，则自动走重启打开）
    connect(httpServer, &Flow::NodeHttpServer::flowFileUploaded, this, &MainWindow::loadFileFromPath);
    const int port = ConfigManager::instance().getHttpServerPort();
    if (!startHttpServerWithRetry(httpServer, port, 3000)) {
        emit initStatus(tr("HTTP Server 启动失败，端口可能被占用: %1").arg(port));
    } else {
        emit initStatus("Initialization Http Server success");
    }
    if (dataflowViewsManger) {
        dataflowViewsManger->setHttpServer(httpServer);
    }
    // 更新默认布局
    connect(menuBar->saveLayout, &QAction::triggered, this, &MainWindow::updateVisualState);
    //恢复布局
    connect(menuBar->restoreLayout, &QAction::triggered, this, &MainWindow::resetVisualState);
    //恢复布局
    connect(menuBar->saveAsAction, &QAction::triggered, this, &MainWindow::saveFileToExplorer);
    //另存为
    connect(menuBar->saveAction, &QAction::triggered, this, &MainWindow::saveFileToPath);
    //保存
    connect(menuBar->loadAction, &QAction::triggered, this, &MainWindow::loadFileFromExplorer);
    // 打开
    connect(menuBar->exitAction, &QAction::triggered, this, &MainWindow::close);
    // 退出
    connect(menuBar->lockAction, &QAction::triggered,this, &MainWindow::locked_switch);
    //锁定切换
    connect(menuBar->pluginsManagerAction, &QAction::triggered, pluginsManagerDlg,
            &PluginsManagerWidget::exec);
    // 显示插件管理器
    connect(menuBar->pluginsFloderAction, &QAction::triggered, pluginsManagerDlg,&PluginsManagerWidget::openPluginsFolder);
    // 刷新菜单栏视图选项
    connect(menuBar->views, &QMenu::aboutToShow, this, [this]() {
        updateViewMenu(menuBar->views);
    });
    //清空数据流
    connect(menuBar->Clear_dataflow, &QAction::triggered, this, [this]() {
        dataflowViewsManger->resetDataflow();
    });
    //    日志清空功能
    connect(menuBar->clearAction, &QAction::triggered, logTable, &LogWidget::clearTableWidget);

    setAcceptDrops(true);

     if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/icons/Flow.png"));
        trayIcon->setToolTip(tr("Flow"));
        // 托盘菜单
        trayMenu = new QMenu(this);
        trayMenu->setWindowFlags(trayMenu->windowFlags() | Qt::NoDropShadowWindowHint);
        trayMenu->setAttribute(Qt::WA_TranslucentBackground, false);
        trayExitAction    = trayMenu->addAction(tr("  退出  "));
        connect(trayExitAction,    &QAction::triggered, this, [this]() {
            // 退出前保存布局
            // saveVisualState();
            qApp->quit();
        });
        trayIcon->setContextMenu(trayMenu);

        // 双击/单击托盘图标时还原
        connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick) {
                switchVisibleFromTray();
            }
        });
        trayIcon->show();
        // 初始就显示图标，否则容易与windows任务栏显示机制有冲突，导致图标第二次无法正常显示
        //启动时加载默认布局
        resetVisualState();

    }

    setupAutosave();
}

void MainWindow::switchVisibleFromTray()
{
    if (this->isHidden()) {
        this->showMaximized();
        this->raise();
        this->activateWindow();
        syncFramelessWindowState();
    }else {
        this->hide();
    }
}

/**
 * @brief 重启进程并在新进程中打开指定项目文件
 * @param path 目标 .flow 文件路径
 * 函数级注释：
 * - 先尽量停止 HTTP Server 释放端口，避免新进程启动后网页服务端口被占用
 * - 再通过 startDetached 启动新进程，并携带 --restart-delay-ms 等待旧进程退出
 * - 置位 isRestarting，确保 closeEvent 不弹出“是否最小化到托盘”的确认框
 * - 最后退出当前进程
 */
void MainWindow::restartAndOpenFlow(const QString& path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (abs.isEmpty() || !QFileInfo::exists(abs)) {
        return;
    }

    if (httpServer && httpServer->running()) {
        httpServer->stop();
        QCoreApplication::processEvents();
        QThread::msleep(200);
    }

    if (autosaveManager) {
        autosaveManager->clearRecovery();
    }

    const QString exe = QCoreApplication::applicationFilePath();
    if (exe.isEmpty() || !QFileInfo::exists(exe)) {
        return;
    }

    // 【幽灵 socket 修复 2026-11-13】QProcess::startDetached 在 Windows 上
    // 以 bInheritHandles=TRUE 创建子进程，会把本进程全部可继承句柄（含
    // Poco UDP 17001 心跳与 17002 素材 HTTP 监听）复制给新进程；旧进程
    // 退出后"幽灵 socket"随新进程存活并劫持端口流量（实证：netstat 属主
    // 显示已死 PID、素材拉取 Timeout、display 状态上报黑洞）。改走
    // CreateProcessW(bInheritHandles=FALSE)，详见 SpawnNoInherit.hpp。
    const QString rawArgs = QStringLiteral("--restart-delay-ms=500 \"%1\"")
                                .arg(QDir::toNativeSeparators(abs));
    const bool started = ms::spawnNoInherit(exe, rawArgs);
    if (!started) {
        QMessageBox::warning(this, "", tr("重启打开失败：无法启动新进程"));
        return;
    }

    // 新进程在 main() 入口即读取 LastShutdownClean，必须在 startDetached 成功后、
    // 旧进程 aboutToQuit 之前同步写入，否则会被误判为异常退出并弹出恢复对话框。
    ProjectPersistence::markCleanShutdown();

    isRestarting = true;
    QCoreApplication::quit();
}

/**
 * @brief 打开最近文件（菜单项触发）
 * @param path 最近文件路径
 * 函数级注释：统一走重启打开，避免运行时直接切换项目导致内存持续增长。
 */
void MainWindow::openRecentFile(const QString& path)
{
    restartAndOpenFlow(path);
}

//显示属性
// void MainWindow::initNodelist() {
//     nodeLibrary=new NodeLibraryWidget();
//     this->nodeDockLibraryWidget->setWidget(nodeLibrary);
//     emit initStatus("Initialization nodes library success");
// }

// 锁定切换
void MainWindow::locked_switch() {
    isLocked=!isLocked;
    dataflowViewsManger->setDataflowLocked(isLocked);
}
//拖拽进入
void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls() && mimeData->urls().size() == 1 && !isLocked) {
        //        锁定状态下无法拖拽
        QString filePath = mimeData->urls().first().toLocalFile();
        QFileInfo fileInfo(filePath);
        if (fileInfo.suffix().toLower() == "flow") {
            //            文件后缀符合，才接收拖拽
            event->acceptProposedAction();
        }
    }
}
/**
 * 拖拽事件
 * 接收拖拽的文件路径，检查是否为 .flow 文件，若符合则清空场景并加载文件。
 */
void MainWindow::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    const QString filePath = mimeData->urls().at(0).toLocalFile();
    if (!QFileInfo::exists(filePath))
        return;

    restartAndOpenFlow(filePath);
    event->acceptProposedAction();

}
/**
 * 从路径打开文件（仅显示状态文字）
 * 通过 initStatus 分阶段展示状态文本（启动阶段由 main.cpp 的启动界面承接）。
 */
void MainWindow::loadFileFromPath(const QString &path)
{
    if (!currentProjectPath.isEmpty()) {
        restartAndOpenFlow(path);
        return;
    }

    ProjectLoadResolution resolution;
    if (forcedLoadResolution_) {
        resolution = *forcedLoadResolution_;
        forcedLoadResolution_.reset();
    } else if (skipRecoveryPrompt_) {
        resolution.loadPath = path;
        resolution.projectPath = path;
    } else {
        resolution = resolveRecoveryLoadPath(path);
    }

    const QString loadPath = resolution.loadPath.isEmpty() ? path : resolution.loadPath;

    QString logicalProjectPath = ProjectPersistence::normalizeProjectPath(resolution.projectPath);
    if (logicalProjectPath.isEmpty() && resolution.fromRecovery) {
        logicalProjectPath = ProjectPersistence::normalizeProjectPath(
            ProjectPersistence::readRecovery().projectPath);
    }
    if (logicalProjectPath.isEmpty()) {
        const QString normalizedArg = ProjectPersistence::normalizeProjectPath(path);
        if (!normalizedArg.isEmpty() && !ProjectPersistence::isAutosavePath(normalizedArg)) {
            logicalProjectPath = normalizedArg;
        }
    }

    struct SplashProxy {
        MainWindow* self = nullptr;
        void updateStatus(const QString& msg) {
            if (self) Q_EMIT self->initStatus(msg);
            QApplication::processEvents();
        }
        void finish(QWidget*) {}
    };

    SplashProxy splashScreen{ this };

    const auto pumpUi = []() {
        QApplication::processEvents();
    };

    const auto reportStatus = [&splashScreen, &pumpUi](const QString& msg) {
        splashScreen.updateStatus(msg);
        pumpUi();
    };

    if (loadPath.isEmpty()) {
        reportStatus(tr("Path is empty"));
        return;
    }

    const ProjectFileReadResult readResult = readProjectFile(loadPath, reportStatus);
    if (!readResult.success) {
        QMessageBox::warning(this, "", readResult.errorMessage);
        return;
    }

    if (autosaveManager) {
        autosaveManager->setPaused(true);
    }

    const auto abortLoad = [&](const QString& msg) {
        if (autosaveManager) {
            autosaveManager->setPaused(false);
        }
        reportStatus(msg);
        QMessageBox::warning(this, "", msg);
    };

    ProjectLoadTargets targets;
    targets.dataflowViewsManger = dataflowViewsManger;
    targets.scheduledTaskWidget = scheduledTaskWidget;
    targets.dockManager = m_DockManager;
    targets.httpServer = httpServer;
    targets.msScene = msScene_; // 新时间轴场景回灌落点（mediaTimeline 段）
    targets.msRuntime = msRuntime_; // 显示端登记回灌落点（mediaDisplays 段）

    ProjectLoadOptions options;
    options.restoreVisualLayout = true;
    options.loadWebLayout = true;
    options.onMissingVisualLayout = [this]() { resetVisualState(); };

    const auto dataflowProgress = [&splashScreen, &pumpUi](const QString& sceneTitle,
                                                           const QString& phase,
                                                           int current,
                                                           int total) {
        if (total > 0) {
            const int pct = qBound(0, (current * 100) / total, 100);
            splashScreen.updateStatus(QObject::tr("DataFlow [%1] %2 %3% (%4/%5)")
                                         .arg(sceneTitle, phase)
                                         .arg(pct)
                                         .arg(current)
                                         .arg(total));
        } else {
            splashScreen.updateStatus(QObject::tr("DataFlow [%1] %2 %3")
                                         .arg(sceneTitle, phase)
                                         .arg(current));
        }
        pumpUi();
    };

    QString loadError;
    if (!loadProjectModulesFromJson(readResult.root,
                                    targets,
                                    options,
                                    reportStatus,
                                    dataflowProgress,
                                    &loadError)) {
        abortLoad(loadError);
        return;
    }

    // 新时间轴场景回灌（2026-11-12 第三步）：loadProjectModulesFromJson
    // 已把 mediaTimeline 段反序列化进 *msScene_（旧文件无段=原样保留默认
    // 空场景）；场景有内容时重灌时间轴模型 + 场景树 + 预览失效。
    if (msScene_
        && (!msScene_->timeline.tracks.empty() || !msScene_->screens.empty()
            || !msScene_->assets.empty())) {
        msTimelineAdapter_->loadSceneToModel(*msScene_);
        msSceneTree_->rebuild(msScene_);
        msPreview_->invalidate();
        // 【打开即追平 2026-11-13】加载项目后立即建立显示端权威快照并
        // 广播——display 端注册应答/上线追平才有场景可发，用户零手动动作
        //（此前打开项目后必须先右键"下发场景"，否则 display 永远空场景）。
        if (msRuntime_ && msRuntime_->running()) {
            msRuntime_->publishScene(*msScene_);
            std::fprintf(stderr, "[ms-main] 项目加载完成 → 场景已自动下发\n");
            std::fflush(stderr);
        }
    }

    const QString normalizedLogicalPath = logicalProjectPath;
    if (!normalizedLogicalPath.isEmpty()) {
        ConfigManager::instance().addRecentFile(normalizedLogicalPath);
        menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());
        onProjectLoaded(normalizedLogicalPath);
        this->setWindowTitle(QFileInfo(normalizedLogicalPath).fileName());
    } else {
        onProjectLoaded(QString());
        this->setWindowTitle(tr("未命名项目"));
    }

    if (resolution.fromRecovery) {
        reportStatus(tr("Recovered from autosave"));
    }
    reportStatus(tr("Load flow file completed"));
}

/**
 * @brief 应用启动前已确认的加载/恢复结果
 * @param resolution 启动前 resolveProjectLoadPath 的解析结果（含是否从 recovery 恢复）
 * @param cmdFilePath 命令行传入的 .flow 路径，可为空
 * 函数级注释：在 Splash 初始化完成后调用；若用户选择恢复自动保存，则加载 recovery 文件；
 *            否则按命令行路径打开，并跳过重复的恢复弹窗。
 */
void MainWindow::applyStartupLoadResolution(const ProjectLoadResolution& resolution,
                                            const QString& cmdFilePath)
{
    if (!currentProjectPath.isEmpty()) {
        return;
    }

    if (resolution.fromRecovery) {
        forcedLoadResolution_ = resolution;
        const QString openArg = ProjectPersistence::normalizeProjectPath(resolution.projectPath);
        loadFileFromPath(openArg);
        return;
    }

    if (cmdFilePath.isEmpty()) {
        return;
    }

    skipRecoveryPrompt_ = true;
    loadFileFromPath(cmdFilePath);
    skipRecoveryPrompt_ = false;
}

//从文件管理器打开文件
/**
 * @brief 从资源管理器选择并打开 .flow 文件
 * 函数级注释：该入口用于用户主动切换项目；为稳定性起见，统一走重启打开。
 */
void MainWindow::loadFileFromExplorer() {
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    tr("Open Flow Scene"),
                                                    QDir::homePath(),
                                                    tr("Flow Scene Files (*.flow)"));
    if (!QFileInfo::exists(fileName))
        return;

    restartAndOpenFlow(fileName);

}

// ============================================================================
// MediaServer 时间轴 OSC 集成（2026-11-12 第四步）
// 参照旧 TimeLineWidget 的 TimelineBindingsController 集成模式：
//   * 控制入站：GlobalEventBus 订阅 /timeline/master/{play,pause,stop,
//     currentFrame}（UDP OSC → StatusContainer::parseOSC → publishCommand
//     → 本槽；MQTT/计划任务/内部命令同链）；
//   * 状态出站：播放态/播放头帧号 publishState → StatusContainer 镜像
//     → statusUpdated → ExternalControler（OSC feedback）+ NodeHttpServer
//     （WebSocket）+ MQTT 出站；
//   * 控件注册：播放/停止按钮 registerWidget（地址→控件反查）。
// 进程内无回环：publishState 是 Feedback 事件，onGlobalEvent 只处理
// Command——反馈不会反向触发命令订阅者。
// ============================================================================

/**
 * @brief 场景自动追平请求（sceneEdited 三路统一入口）
 * 函数级注释：节流 200ms（单参数）——距上次发布满 200ms 立即发（首次
 *            编辑 0 延迟）；不足间隔且无挂起拍 → 挂起补一拍（对齐剩余
 *            时间，节奏均匀）；已挂起则等到点发当下最新状态。停手后由
 *            挂起拍兜底（天然 trailing），编辑过程 display 每 200ms 追平
 *            一次、无"停手才发"的不连续感。
 */
void MainWindow::requestMsAutoPublish() {
    if (!msAutoPublishTimer_) return;
    const qint64 sinceLast = msLastPublishSince_.isValid()
                                 ? msLastPublishSince_.elapsed()
                                 : -1; // 从未发过 → 立即
    if (sinceLast < 0 || sinceLast >= msAutoPublishIntervalMs_) {
        publishMsSceneNow();             // 首发 / 距上次发布已满间隔：立即发
        return;
    }
    if (!msAutoPublishTimer_->isActive())
        msAutoPublishTimer_->start(int(msAutoPublishIntervalMs_ - sinceLast)); // 对齐下一拍
    // 已挂起：不动（到点自然发最新状态——发布聚合）
}

/**
 * @brief 立即发布场景到显示端（自动追平的公共发布体）
 * 函数级注释：模型→工作场景兜底同步（防面板改动未写回）后全量广播；
 *            runtime 未启动/无场景静默跳过。节流到点与立即路径共用，
 *            末尾打点节流基准（下次编辑距此满 200ms 才发下一拍）。
 */
void MainWindow::publishMsSceneNow() {
    if (!msRuntime_ || !msRuntime_->running() || !msScene_) return;
    msTimelineAdapter_->syncModelToWorkScene();
    msRuntime_->publishScene(*msScene_);
    msLastPublishSince_.restart();
    std::fprintf(stderr, "[ms-main] 场景自动追平：已同步发布到显示端\n");
    std::fflush(stderr);
}

/**
 * @brief 播放（工具栏点击与外部 /play 命令同源）
 * 函数级注释：会话广播锚点播放 + 本地预览状态机启动 + 33ms tick 启动，
 *            末尾推送播放态反馈（/play /stop）。
 */
void MainWindow::msHandlePlay() {
    if (msRuntime_ && msRuntime_->running())
        msRuntime_->cmdPlay(); // 广播锚点播放（0 端在线 = no-op）
    if (msTimeline_ && msTimeline_->toolbar)
        msTimeline_->toolbar->setPlaybackState(true); // 按钮态同步（OSC 命令不经动作触发）
    if (!msPreview_) return;
    msPreview_->play();     // Idle→从头 / Paused→续播（状态机幂等）
    if (msPlayTimer_) msPlayTimer_->start(33); // tick 节奏对齐画布 pull 泵
    msPublishPlaybackState();
}

/**
 * @brief 暂停（工具栏点击与外部 /pause 命令同源）
 * 函数级注释：会话冻结于锚点 + 预览暂停 + tick 停止，末尾推送播放态反馈。
 */
void MainWindow::msHandlePause() {
    if (msRuntime_ && msRuntime_->running()) msRuntime_->cmdPause();
    if (msTimeline_ && msTimeline_->toolbar)
        msTimeline_->toolbar->setPlaybackState(false);
    if (!msPreview_) return;
    msPreview_->pause();
    if (msPlayTimer_) msPlayTimer_->stop();
    msPublishPlaybackState();
}

/**
 * @brief 停止（工具栏点击与外部 /stop 命令同源）
 * 函数级注释：会话回 Idle + 预览归零 + tick 停止 + 播放头回显起点，
 *            末尾推送播放态反馈。
 */
void MainWindow::msHandleStop() {
    if (msRuntime_ && msRuntime_->running()) msRuntime_->cmdStop();
    if (msTimeline_ && msTimeline_->toolbar)
        msTimeline_->toolbar->setPlaybackState(false);
    if (!msPreview_) return;
    msPreview_->stop(); // 归零回 Idle
    if (msPlayTimer_) msPlayTimer_->stop();
    // 播放头回显起点（画布 pull 泵检测变化自动重绘首帧）
    if (msTimelineAdapter_) msTimelineAdapter_->setPlayheadFromRuntimeUs(0);
    if (msSceneTree_) msSceneTree_->markPlayhead(0);
    msPublishPlaybackState();
}

/**
 * @brief 播放态反馈推送（/play /stop /currentFrame）
 * 函数级注释：publishState → StatusContainer 镜像（statusUpdated 出站）。
 *            播放态以本地预览状态机为准（与 handle 双源同步翻转）；
 *            非播放态补发 /currentFrame=0 归零帧（停止/暂停后反馈收敛）。
 */
void MainWindow::msPublishPlaybackState() {
    if (!msPreview_) return;
    const bool playing = msPreview_->playing();
    auto* bus = GlobalEventBus::instance();
    bus->publishState(msTimelineAddress(QStringLiteral("/play")), playing);
    bus->publishState(msTimelineAddress(QStringLiteral("/stop")), !playing);
    if (!playing) {
        bus->publishState(msTimelineAddress(QStringLiteral("/currentFrame")), 0);
    }
}

/**
 * @brief OSC 绑定入口（构造期一次）
 * 函数级注释：订阅四个命令地址（单槽分发）+ 播放/停止控件注册到
 *            StatusContainer（地址→控件反查，对齐旧 registerOSCControl
 *            登记语义）+ 推送初始状态（旧 publishCurrentState 语义）。
 */
void MainWindow::msBindTimelineOsc() {
    auto* bus = GlobalEventBus::instance();
    bus->subscribe(msTimelineAddress(QStringLiteral("/play")), this,
                   SLOT(msOnTimelineCommand(GlobalEvent)));
    bus->subscribe(msTimelineAddress(QStringLiteral("/pause")), this,
                   SLOT(msOnTimelineCommand(GlobalEvent)));
    bus->subscribe(msTimelineAddress(QStringLiteral("/stop")), this,
                   SLOT(msOnTimelineCommand(GlobalEvent)));
    bus->subscribe(msTimelineAddress(QStringLiteral("/currentFrame")), this,
                   SLOT(msOnTimelineCommand(GlobalEvent)));
    // 【OSC 地址拖拽】启用工具栏按钮 Ctrl+拖出地址（/play /stop）——
    // 地址入剪贴板 + x-osc-address mime 可落 OSCMessageListWidget 等既有
    // 拖放目标。base 与 msTimelineAddress helper 同源（/timeline/master）。
    if (msTimeline_ && msTimeline_->toolbar) {
        msTimeline_->toolbar->setOscBase(QStringLiteral("/timeline/master"));
    }
    // 控件注册（OSC 出站/入站不依赖控件指针；注册供地址树/反查 UI 使用）
    if (msTimeline_ && msTimeline_->toolbar) {
        if (auto* w = msTimeline_->toolbar->widgetForAction(
                msTimeline_->toolbar->playAction())) {
            StatusContainer::instance()->registerWidget(
                w, msTimelineAddress(QStringLiteral("/play")));
        }
        if (auto* w = msTimeline_->toolbar->widgetForAction(
                msTimeline_->toolbar->stopAction())) {
            StatusContainer::instance()->registerWidget(
                w, msTimelineAddress(QStringLiteral("/stop")));
        }
    }
    // 初始状态推送（/play=false /stop=true /currentFrame=0）
    msPublishPlaybackState();
}

/**
 * @brief GlobalEventBus 命令分发入口（/timeline/master/* 订阅槽）
 * @param ev 全局事件（Command 类）
 * 函数级注释：旧 TimeLineToolBar::onGlobalEvent 同构——单槽按地址分发；
 *            /play payload 缺省或 true = 播放，false = 停止（旧语义）；
 *            /currentFrame payload 为帧号（PAL 25fps 旧交互语义），换算
 *            μs 后走 onSetPlayheadPos（80ms seek 节流内建，拖动风暴安全）。
 *            命令处理后的状态回执由 msHandle* 内的 msPublishPlaybackState
 *            统一承担（与旧版处理后再 stateFeedBack 等效）。
 */
void MainWindow::msOnTimelineCommand(const GlobalEvent& ev) {
    if (ev.kind != GlobalEventKind::Command) return;

    if (ev.address == msTimelineAddress(QStringLiteral("/play"))) {
        if (!ev.payload.isValid() || ev.payload.toBool()) {
            msHandlePlay();
        } else {
            msHandleStop();
        }
        return;
    }
    if (ev.address == msTimelineAddress(QStringLiteral("/pause"))) {
        msHandlePause();
        return;
    }
    if (ev.address == msTimelineAddress(QStringLiteral("/stop"))) {
        msHandleStop();
        return;
    }
    if (ev.address == msTimelineAddress(QStringLiteral("/currentFrame"))) {
        if (!ev.payload.isValid() || !msTimelineAdapter_) return;
        const qint64 frame = ev.payload.toLongLong();
        msTimelineAdapter_->onSetPlayheadPos(
            QtTimeline::framesToUs(static_cast<int>(frame)));
        return;
    }
}

/**
 * @brief 序列化当前项目为 .flow JSON 对象
 * @return 包含 DataFlow、TimeLine、布局等模块的完整项目快照
 * 函数级注释：供手动保存与自动保存共用，确保写入内容一致。
 */
QJsonObject MainWindow::serializeProject(ProjectSaveOrigin origin) const
{
    ProjectSnapshotSources sources;
    sources.dataflowViewsManger = dataflowViewsManger;
    sources.scheduledTaskWidget = scheduledTaskWidget;
    sources.dockManager = m_DockManager;
    sources.httpServer = httpServer;
    sources.msScene = msScene_; // 新时间轴场景（pb-base64 内嵌 mediaTimeline 段）
    sources.msRuntime = msRuntime_; // 显示端登记（mediaDisplays 段，统一入口）
    return buildProjectSnapshot(sources, origin);
}

/**
 * @brief 初始化自动保存管理器
 * 函数级注释：按配置启用周期性 recovery 保存；退出时写入最后一次快照并标记正常关闭。
 */
void MainWindow::setupAutosave()
{
    if (autosaveManager) {
        return;
    }

    autosaveManager = new AutosaveManager(this);
    autosaveManager->setEnabled(ConfigManager::instance().isAutosaveEnabled());
    autosaveManager->setIntervalSeconds(ConfigManager::instance().getAutosaveIntervalSeconds());
    autosaveManager->setSerializer([this]() {
        return serializeProject(ProjectSaveOrigin::Autosave);
    });
    autosaveManager->setProjectPath(QString());
    autosaveManager->start();

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        if (isRestarting) {
            return;
        }
        if (httpServer && httpServer->running()) {
            httpServer->stop();
        }
        finalizeAutosave();
        ProjectPersistence::markCleanShutdown();
    });
}

/**
 * @brief 立即执行一次自动保存
 * 函数级注释：在程序退出或关闭窗口时调用，将当前项目写入 recovery 目录。
 */
void MainWindow::finalizeAutosave()
{
    if (!autosaveManager) {
        return;
    }
    autosaveManager->saveRecoveryNow();
}

/**
 * @brief 项目加载完成后的回调
 * @param projectPath 逻辑项目路径（用户保存用的 .flow 路径，非 recovery 文件路径）
 * 函数级注释：更新 currentProjectPath，并同步自动保存管理器中的项目路径；
 *            若实际加载的是 autosave.flow，则从 meta 中还原原项目路径。
 */
void MainWindow::onProjectLoaded(const QString& projectPath)
{
    QString normalizedPath = ProjectPersistence::normalizeProjectPath(projectPath);
    if (ProjectPersistence::isAutosavePath(normalizedPath)) {
        normalizedPath = ProjectPersistence::normalizeProjectPath(
            ProjectPersistence::readRecovery().projectPath);
    }
    currentProjectPath = normalizedPath;
    if (!autosaveManager) {
        return;
    }
    autosaveManager->setPaused(true);
    autosaveManager->setProjectPath(normalizedPath);
    autosaveManager->setPaused(false);
}

/**
 * @brief 解析启动/打开时应加载的文件路径
 * @param requestedPath 用户请求打开的路径，空表示启动时无命令行文件
 * @return 实际加载路径与逻辑项目路径；若需恢复则 fromRecovery 为 true
 * 函数级注释：检测到异常退出且存在 recovery 时弹出恢复对话框，供 loadFileFromPath 使用。
 */
ProjectLoadResolution MainWindow::resolveRecoveryLoadPath(const QString& requestedPath) const
{
    return resolveProjectLoadPath(requestedPath, const_cast<MainWindow*>(this));
}

//保存文件到路径
void MainWindow::saveFileToPath(){
    if(currentProjectPath.isEmpty()){
        saveFileToExplorer();
        qDebug() << "Saved data to" << currentProjectPath;
        return;
    }
    if (saveProjectSnapshotAtomic(currentProjectPath, serializeProject())) {
        qDebug() << "Saved data to" << currentProjectPath;
        if (autosaveManager) {
            autosaveManager->clearRecovery();
        }
    }
}
//保存文件到资源管理器
void MainWindow::saveFileToExplorer() {
    QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                    tr("Open Flow Scene"),
                                                    QDir::homePath(),
                                                    tr("Flow Scene Files (*.flow)"));
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith("flow", Qt::CaseInsensitive))
            fileName += ".flow";

        if (saveProjectSnapshotAtomic(fileName, serializeProject())) {
            onProjectLoaded(fileName);
            this->setWindowTitle(QFileInfo(fileName).fileName());
            ConfigManager::instance().addRecentFile(currentProjectPath);
            menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());
            qDebug() << "Saved data to" << currentProjectPath;
            if (autosaveManager) {
                autosaveManager->clearRecovery();
            }
        }
    }

}
// 保存布局
void MainWindow::updateVisualState()
{
    QFile file("./DefaultLayout.ini");
    if (file.open(QIODevice::WriteOnly))//不存在则创建,默认覆盖  | QIODevice::Append
        //if (file.open(QIODevice::WriteOnly | QIODevice::Append));//文末追加,不存在则创建
    {
        QDataStream out(&file);
        out << m_DockManager->saveState();
        file.close();
    }
}
//恢复布局
void MainWindow::resetVisualState()
{

    QFile file("./DefaultLayout.ini");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray ba;
        QDataStream in(&file);
        in >> ba;
        file.close();
        m_DockManager->restoreState(ba);
    }

}

/**
 * @brief 主窗口关闭事件
 * @param event 关闭事件
 * 函数级注释：
 * - 正常关闭：若系统托盘可用则询问是否最小化到托盘
 * - 自重启关闭：isRestarting=true 时直接 accept，避免弹窗阻塞重启链路
 */
void MainWindow::closeEvent(QCloseEvent* event)
{
    if (isRestarting) {
        event->accept();
        return;
    }

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        const FramelessConfirmDialog::ButtonTexts buttons{
            tr("最小化到托盘"),
            tr("退出程序"),
            tr("取消"),
        };
        const auto reply = FramelessConfirmDialog::question(
            this,
            tr("退出"),
            tr("关闭窗口时，可以将程序最小化到系统托盘继续在后台运行，也可以直接退出。"),
            buttons,
            FramelessConfirmDialog::Result::Cancel);

        if (reply == FramelessConfirmDialog::Result::Primary) {
            qDebug() << "Minimize to system tray";
            this->hide();
            event->ignore(); // 保持进程运行
        } else if (reply == FramelessConfirmDialog::Result::Secondary) {
            // 先停 HTTP/WebSocket，再打退出日志，避免日志广播 sendFrame 拖慢关闭
            if (httpServer && httpServer->running()) {
                httpServer->stop();
            }
            qDebug() << "The program exits manually";
            finalizeAutosave();
            ProjectPersistence::markCleanShutdown();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        if (httpServer && httpServer->running()) {
            httpServer->stop();
        }
        finalizeAutosave();
        ProjectPersistence::markCleanShutdown();
        event->accept();
    }
}

void MainWindow::updateViewMenu(QMenu* menu)
{
    menu->clear();
    for (auto* DockWidget : m_DockManager->dockWidgetsMap())
    {
        menu->addAction(DockWidget->toggleViewAction());
    }
}

QMenu* MainWindow::makeOptionsMenu(QWidget* parent, const QList<QAction*>& actions) {
    QMenu* menu = new QMenu(tr("Options"), parent);
    QAction* menuAct = menu->menuAction();
    menuAct->setObjectName("optionsMenu");
    menuAct->setIcon(QIcon(":/ads_icons/ads_icons/custom-menu-button.svg"));
    menu->setToolTip(menu->title());
    menu->addActions(actions);
    return menu;
}

/**
 * @brief 构建 QWindowKit 无边框标题栏并完成 WindowAgent 绑定
 *
 * 布局结构（自左向右）：
 *   [应用图标] [Flow 品牌文字] [菜单栏 MenuBarWidget] [居中标题] [最小化] [最大化/还原] [关闭]
 *
 * 说明：
 * - 通过 setMenuWidget(windowBar) 将自定义标题栏挂到 QMainWindow 顶部
 * - WindowAgent 负责无边框拖拽、边缘缩放、Snap Layout 等 Windows 原生行为
 * - 标题栏内除 setHitTestVisible 标记的控件外，其余区域均可拖拽移动窗口
 */
void MainWindow::setupFramelessWindow()
{
    if (!windowAgent || !menuBar) {
        return;
    }

    // 供 QSS 选择器 QMenuBar#win-menu-bar 定位样式
    menuBar->setObjectName(QStringLiteral("win-menu-bar"));

    // --- 应用图标后侧的品牌文字 ---
    // 以菜单栏左上角 corner widget 的形式呈现，视觉上紧随应用图标，
    // 不改动 WindowBar 的内部布局结构；样式见 QSS #win-brand-label
    auto *brandLabel = new QLabel(QStringLiteral("Flow"), this);
    brandLabel->setObjectName(QStringLiteral("win-brand-label"));
    menuBar->setCornerWidget(brandLabel, Qt::TopLeftCorner);

    // 居中显示窗口标题（项目名 / 文件名），样式见 QSS #win-title-label
    auto *titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName(QStringLiteral("win-title-label"));

    // --- 标题栏左侧：应用图标 ---
    // WindowButton 须用 setIconNormal 设置图标，不能仅用 QPushButton::setIcon
    auto *iconButton = new QWK::WindowButton(this);
    iconButton->setObjectName(QStringLiteral("icon-button"));
    iconButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    iconButton->setIconNormal(applicationIcon());

    // --- 标题栏右侧：最小化 / 最大化(可切换) / 关闭 ---
    auto *minButton = new QWK::WindowButton(this);
    minButton->setObjectName(QStringLiteral("min-button"));
    minButton->setProperty("system-button", true);  // QSS 统一样式
    minButton->setIconSize(QSize(16, 16));
    minButton->setFixedHeight(32);
    minButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *maxButton = new QWK::WindowButton(this);
    maxButton->setCheckable(true);  // 最大化时 checked，显示还原图标
    maxButton->setObjectName(QStringLiteral("max-button"));
    maxButton->setProperty("system-button", true);
    maxButton->setIconSize(QSize(16, 16));
    maxButton->setFixedHeight(32);
    maxButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *closeButton = new QWK::WindowButton(this);
    closeButton->setObjectName(QStringLiteral("close-button"));
    closeButton->setProperty("system-button", true);
    closeButton->setIconSize(QSize(16, 16));
    closeButton->setFixedHeight(32);
    closeButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // --- 组装 WindowBar ---
    windowBar = new QWK::WindowBar(this);
    windowBar->setObjectName(QStringLiteral("win-window-bar"));
    // 必须先绑定 hostWidget，再设置 titleLabel / iconButton，
    // 这样 WindowBar 的事件过滤器才能收到 WindowTitleChange / WindowStateChange
    windowBar->setHostWidget(this);
    windowBar->setIconFollowWindow(true);  // 窗口图标变化时自动更新左上角按钮
    windowBar->setIconButton(iconButton);
    windowBar->setMinButton(minButton);
    windowBar->setMaxButton(maxButton);
    windowBar->setCloseButton(closeButton);
    windowBar->setMenuBar(menuBar);
    windowBar->setTitleLabel(titleLabel);

    // --- 注册到 QWindowKit WindowAgent ---
    windowAgent->setTitleBar(windowBar);
    // 声明各按钮的系统角色，使 Windows Snap Layout、任务栏预览等原生特性正常工作
    windowAgent->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, minButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
    // 菜单栏需要接收鼠标点击；未标记 hit-test 的区域默认作为拖拽区
    windowAgent->setHitTestVisible(menuBar, true);

    setMenuWidget(windowBar);

    // --- 信号连接 ---
    // 点击左上角图标：弹出 Windows 系统菜单（还原 / 移动 / 大小 / 最小化 / 最大化 / 关闭）
    connect(iconButton, &QAbstractButton::clicked, this, [this, iconButton]() {
        if (windowAgent) {
            windowAgent->showSystemMenu(
                iconButton->mapToGlobal(QPoint(0, iconButton->height())));
        }
    });
    connect(windowBar, &QWK::WindowBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(windowBar, &QWK::WindowBar::maximizeRequested, this, [this, maxButton](bool max) {
        if (max) {
            showMaximized();
        } else {
            showNormal();
        }
        // 窗口状态改变后，清除最大化按钮残留的 hover 高亮
        emulateLeaveEvent(maxButton);
    });
    connect(windowBar, &QWK::WindowBar::closeRequested, this, &QWidget::close);

    syncFramelessWindowState();
}

/**
 * @brief 同步无边框标题栏的显示状态
 *
 * 在以下场景被调用：初始化完成、showEvent、窗口状态变化、
 * 激活/失活、主题切换、从托盘恢复等。
 *
 * 同步内容：
 * - 标题文字（与 windowTitle() 一致）
 * - 左上角应用图标
 * - 最大化按钮的 checked 状态（最大化 ↔ 还原图标）
 * - 标题栏激活/失活样式（QSS 属性 bar-active）
 */
void MainWindow::syncFramelessWindowState()
{
    if (!windowBar) {
        return;
    }

    if (auto *label = windowBar->titleLabel()) {
        label->setText(windowTitle());
    }
    if (auto *iconBtn = qobject_cast<QWK::WindowButton *>(windowBar->iconButton())) {
        const QIcon icon = windowIcon().isNull() ? applicationIcon() : windowIcon();
        iconBtn->setIconNormal(icon);
    }
    if (auto *maxBtn = windowBar->maxButton()) {
        maxBtn->setChecked(isMaximized());
    }
    if (auto *bar = menuWidget()) {
        bar->setProperty("bar-active", isActiveWindow());
        if (style()) {
            style()->polish(bar);
        }
    }
}

/**
 * @brief 窗口首次显示时同步标题栏状态
 * @note main.cpp 中以 showMaximized() 启动，此处确保最大化按钮图标正确
 */
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    syncFramelessWindowState();
}

/**
 * @brief 窗口状态变化（最大化 / 还原 / 最小化）时同步标题栏
 */
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        syncFramelessWindowState();
    }
}

/**
 * @brief 处理窗口激活/失活，切换标题栏前景样式
 *
 * 激活时 bar-active=true（深色主题 #2f3034 / 浅色主题 #E8E9ED），
 * 失活时 bar-active=false（颜色略暗），对应 DefaultDark/Light.qss。
 */
bool MainWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        syncFramelessWindowState();
        break;
    default:
        break;
    }
    return QMainWindow::event(event);
}

void MainWindow::applyTheme(bool isDark)
{
    const QString qss = isDark ? AppConstants::DARK_STYLESHEET
                               : AppConstants::LIGHT_STYLESHEET;
    QFile qssFile(qss);
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load QSS file:" << qssFile.errorString();
        return;
    }

    QTextStream stream(&qssFile);
    const QString styleSheet = stream.readAll();
    qssFile.close();

    if (styleSheet.isEmpty()) {
        qWarning() << "QSS file is empty or could not be read properly";
        return;
    }

    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setStyleSheet(styleSheet);
    }
    if (m_DockManager) {
        m_DockManager->setStyleSheet(styleSheet);
    }
    setNodeEditorDarkStyle(isDark);
}
