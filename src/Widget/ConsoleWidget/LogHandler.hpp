//
// Created by bwll1 on 2024/9/19.
//

#pragma once

#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>
#include "QTableWidget"
#include <QHeaderView>
#include <QDebug>
#include <QMutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <mutex>
#include <string>
#include "LogWidget.hpp"

class LogHandler {
public:
    LogHandler(LogWidget *tableWidget);

    ~LogHandler();

    /**
     * 初始化日志处理器
     * @return bool 是否初始化成功
     */
    bool initLogHandler();
    
    /**
     * 自定义消息处理器
     * @param QtMsgType type 消息类型
     * @param const QMessageLogContext &context 消息上下文
     * @param const QString &msg 消息内容
     */
    static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    /**
     * 追加日志到表格（图标在 GUI 线程按 level 解析，避免工作线程创建 QIcon）
     */
    static void appendLogToTable(const QString &timestamp, const QString &level, const QString &logMessage);

    /** @brief 启动时按配置裁剪磁盘上多余的日志文件（删最早日期的 txt） */
    static void pruneStoredLogFiles();

    /** @brief 写入 spdlog（调用方负责吞掉写失败异常） */
    static void writeToLogger(QtMsgType type, const std::string &logMsg);

    /**
     * spdlog 日志器（单文件按天滚动：log_YYYY-MM-DD.txt）
     */
    static std::shared_ptr<spdlog::logger> logger;
    
    /**
     * 日志表格
     */
    static LogWidget *logTableWidget;
};
