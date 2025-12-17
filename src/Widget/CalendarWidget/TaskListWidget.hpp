#pragma once

// 顶部包含区域
#include "Elements/QGridView/QGridView.h"  // 新增：使用自定义网格视图
#include <QMenu>
#include <QJsonObject>
#include <QJsonArray>
#include "TaskItemWidget.hpp"
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include <QPixmap>
#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QIcon>
#include <QFont>
#include "TaskItemDelegate.h"
#include "ScheduledTaskModel.hpp" // 引入模型类型
#include "ScheduledTaskFilterProxyModel.hpp"
#include "Elements/FlowWidget/FlowWidget.h"
// 类 TaskListWidget
class TaskListWidget : public QGridView {
    Q_OBJECT
public:
    /**
     * 函数：TaskListWidget::TaskListWidget
     * 作用：构造网格视图控件，绑定模型与委托，开启持久编辑器。
     * 变更点：基类由 QListView 改为 QGridView。
     */
    explicit TaskListWidget(ScheduledTaskModel* taskModel ,QWidget* parent = nullptr);

    /**
     * 函数：TaskListWidget::addScheduled
     * 作用：添加新的 OSC 消息到模型（新建一行）。
     */
    void addScheduled(const OSCMessage& message = OSCMessage());

    /**
     * 函数：TaskListWidget::addScheduled
     * 作用：添加一个完整的计划项（OSC + Scheduled）。
     */
    void addScheduled(const ScheduledTaskItem& m_scheduled);

    /**
     * 函数：TaskListWidget::save
     * 作用：将模型序列化为 JSON。
     */
    QJsonObject save() const;
    
    /**
     * 函数：TaskListWidget::load
     * 作用：从 JSON 反序列化并刷新视图。
     */
    void load(const QJsonObject& json);

    /**
     * 函数：TaskListWidget::setDate
     * 作用：设置当前显示的日期（来自日历选择），驱动过滤刷新。
     */
    void setDate(const QDate& date);

    /**
     * 函数：TaskListWidget::openEditorsForAllRows
     * 作用：为模型内所有行设置 indexWidget（每格一个 TaskItemWidget）。
     */
    void openEditorsForAllRows();

protected:
    /**
     * 函数：TaskListWidget::dragEnterEvent
     * 作用：拖拽进入事件处理（接受特定 MIME）。
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    /**
     * 函数：TaskListWidget::dragMoveEvent
     * 作用：拖拽移动事件处理（接受特定 MIME）。
     */
    void dragMoveEvent(QDragMoveEvent* event) override;
    /**
     * 函数：TaskListWidget::dropEvent
     * 作用：放置事件处理（反序列化为 OSCMessage 并写入模型）。
     */
    void dropEvent(QDropEvent* event) override;

    /**
     * 函数：TaskListWidget::eventFilter
     * 作用：处理 Delete/Backspace 删除快捷键。
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

    /**
     * 函数：TaskListWidget::resizeEvent
     * 作用：窗口缩放时，触发网格延迟布局并刷新索引部件几何。
     */
    void resizeEvent(QResizeEvent* event) override;
private:
    /**
     * 函数：TaskListWidget::showContextMenu
     * 作用：右键菜单显示。
     */
    void showContextMenu(const QPoint& pos);
    /**
     * 函数：TaskListWidget::addNewScheduled
     * 作用：右键菜单：添加消息。
     */
    void addNewScheduled();
    /**
     * 函数：TaskListWidget::deleteSelectedMessage
     * 作用：右键菜单：删除当前选中消息。
     */
    void deleteSelectedMessage();

     /**
     * 函数：TaskListWidget::clearAllScheduled
     * 作用：右键菜单：清除所有计划项。
     */
    void clearAllScheduled();

private:
    // 将单个OSC消息转换为JSON对象
    static QJsonObject messageToJson(const OSCMessage& message);
    
    // 从JSON对象创建OSC消息
    static OSCMessage jsonToMessage(const QJsonObject& json);

    ScheduledTaskModel* m_model = nullptr;
    // 引入日期过滤代理模型
    ScheduledTaskFilterProxyModel* m_proxy = nullptr;
    // 上下文菜单
    QMenu* menu=new QMenu(this);
public:
    QMenu* getMenu() {return menu;}
};

