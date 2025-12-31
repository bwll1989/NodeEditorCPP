#include "OSCMessageListWidget.hpp"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDrag>
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QPainter>
#include <QMouseEvent>
#include <QFont>
#include "ConstantDefines.h"
#include "OSCMessageListModel.hpp"
OSCMessageListWidget::OSCMessageListWidget(bool onlyInternal, QWidget* parent)
    : QGridView(parent), OnlyInternal(onlyInternal)
{
    /**
     * 函数：OSCMessageListWidget::OSCMessageListWidget
     * 作用：初始化“基于 QGridView 的卡片列表”，设置网格参数与数据模型，
     *       绑定右键菜单、快捷键与选中联动。
     */
    setAcceptDrops(true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 网格参数（可按需调整）
    setUniformItemSizes(true);
    if (onlyInternal)
        setGridSize(QSize(200, 120));
    else
        setGridSize(QSize(200, 140));
    setSpacing(8, 8);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);

    // 模型绑定
    m_model = new OSCMessageListModel(this);
    setModel(m_model);
    // 右键菜单（顶层与 viewport）
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &OSCMessageListWidget::showContextMenu);
    // 事件过滤（卡片点击选中等）
    installEventFilter(this);

}

OSCMessageListWidget::OSCMessageListWidget(QWidget* parent)
    : OSCMessageListWidget(false, parent)
{
}
/**
 * 添加新的 OSC 消息卡片
 * 如果 message 非空，应用到卡片；同时默认折叠以保持高度紧凑
 */
void OSCMessageListWidget::addOSCMessage(const OSCMessage& message)
{
    /**
     * 函数：OSCMessageListWidget::addOSCMessage
     * 作用：在模型末尾插入一行，创建并绑定 OSCMessageItemWidget 到该索引。
     */
    const int row = m_model->rowCount();
    m_model->insertRow(row);
    const QModelIndex idx = m_model->index(row, 0);

    auto* widget = new OSCMessageItemWidget(OnlyInternal, viewport());
    connect(widget, &OSCMessageItemWidget::requestDelete, this, [this, widget]() {
        if (m_currentCard == widget) {
            removeCard(m_currentCard);
            m_currentCard = nullptr;
        } else {
            removeCard(widget);
        }
    });

    if (!message.address.isEmpty()) {
        widget->setMessage(message);
    }

    setIndexWidget(idx, widget);

    // 选中新增项
    // setCurrentIndex(idx);
    // setCurrentCard(widget);

    widget->installEventFilter(this);
}

/**
 * 获取当前所有 OSC 消息（按卡片顺序）
 */
QVector<OSCMessage> OSCMessageListWidget::getOSCMessages() const
{
    /**
     * 函数：OSCMessageListWidget::getOSCMessages
     * 作用：遍历模型行，读取每个索引绑定卡片的消息。
     */
    QVector<OSCMessage> messages;
    const int rows = m_model ? m_model->rowCount() : 0;
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = m_model->index(r, 0);
        if (auto* widget = qobject_cast<OSCMessageItemWidget*>(indexWidget(idx))) {
            messages.append(widget->getMessage());
        }
    }
    return messages;
}

/**
 * 批量设置 OSC 消息：清空现有卡片后，依次添加
 */
void OSCMessageListWidget::setOSCMessages(const QVector<OSCMessage>& messages)
{
    /**
     * 函数：OSCMessageListWidget::setOSCMessages
     * 作用：清空现有模型与卡片后，按顺序添加新的消息卡片。
     */
    // 释放旧卡片并清模型
    const int oldRows = m_model->rowCount();
    for (int r = 0; r < oldRows; ++r) {
        const QModelIndex idx = m_model->index(r, 0);
        if (auto* w = qobject_cast<OSCMessageItemWidget*>(indexWidget(idx))) {
            w->removeEventFilter(this);
            w->deleteLater();
        }
    }
    if (oldRows > 0) {
        m_model->removeRows(0, oldRows);
    }
    m_currentCard = nullptr;

    // 添加新卡片
    for (const auto& message : messages) {
        addOSCMessage(message);
    }
}

/**
 * 序列化当前所有消息到 JSON
 * 与原实现保持一致结构：{"messages":[...]}
 */
QJsonObject OSCMessageListWidget::save() const
{
    /**
     * 函数：OSCMessageListWidget::save
     * 作用：序列化当前所有消息到 JSON，保持与原实现一致结构。
     */
    QJsonObject json;
    QJsonArray messagesArray;

    const int rows = m_model ? m_model->rowCount() : 0;
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = m_model->index(r, 0);
        if (auto* widget = qobject_cast<OSCMessageItemWidget*>(indexWidget(idx))) {
            OSCMessage message = widget->getMessage();
            message.value = widget->getExpression();
            messagesArray.append(messageToJson(message));
        }
    }

    json["messages"] = messagesArray;
    return json;
}

int OSCMessageListWidget::count() const
{
    /**
     * 函数：OSCMessageListWidget::count
     * 作用：返回当前模型行数。
     */
    return m_model ? m_model->rowCount() : 0;
}
/**
 * 获取当前选中卡片
 */
OSCMessageItemWidget* OSCMessageListWidget::item(int index) const
{
    /**
     * 函数：OSCMessageListWidget::item
     * 作用：获取指定行对应的卡片部件。
     */
    if (!m_model || index < 0 || index >= m_model->rowCount()) return nullptr;
    const QModelIndex idx = m_model->index(index, 0);
    return qobject_cast<OSCMessageItemWidget*>(indexWidget(idx));
}
/**
 * 从 JSON 加载消息：清空并重建卡片列表
 */
void OSCMessageListWidget::load(const QJsonObject& json)
{
    /**
     * 函数：OSCMessageListWidget::load
     * 作用：从 JSON 加载消息：清空并重建卡片列表。
     */
    setOSCMessages({}); // 清空

    QJsonArray messagesArray = json["messages"].toArray();
    for (const QJsonValue& v : messagesArray) {
        if (!v.isObject()) continue;
        addOSCMessage(jsonToMessage(v.toObject()));
    }
}

/**
 * 拖拽进入：接受 "application/x-osc-address"
 */
void OSCMessageListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    /**
     * 函数：OSCMessageListWidget::dragEnterEvent
     * 作用：拖拽进入：接受 "application/x-osc-address"。
     */
    if (event->mimeData()->hasFormat("application/x-osc-address")) {
        event->acceptProposedAction();
    } else {
        QGridView::dragEnterEvent(event);
    }
}

/**
 * 拖拽移动：接受 "application/x-osc-address"
 */
void OSCMessageListWidget::dragMoveEvent(QDragMoveEvent* event)
{
    /**
     * 函数：OSCMessageListWidget::dragMoveEvent
     * 作用：拖拽移动：接受 "application/x-osc-address"。
     */
    if (event->mimeData()->hasFormat("application/x-osc-address")) {
        event->acceptProposedAction();
    } else {
        QGridView::dragMoveEvent(event);
    }
}

/**
 * 放置事件：从 MIME 数据反序列化并添加为新卡片
 */
void OSCMessageListWidget::dropEvent(QDropEvent* event)
{
    /**
     * 函数：OSCMessageListWidget::dropEvent
     * 作用：放置事件：从 MIME 数据反序列化并添加为新卡片。
     */
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasFormat("application/x-osc-address")) {
        OSCMessage message;

        QByteArray data = mimeData->data("application/x-osc-address");
        QDataStream stream(data);
        if (OnlyInternal) {
            message.host = "127.0.0.1";
            message.port = AppConstants::EXTRA_CONTROL_PORT;
        }
        stream >> message.host >> message.port >> message.address >> message.type >> message.value;
        addOSCMessage(message);
        event->acceptProposedAction();
    } else {
        QGridView::dropEvent(event);
    }
}

/**
 * 鼠标按下：记录拖拽起点，并计算命中的卡片用于选中/拖拽
 */
void OSCMessageListWidget::mousePressEvent(QMouseEvent* event)
{
    /**
     * 函数：OSCMessageListWidget::mousePressEvent
     * 作用：记录拖拽起点并选中命中的卡片索引。
     */
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();

        const QModelIndex idx = indexAt(event->pos());
        if (idx.isValid()) {
            setCurrentIndex(idx);
            if (auto* card = qobject_cast<OSCMessageItemWidget*>(indexWidget(idx))) {
                m_currentCard = card;
            }
        }else {
            setCurrentIndex(QModelIndex());
            m_currentCard = nullptr;
        }
    }
    QGridView::mousePressEvent(event);
}

/**
 * 鼠标移动：当移动距离超过阈值时，对当前卡片发起拖拽
 * 拖拽 MIME 数据为 "application/x-osc-address"
 */
void OSCMessageListWidget::mouseMoveEvent(QMouseEvent* event)
{
    /**
     * 函数：OSCMessageListWidget::mouseMoveEvent
     * 作用：当用户按住 Ctrl 并拖拽当前选中卡片时，导出 OSC 地址的 MIME 数据。
     */
    if (!(event->buttons() & Qt::LeftButton)) {
        QGridView::mouseMoveEvent(event);
        return;
    }

    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        QGridView::mouseMoveEvent(event);
        return;
    }

    if (!(event->modifiers() & Qt::ControlModifier)) {
        QGridView::mouseMoveEvent(event);
        return;
    }

    auto* widget = m_currentCard;
    if (!widget) {
        const QModelIndex idx = currentIndex();
        if (idx.isValid()) {
            widget = qobject_cast<OSCMessageItemWidget*>(indexWidget(idx));
        }
    }
    if (!widget) {
        QGridView::mouseMoveEvent(event);
        return;
    }

    OSCMessage message = widget->getMessage();

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    if (OnlyInternal) {
        message.host = "127.0.0.1";
        message.port = AppConstants::EXTRA_CONTROL_PORT;
    }
    dataStream << message.host << message.port << message.address << message.type << message.value;

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-osc-address", itemData);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    const QSize hint = widget->sizeHint();
    QPixmap pixmap(hint.width(), hint.height());
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(40, 40, 40, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect(), 5, 5);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    QRect textRect = pixmap.rect().adjusted(12, 0, -8, 0);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, message.address);

    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    drag->exec(Qt::CopyAction, Qt::CopyAction);
}

/**
 * 事件过滤：Delete/Backspace 删除当前选中卡片；在 viewport 中处理鼠标按下、移动与拖拽投放
 */
bool OSCMessageListWidget::eventFilter(QObject* obj, QEvent* event)
{
    /**
     * 函数：OSCMessageListWidget::eventFilter
     * 作用：
     *  - 捕捉 Delete/Backspace，删除当前选中卡片；
     *  - 卡片点击选中联动（保留）。
     */
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
            && (obj == this)) {
            deleteSelectedMessage();
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            auto* card = qobject_cast<OSCMessageItemWidget*>(obj);
            if (card) {
                m_currentCard = card;
            }
        }
    }

    return QGridView::eventFilter(obj, event);
}

/**
 * 右键菜单：提供添加、删除、清空
 */
void OSCMessageListWidget::showContextMenu(const QPoint& pos)
{
    /**
     * 函数：OSCMessageListWidget::showContextMenu
     * 作用：右键弹出菜单；根据事件源正确映射位置。
     */
    QMenu menu(this);
    menu.setWindowFlags(menu.windowFlags() | Qt::NoDropShadowWindowHint);
    menu.setAttribute(Qt::WA_TranslucentBackground, false);
    QAction* addAction = menu.addAction(QIcon(":/icons/icons/add.png"),"Add Message");
  
    QAction* deleteAction = menu.addAction(QIcon(":/icons/icons/delete.png"),"Delete Message");
    QAction* clearAction = menu.addAction(QIcon(":/icons/icons/clear.png"),"Clear All Messages");

    connect(clearAction, &QAction::triggered, [this]() {
        setOSCMessages({});
    });
    connect(addAction, &QAction::triggered, this, &OSCMessageListWidget::addNewMessage);
    connect(deleteAction, &QAction::triggered, this, &OSCMessageListWidget::deleteSelectedMessage);

    menu.exec(mapToGlobal(pos));
}

/**
 * 右键：添加新卡片（空消息）
 */
void OSCMessageListWidget::addNewMessage()
{
    /**
     * 函数：OSCMessageListWidget::addNewMessage
     * 作用：添加新空消息卡片。
     */
    addOSCMessage();
}

/**
 * 删除当前选中卡片
 */
void OSCMessageListWidget::deleteSelectedMessage()
{
    /**
     * 函数：OSCMessageListWidget::deleteSelectedMessage
     * 作用：删除当前选中卡片（删除模型行并释放部件）。
     */
    const QModelIndex idx = currentIndex();
    if (!idx.isValid()) return;

    if (auto* w = qobject_cast<OSCMessageItemWidget*>(indexWidget(idx))) {
        w->removeEventFilter(this);
        w->deleteLater();
    }
    m_model->removeRow(idx.row());
    m_currentCard = nullptr;
    setCurrentIndex(QModelIndex());
}

/**
 * 将 OSC 消息转换为 JSON 对象
 */
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

/**
 * 从 JSON 对象解析 OSC 消息
 */
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

// 命中测试 helper 已移除（未使用）

// /**
//  * 设置选中卡片：仅切换边框样式（不使用阴影）
//  */
// void OSCMessageListWidget::setCurrentCard(OSCMessageItemWidget* card)
// {
//     /**
//      * 函数：OSCMessageListWidget::setCurrentCard
//      * 作用：设置选中卡片：仅切换边框样式（不使用阴影）。
//      */
//     if (m_currentCard == card) return;
//
//     if (m_currentCard) {
//         m_currentCard->setSelected(false);
//     }
//
//     m_currentCard = card;
//
//     if (m_currentCard) {
//         m_currentCard->setSelected(true);
//     }
// }

/**
 * 移除指定卡片（从布局与列表中删除并释放）
 */
void OSCMessageListWidget::removeCard(OSCMessageItemWidget* card)
{
    /**
     * 函数：OSCMessageListWidget::removeCard
     * 作用：移除指定卡片（删除模型对应行并释放部件）。
     */
    if (!card || !m_model) return;

    const int rows = m_model->rowCount();
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = m_model->index(r, 0);
        if (indexWidget(idx) == card) {
            card->removeEventFilter(this);
            card->deleteLater();
            m_model->removeRow(r);
            break;
        }
    }
}

