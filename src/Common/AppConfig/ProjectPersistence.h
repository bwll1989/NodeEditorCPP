//
// 项目恢复文件持久化：固定 autosave.flow / autosave.meta.json，原子写入与异常退出检测
//
#pragma once

#include "ConfigManager.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>

/**
 * @brief 自动保存恢复元数据
 */
struct RecoveryMeta {
    QString projectPath;       ///< 原项目 .flow 路径（未命名项目为空）
    QString recoveryFilePath;  ///< 恢复文件路径（autosave.flow）
    QDateTime savedAt;         ///< 自动保存时间
    bool valid = false;        ///< 元数据与恢复文件是否均有效
};

/**
 * @brief 崩溃恢复相关文件的读写与关机状态管理
 *
 * 恢复目录：文档/Flow/Recovery/
 * - autosave.flow：项目快照（覆盖更新）
 * - autosave.meta.json：原路径、保存时间等元数据
 */
class APPCONFIG_EXPORT ProjectPersistence {
public:
    /** @brief 将路径规范为绝对路径，统一使用正斜杠 */
    static QString normalizeProjectPath(const QString& projectPath);
    /** @brief 自动保存项目文件路径 */
    static QString autosaveFlowPath();
    /** @brief 自动保存元数据文件路径 */
    static QString autosaveMetaPath();
    /** @brief 判断给定路径是否为 autosave.flow */
    static bool isAutosavePath(const QString& path);

    /** @brief 原子写入：先写 .tmp，再 rename 替换目标文件 */
    static bool saveBytesAtomic(const QString& path, const QByteArray& data);
    /** @brief 写入 autosave.meta.json */
    static bool writeRecoveryMeta(const QString& projectPath, const QDateTime& savedAt);
    /** @brief 读取当前恢复元数据 */
    static RecoveryMeta readRecovery();
    /** @brief 读取恢复数据，且原项目路径与请求路径一致（未命名项目不校验路径） */
    static RecoveryMeta findRecoveryForProject(const QString& projectPath);
    /** @brief 删除 autosave.flow 与 autosave.meta.json */
    static void removeRecovery();
    /** @brief 确保恢复目录存在 */
    static void ensureRecoveryDir();

    /**
     * @brief 应用启动时调用：缓存上次是否正常退出，并标记本次为“尚未正常退出”
     */
    static void onApplicationStartup();
    /** @brief 正常退出时调用，标记 LastShutdownClean=true */
    static void markCleanShutdown();
    /** @brief 上次进程是否正常退出（启动时缓存，不受本次 onApplicationStartup 影响） */
    static bool wasLastShutdownClean();
};
