#include "OSCMessageItemWidget.hpp"

#include <QLabel>
#include <QValidator>
#include <QRegularExpressionValidator>
#include <QGridLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariantList>
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
    auto* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    outer->addLayout(layout, 1);

    // Host（仅外部模式显示）
    if (!OnlyInternal) {
        hostEdit = new QLineEdit(this);
        hostEdit->setPlaceholderText("ip:port");
        hostEdit->setMinimumHeight(22);
        hostEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QRegularExpression rx(R"(^((\d{1,3}\.){0,3}\d{0,3})(:\d{0,5})?$)");
        hostEdit->setValidator(new QRegularExpressionValidator(rx, this));
    }

    // Address
    addressEdit = new QLineEdit(this);
    addressEdit->setPlaceholderText("address");
    addressEdit->setMinimumHeight(22);
    addressEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Type
    typeCombo = new QComboBox(this);
    typeCombo->addItems({"Int", "Float", "String"});
    typeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Value
    valueEdit = new QLineEdit(this);
    valueEdit->setPlaceholderText("value 或 [0.1,0.2,0.3]");
    valueEdit->setMinimumHeight(22);
    valueEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    if (!OnlyInternal && hostEdit) {
        layout->addWidget(hostEdit);
    }
    layout->addWidget(addressEdit);
    layout->addWidget(typeCombo);
    layout->addWidget(valueEdit);

    layout->addStretch();


}



void OSCMessageItemWidget::connectSignals()
{
    /**
     * 函数：OSCMessageItemWidget::connectSignals
     * 作用：连接控件的信号，确保编辑变更与删除操作可以向外通知。
     */
    if (hostEdit) {
        connect(hostEdit, &QLineEdit::textChanged, this, &OSCMessageItemWidget::messageChanged);
    }
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

    // 解析 host:port（仅外部模式）
    if (hostEdit) {
        QStringList hostParts = hostEdit->text().split(":");
        if (hostParts.size() == 2) {
            message.host = hostParts[0];
            bool ok;
            int port = hostParts[1].toInt(&ok);
            if (ok && port > 0 && port <= 65535) {
                message.port = port;
            }
        }
    }

    message.address = addressEdit->text();
    message.type = typeCombo->currentText();
    const QString value = valueEdit->text();
    const QString trimmed = value.trimmed();

    auto parseList = [this](const QString &text) -> QVariant {
        QJSValue result = engine.evaluate(QStringLiteral("with(Math) { (%1) }").arg(text));
        if (!result.isError() && result.isArray()) {
            QVariantList list;
            const int n = result.property(QStringLiteral("length")).toInt();
            list.reserve(n);
            for (int i = 0; i < n; ++i) {
                const QJSValue el = result.property(i);
                if (el.isNumber()) {
                    list.append(el.toNumber());
                } else if (el.isBool()) {
                    list.append(el.toBool());
                } else {
                    list.append(el.toString());
                }
            }
            return list;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
        if (doc.isArray()) {
            return doc.array().toVariantList();
        }
        return QVariantList{};
    };

    if (trimmed.startsWith(QLatin1Char('['))) {
        message.type = QStringLiteral("List");
        message.value = parseList(trimmed);
    } else if (message.type == QLatin1String("Int")) {
        QJSValue result = engine.evaluate(QStringLiteral("with(Math) { %1 }").arg(value));
        message.value = result.isError() ? 0 : result.toInt();
    } else if (message.type == QLatin1String("Float")) {
        QJSValue result = engine.evaluate(QStringLiteral("with(Math) { %1 }").arg(value));
        message.value = result.isError() ? 0.0 : result.toNumber();
    } else {
        message.type = QStringLiteral("String");
        message.value = value;
    }

    return message;
}

void OSCMessageItemWidget::setMessage(const OSCMessage& message)
{
    // 设置 host:port（仅外部模式）
    if (hostEdit) {
        QString hostPort = message.host;
        if (message.port > 0) {
            hostPort += ":" + QString::number(message.port);
        }
        hostEdit->setText(hostPort);
    }

    addressEdit->setText(message.address);

    // 列表：value 写 JSON 数组即可，type 下拉仍用 Float（兼容旧 type=List）
    if (message.value.typeId() == QMetaType::QVariantList
        || message.type.compare(QStringLiteral("List"), Qt::CaseInsensitive) == 0) {
        typeCombo->setCurrentText(QStringLiteral("Float"));
        if (message.value.typeId() == QMetaType::QVariantList) {
            setExpression(QString::fromUtf8(
                QJsonDocument(QJsonArray::fromVariantList(message.value.toList()))
                    .toJson(QJsonDocument::Compact)));
        } else {
            setExpression(message.value.toString());
        }
        return;
    }

    typeCombo->setCurrentText(message.type.isEmpty() ? QStringLiteral("String") : message.type);
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





