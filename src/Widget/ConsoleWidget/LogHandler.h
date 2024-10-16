//
// Created by bwll1 on 2024/9/19.
//

#ifndef NODEEDITORCPP_LOGHANDLER_H
#define NODEEDITORCPP_LOGHANDLER_H


//
// Created by bwll1 on 2024/9/18.
//

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

class LogHandler {
public:
    LogHandler(QTableWidget *tableWidget) ;

    ~LogHandler();

    bool initLogHandler();

    static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    // 追加日志到 QTableWidget
    static void appendLogToTable(const QString &timestamp, const QString &level,const QIcon &icon,const QString &logMessage) ;
    // 静态成员变量：spdlog 日志器和 QTableWidget
    static std::shared_ptr<spdlog::logger> logger;
    static QTableWidget *logTableWidget;
};



#endif //NODEEDITORCPP_LOGHANDLER_H
