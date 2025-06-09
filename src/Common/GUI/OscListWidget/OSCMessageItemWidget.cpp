#include "OSCMessageItemWidget.hpp"
#include <QValidator>
#include <QRegularExpressionValidator>

OSCMessageItemWidget::OSCMessageItemWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
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

    // 设置IP:Port格式验证器
    QRegularExpression rx("^(?:[0-9]{1,3}\\.){3}[0-9]{1,3}:[0-9]{1,5}$");
    hostEdit->setValidator(new QRegularExpressionValidator(rx, this));
    
    // Address
    addressEdit = new QLineEdit(this);
    addressEdit->setPlaceholderText("address");
    addressEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    
    // Type
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"Int", "Float", "String"});
    typeCombo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    
    // Value
    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText("value");
    valueEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    
    layout->addWidget(hostEdit);
    layout->addWidget(addressEdit);
    layout->addWidget(typeCombo);
    layout->addWidget(valueEdit);
    
    // 设置弹性布局比例
    layout->setStretch(0, 2);  // host:port
    layout->setStretch(1, 2);  // address
    layout->setStretch(2, 1);  // type (固定宽度)
    layout->setStretch(3, 2);  // value
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
    
    if (type == "Int") {
        valueEdit->setValidator(new QIntValidator(this));
    } else if (type == "Float") {
        auto* validator = new QDoubleValidator(this);
        validator->setNotation(QDoubleValidator::StandardNotation);
        valueEdit->setValidator(validator);
    } else {
        valueEdit->setValidator(nullptr);
    }
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
    
    QString type = typeCombo->currentText();
    if (type == "Int") {
        bool ok;
        int value = valueEdit->text().toInt(&ok);
        message.value = ok ? value : 0;
    } else if (type == "Float") {
        bool ok;
        double value = valueEdit->text().toDouble(&ok);
        message.value = ok ? value : 0.0;
    } else if (type == "String") {
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
    if (message.value.typeId() == QMetaType::Int) {
        typeCombo->setCurrentText("Int");
        valueEdit->setText(QString::number(message.value.toInt()));
    } else if (message.value.typeId() == QMetaType::Double) {
        typeCombo->setCurrentText("Float");
        valueEdit->setText(QString::number(message.value.toDouble()));
    } else {
        typeCombo->setCurrentText("String");
        valueEdit->setText(message.value.toString());
    }
}

QVariant OSCMessageItemWidget::getValue() const
{
    QString type = typeCombo->currentText();
    if (type == "Int") {
        return valueEdit->text().toInt();
    } else if (type == "Float") {
        return valueEdit->text().toDouble();
    } else {
        return valueEdit->text();
    }
}

void OSCMessageItemWidget::setValue(QVariant val)
{
    valueEdit->setText(val.toString());
}

QString OSCMessageItemWidget::getAddress() const
{
    return addressEdit->text();
}

void OSCMessageItemWidget::setAddress(QString addr)
{
    addressEdit->setText(addr);
}