#pragma once

#include <QWidget>
#include <QLineEdit>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include <QTimeEdit>
#include <QCheckBox>

#include "PushButton.h"
#include "ScheduledTaskModel.hpp"

class TaskItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskItemWidget(QWidget* parent = nullptr);

    OSCMessage getMessage() const;
    void setMessage(const OSCMessage& message);

    ScheduledInfo getScheduledInfo() const;
    void setScheduledInfo(const ScheduledInfo& info);

    QString getRemarks() const;
    void setRemarks(const QString& remarks);

public slots:
    void testCommand() const;

signals:
    void messageChanged();
    void testRequested();

private:
    void setupUI();
    void connectSignals();
    void updateLoopDaysVisible(bool visible);
    void applyDefaultLoopDayIfNeeded();

    QLineEdit* addressEdit = nullptr;
    QLineEdit* valueEdit = nullptr;
    QLineEdit* remarksEdit = nullptr;
    QTimeEdit* timeEdit = nullptr;
    QPushButton* btnTest = nullptr;

    QCheckBox* loopCheck = nullptr;
    QWidget* daysWidget = nullptr;
    QCheckBox* chkMonday = nullptr;
    QCheckBox* chkTuesday = nullptr;
    QCheckBox* chkWednesday = nullptr;
    QCheckBox* chkThursday = nullptr;
    QCheckBox* chkFriday = nullptr;
    QCheckBox* chkSaturday = nullptr;
    QCheckBox* chkSunday = nullptr;

    QDate m_boundDate;
};
