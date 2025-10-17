#pragma once

// 顶部包含区域
#include <QListView>
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
// 类 TaskListWidget
class TaskListWidget : public QListView {
    Q_OBJECT
public:
    /**
     * @brief 构造函数：绑定模型与委托，打开持久编辑器
     */
    explicit TaskListWidget(ScheduledTaskModel* taskModel ,QWidget* parent = nullptr);

    /**
     * @brief 添加新的 OSC 消息到模型（新建一行）
     */
    void addScheduled(const OSCMessage& message = OSCMessage());

    /**
     * @brief 添加一个完整的计划项（OSC + Scheduled）
     */
    void addScheduled(const ScheduledTaskItem& m_scheduled);

    /**
     * @brief 将模型序列化为 JSON
     */
    QJsonObject save() const;
    
    /**
     * @brief 从 JSON 反序列化并刷新视图
     */
    void load(const QJsonObject& json);
    /**
         * @brief 设置当前显示的日期（来自日历选择），驱动过滤刷新
         */
    void setDate(const QDate& date);

    /**
     * @brief 为模型内所有行打开持久编辑器
     */
    void openEditorsForAllRows();
protected:
    /**
     * @brief 拖拽进入事件处理（接受特定 MIME）
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    /**
     * @brief 拖拽移动事件处理（接受特定 MIME）
     */
    void dragMoveEvent(QDragMoveEvent* event) override;
    /**
     * @brief 放置事件处理（反序列化为 OSCMessage 并写入模型）
     */
    void dropEvent(QDropEvent* event) override;

    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    /**
     * @brief 右键菜单显示
     */
    void showContextMenu(const QPoint& pos);
    /**
     * @brief 右键菜单：添加消息
     */
    void addNewScheduled();
    /**
     * @brief 右键菜单：删除当前选中消息
     */
    void deleteSelectedMessage();

private:

    // 将单个OSC消息转换为JSON对象
    static QJsonObject messageToJson(const OSCMessage& message);
    
    // 从JSON对象创建OSC消息
    static OSCMessage jsonToMessage(const QJsonObject& json);

    ScheduledTaskModel* m_model = nullptr;
    // 引入日期过滤代理模型
    ScheduledTaskFilterProxyModel* m_proxy = nullptr;


};

