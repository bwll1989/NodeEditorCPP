#pragma once

#include <QTableWidget>
#include <QMenu>
#include <QAction>


class LogWidget : public QTableWidget
{
    Q_OBJECT

public:
    LogWidget();

    /**
     * 设置最大日志条目数
     * @param maxEntries 最大条目数
     */
    void setMaxLogEntries(int maxEntries);

    /**
     * 获取当前最大日志条目数
     * @return int 最大条目数
     */
    int maxLogEntries() const;

    /**
     * 设置当前日志过滤级别
     * @param level 日志级别 ("All", "Debug", "Info", "Warn", "Critical", "Fatal")
     */
    void setLogFilter(const QString &level);

protected:
    /**
     * 上下文菜单事件
     * @param QContextMenuEvent *event 上下文菜单事件
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:
    /**
     * 清除表格
     */
    void clearTableWidget();

    /**
     * 导出日志到文件
     */
    void exportLog();

private:
    int m_maxLogEntries = 200; // 默认最大日志条目数
    QString m_currentFilter = "All"; // 当前过滤级别
};
