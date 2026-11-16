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

// ---- MediaServer 编辑器组件 -------------------------------------------------
namespace ms {
struct Scene;                // core/model（header-only；struct——与定义同形防 C4099）
class PreviewRenderer;       // editor/（本地预览渲染器）
class QtTimelineModelAdapter;// editor/（Scene↔帧模型适配器）
class SceneCanvasWidget;     // editor/（本地预览画布，用于装载本地预览渲染器渲染的场景）
class MsRuntimeBridge;       // timeline/（EditorRuntime 桥接壳；PIMPL 隔离重依赖头图）
class TimelineAudioBusHost;  // timeline/（timeline 混音写入宿主；非 QObject）
class TimelinePlaybackController; // timeline/（播放总控，用于控制播放、暂停、停止、seek等操作）
}
class BaseTimelineWidget;    // MediaServer 收编版 QtTimeLine 控件，用于显示时间轴

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
    // ---- MediaServer 编辑器组件 ----
    ms::Scene* msScene_ = nullptr;                      // 工作副本（ms 默认空场景）
    ms::PreviewRenderer* msPreview_ = nullptr;          // 预览渲染器（空素材根=全占位降级）
    ms::QtTimelineModelAdapter* msTimelineAdapter_ = nullptr; // 时间轴适配器（画布/时间轴适配器共享同一实例）
    ms::SceneCanvasWidget* msCanvas_ = nullptr;         // 预览画布（用于装载本地预览渲染器渲染的场景）
    // 场景树已移除：徽标/场景动作/银幕管理分流至画布与 timeline 工具栏。
    BaseTimelineWidget* msTimeline_ = nullptr;          // 时间轴（用于显示时间轴）
    ms::MsRuntimeBridge* msRuntime_ = nullptr;          // 下发运行时桥（PIMPL；本 TU 只见面头）
    // 【timeline 音频 bus】runtime 混音写入宿主：按轨道分 bus（轨道 N → 轨道 N+1）经注册表环形队列进节点图（TimeLineAudioBus 节点读）。
    ms::TimelineAudioBusHost* msAudioBusHost_ = nullptr;
    // TimelinePlaybackController 工具栏电平显示、播放控制、播放头帧号反馈、播放状态反馈。
    ms::TimelinePlaybackController* msPlayback_ = nullptr; // 播放总控（非拥有；构造点在 msAudioBusHost_ 就绪后）
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

