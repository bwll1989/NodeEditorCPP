#pragma once

#include <QCalendarWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDate>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QMap>
#include "Common/Devices/OSCSender/OSCSender.h"
#include "ScheduledTaskModel.hpp" // 模型类型
/**
 * @brief 支持OSC信息拖入的日历控件类
 * 继承自QCalendarWidget，添加了拖拽功能和OSC数据管理
 */
class OscCalendarWidget : public QCalendarWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit OscCalendarWidget(ScheduledTaskModel* model = nullptr,QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~OscCalendarWidget();

    /**
     * @brief 检查指定日期是否有OSC消息
     * @param date 日期
     * @return 是否有OSC消息
     */
    bool hasOscMessageForDate(const QDate& date) const;

protected:
    /**
     * @brief 右键菜单事件处理
     * @param event 右键菜单事件
     */
    void contextMenuEvent(QContextMenuEvent *event) override;
    
    /**
     * @brief 绘制单元格事件处理（用于显示OSC数据标识）
     * @param painter 绘制器
     * @param rect 绘制区域
     * @param date 日期
     */
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;

public slots:
    /**
     * @brief 处理日期选择变化
     * @param date 选中的日期
     */
    void onDateSelectionChanged();
private:
    /**
     * @brief 获取鼠标位置对应的日期
     * @param pos 鼠标位置
     * @return 对应的日期
     */
    QDate getDateAtPosition(const QPoint& pos);
    QMenu* createContextMenu(const QDate& date);

signals:
    /**
     * @brief OSC消息被拖入信号
     * @param date 日期
     * @param oscMessage OSC消息
     */
    void oscMessageDropped(const QDate& date, const OSCMessage& oscMessage);
    
    /**
     * @brief OSC消息被删除信号
     * @param date 日期
     */
    void oscMessageRemoved(const QDate& date);
    
    /**
     * @brief OSC消息被修改信号
     * @param date 日期
     * @param oscMessage 新的OSC消息
     */
    void oscMessageChanged(const QDate& date, const OSCMessage& oscMessage);

private:
    QDate m_contextMenuDate;                  ///< 右键菜单对应的日期
    ScheduledTaskModel* m_model = nullptr;

};