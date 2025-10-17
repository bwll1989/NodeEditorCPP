// 文件顶部的包含
#include "TaskItemWidget.hpp"

#include <QEvent>
#include <QLabel>
#include <QValidator>
#include <QRegularExpressionValidator>

#include <QGroupBox>   // Command组与Loop组
#include <QPushButton> // Test按钮
#include <QFont>       // 大号时间字体

TaskItemWidget::TaskItemWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
}

void TaskItemWidget::setupUI() {

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(6, 6, 6, 6);
    outerLayout->setSpacing(6);
    // 卡片边框容器
    QFrame* card = new QFrame(this);
    card->setObjectName("taskCard");
    card->setFrameShape(QFrame::NoFrame);
    // 边框样式：浅色边框 + 圆角；根据主题可调整透明度
    card->setStyleSheet(
        "#taskCard {"
        "  border: 2px solid rgba(255,255,255,0.25);"
        "  border-radius: 6px;"
        "}"
    );

    // 卡片内部主布局（原 mainLayout 改为挂在 card 上）
    auto* mainLayout = new QGridLayout(card);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(4);

    // 第一行：居中时间显示（使用 timeEdit 的值），大号字体
    timeEdit = new QTimeEdit(this);
    timeEdit->setObjectName("lblTimeDisplay");
    timeEdit->setDisplayFormat("HH:mm:ss");
    timeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    timeEdit->setAlignment(Qt::AlignCenter);
    {
        QFont f = timeEdit->font();
        f.setPointSize(f.pointSize() + 12);
        f.setBold(true);
        timeEdit->setFont(f);
    }
    mainLayout->addWidget(timeEdit, 0, 0, 1, 2);

    // 第二行：备注栏（居中）
    remarkEdit = new QLineEdit(this);
    remarkEdit->setObjectName("remarkEdit");
    remarkEdit->setPlaceholderText("备注（例如：早上开机）");
    remarkEdit->setStyleSheet(
        "QLineEdit {"
        "  padding: 2px;"
        "  font-size: 14px;"
        "  color: rgba(255, 0, 0, 180);"
        "}"
    );
    mainLayout->addWidget(remarkEdit, 0, 2, 1, 5);

    // 顶行控件：Host、Address、Type、Value、Test
    hostEdit = new QLineEdit(this);
    hostEdit->setPlaceholderText("127.0.0.1:8990");

    addressEdit = new QLineEdit(this);
    addressEdit->setPlaceholderText("address");

    typeCombo = new QComboBox(this);
    typeCombo->addItems({ "Int", "Float","String"});
    // typeCombo->installEventFilter(this);

    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText("value");

    // Test 按钮（位于 Command 组右侧）
    btnTest = new QPushButton("Test", this);
    btnTest->setObjectName("btnTest");

    // 将控件加入 Command 组布局
    mainLayout->addWidget(hostEdit,1,0,1,1);
    mainLayout->addWidget(addressEdit,1,1,1,2);
    mainLayout->addWidget(typeCombo,1,3,1,1);
    mainLayout->addWidget(valueEdit,1,4,1,2);
    mainLayout->addWidget(btnTest,1,6,1,1);
    //  设置控件的尺寸策略：输入框为横向可扩展，按钮为固定
    hostEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addressEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    valueEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // typeCombo 默认不会扩展，保持较小宽度；如需可扩展可改为 Expanding
    typeCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnTest->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 关键：为各列设置伸展因子
    // 列 0: host
    mainLayout->setColumnStretch(0, 2);
    // 列 1-2: address（跨两列，两个列都给较大的 stretch）
    mainLayout->setColumnStretch(1, 3);
    mainLayout->setColumnStretch(2, 3);
    // 列 3: type（较小的 stretch）
    mainLayout->setColumnStretch(3, 1);
    // 列 4-5: value（跨两列，同样给较大 stretch）
    mainLayout->setColumnStretch(4, 3);
    mainLayout->setColumnStretch(5, 3);
    // 列 6: Test 按钮（不拉伸）
    mainLayout->setColumnStretch(6, 0);
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



    mainLayout->addWidget(loopGroup, 2, 0, 1, 7);

    // // 初始隐藏（Group 未勾选时）
    // loopDaysContainer->setVisible(false);

    // 将卡片布局挂在外部布局
    outerLayout->addWidget(card);
}

void TaskItemWidget::connectSignals() {
    /**
     * @brief 任何字段变化均发出 messageChanged 以便上层同步
     */
    auto emitChange = [this]() {
        emit messageChanged();
    };

    // 文本类：仅用户编辑触发
    connect(hostEdit, &QLineEdit::textEdited, this, emitChange);
    connect(hostEdit, &QLineEdit::editingFinished, this, emitChange);
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

    // 解析 host:port 格式
    const QString hostText = hostEdit->text().trimmed();
    QString host = hostText;
    int port = 6001; // 默认端口
    const int sep = hostText.lastIndexOf(':');
    if (sep > 0) {
        host = hostText.left(sep);
        bool ok = false;
        int p = hostText.mid(sep + 1).toInt(&ok);
        if (ok) port = p;
    }
    msg.host = host;
    msg.port = port;

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
    if (!message.host.isEmpty()) {
        hostEdit->setText(QString("%1:%2").arg(message.host).arg(message.port));
    }
    addressEdit->setText(message.address);
    typeCombo->setCurrentText(message.type);
    valueEdit->setText(message.value.toString());
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
