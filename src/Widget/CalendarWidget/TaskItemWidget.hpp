#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QJSEngine>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include <QTimeEdit>
#include <QCheckBox>                 // 新增：周几选择控件
#include <QGroupBox>
#include <QTextEdit>

#include "PushButton.h"
#include "ScheduledTaskModel.hpp"    // 新增：使用 ScheduledInfo 结构体
class TaskItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskItemWidget(QWidget* parent = nullptr);
    
    // 获取当前OSC消息
    OSCMessage getMessage() const;
    
    // 设置OSC消息
    void setMessage(const OSCMessage& message);
    // 设置备注
    void setRemark(const QString& remark);
    // 获取备注
    QString getRemark() const;
public slots:
    void updateValueWidget(const QString& type);
    void testCommand() const;
signals:
    void messageChanged();
public:
    /**
     * @brief 获取调度信息（类型与时间 + 循环条件）
     * 当类型为 loop 时返回选中的星期条件；once 时返回空条件
     */
    ScheduledInfo getScheduledInfo() const;

    /**
     * @brief 设置调度信息（类型与时间 + 循环条件）
     * 根据传入的 ScheduledInfo 更新界面显示
     */
    void setScheduledInfo(const ScheduledInfo& info);

private:
    /**
     * 函数：TaskItemWidget::setupUI
     * 作用：构建“紧凑卡片风格”的任务编辑界面，包含左侧强调条与统一控件样式，
     *       与 OSCMessageItemWidget 风格保持一致以提升可读性与一致性。
     */
    void setupUI();
    /**
     * 函数：TaskItemWidget::connectSignals
     * 作用：连接各控件的信号，确保数据编辑变更与发送测试事件能够正确通知。
     */
    void connectSignals();
    /**
     * 函数：TaskItemWidget::applyCardStyle
     * 作用：应用卡片统一样式（圆角、暗色背景、输入控件风格、强调条颜色等）。
     */
    QLineEdit* addressEdit;
    QComboBox* typeCombo;
    QLineEdit* valueEdit;
    QTimeEdit* timeEdit;
    QPushButton* btnTest;
    mutable OSCMessage m_currentMessage;
    QLineEdit* remarkEdit;

    QGroupBox* loopGroup  = nullptr;
    QCheckBox* chkMonday = nullptr;
    QCheckBox* chkTuesday = nullptr;
    QCheckBox* chkWednesday = nullptr;
    QCheckBox* chkThursday = nullptr;
    QCheckBox* chkFriday = nullptr;
    QCheckBox* chkSaturday = nullptr;
    QCheckBox* chkSunday = nullptr;

    // 新增：用于在编辑器生命周期内保存当前任务的日期，避免 getScheduledInfo 使用当前系统日期
    QDate m_boundDate;

    /**
     * 卡片风格元素：与 OSCMessageItemWidget 保持一致
     */
    QFrame* m_accentBar = nullptr;
};
