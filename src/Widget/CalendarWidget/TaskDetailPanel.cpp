#include "TaskDetailPanel.hpp"

#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QTime>

TaskDetailPanel::TaskDetailPanel(ScheduledTaskModel* model,
                                 ScheduledTaskFilterProxyModel* proxy,
                                 QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_proxy(proxy)
{
    setObjectName(QStringLiteral("taskDetailPanel"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    m_titleLabel = new QLabel(tr("任务详情"), this);
    m_titleLabel->setObjectName(QStringLiteral("taskDetailTitle"));
    {
        QFont titleFont = m_titleLabel->font();
        titleFont.setBold(true);
        m_titleLabel->setFont(titleFont);
    }
    root->addWidget(m_titleLabel);

    m_stack = new QStackedWidget(this);

    m_emptyPage = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(m_emptyPage);
    emptyLayout->setContentsMargins(12, 24, 12, 24);
    auto* emptyLabel = new QLabel(tr("选择左侧任务以编辑时间、地址、值和循环"), m_emptyPage);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);
    emptyLabel->setObjectName(QStringLiteral("taskDetailEmpty"));
    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch();
    m_stack->addWidget(m_emptyPage);

    m_editorPage = new QWidget(this);
    auto* editorLayout = new QVBoxLayout(m_editorPage);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    m_editor = new TaskItemWidget(m_editorPage);
    editorLayout->addWidget(m_editor);
    m_stack->addWidget(m_editorPage);

    root->addWidget(m_stack, 1);

    connect(m_editor, &TaskItemWidget::messageChanged, this, &TaskDetailPanel::onEditorChanged);
    connect(m_editor, &TaskItemWidget::testRequested, this, &TaskDetailPanel::onTestClicked);

    if (m_model) {
        connect(m_model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex& topLeft,
                                                                        const QModelIndex& bottomRight,
                                                                        const QList<int>& roles) {
            Q_UNUSED(roles);
            Q_UNUSED(bottomRight);
            if (!m_currentProxyIndex.isValid() || !m_proxy) {
                return;
            }
            const QModelIndex sourceIdx = m_proxy->mapToSource(m_currentProxyIndex);
            if (sourceIdx.isValid()
                && sourceIdx.row() >= topLeft.row()
                && sourceIdx.row() <= bottomRight.row()) {
                refreshTitle();
            }
        });
    }

    clearTask();
}

void TaskDetailPanel::loadTask(const QModelIndex& proxyIndex)
{
    if (!proxyIndex.isValid()) {
        clearTask();
        return;
    }

    m_currentProxyIndex = proxyIndex;
    m_loading = true;
    applyModelDataToEditor(proxyIndex);
    m_loading = false;

    refreshTitle();
    m_stack->setCurrentWidget(m_editorPage);
}

void TaskDetailPanel::clearTask()
{
    m_currentProxyIndex = QModelIndex();
    m_titleLabel->setText(tr("任务详情"));
    m_stack->setCurrentWidget(m_emptyPage);
}

void TaskDetailPanel::onEditorChanged()
{
    if (m_loading || !m_currentProxyIndex.isValid()) {
        return;
    }
    commitCurrentTask();
}

void TaskDetailPanel::onTestClicked()
{
    if (!m_editor) {
        return;
    }
    m_editor->testCommand();
}

void TaskDetailPanel::commitCurrentTask()
{
    if (!m_currentProxyIndex.isValid() || !m_model || !m_proxy) {
        return;
    }

    const QModelIndex sourceIdx = m_proxy->mapToSource(m_currentProxyIndex);
    if (!sourceIdx.isValid()) {
        clearTask();
        return;
    }

    writeEditorToModel(m_editor, m_model, sourceIdx);

    m_currentProxyIndex = m_proxy->mapFromSource(sourceIdx);
    if (!m_currentProxyIndex.isValid()) {
        clearTask();
        emit taskCommitted();
        return;
    }

    refreshTitle();
    emit taskCommitted();
}

void TaskDetailPanel::refreshTitle()
{
    if (!m_currentProxyIndex.isValid()) {
        m_titleLabel->setText(tr("任务详情"));
        return;
    }

    const QTime time = m_currentProxyIndex.data(ScheduledTaskModel::RoleTime).toTime();
    const QString remark = m_currentProxyIndex.data(ScheduledTaskModel::RoleRemarks).toString();
    const QString title = remark.isEmpty()
        ? tr("任务详情 · %1").arg(time.isValid() ? time.toString(QStringLiteral("HH:mm:ss"))
                                                 : QStringLiteral("--:--:--"))
        : tr("任务详情 · %1").arg(remark);
    m_titleLabel->setText(title);
}

void TaskDetailPanel::applyModelDataToEditor(const QModelIndex& index)
{
    if (!m_editor || !index.isValid()) {
        return;
    }

    QSignalBlocker blocker(m_editor);

    OSCMessage msg;
    msg.host = index.data(ScheduledTaskModel::RoleHost).toString();
    msg.port = index.data(ScheduledTaskModel::RolePort).toInt();
    msg.address = index.data(ScheduledTaskModel::RoleAddress).toString();
    msg.type = index.data(ScheduledTaskModel::RoleType).toString();
    msg.value = index.data(ScheduledTaskModel::RoleValue).toString();
    m_editor->setMessage(msg);

    ScheduledInfo info;
    info.type = index.data(ScheduledTaskModel::RoleScheduleType).toString();
    const QTime t = index.data(ScheduledTaskModel::RoleTime).toTime();
    const QDate d = index.data(ScheduledTaskModel::RoleDate).toDate();
    const QDate useDate = d.isValid() ? d : QDate::currentDate();
    info.time = QDateTime(useDate, t.isValid() ? t : QTime(9, 0, 0));
    info.conditions = index.data(ScheduledTaskModel::RoleLoopDays).toStringList();
    m_editor->setScheduledInfo(info);
}

void TaskDetailPanel::writeEditorToModel(TaskItemWidget* editor,
                                         ScheduledTaskModel* model,
                                         const QModelIndex& sourceIndex)
{
    if (!editor || !sourceIndex.isValid() || !model) {
        return;
    }

    model->updateTaskFields(sourceIndex.row(), editor->getMessage(), editor->getScheduledInfo());
}
