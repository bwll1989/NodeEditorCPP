//
// Created by 吴斌 on 2024/1/18.
//

#pragma once

#include <QJsonObject>
#include <QMainWindow>
#include <QElapsedTimer>
#include <optional>
#include <set>
#include <string>
#include "Widget/ConsoleWidget/LogWidget.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
// #include "DockHub/DockHub.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Widget/MenuBarWidget/MenuBarWidget.h"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Widget/MenuBarWidget/MenuBarWidget.h"
// 【2026-11-12】旧时间轴（TimelineWidget/TimeLineModel）退役——
// 主窗口改挂 MediaServer 编辑器组件（场景树/预览画布/时间轴），
// 完整类型仅在 MainWindow.cpp 中 include（本头只留前置声明）。
#include "Widget/ExternalControl/ExternalControler.hpp"
#include "Widget/CalendarWidget/ScheduledTaskWidget.hpp"
#include "Widget/MediaLibraryWidget/MediaLibraryWidget.h"
#include "Widget/NodeWidget/DataflowViewsManger.hpp"
// 新增：系统托盘支持
#include <QSystemTrayIcon>
#include <QMenu>
#include "../ExternalControl/HttpServer.hpp"

namespace QWK {
    class WidgetWindowAgent;
    class WindowBar;
}

class AutosaveManager;
#include "ProjectSnapshotBuilder.hpp"

// ---- MediaServer 编辑器组件（2026-11-12 第二步：UI 挂载 + 动作联动）--------
namespace ms {
struct Scene;                // core/model（header-only；struct——与定义同形防 C4099）
class PreviewRenderer;       // editor/（本地预览渲染器）
class QtTimelineModelAdapter;// editor/（Scene↔帧模型适配器）
class SceneCanvasWidget;     // editor/（预览画布）
class SceneTreeWidget;       // editor/（场景树）
class MsRuntimeBridge;       // timeline/（EditorRuntime 桥接壳；PIMPL 隔离 C++20）
}
class BaseTimelineWidget;    // MediaServer 收编版 QtTimeLine 控件

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    /**
     * @brief 构造函数（支持无头模式）
     * @param parent 父窗口
     * @param headlessMode 是否启用无头模式（不显示任何界面）
     * 函数级注释：无头模式下仍会构造必要的后台对象以保证功能不变。
     */
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    //菜单栏
    MenuBarWidget *menuBar=nullptr;
    //dock管理器
    ads::CDockManager* m_DockManager=nullptr;
    //插件管理器
    PluginsManagerWidget *pluginsManagerDlg=nullptr;
    // 锁定状态
    bool isLocked= false;
    // ---- MediaServer 编辑器组件（2026-11-12 第二步：UI 挂载 + 动作联动）----
    // 工作副本（ms 默认空场景）：画布/场景树/时间轴适配器共享同一实例；
    // 动作联动已接入（EditorWindow 同款装配）：import_media 素材导入钩子
    // （属性面板/拖拽导入落点）+ seek/sceneEdited/导入失败三信号 + 工具栏
    // 播放 → PreviewRenderer 状态机（33ms tick 回显播放头）。
    // 播放定时器与接线均在 init() 装配段局部完成（无成员）；
    // 下发到显示端已接入（第三步 2026-11-12）：msRuntime_ 桥接
    // EditorRuntime（显示端连接/场景下发/素材 HTTP 分发/锚点命令）——
    // 场景树"下发/同步素材/重载/添加显示端"四动作 + 200ms UI 拍回显
    // 连接与素材同步进度；播放权威时钟=桥会话时钟（Playing 时优先）。
    ms::Scene* msScene_ = nullptr;
    ms::PreviewRenderer* msPreview_ = nullptr;          // 预览渲染器（空素材根=全占位降级）
    ms::QtTimelineModelAdapter* msTimelineAdapter_ = nullptr;
    ms::SceneCanvasWidget* msCanvas_ = nullptr;         // 预览画布 dock
    ms::SceneTreeWidget* msSceneTree_ = nullptr;        // 场景树 dock
    BaseTimelineWidget* msTimeline_ = nullptr;          // 时间轴 dock
    ms::MsRuntimeBridge* msRuntime_ = nullptr;          // 下发运行时桥（PIMPL；C++17 TU 安全）
    // 【StatusContainer 集成 2026-11-12 第四步】参照旧 TimeLineWidget 的
    // TimelineBindingsController 模式：时间轴 OSC 地址树（/timeline/master/
    // ...）注册进 StatusContainer/GlobalEventBus——外部 OSC/MQTT/HTTP 控制
    // play/pause/stop/seek，播放态与播放头帧号经 statusUpdated 出站反馈。
    // tick 定时器成员化（槽方法与工具栏 lambda 同源复用）。
    QTimer* msPlayTimer_ = nullptr;
    // 【场景自动追平 2026-11-13】sceneEdited（时间轴/画布/属性面板）→
    // 节流 200ms（单参数）：首次编辑立即发、编辑中每 200ms 发一拍（当下
    // 状态）、停手由挂起拍兜底——编辑过程 display 持续跟手，无"停手才发"
    // 的不连续感。收编恢复链路（原 EditorWindow 有此行为，收编时只留
    // 注释丢了实现）——修复"每次动作都要手动下发场景才生效"。播放中
    // 发布不打断播放（display 换场景不重置会话时钟，2026-11-13 实测）；
    // 0 端在线 = 零成本合法。display 渲染端实测 50Hz 发布满速合成（单帧
    // 0.1~0.7ms）、发送线程零节流——200ms 节流速率零压力。
    QTimer* msAutoPublishTimer_ = nullptr;
    // 节流间隔（ms）——构造时读 Settings.ini Network/SceneAutoPublishMs
    //（默认 200，钳制 20-2000；改 ini 重启生效）
    int msAutoPublishIntervalMs_ = 200;
    // 上次发布时刻（节流基准：距此满间隔才发下一拍）
    QElapsedTimer msLastPublishSince_;
    // sceneEdited 三路统一入口（节流决策，见 cpp 实现）
    void requestMsAutoPublish();
    // 立即发布（syncModelToWorkScene + publishScene；节流到点与立即路径
    // 共用，末尾打点节流基准）
    void publishMsSceneNow();
    // 上一拍在线显示端集合（msUiTimer 拍内对比检出"新上线"→ 自动追平下发）
    std::set<std::string> msLastConnected_;
    // osc 日历控件
    ScheduledTaskWidget *scheduledTaskWidget=nullptr;
    // 媒体库
    MediaLibraryWidget *mediaLibraryWidget=nullptr;
    //节点视图
    DataflowViewsManger *dataflowViewsManger=nullptr;
    // http 服务器
    Flow::NodeHttpServer *httpServer=nullptr;
Q_SIGNALS:
    //初始化状态信号
    void initStatus(const QString &message);
public Q_SLOTS:
    /**
     * @brief 授权状态变化后刷新应用图标（未授权红色警示，授权后品牌绿）
     * 函数级注释：同步 QApplication 图标、窗口图标与自定义标题栏的图标按钮。
     */
    void refreshAppIcon();
    /**
     * @brief 初始化主窗口与各功能模块
     * 函数级注释：创建 DockManager 与各 DockWidget，初始化菜单栏、日志、节点编辑器、时间线、舞台、媒体库、外部控制与 HTTP 服务等。
     */
    void init();
    /**
     * 更新默认可视化布局状态
     */
    void updateVisualState();
    /**
     * 恢复默认可视布局
     */
    void resetVisualState();
    /**
     * 锁定切换
     */
    void locked_switch();
    /**
     * 初始化正常模式，显示所有界面
     */
    /**
     * 保存文件到资源管理器
     */
    void saveFileToExplorer();
    /**
     * @brief 从路径加载文件
     *
     * @param path 文件路径
     */
    /**
     * @brief 从路径加载 .flow 项目文件
     * @param path 文件路径
     * 函数级注释：启动阶段（尚未加载任何项目）在当前进程中完成加载；
     *            若已加载过项目，则不在当前进程卸载并重建节点，而是触发重启打开新项目，避免第三方节点质量问题导致的内存持续增长。
     */
    void loadFileFromPath(const QString &path);
    /**
     * @brief 应用启动前已确认的加载/恢复结果（Splash 之前弹窗）
     * @param resolution 启动前 resolveProjectLoadPath 的结果
     * @param cmdFilePath 命令行传入的 .flow 路径（可为空）
     */
    void applyStartupLoadResolution(const ProjectLoadResolution& resolution,
                                    const QString& cmdFilePath = QString());
    /**
     * 从资源管理器加载文件
     */
    /**
     * @brief 通过文件对话框选择并打开 .flow 文件
     * 函数级注释：为避免运行时切换项目带来的内存风险，该入口会走“重启打开”。
     */
    void loadFileFromExplorer();
    /**
     * 保存文件到路径
     */
    void saveFileToPath();
    /**
     * 更新菜单栏视图action
     */
    void updateViewMenu(QMenu* menu);

    /**
     * @brief 从系统托盘恢复窗口
     * 函数级注释：托盘图标双击，显示主窗口并隐藏托盘图标。
     */
    void switchVisibleFromTray();


    /**
     * @brief 打开最近文件（菜单项触发）
     * @param path 最近文件路径
     * 函数级注释：统一走“重启打开”，保证与拖拽/文件对话框行为一致。
     */
    void openRecentFile(const QString& path);

    // ---- MediaServer 时间轴 OSC 集成（2026-11-12 第四步）--------------------
    // 参照旧 TimeLineWidget 的 TimelineBindingsController 模式：
    //   工具栏播放三动作 → 统一处理方法（GlobalEventBus 命令同源复用）；
    //   /timeline/master/{play,pause,stop,currentFrame} 注册进全局事件
    //   总线（控制入站）+ StatusContainer（状态出站镜像）。
private:
    /// 播放（工具栏点击与外部 /play 命令同源）：会话广播 + 本地预览 + tick 启动
    void msHandlePlay();
    /// 暂停（同上双源）：会话冻结 + 预览暂停 + tick 停止
    void msHandlePause();
    /// 停止（同上双源）：会话回 Idle + 预览归零 + 播放头回显起点
    void msHandleStop();
    /// 播放态反馈推送（/play /stop /currentFrame → StatusContainer 出站镜像）
    void msPublishPlaybackState();
    /// OSC 绑定入口：订阅命令地址 + 注册控件 + 推送初始状态（构造期一次）
    void msBindTimelineOsc();

public Q_SLOTS:
    /// GlobalEventBus 命令分发入口（/timeline/master/* 订阅槽；旧
    /// TimeLineToolBar::onGlobalEvent 同构：单槽按地址分发 + 回执反馈）
    void msOnTimelineCommand(const GlobalEvent& ev);

protected:
    bool event(QEvent *event) override;
    /**
     * @brief 关闭事件
     *
     * @param event 关闭事件指针
     */
    void closeEvent(QCloseEvent* event) override;
    /**
     * @brief 拖拽进入事件
     *
     * @param event 拖拽进入事件指针
     */
    void dragEnterEvent(QDragEnterEvent *event) override;
    /**
     * @brief 拖拽释放事件
     *
     * @param event 拖拽释放事件指针
     */
    void dropEvent(QDropEvent *event) override;
    /**
     * 添加菜单项
     * @param parent
     * @param actions
     * @return
     */
    QMenu* makeOptionsMenu(QWidget* parent, const QList<QAction*>& actions);

    /** @brief 按配置在启动时应用主题（不支持运行时切换） */
    void applyTheme(bool isDark);

    /** @brief 构建 QWindowKit 无边框标题栏并绑定 WindowAgent */
    void setupFramelessWindow();
    /** @brief 同步无边框标题栏的标题、图标与窗口状态 */
    void syncFramelessWindowState();
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;

    /**
     * @brief 重启进程并在新进程中打开指定 .flow 文件
     * @param path 目标项目文件路径
     * 函数级注释：启动新进程后退出当前进程；重启时跳过托盘最小化确认弹窗；
     *            并在重启前尽量释放 HTTP 端口（若启用）以保证网页可访问。
     */
    void restartAndOpenFlow(const QString& path);

    QJsonObject serializeProject(ProjectSaveOrigin origin = ProjectSaveOrigin::User) const;
    void setupAutosave();
    void finalizeAutosave();
    void onProjectLoaded(const QString& projectPath);
    ProjectLoadResolution resolveRecoveryLoadPath(const QString& requestedPath) const;

private:
    //日志表
    LogWidget *logTable;
    //日志处理器
    LogHandler *log;
    // 当前项目路径：非空表示当前进程已加载过项目（用于判断是否需要走重启打开）
    QString currentProjectPath;
    //外部控制器
    ExternalControler *controller;

    // 新增：系统托盘相关成员
    QSystemTrayIcon* trayIcon = nullptr;
    QMenu* trayMenu = nullptr;
    QAction* trayExitAction = nullptr;
    // 标记当前是否处于“自重启退出”流程；用于绕过 closeEvent 中的“是否最小化到托盘”弹窗
    bool isRestarting = false;
    AutosaveManager* autosaveManager = nullptr;
    std::optional<ProjectLoadResolution> forcedLoadResolution_;
    bool skipRecoveryPrompt_ = false;

    QWK::WidgetWindowAgent *windowAgent = nullptr;  ///< QWindowKit 无边框窗口代理
    QWK::WindowBar *windowBar = nullptr;          ///< 自定义标题栏（菜单 + 标题 + 系统按钮）
};

