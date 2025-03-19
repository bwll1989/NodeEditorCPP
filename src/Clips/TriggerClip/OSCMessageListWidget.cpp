#include "OSCMessageListWidget.hpp"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDrag>
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

OSCMessageListWidget::OSCMessageListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 设置右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested,
            this, &OSCMessageListWidget::showContextMenu);
}

void OSCMessageListWidget::addOSCMessage(const OSCMessage& message) 
{
    auto* item = new QListWidgetItem(this);
    auto* widget = new OSCMessageItemWidget(this);
    
    if (!message.address.isEmpty()) {
        widget->setMessage(message);
    }
    
    item->setSizeHint(widget->sizeHint());
    addItem(item);
    setItemWidget(item, widget);
}

QVector<OSCMessage> OSCMessageListWidget::getOSCMessages() const 
{
    QVector<OSCMessage> messages;
    qDebug() << "count: " << count();
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem* currentItem = item(i);
        if (!currentItem) {
            continue;
        }
        auto* widget = static_cast<OSCMessageItemWidget*>(itemWidget(currentItem));
        if (widget) {
            messages.append(widget->getMessage());
        }
    }
    return messages;
}

void OSCMessageListWidget::setOSCMessages(const QVector<OSCMessage>& messages) 
{
    clear();
    for (const auto& message : messages) {
        addOSCMessage(message);
    }
}

void OSCMessageListWidget::showContextMenu(const QPoint& pos) 
{
    QMenu menu(this);
    QAction* addAction = menu.addAction("Add Message");
    QAction* deleteAction = menu.addAction("Delete Message");
    
    connect(addAction, &QAction::triggered, this, &OSCMessageListWidget::addNewMessage);
    connect(deleteAction, &QAction::triggered, this, &OSCMessageListWidget::deleteSelectedMessage);
    
    menu.exec(mapToGlobal(pos));
}

void OSCMessageListWidget::addNewMessage() 
{
    addOSCMessage();
}

void OSCMessageListWidget::deleteSelectedMessage() 
{
    QListWidgetItem* item = currentItem();
    if (item) {
        delete item;
    }
}

// 拖拽相关实现
void OSCMessageListWidget::dragEnterEvent(QDragEnterEvent* event) 
{
    if (event->mimeData()->hasFormat("application/x-oscmessage")) {
        event->acceptProposedAction();
    }
}

void OSCMessageListWidget::dragMoveEvent(QDragMoveEvent* event) 
{
    if (event->mimeData()->hasFormat("application/x-oscmessage")) {
        event->acceptProposedAction();
    }
}

void OSCMessageListWidget::dropEvent(QDropEvent* event) 
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-oscmessage")) {
        QByteArray itemData = mimeData->data("application/x-oscmessage");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);
        
        OSCMessage message;
        dataStream >> message.host >> message.port >> message.address 
                  >> message.value;
        
        addOSCMessage(message);
        event->acceptProposedAction();
    }
}

void OSCMessageListWidget::mousePressEvent(QMouseEvent* event) 
{
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
    QListWidget::mousePressEvent(event);
}

void OSCMessageListWidget::mouseMoveEvent(QMouseEvent* event) 
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    
    if ((event->pos() - dragStartPosition).manhattanLength() 
        < QApplication::startDragDistance()) {
        return;
    }
    
    QListWidgetItem* item = currentItem();
    if (!item) return;
    
    auto* widget = qobject_cast<OSCMessageItemWidget*>(itemWidget(item));
    if (!widget) return;
    
    OSCMessage message = widget->getMessage();
    
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << message.host << message.port << message.address 
               << message.value;
    
    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-oscmessage", itemData);
    
    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    
    if (drag->exec(Qt::MoveAction) == Qt::MoveAction) {
        delete item;
    }
} 

QJsonObject OSCMessageListWidget::messageToJson(const OSCMessage& message)
{
    QJsonObject messageJson;
    messageJson["host"] = message.host;
    messageJson["port"] = message.port;
    messageJson["address"] = message.address;
    
    // 根据值类型保存
    switch (message.value.typeId()) {
        case QMetaType::Int:
            messageJson["type"] = "Int";
            messageJson["value"] = message.value.toInt();
            break;
        case QMetaType::Double:
            messageJson["type"] = "Float";
            messageJson["value"] = message.value.toDouble();
            break;
        default:
            messageJson["type"] = "String";
            messageJson["value"] = message.value.toString();
    }
    
    return messageJson;
}

OSCMessage OSCMessageListWidget::jsonToMessage(const QJsonObject& json)
{
    OSCMessage message;
    message.host = json["host"].toString();
    message.port = json["port"].toInt();
    message.address = json["address"].toString();
    
    QString type = json["type"].toString();
    QJsonValue value = json["value"];
    
    if (type == "Int") {
        message.value = value.toInt();
    } else if (type == "Float") {
        message.value = value.toDouble();
    } else {
        message.value = value.toString();
    }
    
    return message;
}

QJsonObject OSCMessageListWidget::save() const
{
    QJsonObject json;
    QJsonArray messagesArray;
    
    // 遍历所有消息项并序列化
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem* currentItem = item(i);
        if (!currentItem) continue;
        
        auto* widget = static_cast<OSCMessageItemWidget*>(itemWidget(currentItem));
        if (!widget) continue;
        
        OSCMessage message = widget->getMessage();
        messagesArray.append(messageToJson(message));
    }
    
    json["messages"] = messagesArray;
    return json;
}

void OSCMessageListWidget::load(const QJsonObject& json)
{
    clear();  // 清除现有项
    
    QJsonArray messagesArray = json["messages"].toArray();
    for (const QJsonValue& value : messagesArray) {
        if (!value.isObject()) continue;
        
        OSCMessage message = jsonToMessage(value.toObject());
        addOSCMessage(message);
    }
}
