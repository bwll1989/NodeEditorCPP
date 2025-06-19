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
    if (event->mimeData()->hasFormat("application/x-osc-address")) {
        event->acceptProposedAction();
    }
}

void OSCMessageListWidget::dragMoveEvent(QDragMoveEvent* event) 
{
    if (event->mimeData()->hasFormat("application/x-osc-address")) {
        event->acceptProposedAction();
    }
}

void OSCMessageListWidget::dropEvent(QDropEvent* event) 
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-osc-address")) {
        OSCMessage message;
        
        // 反序列化消息数据
        QByteArray data = mimeData->data("application/x-osc-address");
        QDataStream stream(data);
        stream >> message.host >> message.port >> message.address >> message.value;
        
        // 添加消息到列表
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
    
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    {
        return;
    }
    
    QListWidgetItem* item = currentItem();
    if (!item) return;
    
    auto* widget = qobject_cast<OSCMessageItemWidget*>(itemWidget(item));
    if (!widget) return;
    
    OSCMessage message = widget->getMessage();
    
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << message.host << message.port << message.address << message.value;
    
    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-osc-address", itemData);
    
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
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, message.address);

    // 设置拖拽预览
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));  // 热点在中心
    
    Qt::DropAction defaultAction = event->modifiers() & Qt::ControlModifier ? 
                                 Qt::CopyAction : Qt::MoveAction;
    
    if (drag->exec(Qt::CopyAction | Qt::MoveAction, defaultAction) == Qt::MoveAction) {
        delete item;
    }
} 

QJsonObject OSCMessageListWidget::messageToJson(const OSCMessage& message)
{
    QJsonObject messageJson;
    messageJson["host"] = message.host;
    messageJson["port"] = message.port;
    messageJson["address"] = message.address;
    messageJson["type"] = message.type;
    messageJson["value"] = message.value.toString();
    return messageJson;
}

OSCMessage OSCMessageListWidget::jsonToMessage(const QJsonObject& json)
{
    OSCMessage message;
    message.host = json["host"].toString();
    message.port = json["port"].toInt();
    message.address = json["address"].toString();
    message.type = json["type"].toString();
    message.value = json["value"].toString();
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
        message.value=widget->getExpression();
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
