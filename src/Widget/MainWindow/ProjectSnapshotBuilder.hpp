//
// 项目快照序列化、手动保存与崩溃恢复加载解析
//
#pragma once

#include <QJsonObject>
#include <QString>
#include <functional>

class DataflowViewsManger;
class ScheduledTaskWidget;
class ScheduledTaskModel;
// 【2026-11-12】旧时间轴（TimelineWidget/TimeLineModel）退役——
// .flow 快照不再包含旧 TimeLine 段（用户拍板：不做旧文件兼容）。
// 【2026-11-12 第二步】新 MediaServer 时间轴工程序列化接入：快照携带
// "mediaTimeline" 段（ms::Scene pb 的 base64——pb 转换在 C++20 侧
// MsRuntimeBridge，本头仅前置声明）。
namespace ads { class CDockManager; }
namespace Flow { class NodeHttpServer; }
namespace ms { struct Scene; class MsRuntimeBridge; }

/**
 * @brief 构建项目快照所需的各模块指针
 */
struct ProjectSnapshotSources {
    DataflowViewsManger* dataflowViewsManger = nullptr;
    ScheduledTaskWidget* scheduledTaskWidget = nullptr;
    ScheduledTaskModel* scheduledTaskModel = nullptr;
    ads::CDockManager* dockManager = nullptr;
    Flow::NodeHttpServer* httpServer = nullptr;
    ms::Scene* msScene = nullptr;   ///< MediaServer 时间轴工作副本（可空）
    // 【2026-11-12 统一入口】显示端登记桥（可空）：非空时快照携带
    // "mediaDisplays" 段（登记态 id/host/port/enabled/pairingCode），
    // 使 .flow 一个文件恢复"节点图 + 场景 + 显示端登记"全部工作状态。
    ms::MsRuntimeBridge* msRuntime = nullptr;
};

/** @brief 项目快照写入来源（写入 meta.savedBy，便于追溯） */
enum class ProjectSaveOrigin {
    User,     ///< 用户手动保存到 .flow
    Autosave  ///< 周期性/退出时的 recovery 快照
};

/** @brief 从各模块收集 DataFlow / TimeLine / 布局等，组装为 .flow JSON */
QJsonObject buildProjectSnapshot(const ProjectSnapshotSources& sources,
                                 ProjectSaveOrigin origin = ProjectSaveOrigin::User);
/** @brief 原子写入用户指定的 .flow 文件 */
bool saveProjectSnapshotAtomic(const QString& path, const QJsonObject& json);

/**
 * @brief 加载项目 JSON 时写入的目标模块
 */
struct ProjectLoadTargets {
    DataflowViewsManger* dataflowViewsManger = nullptr;
    ScheduledTaskWidget* scheduledTaskWidget = nullptr;
    ScheduledTaskModel* scheduledTaskModel = nullptr;
    ads::CDockManager* dockManager = nullptr;
    Flow::NodeHttpServer* httpServer = nullptr;
    ms::Scene* msScene = nullptr;   ///< MediaServer 时间轴工作副本（可空；
                                    ///< 非空时 "mediaTimeline" 段反解入此）
    // 【2026-11-12 统一入口】显示端登记回灌落点（可空；headless 无桥
    // 实例传空即跳过段）。非空时 "mediaDisplays" 段先清空现有登记再
    // 逐条恢复（addDisplay + setDisplayEnabled）。
    ms::MsRuntimeBridge* msRuntime = nullptr;
};

/**
 * @brief 加载项目 JSON 时的行为选项
 */
struct ProjectLoadOptions {
    bool restoreVisualLayout = true;
    bool loadWebLayout = true;
    std::function<void()> onMissingVisualLayout;
};

/**
 * @brief 从磁盘读取 .flow 文件的结果
 */
struct ProjectFileReadResult {
    bool success = false;
    QString absolutePath;
    QJsonObject root;
    QString errorMessage;
};

using ProjectLoadStatusReporter = std::function<void(const QString&)>;
using ProjectLoadProgressReporter =
    std::function<void(const QString& sceneTitle, const QString& phase, int current, int total)>;

/** @brief 从路径读取并解析 .flow 文件 */
ProjectFileReadResult readProjectFile(const QString& path,
                                      const ProjectLoadStatusReporter& reportStatus = {});

/** @brief 将已解析的项目 JSON 加载到各模块 */
bool loadProjectModulesFromJson(const QJsonObject& root,
                                const ProjectLoadTargets& targets,
                                const ProjectLoadOptions& options,
                                const ProjectLoadStatusReporter& reportStatus,
                                const ProjectLoadProgressReporter& dataflowProgress = {},
                                QString* errorMessage = nullptr);

/**
 * @brief 打开项目时的路径解析结果
 */
struct ProjectLoadResolution {
    QString loadPath;      ///< 实际读取的文件路径（可能是 autosave.flow）
    QString projectPath;   ///< 逻辑项目路径（保存/Ctrl+S 使用）
    bool fromRecovery = false;
};

/**
 * @brief 判断是否应提示恢复（不弹窗）
 * @param requestedPath 请求打开的路径，空表示启动时无命令行文件
 */
bool shouldOfferRecovery(const QString& requestedPath);

/**
 * @brief 恢复提示策略
 */
enum class RecoveryPromptPolicy {
    Dialog,       ///< 弹出是/否对话框（parent 可为 nullptr）
    AutoRestore   ///< 不弹窗，直接采用自动保存（无头模式）
};

/**
 * @brief 解析应加载的路径
 */
ProjectLoadResolution resolveProjectLoadPath(const QString& requestedPath,
                                             QWidget* parentForDialog = nullptr,
                                             RecoveryPromptPolicy policy = RecoveryPromptPolicy::Dialog);
