#include "TaskEditPopup.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHideEvent>
#include <QApplication>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QEvent>

TaskEditPopup::TaskEditPopup(QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setObjectName(QStringLiteral("taskEditPopup"));
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumWidth(320);
    setMaximumWidth(420);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(8);

    // 标题栏：时间·地址 + 关闭
    auto* titleRow = new QHBoxLayout();
    m_titleLabel = new QLabel(tr("编辑任务"), this);
    m_titleLabel->setObjectName(QStringLiteral("taskEditPopupTitle"));
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    titleRow->addWidget(m_titleLabel, 1);

    m_btnClose = new QPushButton(QStringLiteral("×"), this);
    m_btnClose->setObjectName(QStringLiteral("taskEditPopupClose"));
    m_btnClose->setFlat(true);
    m_btnClose->setFixedSize(24, 24);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    titleRow->addWidget(m_btnClose);
    root->addLayout(titleRow);

    m_editor = new TaskItemWidget(this);
    root->addWidget(m_editor);

    auto* actionRow = new QHBoxLayout();
    m_btnDelete = new QPushButton(tr("删除"), this);
    m_btnDelete->setObjectName(QStringLiteral("taskEditPopupDelete"));
    actionRow->addWidget(m_btnDelete);
    actionRow->addStretch();
    root->addLayout(actionRow);

    connect(m_editor, &TaskItemWidget::messageChanged, this, [this]() {
        if (m_loading) {
            return;
        }
        commit();
        refreshTitle();
        emit taskChanged();
    });
    connect(m_editor, &TaskItemWidget::testRequested, this, [this]() {
        if (m_editor) {
            m_editor->testCommand();
        }
    });
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() {
        const int row = m_sourceRow;
        closePopup();
        if (row >= 0) {
            emit taskDeleted(row);
        }
    });
    connect(m_btnClose, &QPushButton::clicked, this, &TaskEditPopup::closePopup);

    // 用于检测点击浮层外部
    qApp->installEventFilter(this);
}

void TaskEditPopup::editTask(ScheduledTaskModel* model, int sourceRow)
{
    m_model = model;
    m_sourceRow = sourceRow;
    if (!m_model || sourceRow < 0 || sourceRow >= m_model->rowCount()) {
        closePopup();
        return;
    }

    m_loading = true;
    loadFromModel();
    m_loading = false;
    refreshTitle();
    show();
    raise();
    activateWindow();
}

void TaskEditPopup::closePopup()
{
    hide();
}

void TaskEditPopup::commit()
{
    if (!m_model || !m_editor || m_sourceRow < 0 || m_sourceRow >= m_model->rowCount()) {
        return;
    }
    m_model->updateTaskFields(m_sourceRow,
                              m_editor->getMessage(),
                              m_editor->getScheduledInfo(),
                              m_editor->getRemarks());
}

void TaskEditPopup::refreshTitle()
{
    if (!m_editor || !m_titleLabel) {
        return;
    }
    const ScheduledInfo info = m_editor->getScheduledInfo();
    const QString timeText = info.time.time().toString(QStringLiteral("HH:mm"));
    const QString remarks = m_editor->getRemarks();
    const OSCMessage msg = m_editor->getMessage();
    const QString titleText = !remarks.isEmpty()
        ? remarks
        : (msg.address.isEmpty() ? tr("未设置地址") : msg.address);
    m_titleLabel->setText(QStringLiteral("%1 · %2").arg(timeText, titleText));
}

void TaskEditPopup::loadFromModel()
{
    if (!m_model || !m_editor || m_sourceRow < 0) {
        return;
    }

    const QModelIndex index = m_model->index(m_sourceRow, 0);
    QSignalBlocker blocker(m_editor);

    OSCMessage msg;
    msg.host = index.data(ScheduledTaskModel::RoleHost).toString();
    msg.port = index.data(ScheduledTaskModel::RolePort).toInt();
    msg.address = index.data(ScheduledTaskModel::RoleAddress).toString();
    msg.type = index.data(ScheduledTaskModel::RoleType).toString();
    msg.value = index.data(ScheduledTaskModel::RoleValue).toString();
    m_editor->setMessage(msg);
    m_editor->setRemarks(index.data(ScheduledTaskModel::RoleRemarks).toString());

    ScheduledInfo info;
    info.type = index.data(ScheduledTaskModel::RoleScheduleType).toString();
    const QTime t = index.data(ScheduledTaskModel::RoleTime).toTime();
    const QDate d = index.data(ScheduledTaskModel::RoleDate).toDate();
    info.time = QDateTime(d.isValid() ? d : QDate::currentDate(),
                          t.isValid() ? t : QTime(9, 0, 0));
    info.conditions = index.data(ScheduledTaskModel::RoleLoopDays).toStringList();
    m_editor->setScheduledInfo(info);
}

bool TaskEditPopup::eventFilter(QObject* watched, QEvent* event)
{
    if (isVisible() && event->type() == QEvent::MouseButtonPress) {
        const QPoint globalPos = static_cast<QMouseEvent*>(event)->globalPosition().toPoint();
        if (!frameGeometry().contains(globalPos)) {
            closePopup();
        }
    }
    return QFrame::eventFilter(watched, event);
}

void TaskEditPopup::hideEvent(QHideEvent* event)
{
    QFrame::hideEvent(event);
    m_sourceRow = -1;
    emit closed();
}
