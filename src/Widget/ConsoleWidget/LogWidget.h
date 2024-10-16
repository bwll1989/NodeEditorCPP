#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QTableWidget>
#include <QTextBrowser>

class LogWidget : public QTableWidget
{
public:
    LogWidget();
public slots:
    void clearTableWidget();
};

#endif // LOGWIDGET_H
