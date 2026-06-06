//
// 项目快照与恢复加载实现
//
#include "ProjectSnapshotBuilder.hpp"

#include "Common/AppConfig/ProjectPersistence.h"
#include "Widget/CalendarWidget/ScheduledTaskModel.hpp"
#include "Widget/CalendarWidget/ScheduledTaskWidget.hpp"
#include "Widget/ExternalControl/HttpServer.hpp"
#include "Widget/NodeWidget/DataflowViewsManger.hpp"
#include "Widget/TimeLineWidget/TimeLineModel.h"
#include "Widget/TimeLineWidget/TimeLineWidget.hpp"

#include "DockManager.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QObject>

#include <exception>

/**
 * @brief 从各模块收集数据，组装为 .flow 项目快照 JSON
 * @param sources 数据流、时间轴、计划任务、停靠布局、网页布局等模块指针
 * @return 完整的项目 JSON 对象，供保存与自动保存使用
 * 函数级注释：各模块独立序列化后合并为单一根对象，缺失的模块对应字段可省略或为空。
 */
QJsonObject buildProjectSnapshot(const ProjectSnapshotSources& sources)
{
    QJsonObject flowJson;
    if (sources.dataflowViewsManger) {
        flowJson[QStringLiteral("DataFlow")] = sources.dataflowViewsManger->save();
    }
    if (sources.timeline) {
        flowJson[QStringLiteral("TimeLine")] = sources.timeline->save();
    }
    if (sources.scheduledTaskWidget) {
        flowJson[QStringLiteral("ScheduledTasks")] = sources.scheduledTaskWidget->save();
    } else if (sources.scheduledTaskModel) {
        flowJson[QStringLiteral("ScheduledTasks")] = sources.scheduledTaskModel->toJson();
    }
    if (sources.dockManager) {
        const QByteArray layoutBytes = sources.dockManager->saveState();
        flowJson[QStringLiteral("VisualLayout")]
            = QString::fromLatin1(layoutBytes.toBase64());
    }
    flowJson[QStringLiteral("WebLayout")]
        = sources.httpServer ? sources.httpServer->save() : QJsonObject{};
    return flowJson;
}

/**
 * @brief 将项目快照原子写入指定 .flow 文件
 * @param path 目标文件路径
 * @param json buildProjectSnapshot 生成的项目 JSON
 * @return 写入成功返回 true
 * 函数级注释：先写入临时文件再替换，避免写入中断导致原文件损坏。
 */
bool saveProjectSnapshotAtomic(const QString& path, const QJsonObject& json)
{
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    return ProjectPersistence::saveBytesAtomic(path, data);
}

/**
 * @brief 从路径读取并解析 .flow 文件
 */
ProjectFileReadResult readProjectFile(const QString& path,
                                      const ProjectLoadStatusReporter& reportStatus)
{
    ProjectFileReadResult result;

    const auto report = [&reportStatus](const QString& message) {
        if (reportStatus) {
            reportStatus(message);
        }
    };

    if (path.isEmpty()) {
        result.errorMessage = QObject::tr("Path is empty");
        report(result.errorMessage);
        return result;
    }

    QFileInfo fileInfo(path);
    result.absolutePath = fileInfo.absoluteFilePath().replace('\\', '/');
    report(QObject::tr("Prepare to open: %1").arg(path));
    report(QObject::tr("Analyze path: %1").arg(result.absolutePath));

    QFile file(result.absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QObject::tr("Open file failed: %1").arg(file.errorString());
        report(result.errorMessage);
        return result;
    }

    const qint64 fileSize = file.size();
    if (fileSize > 2147483647LL) {
        result.errorMessage = QObject::tr("File is too large: %1 bytes").arg(fileSize);
        report(result.errorMessage);
        return result;
    }

    report(QObject::tr("Read file: %1").arg(result.absolutePath));

    QByteArray wholeFile;
    wholeFile.reserve(static_cast<int>(qMax<qint64>(0, fileSize)));
    qint64 readBytes = 0;
    const qint64 chunkSize = 1024 * 1024;
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(chunkSize);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            break;
        }
        wholeFile.append(chunk);
        readBytes += chunk.size();

        if (fileSize > 0) {
            int pct = static_cast<int>((readBytes * 100) / fileSize);
            pct = qMin(pct, 100);
            report(QObject::tr("Read file... %1% (%2/%3)").arg(pct).arg(readBytes).arg(fileSize));
        } else {
            report(QObject::tr("Read file... %1 bytes").arg(readBytes));
        }
    }

    if (file.error() != QFile::NoError) {
        result.errorMessage = QObject::tr("Read file failed: %1").arg(file.errorString());
        report(result.errorMessage);
        return result;
    }

    report(QObject::tr("Parse the flow file..."));

    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(wholeFile, &parseError);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        const QString reason = parseError.error == QJsonParseError::NoError
                                   ? QObject::tr("Unknown error")
                                   : parseError.errorString();
        result.errorMessage = QObject::tr("Parse the flow file failed: %1").arg(reason);
        report(result.errorMessage);
        return result;
    }

    result.root = jsonDoc.object();
    result.success = true;
    return result;
}

/**
 * @brief 将已解析的项目 JSON 加载到各模块
 */
bool loadProjectModulesFromJson(const QJsonObject& root,
                                const ProjectLoadTargets& targets,
                                const ProjectLoadOptions& options,
                                const ProjectLoadStatusReporter& reportStatus,
                                const ProjectLoadProgressReporter& dataflowProgress,
                                QString* errorMessage)
{
    const auto report = [&reportStatus](const QString& message) {
        if (reportStatus) {
            reportStatus(message);
        }
    };
    const auto fail = [&](const QString& message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        report(message);
        return false;
    };

    report(QObject::tr("Load the dataflow model ..."));
    if (!targets.dataflowViewsManger) {
        return fail(QObject::tr("DataflowViewsManger is null"));
    }

    QMetaObject::Connection dfConn;
    if (dataflowProgress) {
        dfConn = QObject::connect(targets.dataflowViewsManger,
                                  &DataflowViewsManger::loadProgress,
                                  targets.dataflowViewsManger,
                                  [dataflowProgress](const QString& sceneTitle,
                                                     const QString& phase,
                                                     int current,
                                                     int total) {
                                      dataflowProgress(sceneTitle, phase, current, total);
                                  });
    }

    try {
        const QJsonValue dataFlowValue = root.value(QStringLiteral("DataFlow"));
        if (!dataFlowValue.isObject()) {
            if (dfConn) {
                QObject::disconnect(dfConn);
            }
            return fail(QObject::tr("DataFlow 数据缺失或格式错误"));
        }
        targets.dataflowViewsManger->load(dataFlowValue.toObject());
    } catch (const std::exception& e) {
        if (dfConn) {
            QObject::disconnect(dfConn);
        }
        return fail(QObject::tr("加载 DataFlow 失败: %1").arg(QString::fromUtf8(e.what())));
    } catch (...) {
        if (dfConn) {
            QObject::disconnect(dfConn);
        }
        return fail(QObject::tr("加载 DataFlow 失败"));
    }
    if (dfConn) {
        QObject::disconnect(dfConn);
    }

    report(QObject::tr("Load the timeline model ..."));
    if (!targets.timeline && !targets.timelineModel) {
        return fail(QObject::tr("Timeline is null"));
    }
    try {
        const QJsonValue timelineValue = root.value(QStringLiteral("TimeLine"));
        if (!timelineValue.isObject()) {
            return fail(QObject::tr("TimeLine 数据缺失或格式错误"));
        }
        const QJsonObject timelineJson = timelineValue.toObject();
        if (targets.timeline) {
            targets.timeline->load(timelineJson);
        } else {
            targets.timelineModel->load(timelineJson);
        }
    } catch (const std::exception& e) {
        return fail(QObject::tr("加载 TimeLine 失败: %1").arg(QString::fromUtf8(e.what())));
    } catch (...) {
        return fail(QObject::tr("加载 TimeLine 失败"));
    }

    report(QObject::tr("Load the scheduled tasks model ..."));
    if (!targets.scheduledTaskWidget && !targets.scheduledTaskModel) {
        return fail(QObject::tr("Scheduled task model is null"));
    }
    try {
        const QJsonValue scheduledTasksValue = root.value(QStringLiteral("ScheduledTasks"));
        if (!scheduledTasksValue.isObject()) {
            return fail(QObject::tr("ScheduledTasks 数据缺失或格式错误"));
        }
        const QJsonObject scheduledTasksJson = scheduledTasksValue.toObject();
        if (targets.scheduledTaskWidget) {
            targets.scheduledTaskWidget->load(scheduledTasksJson);
        } else {
            targets.scheduledTaskModel->fromJson(scheduledTasksJson);
        }
    } catch (const std::exception& e) {
        return fail(QObject::tr("加载 ScheduledTasks 失败: %1").arg(QString::fromUtf8(e.what())));
    } catch (...) {
        return fail(QObject::tr("加载 ScheduledTasks 失败"));
    }

    if (options.restoreVisualLayout && targets.dockManager) {
        const QString layoutBase64 = root.value(QStringLiteral("VisualLayout")).toString();
        if (!layoutBase64.isEmpty()) {
            report(QObject::tr("Load the visual layout ..."));
            const QByteArray layoutBytes = QByteArray::fromBase64(layoutBase64.toLatin1());
            targets.dockManager->restoreState(layoutBytes);
        } else if (options.onMissingVisualLayout) {
            options.onMissingVisualLayout();
        }
    }

    if (options.loadWebLayout && targets.httpServer) {
        const QJsonObject webLayout = root.value(QStringLiteral("WebLayout")).toObject();
        if (!webLayout.isEmpty()) {
            report(QObject::tr("Load the web layout ..."));
            targets.httpServer->load(webLayout);
        }
    }

    return true;
}

/**
 * @brief 判断是否应向用户提示恢复自动保存（不弹窗，仅做条件判断）
 * @param requestedPath 请求打开的路径，空表示启动时无命令行文件
 * @return 满足恢复条件时返回 true
 * 函数级注释：上次非正常退出、存在有效 recovery 且原文件不比 recovery 更新时才提示。
 */
bool shouldOfferRecovery(const QString& requestedPath)
{
    if (ProjectPersistence::wasLastShutdownClean()) {
        return false;
    }

    const QString normalizedRequested = ProjectPersistence::normalizeProjectPath(requestedPath);
    const RecoveryMeta meta = normalizedRequested.isEmpty()
                                  ? ProjectPersistence::readRecovery()
                                  : ProjectPersistence::findRecoveryForProject(normalizedRequested);
    if (!meta.valid) {
        return false;
    }

    const QFileInfo recoveryInfo(meta.recoveryFilePath);
    if (!recoveryInfo.exists()) {
        return false;
    }

    // 磁盘上的原文件比自动保存更新，则不提示恢复
    if (!normalizedRequested.isEmpty()) {
        const QFileInfo originalInfo(normalizedRequested);
        if (originalInfo.exists()
            && originalInfo.lastModified() >= recoveryInfo.lastModified()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 解析启动或打开项目时应实际加载的文件路径
 * @param requestedPath 用户请求打开的路径，空表示启动时无命令行文件
 * @param parentForDialog 恢复对话框的父窗口，可为 nullptr
 * @param policy Dialog 弹窗询问；AutoRestore 无头模式直接恢复
 * @return 实际加载路径、逻辑项目路径及是否来自 recovery
 * 函数级注释：根据 shouldOfferRecovery 决定是否弹窗；用户选「是」则加载 autosave.flow，
 *            选「否」则使用原 .flow 文件。
 */
ProjectLoadResolution resolveProjectLoadPath(const QString& requestedPath,
                                             QWidget* parentForDialog,
                                             RecoveryPromptPolicy policy)
{
    ProjectLoadResolution result;
    result.loadPath = requestedPath;
    result.projectPath = requestedPath;

    if (!shouldOfferRecovery(requestedPath)) {
        return result;
    }

    const QString normalizedRequested = ProjectPersistence::normalizeProjectPath(requestedPath);
    const RecoveryMeta meta = normalizedRequested.isEmpty()
                                  ? ProjectPersistence::readRecovery()
                                  : ProjectPersistence::findRecoveryForProject(normalizedRequested);

    const QString normalizedMetaPath = ProjectPersistence::normalizeProjectPath(meta.projectPath);

    if (policy == RecoveryPromptPolicy::AutoRestore) {
        qWarning() << "Auto-restoring project from recovery:" << meta.recoveryFilePath;
        result.loadPath = meta.recoveryFilePath;
        result.projectPath = normalizedMetaPath.isEmpty() ? normalizedRequested : normalizedMetaPath;
        result.fromRecovery = true;
        return result;
    }

    const QString savedAtText = meta.savedAt.toLocalTime().toString(
        QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    const QMessageBox::StandardButton reply = QMessageBox::question(
        parentForDialog,
        QObject::tr("恢复自动保存"),
        QObject::tr("检测到上次异常退出。\n\n"
                    "自动保存时间：%1\n"
                    "原项目路径：%2\n\n"
                    "是否恢复自动保存的版本？")
            .arg(savedAtText,
                 meta.projectPath.isEmpty() ? QObject::tr("（未命名项目）") : meta.projectPath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (reply == QMessageBox::Yes) {
        result.loadPath = meta.recoveryFilePath;
        result.projectPath = normalizedMetaPath.isEmpty() ? normalizedRequested : normalizedMetaPath;
        result.fromRecovery = true;
        return result;
    }
    // “否”：使用原项目文件
    return result;
}
