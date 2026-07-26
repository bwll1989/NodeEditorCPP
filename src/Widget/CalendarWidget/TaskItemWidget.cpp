#include "TaskItemWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QDate>

#include "../../Common/AppConfig/ConfigManager.h"
#include "../../Common/Devices/StatusContainer/StatusContainer.h"

TaskItemWidget::TaskItemWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
}

void TaskItemWidget::setupUI()
{
    setObjectName(QStringLiteral("taskDetailEditor"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    auto* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(1, 1);

    auto addFieldRow = [this, grid](int row, const QString& labelText, QWidget* field) {
        auto* label = new QLabel(labelText, this);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        grid->addWidget(label, row, 0);
        grid->addWidget(field, row, 1);
    };

    timeEdit = new QTimeEdit(this);
    timeEdit->setDisplayFormat(QStringLiteral("HH:mm:ss"));
    timeEdit->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    timeEdit->setMinimumHeight(28);
    addFieldRow(0, tr("时间"), timeEdit);

    remarksEdit = new QLineEdit(this);
    remarksEdit->setPlaceholderText(tr("可选，用于日历显示"));
    remarksEdit->setMinimumHeight(28);
    addFieldRow(1, tr("备注"), remarksEdit);

    addressEdit = new QLineEdit(this);
    addressEdit->setPlaceholderText(QStringLiteral("/address"));
    addressEdit->setMinimumHeight(28);
    addFieldRow(2, tr("地址"), addressEdit);

    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText(tr("值"));
    valueEdit->setMinimumHeight(28);
    addFieldRow(3, tr("数值"), valueEdit);

    loopCheck = new QCheckBox(tr("按星期循环"), this);
    auto* loopLabel = new QLabel(tr("循环"), this);
    loopLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(loopLabel, 4, 0);
    grid->addWidget(loopCheck, 4, 1);

    daysWidget = new QWidget(this);
    auto* daysGrid = new QGridLayout(daysWidget);
    daysGrid->setContentsMargins(0, 0, 0, 0);
    daysGrid->setHorizontalSpacing(4);
    daysGrid->setVerticalSpacing(4);

    const QStringList dayLabels = {
        tr("一"), tr("二"), tr("三"), tr("四"), tr("五"), tr("六"), tr("日")
    };
    QList<QCheckBox**> dayChecks = {
        &chkMonday, &chkTuesday, &chkWednesday, &chkThursday,
        &chkFriday, &chkSaturday, &chkSunday
    };

    for (int i = 0; i < dayLabels.size(); ++i) {
        auto* chk = new QCheckBox(dayLabels[i], daysWidget);
        chk->setObjectName(QStringLiteral("dayPill"));
        chk->setMinimumWidth(32);
        chk->setMinimumHeight(26);
        *dayChecks[i] = chk;
        daysGrid->addWidget(chk, 0, i);
    }

    grid->addWidget(daysWidget, 5, 0, 1, 2, Qt::AlignLeft);
    daysWidget->setVisible(false);

    root->addLayout(grid);

    auto* actionRow = new QHBoxLayout();
    actionRow->setContentsMargins(0, 4, 0, 0);
    actionRow->addStretch();
    btnTest = new QPushButton(tr("测试发送"), this);
    btnTest->setObjectName(QStringLiteral("btnTestSend"));
    btnTest->setMinimumHeight(30);
    btnTest->setMinimumWidth(96);
    actionRow->addWidget(btnTest);
    root->addLayout(actionRow);
}

void TaskItemWidget::connectSignals()
{
    const auto emitChange = [this]() {
        emit messageChanged();
    };

    connect(addressEdit, &QLineEdit::textEdited, this, emitChange);
    connect(addressEdit, &QLineEdit::editingFinished, this, emitChange);
    connect(valueEdit, &QLineEdit::textEdited, this, emitChange);
    connect(valueEdit, &QLineEdit::editingFinished, this, emitChange);
    connect(remarksEdit, &QLineEdit::textEdited, this, emitChange);
    connect(remarksEdit, &QLineEdit::editingFinished, this, emitChange);
    connect(timeEdit, &QTimeEdit::timeChanged, this, emitChange);
    connect(timeEdit, &QTimeEdit::editingFinished, this, emitChange);

    connect(loopCheck, &QCheckBox::toggled, this, [this](bool checked) {
        updateLoopDaysVisible(checked);
        if (checked) {
            applyDefaultLoopDayIfNeeded();
        }
        emit messageChanged();
    });

    const QList<QCheckBox*> dayChecks = {
        chkMonday, chkTuesday, chkWednesday, chkThursday,
        chkFriday, chkSaturday, chkSunday
    };
    for (QCheckBox* chk : dayChecks) {
        connect(chk, &QCheckBox::clicked, this, emitChange);
    }

    connect(btnTest, &QPushButton::clicked, this, [this]() {
        emit testRequested();
    });
}

void TaskItemWidget::updateLoopDaysVisible(bool visible)
{
    if (daysWidget) {
        daysWidget->setVisible(visible);
    }
}

void TaskItemWidget::applyDefaultLoopDayIfNeeded()
{
    const QList<QCheckBox*> dayChecks = {
        chkMonday, chkTuesday, chkWednesday, chkThursday,
        chkFriday, chkSaturday, chkSunday
    };
    for (QCheckBox* chk : dayChecks) {
        if (chk && chk->isChecked()) {
            return;
        }
    }

    const QDate date = m_boundDate.isValid() ? m_boundDate : QDate::currentDate();
    QCheckBox* target = nullptr;
    switch (date.dayOfWeek()) {
    case 1: target = chkMonday; break;
    case 2: target = chkTuesday; break;
    case 3: target = chkWednesday; break;
    case 4: target = chkThursday; break;
    case 5: target = chkFriday; break;
    case 6: target = chkSaturday; break;
    case 7: target = chkSunday; break;
    default: break;
    }
    if (target) {
        target->setChecked(true);
    }
}

void TaskItemWidget::testCommand() const
{
    StatusContainer::instance()->parseOSC(getMessage());
}

OSCMessage TaskItemWidget::getMessage() const
{
    OSCMessage msg;
    msg.host = ConfigManager::instance().getOscInternalControlHost();
    msg.port = ConfigManager::instance().getExtraControlPort();
    msg.address = addressEdit->text().trimmed();
    msg.value = valueEdit->text();
    return msg;
}

void TaskItemWidget::setMessage(const OSCMessage& message)
{
    addressEdit->setText(message.address);
    if (message.type == QStringLiteral("Int")) {
        valueEdit->setText(QString::number(message.value.toInt()));
    } else if (message.type == QStringLiteral("Float")) {
        valueEdit->setText(QString::number(message.value.toFloat()));
    } else {
        valueEdit->setText(message.value.toString());
    }
}

ScheduledInfo TaskItemWidget::getScheduledInfo() const
{
    ScheduledInfo info;
    info.type = loopCheck->isChecked() ? QStringLiteral("loop") : QStringLiteral("once");
    const QDate useDate = m_boundDate.isValid() ? m_boundDate : QDate::currentDate();
    info.time = QDateTime(useDate, timeEdit->time());
    info.conditions.clear();

    if (info.type.compare(QStringLiteral("loop"), Qt::CaseInsensitive) == 0) {
        if (chkMonday->isChecked())    info.conditions.append(QStringLiteral("Monday"));
        if (chkTuesday->isChecked())   info.conditions.append(QStringLiteral("Tuesday"));
        if (chkWednesday->isChecked()) info.conditions.append(QStringLiteral("Wednesday"));
        if (chkThursday->isChecked())  info.conditions.append(QStringLiteral("Thursday"));
        if (chkFriday->isChecked())    info.conditions.append(QStringLiteral("Friday"));
        if (chkSaturday->isChecked())  info.conditions.append(QStringLiteral("Saturday"));
        if (chkSunday->isChecked())    info.conditions.append(QStringLiteral("Sunday"));
    }

    return info;
}

void TaskItemWidget::setScheduledInfo(const ScheduledInfo& info)
{
    const bool isLoop = info.type.compare(QStringLiteral("loop"), Qt::CaseInsensitive) == 0;
    loopCheck->setChecked(isLoop);
    updateLoopDaysVisible(isLoop);

    m_boundDate = info.time.date();
    const QTime t = info.time.time().isValid() ? info.time.time() : QTime(9, 0, 0);
    timeEdit->setTime(t);

    auto containsDay = [&](const char* day) {
        return info.conditions.contains(QString::fromLatin1(day), Qt::CaseInsensitive);
    };

    chkMonday->setChecked(isLoop && containsDay("Monday"));
    chkTuesday->setChecked(isLoop && containsDay("Tuesday"));
    chkWednesday->setChecked(isLoop && containsDay("Wednesday"));
    chkThursday->setChecked(isLoop && containsDay("Thursday"));
    chkFriday->setChecked(isLoop && containsDay("Friday"));
    chkSaturday->setChecked(isLoop && containsDay("Saturday"));
    chkSunday->setChecked(isLoop && containsDay("Sunday"));
}

QString TaskItemWidget::getRemarks() const
{
    return remarksEdit ? remarksEdit->text().trimmed() : QString();
}

void TaskItemWidget::setRemarks(const QString& remarks)
{
    if (remarksEdit) {
        remarksEdit->setText(remarks);
    }
}
