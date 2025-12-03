#include "TaskListWidget.hpp"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDrag>
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

TaskListWidget::TaskListWidget(ScheduledTaskModel* model,QWidget* parent)
    : QListView(parent),
      m_model(model)
{
    // 视图基础配置
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);
    installEventFilter(this);

    // 设置右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, &TaskListWidget::showContextMenu);

    // 绑定模型与委托
    if (m_model) {
        // 使用过滤代理模型作为视图模型
        m_proxy = new ScheduledTaskFilterProxyModel(this);
        m_proxy->setSourceModel(m_model);
        setModel(m_proxy);
    }
    auto* delegate = new TaskItemDelegate(this);
    setItemDelegate(delegate);

    // 列表视图显示参数优化：
    // 1) 关闭统一行高，允许每行按委托 sizeHint 动态高度
    setUniformItemSizes(false);
    // 2) 显示为列表模式（保证水平不换列）
    setViewMode(QListView::ListMode);
    // 3) 设置项间距，避免视觉过于紧凑
    setSpacing(4);

    // 打开持久编辑器，让每一行永久显示 TaskItemWidget
    openEditorsForAllRows();
    if (m_model) {
        connect(m_model, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int, int){
            openEditorsForAllRows();
        });
        connect(m_model, &QAbstractItemModel::modelReset, this, [this](){
            openEditorsForAllRows();
        });
    }
}

void TaskListWidget::addScheduled(const OSCMessage& message)
{
    // 安全检查：模型不能为空
    if (!m_model) return;
    // 将 OSC 消息包装为计划项并写入模型
    ScheduledTaskItem task;
    task.osc = message;
    task.scheduled.type="once";
    task.scheduled.time=QDateTime(m_proxy->getFilterDate(),QTime(9,0,0));
    m_model->addItem(task);
}


void TaskListWidget::addScheduled(const ScheduledTaskItem& m_scheduled) {
    if (!m_model) return;
    m_model->addItem(m_scheduled);
}



void TaskListWidget::showContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* addAction = menu.addAction("add Scheduled");
    addAction->setIcon(QIcon(":/icons/icons/add.png"));
    QAction* deleteAction = menu.addAction("Delete Scheduled");
    deleteAction->setIcon(QIcon(":/icons/icons/delete.png"));
    QAction* clearAction = menu.addAction("Clear All Messages");
    clearAction->setIcon(QIcon(":/icons/icons/clear.png"));

    // QListView 不提供 clear()，如需清空请调用模型 API：m_model->setItems({})
    // connect(clearAction, &QAction::triggered, this, &TaskListWidget::clear);
    connect(addAction, &QAction::triggered, this, &TaskListWidget::addNewScheduled);
    connect(deleteAction, &QAction::triggered, this, &TaskListWidget::deleteSelectedMessage);
    
    menu.exec(mapToGlobal(pos));
}

void TaskListWidget::addNewScheduled()
{
    addScheduled();
}

void TaskListWidget::deleteSelectedMessage()
{
    if (!m_model) return;
    const QModelIndex idx = currentIndex();
    if (idx.isValid()) {
        m_model->removeItem(idx.row());
    }
    clearSelection();
}

// 拖拽相关实现
void TaskListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-osc-address")) {
        event->acceptProposedAction();
    }
}

void TaskListWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-osc-address")) {
        event->acceptProposedAction();
    }
}

void TaskListWidget::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-osc-address")) {
        OSCMessage message;
        
        // 反序列化消息数据
        QByteArray data = mimeData->data("application/x-osc-address");
        QDataStream stream(data);
        stream >> message.host >> message.port >> message.address >> message.value;
        
        // 添加消息到列表
        addScheduled(message);
        event->acceptProposedAction();
        clearSelection();
    }
}


QJsonObject TaskListWidget::messageToJson(const OSCMessage& message)
{
    QJsonObject messageJson;
    messageJson["host"] = message.host;
    messageJson["port"] = message.port;
    messageJson["address"] = message.address;
    messageJson["type"] = message.type;
    messageJson["value"] = message.value.toString();
    return messageJson;
}

OSCMessage TaskListWidget::jsonToMessage(const QJsonObject& json)
{
    OSCMessage message;
    message.host = json["host"].toString();
    message.port = json["port"].toInt();
    message.address = json["address"].toString();
    message.type = json["type"].toString();
    message.value = json["value"].toString();
    return message;
}

QJsonObject TaskListWidget::save() const
{
    // 直接委托模型序列化
    if (!m_model) return {};
    return m_model->toJson();
}

void TaskListWidget::load(const QJsonObject& json)
{
    // 直接委托模型反序列化（并触发视图刷新）
    if (!m_model) return;
    m_model->fromJson(json);
}

bool TaskListWidget::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if ((keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
            && object == this) {
            deleteSelectedMessage();
            return true;
        }
    }
    return QListView::eventFilter(object, event);
}

// 打开所有行的持久编辑器
void TaskListWidget::openEditorsForAllRows()
{
    // 使用视图当前绑定的模型（可能是代理）来开启持久编辑器
    const QAbstractItemModel* mdl = model();
    if (!mdl) return;
    const int rows = mdl->rowCount();
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = mdl->index(r, 0);
        if (idx.isValid()) {
            openPersistentEditor(idx);
        }
    }
}

void TaskListWidget::setDate(const QDate& date)
{
    if (!m_proxy) return;
    m_proxy->setFilterDate(date);
    // 过滤变动后，刷新持久编辑器集合
    openEditorsForAllRows();
}