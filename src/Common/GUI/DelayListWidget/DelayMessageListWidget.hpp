#pragma once

#include <QListWidget>
#include <QMenu>
#include <QJsonObject>
#include <QJsonArray>
#include "DelayMessageItemWidget.hpp"
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include <QPixmap>
#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QIcon>
#include <QFont>

#include "DelayMessageItemWidget.hpp"
#if defined(DELAYLISTWIDGET_LIBRARY)
#define DELAYLISTWIDGET_EXPORT Q_DECL_EXPORT
#else
#define DELAYLISTWIDGET_EXPORT Q_DECL_IMPORT
#endif
class DELAYLISTWIDGET_EXPORT DelayMessageListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit DelayMessageListWidget(QWidget* parent = nullptr);

    // 添加新的OSC消息
    void addDelayMessage(const delay_item& message = delay_item());
    
    // 获取所有OSC消息
    QVector<delay_item> getDelayMessages() const;
    
    // 设置OSC消息列表
    void setDelayMessages(const QVector<delay_item>& messages);

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
    bool eventFilter(QObject* obj, QEvent* event) override;
private slots:
    void showContextMenu(const QPoint& pos);
    void addNewMessage();
    void deleteSelectedMessage();

private:
    QPoint dragStartPosition;
    static const int DRAG_DISTANCE = 10;

    // 将单个OSC消息转换为JSON对象
    static QJsonObject messageToJson(const delay_item& message);
    
    // 从JSON对象创建OSC消息
    static delay_item jsonToMessage(const QJsonObject& json);
};
