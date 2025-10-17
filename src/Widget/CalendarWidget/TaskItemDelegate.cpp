//
// Created by WuBin on 2025/10/15.
//

#include "TaskItemDelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QPainter>
#include "TaskItemWidget.hpp"
#include <QApplication>
QWidget* TaskItemDelegate::createEditor(QWidget* parent,
                                                        const QStyleOptionViewItem& /*option*/,
                                                        const QModelIndex& /*index*/) const
{
    auto* w = new TaskItemWidget(parent);

    /**
     * @brief 当编辑器内容变更时，将提交延迟到下一事件循环
     * 原因：openPersistentEditor 创建期间，视图尚未完成对 editor 的注册，立即触发 commitData 会导致
     * "editor that does not belong to this view" 的告警。排队到下一拍可确保 editor 已被视图认领。
     */
    QObject::connect(w, &TaskItemWidget::messageChanged, [this, w]() {
        QTimer::singleShot(0, this, [this, w]() {
            emit const_cast<TaskItemDelegate*>(this)->commitData(w);
        });
    });
    return w;
}

// 所属类：TaskItemDelegate
void TaskItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* w = qobject_cast<TaskItemWidget*>(editor);
    if (!w || !index.isValid()) return;

    QSignalBlocker blockSignals(w);

    const QAbstractItemModel* mdl = index.model();
    if (!mdl) return;

    // 从模型角色填充 OSCMessage
    OSCMessage msg;
    msg.host    = mdl->data(index, ScheduledTaskModel::RoleHost).toString();
    msg.port    = mdl->data(index, ScheduledTaskModel::RolePort).toInt();
    msg.address = mdl->data(index, ScheduledTaskModel::RoleAddress).toString();
    msg.type    = mdl->data(index, ScheduledTaskModel::RoleType).toString();
    msg.value   = mdl->data(index, ScheduledTaskModel::RoleValue).toString();
    w->setMessage(msg);
    w->setRemark(mdl->data(index, ScheduledTaskModel::RoleRemarks).toString());
    // 从模型角色填充 ScheduledInfo
    ScheduledInfo info;
    info.type = mdl->data(index, ScheduledTaskModel::RoleScheduleType).toString();
    const QTime t = mdl->data(index, ScheduledTaskModel::RoleTime).toTime();
    const QDate d = mdl->data(index, ScheduledTaskModel::RoleDate).toDate();
    const QDate useDate = d.isValid() ? d : QDate::currentDate();
    info.time = QDateTime(useDate, t.isValid() ? t : QTime(0,0,0));
    info.conditions = mdl->data(index, ScheduledTaskModel::RoleLoopDays).toStringList();
    w->setScheduledInfo(info);
}

void TaskItemDelegate::setModelData(QWidget* editor,
                                                    QAbstractItemModel* model,
                                                    const QModelIndex& index) const
{
    auto* w = qobject_cast<TaskItemWidget*>(editor);
    if (!w || !index.isValid() || !model) return;
    // 将编辑器的 OSCMessage 写回模型
    const OSCMessage msg = w->getMessage();
    model->setData(index, msg.host,    ScheduledTaskModel::RoleHost);
    model->setData(index, msg.port,    ScheduledTaskModel::RolePort);
    model->setData(index, msg.address, ScheduledTaskModel::RoleAddress);
    model->setData(index, msg.type,    ScheduledTaskModel::RoleType);
    model->setData(index, msg.value,   ScheduledTaskModel::RoleValue);
    const ScheduledInfo info = w->getScheduledInfo();
    model->setData(index, info.type,   ScheduledTaskModel::RoleScheduleType);
    model->setData(index, info.time.time(), ScheduledTaskModel::RoleTime);
    model->setData(index, info.time.date(), ScheduledTaskModel::RoleDate);
    model->setData(index, info.conditions, ScheduledTaskModel::RoleLoopDays);
    model->setData(index, w->getRemark(), ScheduledTaskModel::RoleRemarks);
}

void TaskItemDelegate::updateEditorGeometry(QWidget* editor,
                                                            const QStyleOptionViewItem& option,
                                                            const QModelIndex& /*index*/) const
{
    editor->setGeometry(option.rect);
}

/**
 * @brief 返回行的推荐尺寸
 * - 基础高度来自一个临时 TaskItemWidget 的 sizeHint
 * - 若为 loop 模式，额外加上周几选择行的高度
 */
QSize TaskItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    Q_UNUSED(option);

    // 最低保障高度，避免主题过紧
    return QSize(120,150);
}

/**
 * @brief 自定义绘制
 * 只绘制背景和选中高亮，不绘制文本，避免与持久编辑器叠加造成“挤在一起”的错觉
 */
// 所属类：TaskItemDelegate
void TaskItemDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    /**
     * @brief 仅绘制行的背景与选中高亮，不绘制文本与图标
     * 原因：
     * - 我们使用持久编辑器(TaskItemWidget)来展示完整内容，若这里再绘制文本会与编辑器叠加，造成“控件挤在一起”的错觉。
     * - QWidget 指针应使用 option.widget（视图的 viewport），而不是从 painter->device() 强转。
     */
    QStyleOptionViewItem opt(option);
    // 初始化样式选项（保持选中/焦点等状态）
    initStyleOption(&opt, index);

    // 优先使用视图的样式
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    // 只绘制面板背景及选中高亮
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    // 不绘制 text/icon，让持久编辑器独占内容区域
}