//
// Created by bwll1 on 2024/9/19.
//

#include "LogHandler.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMetaObject>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVector>
#include <algorithm>
#include <mutex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

#include "../../Common/AppConfig/ConfigManager.h"
#include "../../Common/AppConfig/ConstantDefines.h"

namespace {

QIcon iconForLevel(const QString &level)
{
    if (level == QStringLiteral("Debug")) {
        return QIcon(QStringLiteral(":/icons/icons/debug.png"));
    }
    if (level == QStringLiteral("Warn")) {
        return QIcon(QStringLiteral(":/icons/icons/warn.png"));
    }
    if (level == QStringLiteral("Critical")) {
        return QIcon(QStringLiteral(":/icons/icons/trace.png"));
    }
    if (level == QStringLiteral("Info")) {
        return QIcon(QStringLiteral(":/icons/icons/info.png"));
    }
    if (level == QStringLiteral("Fatal")) {
        return QIcon(QStringLiteral(":/icons/icons/critical.png"));
    }
    return {};
}

} // namespace

std::shared_ptr<spdlog::logger> LogHandler::logger = nullptr;
LogWidget *LogHandler::logTableWidget = nullptr;

void LogHandler::writeToLogger(QtMsgType type, const std::string &logMsg)
{
    if (!logger) {
        return;
    }

    switch (type) {
    case QtDebugMsg:
        logger->debug(logMsg);
        break;
    case QtWarningMsg:
        logger->warn(logMsg);
        break;
    case QtCriticalMsg:
        logger->critical(logMsg);
        break;
    case QtInfoMsg:
        logger->info(logMsg);
        break;
    case QtFatalMsg:
        logger->critical(logMsg);
        break;
    }
}

LogHandler::LogHandler(LogWidget *tableWidget)
{
    LogHandler::logTableWidget = tableWidget;
    if (initLogHandler()) {
        qInstallMessageHandler(LogHandler::customMessageHandler);
    }
}

LogHandler::~LogHandler()
{
    qInstallMessageHandler(nullptr);
    if (logger) {
        try {
            logger->flush();
        } catch (...) {
        }
    }
}

void LogHandler::pruneStoredLogFiles()
{
    const int maxFiles = qMax(1, ConfigManager::instance().getMaxLogSaveEntries());
    QDir dir(AppConstants::LOGS_STORAGE_DIR);
    if (!dir.exists()) {
        return;
    }

    struct LogFileInfo {
        QString path;
        QDateTime modified;
    };

    QVector<LogFileInfo> files;
    const QStringList names = dir.entryList(QStringList{QStringLiteral("log*.txt")}, QDir::Files);
    files.reserve(names.size());
    for (const QString &name : names) {
        LogFileInfo info;
        info.path = dir.absoluteFilePath(name);
        info.modified = QFileInfo(info.path).lastModified();
        files.push_back(std::move(info));
    }

    if (files.size() <= maxFiles) {
        return;
    }

    std::sort(files.begin(), files.end(), [](const LogFileInfo &a, const LogFileInfo &b) {
        return a.modified < b.modified;
    });

    const int toRemove = files.size() - maxFiles;
    for (int i = 0; i < toRemove; ++i) {
        QFile::remove(files[i].path);
    }
}

bool LogHandler::initLogHandler()
{
    try {
        QDir().mkpath(AppConstants::LOGS_STORAGE_DIR);
        pruneStoredLogFiles();

        const std::string logFilePath =
            (AppConstants::LOGS_STORAGE_DIR + QStringLiteral("/log.txt")).toStdString();

        if (auto existing = spdlog::get("logger")) {
            logger = existing;
        } else {
            auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFilePath, 0, 0);
            logger = std::make_shared<spdlog::logger>("logger", dailySink);
            spdlog::register_logger(logger);
        }

        spdlog::set_default_logger(logger);
        logger->set_level(spdlog::level::debug);
        // 全量落盘：每条 debug 也立即 flush，保证信息尽量不丢
        logger->flush_on(spdlog::level::debug);
        // 写失败不抛到业务/QML（记事本长时间占用日志文件时只丢本条）
        spdlog::set_error_handler([](const std::string &) {});
        logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

        return true;
    } catch (const spdlog::spdlog_ex &ex) {
        qWarning() << "Failed to initialize log handler with spdlog exception:" << ex.what();
        return false;
    } catch (const std::exception &ex) {
        qWarning() << "Failed to initialize log handler with std exception:" << ex.what();
        return false;
    }
}

void LogHandler::customMessageHandler(QtMsgType type,
                                      const QMessageLogContext &context,
                                      const QString &msg)
{
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    const QString logMessage = QStringLiteral("[%1:%2] %3")
                                   .arg(context.function)
                                   .arg(context.line)
                                   .arg(msg);

    QString level;
    switch (type) {
    case QtDebugMsg:
        level = QStringLiteral("Debug");
        break;
    case QtWarningMsg:
        level = QStringLiteral("Warn");
        break;
    case QtCriticalMsg:
        level = QStringLiteral("Critical");
        break;
    case QtInfoMsg:
        level = QStringLiteral("Info");
        break;
    case QtFatalMsg:
        level = QStringLiteral("Fatal");
        break;
    }

    try {
        writeToLogger(type, logMessage.toStdString());
    } catch (const std::exception &) {
        // 文件被占用、磁盘满等：丢弃本条，保证进程继续
    } catch (...) {
    }

    appendLogToTable(timestamp, level, logMessage);
}

void LogHandler::appendLogToTable(const QString &timestamp,
                                  const QString &level,
                                  const QString &logMessage)
{
    if (!logTableWidget) {
        return;
    }

    QMetaObject::invokeMethod(
        logTableWidget,
        [timestamp, level, logMessage]() {
            const QString currentFilter = logTableWidget->logFilter();
            if (currentFilter != QStringLiteral("All") && currentFilter != level) {
                return;
            }

            const QRegularExpression regex(QStringLiteral("\\[|\\]"));
            const QStringList logParts = logMessage.split(regex, Qt::SkipEmptyParts);

            const int maxEntries = qMax(1, ConfigManager::instance().getMaxLogEntries());
            while (logTableWidget->rowCount() >= maxEntries) {
                logTableWidget->removeRow(0);
            }

            const int rowCount = logTableWidget->rowCount();
            logTableWidget->insertRow(rowCount);
            logTableWidget->setItem(rowCount, 0, new QTableWidgetItem(timestamp));

            auto *levelItem = new QTableWidgetItem(iconForLevel(level), level);
            logTableWidget->setItem(rowCount, 1, levelItem);

            const int colCount = logTableWidget->columnCount();
            for (int i = 0; i < logParts.size() && i + 2 < colCount; ++i) {
                logTableWidget->setItem(rowCount, i + 2, new QTableWidgetItem(logParts[i].trimmed()));
            }

            static int lastScrollRow = -1;
            if (lastScrollRow != rowCount) {
                logTableWidget->scrollToBottom();
                lastScrollRow = rowCount;
            }
        },
        Qt::QueuedConnection);
}
