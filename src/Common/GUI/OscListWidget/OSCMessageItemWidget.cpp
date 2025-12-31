#include "OSCMessageItemWidget.hpp"

#include <QLabel>
#include <QValidator>
#include <QRegularExpressionValidator>
#include <QGridLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
OSCMessageItemWidget::OSCMessageItemWidget(bool onlyInternal, QWidget* parent)
    : QWidget(parent), OnlyInternal(onlyInternal)
{
    setupUI();
    engine.globalObject().setProperty("PI", M_PI);
    engine.globalObject().setProperty("E", M_E);

    connectSignals();
}

void OSCMessageItemWidget::setupUI()
{
    /**
     * 函数：OSCMessageItemWidget::setupUI
     * 作用：将所有控件直接放置在当前 QWidget 上，移除外层卡片 QFrame；
     *       仍保留左侧强调条与紧凑的表单布局，并在右上角提供删除按钮。
     * 关键点：
     *  - 使用当前 QWidget 作为卡片容器（设置 objectName="oscCard"）
     *  - 左侧强调条 + 右侧表单的水平布局
     *  - 收紧边距与间距，统一控件最小高度
     *  - 标签列最小化，输入列弹性扩展
     */
    // 根布局：直接作用于本控件
    this->setObjectName("oscCard");
    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(6, 6, 6, 6);
    outer->setSpacing(8);

    // 左侧强调条
    m_accentBar = new QFrame(this);
    m_accentBar->setFixedWidth(3);
    m_accentBar->setStyleSheet(
    "QFrame{background-color:#00aa00;border-radius:2px;}");
    outer->addWidget(m_accentBar);

    // 表单区域（右侧）
    auto* layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(4);
    outer->addLayout(layout);

    // Host
    hostEdit = new QLineEdit();
    hostEdit->setPlaceholderText("ip:port");
    hostEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    hostEdit->setMinimumHeight(22);
    QRegularExpression rx(R"(^((\d{1,3}\.){0,3}\d{0,3})(:\d{0,5})?$)");
    hostEdit->setValidator(new QRegularExpressionValidator(rx, this));

    // Address
    addressEdit = new QLineEdit(this);
    addressEdit->setPlaceholderText("address");

    // Type
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"Int", "Float", "String"});


    // Value
    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText("value");


    // 标签（缩小宽度，减少占用）
    auto* hostLabel    = new QLabel(tr("目标"));
    auto* addressLabel = new QLabel(tr("地址"));
    auto* typeLabel    = new QLabel(tr("类型"));
    auto* valueLabel   = new QLabel(tr("值"));
    hostLabel->setMinimumWidth(20);
    addressLabel->setMinimumWidth(20);
    typeLabel->setMinimumWidth(20);
    valueLabel->setMinimumWidth(20);
    
    // m_deleteBtn = new QPushButton();
    // m_deleteBtn->setIcon(QIcon(":/icons/icons/remove.png"));
    //
    // m_deleteBtn->setToolTip(tr("删除此消息"));
    // m_deleteBtn->setFixedHeight(m_deleteBtn->sizeHint().height());
    // m_deleteBtn->setFlat(true);
    // m_deleteBtn->setFocusPolicy(Qt::NoFocus);


    int currentRow = 0;
    if (!OnlyInternal) {
        layout->addWidget(hostLabel,   currentRow, 0, Qt::AlignLeft);
        layout->addWidget(hostEdit,    currentRow, 1);
        currentRow++;

        layout->addWidget(addressLabel, currentRow, 0, Qt::AlignLeft);
        layout->addWidget(addressEdit,  currentRow, 1);
        currentRow++;

        layout->addWidget(typeLabel,    currentRow, 0, Qt::AlignLeft);
        layout->addWidget(typeCombo,    currentRow, 1);
        currentRow++;

        layout->addWidget(valueLabel,   currentRow, 0, Qt::AlignLeft);
        layout->addWidget(valueEdit,    currentRow, 1);
    } else {
        layout->addWidget(addressLabel, currentRow, 0, Qt::AlignLeft);
        layout->addWidget(addressEdit,  currentRow, 1);
        currentRow++;

        layout->addWidget(typeLabel,    currentRow, 0, Qt::AlignLeft);
        layout->addWidget(typeCombo,    currentRow, 1);
        currentRow++;

        layout->addWidget(valueLabel,   currentRow, 0, Qt::AlignLeft);
        layout->addWidget(valueEdit,    currentRow, 1);
    }

    // 右侧放置拖动图标
    // layout->addWidget(m_deleteBtn, 0, 2, currentRow + 1, 1, Qt::AlignTop);

    // 列弹性：标签列最小，输入列伸展，图标列固定
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 4);


}



void OSCMessageItemWidget::connectSignals()
{
    /**
     * 函数：OSCMessageItemWidget::connectSignals
     * 作用：连接控件的信号，确保编辑变更与删除操作可以向外通知。
     */
    connect(hostEdit, &QLineEdit::textChanged, this, &OSCMessageItemWidget::messageChanged);
    connect(addressEdit, &QLineEdit::textChanged, this, &OSCMessageItemWidget::messageChanged);
    connect(typeCombo, &QComboBox::currentTextChanged, this, [this](const QString& type) {
        updateValueWidget(type);
        emit messageChanged();
    });
    connect(valueEdit, &QLineEdit::textChanged, this, &OSCMessageItemWidget::messageChanged);

    // 新增：删除按钮点击后发出删除请求信号，由外部处理实际删除
    // connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
    //     emit requestDelete();
    // });
}

void OSCMessageItemWidget::updateValueWidget(const QString& type)
{
    if (valueEdit->validator()) {
        delete valueEdit->validator();
    }
    
    // if (type == "Int") {
    //     // valueEdit->setValidator(new QIntValidator(this));
    // } else if (type == "Float") {
    //     auto* validator = new QDoubleValidator(this);
    //     validator->setNotation(QDoubleValidator::StandardNotation);
    //     valueEdit->setValidator(validator);
    // } else {
    //     valueEdit->setValidator(nullptr);
    // }
}

OSCMessage OSCMessageItemWidget::getMessage() const
{
    OSCMessage message;

    // 解析 host:port
    QStringList hostParts = hostEdit->text().split(":");
    if (hostParts.size() == 2) {
        message.host = hostParts[0];
        bool ok;
        int port = hostParts[1].toInt(&ok);
        if (ok && port > 0 && port <= 65535) {
            message.port = port;
        }
    }
    
    message.address = addressEdit->text();
    
    message.type = typeCombo->currentText();
    QString value = valueEdit->text();
    if (message.type == "Int") {
        QJSValue result = engine.evaluate("with(Math) { " + value + " }");
        if (result.isError()) {
            // qWarning() << "表达式错误:" << expr << "->" << result.toString();
            message.value=0; // 回退到直接转换
        }

        message.value = result.toInt();
    } else if (message.type == "Float") {
        QJSValue result = engine.evaluate("with(Math) { " + value + " }");
        if (result.isError()) {
            // qWarning() << "表达式错误:" << expr << "->" << result.toString();
            message.value=0.0; // 回退到直接转换
        }
        message.value = result.toNumber();
    } else if (message.type == "String") {
        message.value = valueEdit->text();
    }
    
    return message;
}

void OSCMessageItemWidget::setMessage(const OSCMessage& message)
{
    // 设置 host:port
    QString hostPort = message.host;
    if (message.port > 0) {
        hostPort += ":" + QString::number(message.port);
    }
    hostEdit->setText(hostPort);
    
    addressEdit->setText(message.address);
    
    // 设置类型和值
    typeCombo->setCurrentText(message.type.isEmpty() ? "String" : message.type);

    setExpression(message.value.toString());
}

QString OSCMessageItemWidget::getExpression() const
{
    return valueEdit->text();
}

void OSCMessageItemWidget::setExpression(QString val)
{
    valueEdit->setText(val);
}

QString OSCMessageItemWidget::getAddress() const
{
    return addressEdit->text();
}

void OSCMessageItemWidget::setAddress(QString addr)
{
    addressEdit->setText(addr);
}

QJSEngine* OSCMessageItemWidget::getJSEngine()
{
    return &engine;
}





