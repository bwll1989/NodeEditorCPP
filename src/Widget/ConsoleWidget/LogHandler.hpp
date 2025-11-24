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
     * 追加日志到表格
     * @param const QString &timestamp 时间戳
     * @param const QString &level 日志级别
     * @param const QIcon &icon 图标
     * @param const QString &logMessage 日志消息
     */
    static void appendLogToTable(const QString &timestamp, const QString &level, const QIcon &icon, const QString &logMessage);
    
    /**
     * spdlog 日志器
     */
    static std::shared_ptr<spdlog::logger> logger;
    
    /**
     * 日志表格
     */
    static LogWidget *logTableWidget;
};
