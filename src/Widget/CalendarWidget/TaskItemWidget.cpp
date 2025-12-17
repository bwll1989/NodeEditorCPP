// 文件顶部的包含
#include "TaskItemWidget.hpp"

#include <QEvent>
#include <QLabel>
#include <QValidator>
#include <QRegularExpressionValidator>

#include <QGroupBox>   // Command组与Loop组
#include <QPushButton> // Test按钮
#include <QFont>       // 大号时间字体
#include "ConstantDefines.h"
#include <QGraphicsDropShadowEffect>
TaskItemWidget::TaskItemWidget(QWidget* parent)
    : QWidget(parent)
{
    /**
     * 函数：TaskItemWidget::TaskItemWidget
     * 作用：构造任务项编辑卡片，初始化统一的“紧凑卡片风格”界面并连接信号。
     */
    setupUI();
    connectSignals();
}

void TaskItemWidget::setupUI() {
    /**
     * 函数：TaskItemWidget::setupUI
     * 作用：构建紧凑卡片式编辑界面，并在右上角放置测试按钮，
     *       左侧强调条与控件样式对齐 OSCMessageItemWidget 的风格。
     */
    auto* outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(2, 2, 2, 2);
    outerLayout->setSpacing(0);

    // 卡片内部：左侧强调条 + 右侧表单
    auto* cardHBox = new QHBoxLayout();
    cardHBox->setContentsMargins(6, 6, 6, 6);
    cardHBox->setSpacing(8);
    outerLayout->addLayout(cardHBox);

    m_accentBar = new QFrame(this);
    m_accentBar->setObjectName("accentBar");
    m_accentBar->setFixedWidth(3);
    cardHBox->addWidget(m_accentBar);

    auto* mainLayout = new QGridLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setHorizontalSpacing(8);
    mainLayout->setVerticalSpacing(2);
    cardHBox->addLayout(mainLayout);

    // 第一行：居中时间显示（使用 timeEdit 的值），大号字体
    timeEdit = new QTimeEdit(this);
    timeEdit->setObjectName("lblTimeDisplay");
    timeEdit->setDisplayFormat("HH:mm:ss");
    timeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    timeEdit->setAlignment(Qt::AlignCenter);
    {
        QFont f = timeEdit->font();
        f.setPointSize(f.pointSize() + 10);
        f.setBold(true);
        timeEdit->setFont(f);
    }
    // 第一行：时间（占左侧三列）
    mainLayout->addWidget(timeEdit, 0, 0, 1, 3);

    // 第二行：备注栏（整行）
    remarkEdit = new QLineEdit(this);
    remarkEdit->setObjectName("remarkEdit");
    remarkEdit->setPlaceholderText("备注（例如：早上开机）");
    remarkEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(remarkEdit, 1, 0, 1, 4);
    addressEdit = new QLineEdit(this);
    addressEdit->setStyleSheet(
        ""
    );
    addressEdit->setPlaceholderText("address");

    typeCombo = new QComboBox(this);
    typeCombo->addItems({ "Int", "Float","String"});


    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText("value");
    valueEdit->setStyleSheet("");
    
    // Test 按钮（位于 Command 组右侧）
    btnTest = new QPushButton(QIcon(":/icons/icons/send.png"), "", this);
    btnTest->setFlat(true);

    btnTest->setObjectName("btnTest");
    btnTest->setFixedSize(16, 16);
    btnTest->setFocusPolicy(Qt::NoFocus);
    // 右上角发送按钮
    mainLayout->addWidget(btnTest, 0, 3, 1, 1, Qt::AlignRight | Qt::AlignTop);

    // 标签（缩小宽度，增强可读性）
    auto* addressLabel = new QLabel(tr("地址"), this);
    auto* typeLabel    = new QLabel(tr("类型"), this);
    addressLabel->setMinimumWidth(36);
    typeLabel->setMinimumWidth(36);

    // 第三行：地址（标签+输入）
    int currentRow = 2;
    mainLayout->addWidget(addressLabel, currentRow, 0, 1, 1);
    mainLayout->addWidget(addressEdit,  currentRow, 1, 1, 3);
    currentRow++;
    // 第四行：类型与值（类型标签+选择、值标签+输入）
    mainLayout->addWidget(typeLabel,    currentRow, 0, 1, 1);
    mainLayout->addWidget(typeCombo,    currentRow, 1, 1, 1);

    mainLayout->addWidget(valueEdit,    currentRow, 2, 1, 2);
    currentRow++;
    //  设置控件的尺寸策略：输入框为横向可扩展，按钮为固定
    // hostEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addressEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    valueEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // typeCombo 默认不会扩展，保持较小宽度；如需可扩展可改为 Expanding
    typeCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnTest->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 列弹性：标签列最小，输入列伸展，按钮列固定
    mainLayout->setColumnStretch(0, 0);
    mainLayout->setColumnStretch(1, 1);
    mainLayout->setColumnStretch(2, 0);
    mainLayout->setColumnStretch(3, 1);
    /**
     * @brief Loop 组（仅在勾选时显示周几）
     */
    loopGroup = new QGroupBox("Loop", this);
    loopGroup->setObjectName("loopGroup");
    loopGroup->setCheckable(true);
    loopGroup->setChecked(false); // 初始 once
    auto* daysLayout = new QHBoxLayout(loopGroup);
    daysLayout->setContentsMargins(2, 2, 2, 2);
    daysLayout->setSpacing(0);

    chkMonday = new QCheckBox("Mon", loopGroup);
    chkTuesday = new QCheckBox("Tues", loopGroup);
    chkWednesday = new QCheckBox("Wed", loopGroup);
    chkThursday = new QCheckBox("Thur", loopGroup);
    chkFriday = new QCheckBox("Fri", loopGroup);
    chkSaturday = new QCheckBox("Sat", loopGroup);
    chkSunday = new QCheckBox("Sun", loopGroup);

    daysLayout->addWidget(chkMonday);
    daysLayout->addWidget(chkTuesday);
    daysLayout->addWidget(chkWednesday);
    daysLayout->addWidget(chkThursday);
    daysLayout->addWidget(chkFriday);
    daysLayout->addWidget(chkSaturday);
    daysLayout->addWidget(chkSunday);

    // 第五、六行：Loop 组（跨两行四列）
    mainLayout->addWidget(loopGroup, 4, 0, 2, 4);

}

void TaskItemWidget::connectSignals() {
    /**
     * @brief 任何字段变化均发出 messageChanged 以便上层同步
     */
    auto emitChange = [this]() {
        emit messageChanged();
    };

    connect(addressEdit, &QLineEdit::textEdited, this, emitChange);
    connect(addressEdit, &QLineEdit::editingFinished, this, emitChange);
    connect(valueEdit, &QLineEdit::textEdited, this, emitChange);
    connect(valueEdit, &QLineEdit::editingFinished, this, emitChange);

    // 类型选择：只响应用户选择（避免程序化设置或滚轮误触发）
    connect(typeCombo, QOverload<int>::of(&QComboBox::activated), this, emitChange);

    connect(remarkEdit, &QLineEdit::textEdited, this, emitChange);
    connect(remarkEdit, &QLineEdit::editingFinished, this, emitChange);
    // 时间编辑：屏蔽滚轮后保留 timeChanged，增加 editingFinished 兜底
    connect(timeEdit, &QTimeEdit::timeChanged, this, emitChange);
    connect(timeEdit, &QTimeEdit::editingFinished, this, emitChange);
    //
    connect(remarkEdit, &QLineEdit::textEdited, this, emitChange);
    // 只在用户选择时触发数据变更
    connect(loopGroup, QOverload<bool>::of(&QGroupBox::toggled),
            this, emitChange);
    connect(chkMonday, &QCheckBox::clicked, this, emitChange);
    connect(chkTuesday, &QCheckBox::clicked, this, emitChange);
    connect(chkWednesday, &QCheckBox::clicked, this, emitChange);
    connect(chkThursday, &QCheckBox::clicked, this, emitChange);
    connect(chkFriday, &QCheckBox::clicked, this, emitChange);
    connect(chkSaturday, &QCheckBox::clicked, this, emitChange);
    connect(chkSunday, &QCheckBox::clicked, this, emitChange);
    connect(btnTest, &QPushButton::clicked, this, &TaskItemWidget::testCommand);
}

void TaskItemWidget::updateValueWidget(const QString& type)
{
    if (valueEdit->validator()) {
        delete valueEdit->validator();
    }
}

void TaskItemWidget::testCommand() const
{
    auto osc=getMessage();
    OSCSender::instance()->sendOSCMessageWithQueue(osc);

}

OSCMessage TaskItemWidget::getMessage() const
{
    OSCMessage msg;

    msg.host=AppConstants::OSC_INTERNAL_CONTROL_HOST;
    msg.port=AppConstants::EXTRA_CONTROL_PORT;
    // 地址与类型/值
    msg.address = addressEdit->text().trimmed();
    msg.type = typeCombo->currentText();

    // 简单将值作为字符串保存（如需更严格类型转换，可扩展）
    msg.value = valueEdit->text();

    return msg;
}
void TaskItemWidget::setRemark(const QString& remark) {
    remarkEdit->setText(remark);
}
QString TaskItemWidget::getRemark() const {
    return remarkEdit->text();
}
void TaskItemWidget::setMessage(const OSCMessage& message)
{
    /**
     * @brief 将 OSCMessage 显示到 UI（host:port）
     */
    // if (!message.host.isEmpty()) {
    //     hostEdit->setText(QString("%1:%2").arg(message.host).arg(message.port));
    // }
    addressEdit->setText(message.address);
    typeCombo->setCurrentText(message.type);
    if (message.type == "Int") {
        valueEdit->setText(QString::number(message.value.toInt()));

    } else if (message.type == "Float") {
        valueEdit->setText(QString::number(message.value.toFloat()));
    } else if (message.type == "String") {
        valueEdit->setText(message.value.toString());
    }
}

/**
 * @brief 从 UI 构建 ScheduledInfo（loop 支持多选条件）
 * 注意：此处日期部分暂以当前日期填充，上层可根据具体需求重设日期。
 */
ScheduledInfo TaskItemWidget::getScheduledInfo() const
{
    ScheduledInfo info;
    info.type = loopGroup->isChecked() ? "loop" : "once";
    const QDate useDate = m_boundDate.isValid() ? m_boundDate : QDate::currentDate();
    info.time = QDateTime(useDate, timeEdit->time());
    info.conditions.clear();

    if (info.type.compare("loop", Qt::CaseInsensitive) == 0) {
        if (chkMonday->isChecked())    info.conditions.append("Monday");
        if (chkTuesday->isChecked())   info.conditions.append("Tuesday");
        if (chkWednesday->isChecked()) info.conditions.append("Wednesday");
        if (chkThursday->isChecked())  info.conditions.append("Thursday");
        if (chkFriday->isChecked())    info.conditions.append("Friday");
        if (chkSaturday->isChecked())  info.conditions.append("Saturday");
        if (chkSunday->isChecked())    info.conditions.append("Sunday");
    }else {
        info.type = "once";
    }

    return info;
}

/**
 * @brief 将模型中的 ScheduledInfo 显示到控件
 * 设置调度类型、时间，以及（当为 loop 时）周几复选框，并同步显示状态
 */
void TaskItemWidget::setScheduledInfo(const ScheduledInfo& info)
{
    /**
     * @brief 将模型中的 ScheduledInfo 显示到控件，同时保留日期部分用于后续写回
     * 修复点：记录 info.time.date() 到 m_boundDate
     */
    loopGroup->setChecked(info.type == "loop");
    // 记录绑定日期供 getScheduledInfo 使用
    m_boundDate = info.time.date();
    // 统一设置时间：once/loop均使用 time 部分
    const QTime t = info.time.time().isValid() ? info.time.time() : QTime(9,0,0);
    timeEdit->setTime(t);
    timeEdit->update();
    // 根据类型设置周几选择
    const bool isLoop = (info.type.compare("loop", Qt::CaseInsensitive) == 0);

    auto containsDay = [&](const char* day){
        return info.conditions.contains(QString::fromLatin1(day), Qt::CaseInsensitive);
    };

    chkMonday->setChecked(isLoop && containsDay("Monday"));
    chkTuesday->setChecked(isLoop && containsDay("Tuesday"));
    chkWednesday->setChecked(isLoop && containsDay("Wednesday"));
    chkThursday->setChecked(isLoop && containsDay("Thursday"));
    chkFriday->setChecked(isLoop && containsDay("Friday"));
    chkSaturday->setChecked(isLoop && containsDay("Saturday"));
    chkSunday->setChecked(isLoop && containsDay("Sunday"));

}

