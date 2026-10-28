#include "OSCMessageListWidget.hpp"

#include <QApplication>
#include <QDataStream>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

#include "Common/AppConfig/ConfigManager.h"

namespace {
QPoint mouseGlobalPos(const QMouseEvent* event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
}
}

OSCMessageListWidget::OSCMessageListWidget(bool onlyInternal, QWidget* parent)
    : QWidget(parent), OnlyInternal(onlyInternal)
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setAlignment(Qt::AlignTop);
    m_scroll->setAcceptDrops(true);
    m_scroll->viewport()->setAcceptDrops(true);

    m_container = new QWidget(m_scroll);
    m_container->setAcceptDrops(true);
    m_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_layout = new QVBoxLayout(m_container);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(2);
    m_layout->addStretch(1);
    m_scroll->setWidget(m_container);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_scroll);

    setContextMenuPolicy(Qt::CustomContextMenu);
    m_scroll->setContextMenuPolicy(Qt::CustomContextMenu);
    m_scroll->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
    m_container->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &OSCMessageListWidget::showContextMenu);
    connect(m_scroll, &QWidget::customContextMenuRequested, this, &OSCMessageListWidget::showContextMenu);
    connect(m_scroll->viewport(), &QWidget::customContextMenuRequested, this, &OSCMessageListWidget::showContextMenu);
    connect(m_container, &QWidget::customContextMenuRequested, this, &OSCMessageListWidget::showContextMenu);

    installEventFilter(this);
    m_scroll->viewport()->installEventFilter(this);
    m_container->installEventFilter(this);
}

OSCMessageListWidget::OSCMessageListWidget(QWidget* parent)
    : OSCMessageListWidget(false, parent)
{
}

void OSCMessageListWidget::watchCard(OSCMessageItemWidget* widget)
{
    const auto hook = [this](QWidget* target) {
        target->installEventFilter(this);
        target->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(target, &QWidget::customContextMenuRequested, this, &OSCMessageListWidget::showContextMenu);
    };

    hook(widget);
    const auto children = widget->findChildren<QWidget*>();
    for (QWidget* child : children) {
        hook(child);
    }

    connect(widget, &OSCMessageItemWidget::requestDelete, this, [this, widget]() {
        removeCard(widget);
    });
}

void OSCMessageListWidget::addOSCMessage(const OSCMessage& message)
{
    auto* widget = new OSCMessageItemWidget(OnlyInternal, m_container);
    watchCard(widget);

    if (!message.address.isEmpty()) {
        widget->setMessage(message);
    }

    m_layout->insertWidget(m_layout->count() - 1, widget);
    m_items.append(widget);
}

QVector<OSCMessage> OSCMessageListWidget::getOSCMessages() const
{
    QVector<OSCMessage> messages;
    messages.reserve(m_items.size());
    for (OSCMessageItemWidget* widget : m_items) {
        if (widget) {
            messages.append(widget->getMessage());
        }
    }
    return messages;
}

void OSCMessageListWidget::clearItems()
{
    const QVector<OSCMessageItemWidget*> items = m_items;
    m_items.clear();
    m_currentCard = nullptr;
    for (OSCMessageItemWidget* card : items) {
        if (!card) {
            continue;
        }
        card->removeEventFilter(this);
        const auto children = card->findChildren<QWidget*>();
        for (QWidget* child : children) {
            child->removeEventFilter(this);
        }
        m_layout->removeWidget(card);
        card->deleteLater();
    }
}

void OSCMessageListWidget::setOSCMessages(const QVector<OSCMessage>& messages)
{
    clearItems();
    for (const auto& message : messages) {
        addOSCMessage(message);
    }
}

QJsonObject OSCMessageListWidget::save() const
{
    QJsonObject json;
    QJsonArray messagesArray;
    for (OSCMessageItemWidget* widget : m_items) {
        if (!widget) {
            continue;
        }
        OSCMessage message = widget->getMessage();
        message.value = widget->getExpression();
        messagesArray.append(messageToJson(message));
    }
    json["messages"] = messagesArray;
    return json;
}

int OSCMessageListWidget::count() const
{
    return m_items.size();
}

OSCMessageItemWidget* OSCMessageListWidget::item(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }
    return m_items.at(index);
}

void OSCMessageListWidget::load(const QJsonObject& json)
{
    setOSCMessages({});

    const QJsonArray messagesArray = json["messages"].toArray();
    for (const QJsonValue& v : messagesArray) {
        if (!v.isObject()) {
            continue;
        }
        addOSCMessage(jsonToMessage(v.toObject()));
    }
}

QSize OSCMessageListWidget::sizeHint() const
{
    return QSize(260, 72);
}

QSize OSCMessageListWidget::minimumSizeHint() const
{
    return QSize(160, 28);
}

bool OSCMessageListWidget::acceptOscMime(const QMimeData* mimeData) const
{
    return mimeData && mimeData->hasFormat("application/x-osc-address");
}

bool OSCMessageListWidget::handleMimeDrop(const QMimeData* mimeData)
{
    if (!mimeData || !mimeData->hasFormat("application/x-osc-address")) {
        return false;
    }

    OSCMessage message;
    QByteArray data = mimeData->data("application/x-osc-address");
    QDataStream stream(data);
    if (OnlyInternal) {
        message.host = "127.0.0.1";
        message.port = ConfigManager::instance().getExtraControlPort();
    }
    stream >> message.host >> message.port >> message.address >> message.type >> message.value;
    addOSCMessage(message);
    return true;
}

void OSCMessageListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (acceptOscMime(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void OSCMessageListWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (acceptOscMime(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        QWidget::dragMoveEvent(event);
    }
}

void OSCMessageListWidget::dropEvent(QDropEvent* event)
{
    if (handleMimeDrop(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        QWidget::dropEvent(event);
    }
}

void OSCMessageListWidget::setCurrentCard(OSCMessageItemWidget* card)
{
    m_currentCard = card;
}

OSCMessageItemWidget* OSCMessageListWidget::cardFromObject(QObject* obj) const
{
    auto* widget = qobject_cast<QWidget*>(obj);
    while (widget) {
        if (auto* card = qobject_cast<OSCMessageItemWidget*>(widget)) {
            return card;
        }
        widget = widget->parentWidget();
    }
    return nullptr;
}

void OSCMessageListWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = mouseGlobalPos(event);
        m_pressOnCard = false;
    }
    QWidget::mousePressEvent(event);
}

void OSCMessageListWidget::startDrag(OSCMessageItemWidget* widget)
{
    if (!widget) {
        return;
    }

    OSCMessage message = widget->getMessage();
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    if (OnlyInternal) {
        message.host = "127.0.0.1";
        message.port = ConfigManager::instance().getExtraControlPort();
    }
    dataStream << message.host << message.port << message.address << message.type << message.value;

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-osc-address", itemData);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    QSize hint = widget->size();
    if (hint.width() < 40 || hint.height() < 16) {
        hint = QSize(200, 28);
    }
    QPixmap pixmap(hint);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(40, 40, 40, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect(), 4, 4);
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    painter.drawText(pixmap.rect().adjusted(8, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft, message.address);

    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));
    drag->exec(Qt::CopyAction, Qt::CopyAction);
}

void OSCMessageListWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton) || !(event->modifiers() & Qt::ControlModifier)
        || !m_pressOnCard || !m_currentCard) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    if ((mouseGlobalPos(event) - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    if (m_dragActive) {
        return;
    }
    m_dragActive = true;
    startDrag(m_currentCard);
    m_dragActive = false;
}

bool OSCMessageListWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
            && (obj == this || obj == m_scroll || obj == m_scroll->viewport() || obj == m_container)) {
            deleteSelectedMessage();
            return true;
        }
    }

    if (event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (acceptOscMime(dragEvent->mimeData())) {
            dragEvent->acceptProposedAction();
            return true;
        }
    } else if (event->type() == QEvent::DragMove) {
        auto* dragEvent = static_cast<QDragMoveEvent*>(event);
        if (acceptOscMime(dragEvent->mimeData())) {
            dragEvent->acceptProposedAction();
            return true;
        }
    } else if (event->type() == QEvent::Drop) {
        auto* drop = static_cast<QDropEvent*>(event);
        if (handleMimeDrop(drop->mimeData())) {
            drop->acceptProposedAction();
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton || me->button() == Qt::RightButton) {
            dragStartPosition = mouseGlobalPos(me);
            if (OSCMessageItemWidget* card = cardFromObject(obj)) {
                setCurrentCard(card);
                m_pressOnCard = true;
            } else {
                m_pressOnCard = false;
                if (obj == m_container || obj == m_scroll->viewport()) {
                    setFocus();
                }
            }
        }
    } else if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (m_pressOnCard
            && (me->buttons() & Qt::LeftButton)
            && (me->modifiers() & Qt::ControlModifier)
            && m_currentCard
            && !m_dragActive
            && (mouseGlobalPos(me) - dragStartPosition).manhattanLength() >= QApplication::startDragDistance()) {
            m_dragActive = true;
            startDrag(m_currentCard);
            m_dragActive = false;
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void OSCMessageListWidget::showContextMenu(const QPoint& pos)
{
    QWidget* source = qobject_cast<QWidget*>(sender());
    const QPoint global = source ? source->mapToGlobal(pos) : mapToGlobal(pos);
    if (OSCMessageItemWidget* card = cardFromObject(source)) {
        setCurrentCard(card);
    }

    QLineEdit* edit = qobject_cast<QLineEdit*>(source);
    QMenu* editMenu = edit ? edit->createStandardContextMenu() : nullptr;

    {
        QMenu menu(this);
        menu.setWindowFlags(menu.windowFlags() | Qt::NoDropShadowWindowHint);
        menu.setAttribute(Qt::WA_TranslucentBackground, false);
        QAction* addAction = menu.addAction(QIcon(":/icons/icons/add.png"), "Add Message");
        QAction* deleteAction = menu.addAction(QIcon(":/icons/icons/delete.png"), "Delete Message");
        QAction* clearAction = menu.addAction(QIcon(":/icons/icons/clear.png"), "Clear All Messages");

        if (editMenu && !editMenu->actions().isEmpty()) {
            menu.addSeparator();
            menu.addActions(editMenu->actions());
        }

        connect(clearAction, &QAction::triggered, this, [this]() {
            setOSCMessages({});
        });
        connect(addAction, &QAction::triggered, this, &OSCMessageListWidget::addNewMessage);
        connect(deleteAction, &QAction::triggered, this, &OSCMessageListWidget::deleteSelectedMessage);

        menu.exec(global);
    }

    delete editMenu;
}

void OSCMessageListWidget::addNewMessage()
{
    addOSCMessage();
}

void OSCMessageListWidget::deleteSelectedMessage()
{
    if (!m_currentCard) {
        return;
    }
    removeCard(m_currentCard);
}

QJsonObject OSCMessageListWidget::messageToJson(const OSCMessage& message)
{
    QJsonObject messageJson;
    messageJson["host"] = message.host;
    messageJson["port"] = message.port;
    messageJson["address"] = message.address;
    messageJson["type"] = message.type;
    if (message.value.typeId() == QMetaType::QVariantList) {
        messageJson["value"] = QJsonArray::fromVariantList(message.value.toList());
    } else {
        messageJson["value"] = QJsonValue::fromVariant(message.value);
    }
    return messageJson;
}

OSCMessage OSCMessageListWidget::jsonToMessage(const QJsonObject& json)
{
    OSCMessage message;
    message.host = json["host"].toString();
    message.port = json["port"].toInt();
    message.address = json["address"].toString();
    message.type = json["type"].toString();
    const QJsonValue value = json["value"];
    if (value.isArray()) {
        message.value = value.toArray().toVariantList();
        if (message.type.isEmpty()) {
            message.type = QStringLiteral("List");
        }
    } else if (value.isDouble()) {
        message.value = value.toDouble();
    } else if (value.isBool()) {
        message.value = value.toBool();
    } else {
        message.value = value.toString();
    }
    return message;
}

void OSCMessageListWidget::removeCard(OSCMessageItemWidget* card)
{
    if (!card || !m_items.contains(card)) {
        return;
    }
    if (m_currentCard == card) {
        m_currentCard = nullptr;
    }
    card->removeEventFilter(this);
    const auto children = card->findChildren<QWidget*>();
    for (QWidget* child : children) {
        child->removeEventFilter(this);
    }
    m_items.removeOne(card);
    m_layout->removeWidget(card);
    card->deleteLater();
}
