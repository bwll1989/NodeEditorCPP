#pragma once

#include <QListView>
#include <QMenu>
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>
#include <QLabel>
#include <QModelIndex>

#include "../../Common/Devices/OSCSender/OSCSender.h"
#include "ScheduledTaskModel.hpp"
#include "ScheduledTaskFilterProxyModel.hpp"

class TaskRowDelegate;

class TaskListWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskListWidget(ScheduledTaskModel* taskModel, QWidget* parent = nullptr);

    void addScheduled(const OSCMessage& message = OSCMessage());
    void addScheduled(const ScheduledTaskItem& scheduled);
    QJsonObject save() const;
    void load(const QJsonObject& json);
    void setDate(const QDate& date);

    QListView* listView() const { return m_listView; }
    ScheduledTaskFilterProxyModel* proxyModel() const { return m_proxy; }
    QMenu* getMenu() { return menu; }

signals:
    void currentTaskChanged(const QModelIndex& proxyIndex);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUi();
    void setupMenu();
    void showContextMenu(const QPoint& pos);
    void addNewScheduled();
    void deleteSelectedMessage();
    void clearAllScheduled();
    void updateHeader();

    static QJsonObject messageToJson(const OSCMessage& message);
    static OSCMessage jsonToMessage(const QJsonObject& json);

    ScheduledTaskModel* m_model = nullptr;
    ScheduledTaskFilterProxyModel* m_proxy = nullptr;
    QListView* m_listView = nullptr;
    QLabel* m_headerLabel = nullptr;
    QMenu* menu = nullptr;
};
