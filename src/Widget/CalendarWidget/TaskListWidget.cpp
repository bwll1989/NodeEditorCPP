#include "TaskListWidget.hpp"

#include "TaskRowDelegate.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QItemSelectionModel>
#include <QIcon>
#include <QFrame>
#include <QTimer>

TaskListWidget::TaskListWidget(ScheduledTaskModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    setupUi();
    setupMenu();

    if (m_model) {
        m_proxy = new ScheduledTaskFilterProxyModel(this);
        m_proxy->setSourceModel(m_model);
        m_listView->setModel(m_proxy);

        connect(m_model, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int first, int last) {
            updateHeader();
            QTimer::singleShot(0, this, [this, first, last]() {
                if (!m_proxy || !m_listView) {
                    return;
                }
                for (int r = first; r <= last; ++r) {
                    const QModelIndex sourceIdx = m_model->index(r, 0);
                    const QModelIndex proxyIdx = m_proxy->mapFromSource(sourceIdx);
                    if (proxyIdx.isValid()) {
                        m_listView->setCurrentIndex(proxyIdx);
                        emit currentTaskChanged(proxyIdx);
                        return;
                    }
                }
            });
        });
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, [this]() {
            updateHeader();
            emit currentTaskChanged(m_listView->currentIndex());
        });
        connect(m_model, &QAbstractItemModel::modelReset, this, [this]() {
            updateHeader();
            emit currentTaskChanged(m_listView->currentIndex());
        });
        connect(m_model, &QAbstractItemModel::dataChanged, this, &TaskListWidget::updateHeader);
    }

    if (auto* selection = m_listView->selectionModel()) {
        connect(selection, &QItemSelectionModel::currentChanged,
                this, [this](const QModelIndex& current, const QModelIndex& previous) {
            Q_UNUSED(previous);
            emit currentTaskChanged(current);
        });
    }
}

void TaskListWidget::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_headerLabel = new QLabel(this);
    m_headerLabel->setObjectName(QStringLiteral("taskListHeader"));
    {
        QFont f = m_headerLabel->font();
        f.setPointSize(f.pointSize() - 1);
        m_headerLabel->setFont(f);
    }
    layout->addWidget(m_headerLabel);

    m_listView = new QListView(this);
    m_listView->setObjectName(QStringLiteral("taskListView"));
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setDragDropMode(QAbstractItemView::InternalMove);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setUniformItemSizes(true);
    m_listView->setSpacing(1);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setEditTriggers(QAbstractItemView::SelectedClicked
                                | QAbstractItemView::DoubleClicked
                                | QAbstractItemView::EditKeyPressed);
    m_listView->setItemDelegate(new TaskRowDelegate(m_listView));
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->installEventFilter(this);

    connect(m_listView, &QWidget::customContextMenuRequested,
            this, &TaskListWidget::showContextMenu);

    layout->addWidget(m_listView, 1);
    updateHeader();
}

void TaskListWidget::setupMenu()
{
    menu = new QMenu(this);
    QAction* addAction = menu->addAction(tr("添加任务"));
    addAction->setIcon(QIcon(QStringLiteral(":/icons/icons/add.png")));
    QAction* deleteAction = menu->addAction(tr("删除任务"));
    deleteAction->setIcon(QIcon(QStringLiteral(":/icons/icons/delete.png")));
    QAction* clearAction = menu->addAction(tr("清空全部"));
    clearAction->setIcon(QIcon(QStringLiteral(":/icons/icons/clear.png")));

    connect(addAction, &QAction::triggered, this, &TaskListWidget::addNewScheduled);
    connect(deleteAction, &QAction::triggered, this, &TaskListWidget::deleteSelectedMessage);
    connect(clearAction, &QAction::triggered, this, &TaskListWidget::clearAllScheduled);
}

void TaskListWidget::addScheduled(const OSCMessage& message)
{
    if (!m_model || !m_proxy) {
        return;
    }

    ScheduledTaskItem task;
    task.osc = message;
    task.scheduled.type = QStringLiteral("once");
    task.scheduled.time = QDateTime(m_proxy->getFilterDate(), QTime(9, 0, 0));
    m_model->addItem(task);
}

void TaskListWidget::addScheduled(const ScheduledTaskItem& scheduled)
{
    if (!m_model) {
        return;
    }
    m_model->addItem(scheduled);
}

void TaskListWidget::showContextMenu(const QPoint& pos)
{
    menu->exec(m_listView->mapToGlobal(pos));
}

void TaskListWidget::addNewScheduled()
{
    addScheduled();
}

void TaskListWidget::deleteSelectedMessage()
{
    if (!m_model) {
        return;
    }

    const QModelIndex proxyIdx = m_listView->currentIndex();
    if (!proxyIdx.isValid()) {
        m_listView->clearSelection();
        return;
    }

    QModelIndex sourceIdx = proxyIdx;
    if (m_proxy) {
        sourceIdx = m_proxy->mapToSource(proxyIdx);
    }

    if (sourceIdx.isValid()) {
        m_model->removeItem(sourceIdx.row());
    }

    m_listView->clearSelection();
}

void TaskListWidget::clearAllScheduled()
{
    if (!m_model) {
        return;
    }

    for (int r = m_model->rowCount() - 1; r >= 0; --r) {
        m_model->removeItem(r);
    }

    m_listView->clearSelection();
}

void TaskListWidget::updateHeader()
{
    if (!m_headerLabel || !m_proxy) {
        return;
    }

    const QDate date = m_proxy->getFilterDate();
    const int count = m_proxy->rowCount();
    const QString dateText = date.isValid()
        ? date.toString(tr("yyyy年M月d日 dddd"))
        : tr("未选择日期");
    m_headerLabel->setText(tr("%1 · %2 项任务").arg(dateText).arg(count));
}

QJsonObject TaskListWidget::save() const
{
    if (!m_model) {
        return {};
    }
    return m_model->toJson();
}

void TaskListWidget::load(const QJsonObject& json)
{
    if (!m_model) {
        return;
    }
    m_model->fromJson(json);
}

void TaskListWidget::setDate(const QDate& date)
{
    if (!m_proxy) {
        return;
    }

    const QModelIndex currentProxy = m_listView->currentIndex();
    const QModelIndex currentSource = currentProxy.isValid()
        ? m_proxy->mapToSource(currentProxy)
        : QModelIndex();
    const int selectedSourceRow = currentSource.isValid() ? currentSource.row() : -1;

    if (m_proxy->getFilterDate() == date) {
        updateHeader();
        return;
    }

    m_proxy->setFilterDate(date);
    updateHeader();

    if (selectedSourceRow >= 0 && m_model) {
        const QModelIndex restoredProxy = m_proxy->mapFromSource(m_model->index(selectedSourceRow, 0));
        if (restoredProxy.isValid()) {
            m_listView->setCurrentIndex(restoredProxy);
            emit currentTaskChanged(restoredProxy);
            return;
        }
    }

    m_listView->clearSelection();
    emit currentTaskChanged(QModelIndex());
}

bool TaskListWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_listView && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace) {
            deleteSelectedMessage();
            return true;
        }
    }

    if (obj == m_listView && event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (dragEvent->mimeData()->hasFormat(QStringLiteral("application/x-osc-address"))) {
            dragEvent->acceptProposedAction();
            return true;
        }
    }

    if (obj == m_listView && event->type() == QEvent::DragMove) {
        auto* dragEvent = static_cast<QDragMoveEvent*>(event);
        if (dragEvent->mimeData()->hasFormat(QStringLiteral("application/x-osc-address"))) {
            dragEvent->acceptProposedAction();
            return true;
        }
    }

    if (obj == m_listView && event->type() == QEvent::Drop) {
        auto* dropEvent = static_cast<QDropEvent*>(event);
        const QMimeData* mimeData = dropEvent->mimeData();
        if (mimeData->hasFormat(QStringLiteral("application/x-osc-address"))) {
            OSCMessage message;
            QByteArray data = mimeData->data(QStringLiteral("application/x-osc-address"));
            QDataStream stream(data);
            stream >> message.host >> message.port >> message.address >> message.type >> message.value;
            addScheduled(message);
            dropEvent->acceptProposedAction();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

QJsonObject TaskListWidget::messageToJson(const OSCMessage& message)
{
    QJsonObject messageJson;
    messageJson[QStringLiteral("host")] = message.host;
    messageJson[QStringLiteral("port")] = message.port;
    messageJson[QStringLiteral("address")] = message.address;
    messageJson[QStringLiteral("type")] = message.type;
    messageJson[QStringLiteral("value")] = message.value.toString();
    return messageJson;
}

OSCMessage TaskListWidget::jsonToMessage(const QJsonObject& json)
{
    OSCMessage message;
    message.host = json[QStringLiteral("host")].toString();
    message.port = json[QStringLiteral("port")].toInt();
    message.address = json[QStringLiteral("address")].toString();
    message.type = json[QStringLiteral("type")].toString();
    message.value = json[QStringLiteral("value")].toString();
    return message;
}
