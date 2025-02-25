#ifndef LOGWIDGET_HPP
#define LOGWIDGET_HPP

#include <QTableWidget>

#include <QMenu>
#include <QAction>
class LogWidget : public QTableWidget
{
    Q_OBJECT

public:

    LogWidget();

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

};
#endif // LOGWIDGET_HPP
