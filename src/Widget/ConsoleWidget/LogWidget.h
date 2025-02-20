#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QTableWidget>

#include <QMenu>
#include <QAction>
class LogWidget : public QTableWidget
{
    Q_OBJECT

public:

    LogWidget();

protected:

    void contextMenuEvent(QContextMenuEvent *event) override;

public slots:

    void clearTableWidget();

};
#endif // LOGWIDGET_H
