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
     * 设置当前日志过滤级别
     * @param level 日志级别 ("All", "Debug", "Info", "Warn", "Critical", "Fatal")
     */
    void setLogFilter(const QString &level);
    /**
     * 获取所有操作项
     * @return QList<QAction*> 操作项列表
     */
    QList<QAction*> getActions();
protected:
    /**
     * 上下文菜单事件
     * @param QContextMenuEvent *event 上下文菜单事件
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

    void initializeActions();
public slots:
    /**
     * 清除表格
     */
    void clearTableWidget();

    /**
     * 导出日志到文件
     */
    void exportLog();
public:
    QMenu *menu = new QMenu(this);

private:
    QString m_currentFilter = "All"; // 当前过滤级别
};
