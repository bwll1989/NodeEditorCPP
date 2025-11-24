#include "DelayMessageListWidget.hpp"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDrag>
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

DelayMessageListWidget::DelayMessageListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);
    installEventFilter(this);
    // 设置右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested,
            this, &DelayMessageListWidget::showContextMenu);
}

void DelayMessageListWidget::addDelayMessage(const delay_item& message)
{
    auto* item = new QListWidgetItem(this);
    auto* widget = new DelayMessageItemWidget(this);
    widget->setMessage(message);
    item->setSizeHint(widget->sizeHint());
    addItem(item);
    setItemWidget(item, widget);
}

QVector<delay_item> DelayMessageListWidget::getDelayMessages() const
{
    QVector<delay_item> messages;
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem* currentItem = item(i);

        if (!currentItem) {
            continue;
        }
        auto* widget = static_cast<DelayMessageItemWidget*>(itemWidget(currentItem));
        if (widget) {
            messages.append(widget->getMessage());
        }
    }
    return messages;
}

void DelayMessageListWidget::setDelayMessages(const QVector<delay_item>& messages)
{
    clear();
    for (const auto& message : messages) {
        addDelayMessage(message);
    }
}

void DelayMessageListWidget::showContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* addAction = menu.addAction("Add Message");
    QAction* deleteAction = menu.addAction("Delete Message");
    QAction* clearAction = menu.addAction("Clear All Messages");

    connect(clearAction, &QAction::triggered, this, &DelayMessageListWidget::clear);
    connect(addAction, &QAction::triggered, this, &DelayMessageListWidget::addNewMessage);
    connect(deleteAction, &QAction::triggered, this, &DelayMessageListWidget::deleteSelectedMessage);
    
    menu.exec(mapToGlobal(pos));
}

void DelayMessageListWidget::addNewMessage()
{
    addDelayMessage(delay_item());
}

void DelayMessageListWidget::deleteSelectedMessage()
{
    QListWidgetItem* item = currentItem();
    if (item) {
        delete item;
    }
    clearSelection();
}

// 拖拽相关实现
void DelayMessageListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-delay-message")) {
        event->acceptProposedAction();
    }
}

void DelayMessageListWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-delay-message")) {
        event->acceptProposedAction();
    }
}

void DelayMessageListWidget::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-delay-message")) {
        delay_item message;
        
        // 反序列化消息数据
        QByteArray data = mimeData->data("application/x-delay-message");
        QDataStream stream(data);
        stream >> message.port >> message.time>> message.command;

        // 添加消息到列表
        addDelayMessage(message);
        event->acceptProposedAction();
        clearSelection();
    }
}

void DelayMessageListWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
    QListWidget::mousePressEvent(event);
}

void DelayMessageListWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    {
        return;
    }
    
    QListWidgetItem* item = currentItem();
    if (!item) return;
    
    auto* widget = qobject_cast<DelayMessageItemWidget*>(itemWidget(item));
    if (!widget) return;

    delay_item message = widget->getMessage();
    
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << message.port <<  message.time << message.command;
    
    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-delay-message", itemData);
    
    QDrag* drag = new QDrag(this);
    
    drag->setMimeData(mimeData);
    QPixmap pixmap(widget->width(), widget->height());
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    QColor bgColor(40, 40, 40, 200);  // 半透明深灰色
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect(), 5, 5);  // 圆角矩形
    // 绘制文本
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    QRect textRect = pixmap.rect().adjusted(30, 0, -8, 0);  // 图标右侧的文本区域
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, QString::number(message.time));

    // 设置拖拽预览
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));  // 热点在中心
    
    Qt::DropAction defaultAction = event->modifiers() & Qt::ControlModifier ? 
                                 Qt::CopyAction : Qt::MoveAction;
    
    if (drag->exec(Qt::CopyAction | Qt::MoveAction, defaultAction) == Qt::MoveAction) {
        delete item;
    }
} 

QJsonObject DelayMessageListWidget::messageToJson(const delay_item& message)
{
    QJsonObject messageJson;

    messageJson["port"] = message.port;
    messageJson["time"] = message.time;
    messageJson["command"] = message.command;
    return messageJson;
}

delay_item DelayMessageListWidget::jsonToMessage(const QJsonObject& json)
{
    delay_item message;
    message.port = json["port"].toInt();
    message.time = json["time"].toInt();
    message.command = json["command"].toString();
    return message;
}

QJsonObject DelayMessageListWidget::save() const
{
    QJsonObject json;
    QJsonArray messagesArray;
    
    // 遍历所有消息项并序列化
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem* currentItem = item(i);
        if (!currentItem) continue;
        
        auto* widget = static_cast<DelayMessageItemWidget*>(itemWidget(currentItem));
        if (!widget) continue;
        delay_item message = widget->getMessage();
        messagesArray.append(messageToJson(message));
    }
    
    json["messages"] = messagesArray;
    return json;
}

void DelayMessageListWidget::load(const QJsonObject& json)
{
    clear();  // 清除现有项
    
    QJsonArray messagesArray = json["messages"].toArray();
    for (const QJsonValue& value : messagesArray) {
        if (!value.isObject()) continue;
        
        delay_item message = jsonToMessage(value.toObject());
        addDelayMessage(message);
    }
}

bool DelayMessageListWidget::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // 处理Delete键和Backspace键
        if ((keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
            && object == this) {
            deleteSelectedMessage();
            return true;
            }
    }
    return QListWidget::eventFilter(object, event);
}