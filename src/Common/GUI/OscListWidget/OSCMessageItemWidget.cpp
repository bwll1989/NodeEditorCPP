#include "OSCMessageItemWidget.hpp"

#include <QLabel>
#include <QValidator>
#include <QRegularExpressionValidator>

OSCMessageItemWidget::OSCMessageItemWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    engine.globalObject().setProperty("PI", M_PI);
    engine.globalObject().setProperty("E", M_E);

    connectSignals();
}

void OSCMessageItemWidget::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);
    
    // Host
    hostEdit = new QLineEdit(this);
    hostEdit->setPlaceholderText("ip:port");
    hostEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    // // 设置IP:Port格式验证器
    QRegularExpression rx(R"(^((\d{1,3}\.){0,3}\d{0,3})(:\d{0,5})?$)");
    hostEdit->setValidator(new QRegularExpressionValidator(rx, this));
    
    // Address
    addressEdit = new QLineEdit(this);
    addressEdit->setPlaceholderText("address");
    addressEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    
    // Type
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"Int", "Float", "String"});
    typeCombo->setFixedWidth(70);
    typeCombo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);

    
    // Value
    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText("value");
    valueEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    QLabel* moveLabel = new QLabel();
    moveLabel->setFixedWidth(10);
    moveLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    moveLabel->setPixmap(QPixmap(":/icons/icons/move.png").scaled(moveLabel->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    moveLabel->setAlignment(Qt::AlignCenter);


    layout->addWidget(hostEdit);
    layout->addWidget(addressEdit);
    layout->addWidget(typeCombo);
    layout->addWidget(valueEdit);
    layout->addWidget(moveLabel);
    
    // 设置弹性布局比例
    layout->setStretch(0, 2);  // host:port
    layout->setStretch(1, 2);  // address
    layout->setStretch(2, 1);  // type (固定宽度)
    layout->setStretch(3, 2);  // value
    layout->setStretch(4, 1);  // icon

}

void OSCMessageItemWidget::connectSignals()
{
    connect(hostEdit, &QLineEdit::textChanged, this, &OSCMessageItemWidget::messageChanged);
    connect(addressEdit, &QLineEdit::textChanged, this, &OSCMessageItemWidget::messageChanged);
    connect(typeCombo, &QComboBox::currentTextChanged, this, [this](const QString& type) {
        updateValueWidget(type);
        emit messageChanged();
    });
    connect(valueEdit, &QLineEdit::textChanged, this, &OSCMessageItemWidget::messageChanged);
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
    typeCombo->setCurrentText(message.type);
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