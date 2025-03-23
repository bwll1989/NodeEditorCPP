#ifndef OSCMESSAGELISTWIDGET_HPP
#define OSCMESSAGELISTWIDGET_HPP

#include <QListWidget>
#include <QMenu>
#include <QJsonObject>
#include <QJsonArray>
#include "OSCMessageItemWidget.hpp"
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include <QPixmap>
#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QIcon>
#include <QFont>
class OSCMessageListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit OSCMessageListWidget(QWidget* parent = nullptr);

    // 添加新的OSC消息
    void addOSCMessage(const OSCMessage& message = OSCMessage());
    
    // 获取所有OSC消息
    QVector<OSCMessage> getOSCMessages() const;
    
    // 设置OSC消息列表
    void setOSCMessages(const QVector<OSCMessage>& messages);

    // 序列化当前所有消息到JSON
    QJsonObject save() const;
    
    // 从JSON加载消息
    void load(const QJsonObject& json);

protected:
    // 拖拽事件处理
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void showContextMenu(const QPoint& pos);
    void addNewMessage();
    void deleteSelectedMessage();

private:
    QPoint dragStartPosition;
    static const int DRAG_DISTANCE = 10;

    // 将单个OSC消息转换为JSON对象
    static QJsonObject messageToJson(const OSCMessage& message);
    
    // 从JSON对象创建OSC消息
    static OSCMessage jsonToMessage(const QJsonObject& json);
};

#endif // OSCMESSAGELISTWIDGET_HPP 