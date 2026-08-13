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
class TimelineWidget;
class TimeLineModel;
namespace ads { class CDockManager; }
namespace Flow { class NodeHttpServer; }

/**
 * @brief 构建项目快照所需的各模块指针
 */
struct ProjectSnapshotSources {
    DataflowViewsManger* dataflowViewsManger = nullptr;
    TimelineWidget* timeline = nullptr;
    ScheduledTaskWidget* scheduledTaskWidget = nullptr;
    ScheduledTaskModel* scheduledTaskModel = nullptr;
    ads::CDockManager* dockManager = nullptr;
    Flow::NodeHttpServer* httpServer = nullptr;
};

/** @brief 从各模块收集 DataFlow / TimeLine / 布局等，组装为 .flow JSON */
QJsonObject buildProjectSnapshot(const ProjectSnapshotSources& sources);
/** @brief 原子写入用户指定的 .flow 文件 */
bool saveProjectSnapshotAtomic(const QString& path, const QJsonObject& json);

/**
 * @brief 加载项目 JSON 时写入的目标模块
 */
struct ProjectLoadTargets {
    DataflowViewsManger* dataflowViewsManger = nullptr;
    TimelineWidget* timeline = nullptr;
    TimeLineModel* timelineModel = nullptr;
    ScheduledTaskWidget* scheduledTaskWidget = nullptr;
    ScheduledTaskModel* scheduledTaskModel = nullptr;
    ads::CDockManager* dockManager = nullptr;
    Flow::NodeHttpServer* httpServer = nullptr;
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
