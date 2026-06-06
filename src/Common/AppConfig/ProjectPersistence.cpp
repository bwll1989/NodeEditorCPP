//
// 项目恢复文件持久化实现
//
#include "ProjectPersistence.h"

#include "ConstantDefines.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace {

/** @brief Settings.ini 路径（与 ConfigManager 共用，Recovery 段存关机标志） */
QString recoverySettingsPath()
{
    return AppConstants::RECENT_FILES_STORAGE_DIR + QStringLiteral("/Settings.ini");
}

/** @brief 上一次启动前进程是否正常退出（onApplicationStartup 时从配置读出并缓存） */
bool g_previousShutdownWasClean = true;

} // namespace

QString ProjectPersistence::normalizeProjectPath(const QString& projectPath)
{
    if (projectPath.isEmpty()) {
        return QString();
    }
    return QFileInfo(projectPath).absoluteFilePath().replace(QLatin1Char('\\'), QLatin1Char('/'));
}

QString ProjectPersistence::autosaveFlowPath()
{
    ensureRecoveryDir();
    return AppConstants::RECOVERY_STORAGE_DIR + QStringLiteral("/autosave.flow");
}

QString ProjectPersistence::autosaveMetaPath()
{
    ensureRecoveryDir();
    return AppConstants::RECOVERY_STORAGE_DIR + QStringLiteral("/autosave.meta.json");
}

bool ProjectPersistence::isAutosavePath(const QString& path)
{
    const QString normalized = normalizeProjectPath(path);
    if (normalized.isEmpty()) {
        return false;
    }
    return normalized == normalizeProjectPath(autosaveFlowPath());
}

bool ProjectPersistence::saveBytesAtomic(const QString& path, const QByteArray& data)
{
    // 写入临时文件，避免崩溃时留下半截文件
    const QString tmpPath = path + QStringLiteral(".tmp");
    QFile tmp(tmpPath);
    if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    if (tmp.write(data) != data.size()) {
        tmp.close();
        QFile::remove(tmpPath);
        return false;
    }
    tmp.flush();
    tmp.close();

    if (QFile::exists(path) && !QFile::remove(path)) {
        QFile::remove(tmpPath);
        return false;
    }
    if (!QFile::rename(tmpPath, path)) {
        QFile::remove(tmpPath);
        return false;
    }
    return true;
}

bool ProjectPersistence::writeRecoveryMeta(const QString& projectPath, const QDateTime& savedAt)
{
    QJsonObject meta;
    meta[QStringLiteral("projectPath")] = projectPath;
    meta[QStringLiteral("savedAt")] = savedAt.toString(Qt::ISODateWithMs);
    meta[QStringLiteral("appVersion")] = QStringLiteral(PRODUCT_VERSION);

    const QByteArray bytes = QJsonDocument(meta).toJson(QJsonDocument::Compact);
    return saveBytesAtomic(autosaveMetaPath(), bytes);
}

RecoveryMeta ProjectPersistence::readRecovery()
{
    RecoveryMeta meta;
    meta.recoveryFilePath = autosaveFlowPath();

    QFile file(autosaveMetaPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return meta;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return meta;
    }

    const QJsonObject obj = doc.object();
    meta.projectPath = obj.value(QStringLiteral("projectPath")).toString();
    meta.savedAt = QDateTime::fromString(obj.value(QStringLiteral("savedAt")).toString(),
                                           Qt::ISODateWithMs);
    meta.valid = QFile::exists(meta.recoveryFilePath) && meta.savedAt.isValid();
    return meta;
}

RecoveryMeta ProjectPersistence::findRecoveryForProject(const QString& projectPath)
{
    RecoveryMeta meta = readRecovery();
    if (!meta.valid) {
        return {};
    }

    // 打开指定项目时，自动保存必须属于同一逻辑路径
    const QString normalized = normalizeProjectPath(projectPath);
    const QString metaPath = normalizeProjectPath(meta.projectPath);
    if (!normalized.isEmpty() && !metaPath.isEmpty() && normalized != metaPath) {
        return {};
    }
    return meta;
}

void ProjectPersistence::removeRecovery()
{
    QFile::remove(autosaveFlowPath());
    QFile::remove(autosaveMetaPath());
}

void ProjectPersistence::ensureRecoveryDir()
{
    QDir().mkpath(AppConstants::RECOVERY_STORAGE_DIR);
}

void ProjectPersistence::onApplicationStartup()
{
    QSettings settings(recoverySettingsPath(), QSettings::IniFormat);
    // 先读出“上次是否干净退出”，再标记本次为未干净退出
    g_previousShutdownWasClean =
        settings.value(QStringLiteral("Recovery/LastShutdownClean"), true).toBool();
    settings.setValue(QStringLiteral("Recovery/LastShutdownClean"), false);
    settings.sync();
}

void ProjectPersistence::markCleanShutdown()
{
    QSettings settings(recoverySettingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("Recovery/LastShutdownClean"), true);
    settings.sync();
}

bool ProjectPersistence::wasLastShutdownClean()
{
    return g_previousShutdownWasClean;
}
