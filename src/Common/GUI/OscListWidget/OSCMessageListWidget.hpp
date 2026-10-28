#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QVector>
#include <QPoint>
#include "OSCMessageItemWidget.hpp"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QMouseEvent;
class QMimeData;
class QScrollArea;
class QVBoxLayout;

#if defined(OSCLISTWIDGET_LIBRARY)
#define OSCLISTWIDGET_EXPORT Q_DECL_EXPORT
#else
#define OSCLISTWIDGET_EXPORT Q_DECL_IMPORT
#endif
class OSCLISTWIDGET_EXPORT OSCMessageListWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * 函数：OSCMessageListWidget::OSCMessageListWidget
     * 作用：构造纵向排列的 OSC 消息列表，每条消息占一行。
     */
    explicit OSCMessageListWidget(bool onlyInternal = false, QWidget* parent = nullptr);
    explicit OSCMessageListWidget(QWidget* parent);

    /**
     * 函数：OSCMessageListWidget::addOSCMessage
     * 作用：新增一条消息并在对应索引上绑定卡片控件。
     */
    void addOSCMessage(const OSCMessage& message = OSCMessage());

    /**
     * 函数：OSCMessageListWidget::getOSCMessages
     * 作用：遍历模型索引并从卡片读取消息内容。
     */
    QVector<OSCMessage> getOSCMessages() const;

    /**
     * 函数：OSCMessageListWidget::setOSCMessages
     * 作用：清空并按顺序添加消息卡片。
     */
    void setOSCMessages(const QVector<OSCMessage>& messages);

    /**
     * 函数：OSCMessageListWidget::save
     * 作用：序列化当前卡片到 JSON，保持原结构。
     */
    QJsonObject save() const;

    /**
     * 函数：OSCMessageListWidget::load
     * 作用：从 JSON 加载消息。
     */
    void load(const QJsonObject& json);

    /**
     * 函数：OSCMessageListWidget::count
     * 作用：返回当前消息数量。
     */
    int count() const;

    /**
     * 函数：OSCMessageListWidget::item
     * 作用：获取指定行的卡片控件。
     */
    OSCMessageItemWidget* item(int index) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    /**
     * 函数：OSCMessageListWidget::dragEnterEvent
     * 作用：拖拽进入时接受 "application/x-osc-address"。
     */
    void dragEnterEvent(QDragEnterEvent* event) override;

    /**
     * 函数：OSCMessageListWidget::dragMoveEvent
     * 作用：拖拽移动事件，维持接受状态。
     */
    void dragMoveEvent(QDragMoveEvent* event) override;

    /**
     * 函数：OSCMessageListWidget::dropEvent
     * 作用：放置事件，反序列化并新增消息卡片。
     */
    void dropEvent(QDropEvent* event) override;

    /**
     * 函数：OSCMessageListWidget::mousePressEvent
     * 作用：记录拖拽起点并选中命中索引。
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * 函数：OSCMessageListWidget::mouseMoveEvent
     * 作用：Ctrl+拖拽导出 OSC 地址的 MIME 数据。
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * 函数：OSCMessageListWidget::eventFilter
     * 作用：处理 Delete/Backspace 删除与卡片点击选中联动。
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * 函数：OSCMessageListWidget::showContextMenu
     * 作用：右键菜单，提供添加/删除/清空。
     */
    void showContextMenu(const QPoint& pos);

    /**
     * 函数：OSCMessageListWidget::addNewMessage
     * 作用：添加空消息卡片。
     */
    void addNewMessage();

    /**
     * 函数：OSCMessageListWidget::deleteSelectedMessage
     * 作用：删除当前选中卡片。
     */
    void deleteSelectedMessage();

private:
    /**
     * 函数：OSCMessageListWidget::messageToJson
     * 作用：将单个 OSCMessage 转为 JSON 对象。
     */
    static QJsonObject messageToJson(const OSCMessage& message);

    /**
     * 函数：OSCMessageListWidget::jsonToMessage
     * 作用：从 JSON 对象解析 OSCMessage。
     */
    static OSCMessage jsonToMessage(const QJsonObject& json);

    /**
     * 函数：OSCMessageListWidget::removeCard
     * 作用：根据控件定位模型行并删除。
     */
    void removeCard(OSCMessageItemWidget* card);

    void watchCard(OSCMessageItemWidget* card);
    void clearItems();
    void setCurrentCard(OSCMessageItemWidget* card);
    void startDrag(OSCMessageItemWidget* card);
    bool acceptOscMime(const QMimeData* mimeData) const;
    bool handleMimeDrop(const QMimeData* mimeData);
    OSCMessageItemWidget* cardFromObject(QObject* obj) const;

    QScrollArea* m_scroll = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_layout = nullptr;
    QVector<OSCMessageItemWidget*> m_items;
    OSCMessageItemWidget* m_currentCard = nullptr;
    QPoint dragStartPosition;
    bool m_dragActive = false;
    bool m_pressOnCard = false;

    // 仅内部（隐藏 host:port，固定到本地）
    bool OnlyInternal = false;
};
