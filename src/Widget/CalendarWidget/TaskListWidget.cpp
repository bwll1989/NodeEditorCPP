#include "TaskListWidget.hpp"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDrag>
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

TaskListWidget::TaskListWidget(ScheduledTaskModel* model,QWidget* parent)
    : QGridView(parent),     // 变更：改为 QGridView 基类构造
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

    // 网格显示参数：
    // - 使用统一尺寸的网格，保持卡片一致大小（与 TaskItemWidget 最小尺寸一致）
    setUniformItemSizes(true);
    setGridSize(QSize(370, 200));
    // - 网格间距（水平/垂直）
    setSpacing(4, 4);

    // 打开（设置）所有行的索引部件，让每个格子显示 TaskItemWidget
    openEditorsForAllRows();
    if (m_model) {
        connect(m_model, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int, int){
            openEditorsForAllRows();
        });
        connect(m_model, &QAbstractItemModel::modelReset, this, [this](){
            openEditorsForAllRows();
        });
    }
    QAction* addAction = menu->addAction("add Scheduled");
    addAction->setIcon(QIcon(":/icons/icons/add.png"));
    QAction* deleteAction = menu->addAction("Delete Scheduled");
    deleteAction->setIcon(QIcon(":/icons/icons/delete.png"));
    QAction* clearAction = menu->addAction("Clear All Messages");
    clearAction->setIcon(QIcon(":/icons/icons/clear.png"));

    connect(addAction, &QAction::triggered, this, &TaskListWidget::addNewScheduled);
    connect(deleteAction, &QAction::triggered, this, &TaskListWidget::deleteSelectedMessage);
    connect(clearAction, &QAction::triggered, this, &TaskListWidget::clearAllScheduled); // 新增连接
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
    menu->exec(mapToGlobal(pos));
}

void TaskListWidget::addNewScheduled()
{
    addScheduled();
}

void TaskListWidget::deleteSelectedMessage()
{
    /**
     * 函数：TaskListWidget::deleteSelectedMessage
     * 作用：删除当前选中的任务项。
     * 说明：视图绑定的是过滤代理模型（m_proxy），因此需要将当前索引
     *       从代理模型映射回源模型（m_model）后再删除正确的源行。
     */
    if (!m_model) return;

    // 当前选中的是“代理模型索引”
    const QModelIndex proxyIdx = currentIndex();
    if (!proxyIdx.isValid()) {
        clearSelection();
        return;
    }

    // 将代理索引映射回源模型索引
    QModelIndex sourceIdx = proxyIdx;
    if (m_proxy) {
        sourceIdx = m_proxy->mapToSource(proxyIdx);
    }

    // 删除源模型中的对应行
    if (sourceIdx.isValid()) {
        m_model->removeItem(sourceIdx.row());
    }

    // 清空选择，避免残留到已删除的行
    clearSelection();
}

void TaskListWidget::clearAllScheduled()
{
    /**
     * 函数：TaskListWidget::clearAllScheduled
     * 作用：清空模型中所有任务项（操作源模型 m_model）。
     * 说明：视图绑定的是过滤代理模型 m_proxy；直接清空源模型可保证无论过滤
     *       条件为何，最终都移除所有数据行。
     */
    if (!m_model) return;

    // 从末尾到开头逐行删除，避免 row 变化影响遍历
    for (int r = m_model->rowCount() - 1; r >= 0; --r) {
        m_model->removeItem(r);
    }

    // 清空选择并请求重新布局
    clearSelection();
    scheduleDelayedItemsLayout();
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
        stream >> message.host >> message.port >> message.address >>  message.type >> message.value;
        
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
    return QGridView::eventFilter(object, event); // 变更：回退到 QGridView
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
    // 布局刷新与几何更新
    scheduleDelayedItemsLayout();
}

void TaskListWidget::setDate(const QDate& date)
{
    if (!m_proxy) return;
    m_proxy->setFilterDate(date);
    // 过滤变动后，刷新持久编辑器集合
    openEditorsForAllRows();
}

void TaskListWidget::resizeEvent(QResizeEvent* event)
{
    /**
     * 函数：TaskListWidget::resizeEvent
     * 作用：窗口缩放时，触发网格延迟布局并刷新索引部件几何。
     */
    QGridView::resizeEvent(event);
    scheduleDelayedItemsLayout();
}