//
// Created by bwll1 on 2024/9/19.
//

#include "LogHandler.hpp"
#include "spdlog/sinks/rotating_file_sink.h"

#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QHeaderView>
#include <QDebug>
#include <QMutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <mutex>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>
#include "ConstantDefines.h"
LogHandler::LogHandler(LogWidget *tableWidget) {
    // 设置静态成员
    LogHandler::logTableWidget = tableWidget;
    // 安装自定义的消息处理器
    if(initLogHandler()){
        qInstallMessageHandler(LogHandler::customMessageHandler);
    }
}

LogHandler::~LogHandler() {
    // 解除消息处理器
    qInstallMessageHandler(nullptr);
}

bool LogHandler::initLogHandler() {
    // 初始化 spdlog 日志器
    try{
        QDir().mkpath(AppConstants::LOGS_STORAGE_DIR);
        std::string logFilePath = (AppConstants::LOGS_STORAGE_DIR.toStdString()+"/log.txt");
        
        // 使用rotating_file_sink替代daily_file_sink，可以限制日志文件大小和数量
        auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFilePath, 0, 0);
        logger = std::make_shared<spdlog::logger>("logger", dailySink);
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        logger->set_level(spdlog::level::debug);  // 设置日志级别
        logger->flush_on(spdlog::level::debug);   // 确保每条日志立即写入
        logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");// 设置日志格式，包含文件名和行号

        return true;
    }
    catch (const spdlog::spdlog_ex& ex) {
        QMessageBox::warning(nullptr, "日志初始化失败", QString("无法初始化日志系统：%1").arg(ex.what()));
        return false;
    }
}

void LogHandler::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);  // 确保多线程安全
    
    // 生成时间戳
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    // 提取debug信息中的函数名、文件名和行号
    QString logMessage = QString("[%1:%2] %3").arg(context.function)
            .arg(context.line)
            .arg(msg);
    std::string log_msg = logMessage.toStdString();
    QString level;
    QIcon icon;
    
    // 将日志重定向到 spdlog
    switch (type) {
        case QtDebugMsg:
            logger->debug(log_msg);
            level="Debug";
            icon=QIcon(":/icons/icons/debug.png");
            break;
        case QtWarningMsg:
            logger->warn(log_msg);
            level="Warn";
            icon=QIcon(":/icons/icons/warn.png");
            break;
        case QtCriticalMsg:
            logger->critical(log_msg);
            level="Critical";
            icon=QIcon(":/icons/icons/trace.png");
            break;
        case QtInfoMsg:
            logger->info(log_msg);
            level="Info";
            icon=QIcon(":/icons/icons/info.png");
            break;
        case QtFatalMsg:
            logger->critical(log_msg);
            level="Fatal";
            icon=QIcon(":/icons/icons/critical.png");
            // abort();
    }

    // 将日志显示在 QTableWidget 中
    appendLogToTable(timestamp, level, icon, logMessage);
}

// 追加日志到 QTableWidget
void LogHandler::appendLogToTable(const QString &timestamp, const QString &level, const QIcon &icon, const QString &logMessage) {
    if (!logTableWidget) return;
    
    // 将 UI 更新操作转移到主线程
    QMetaObject::invokeMethod(logTableWidget, [=]() {
        // 检查是否需要应用过滤器
        QString currentFilter = logTableWidget->property("currentFilter").toString();
        if (!currentFilter.isEmpty() && currentFilter != "All" && currentFilter != level) {
            return; // 如果不符合过滤条件，不添加此日志
        }
        
        // 解析 spdlog 格式的日志信息，按 [] 分隔
        QRegularExpression regex("\\[|\\]");
        QStringList logParts = logMessage.split(regex, Qt::SkipEmptyParts);
        
        // 检查是否需要删除旧日志条目以保持在最大条目数限制内
        int maxEntries = AppConstants::MaxLogEntries;
        while (logTableWidget->rowCount() >= maxEntries) {
            logTableWidget->removeRow(0); // 删除最早的日志条目
        }
        
        // 插入新行并填充数据
        int rowCount = logTableWidget->rowCount();
        logTableWidget->insertRow(rowCount);
        
        // 使用setItem而不是每次创建新的QTableWidgetItem，减少内存分配
        logTableWidget->setItem(rowCount, 0, new QTableWidgetItem(timestamp));  // 第一列为时间戳
        
        QTableWidgetItem *levelItem = new QTableWidgetItem(icon, level);
        logTableWidget->setItem(rowCount, 1, levelItem);  // 第二列为类型
        
        // 优化循环，减少不必要的条件检查
        int colCount = logTableWidget->columnCount();
        for (int i = 0; i < logParts.size() && i + 2 < colCount; ++i) {
            logTableWidget->setItem(rowCount, i + 2, new QTableWidgetItem(logParts[i].trimmed()));
        }
        
        // 仅在添加新日志时滚动到底部，避免频繁重绘
        static int lastScrollRow = -1;
        if (lastScrollRow != rowCount) {
            logTableWidget->scrollToBottom();
            lastScrollRow = rowCount;
        }
    }, Qt::QueuedConnection);
}

// 静态成员的初始化
std::shared_ptr<spdlog::logger> LogHandler::logger = nullptr;
LogWidget* LogHandler::logTableWidget = nullptr;



