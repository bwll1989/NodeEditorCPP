#include "JavaScriptDataModel.hpp"

#include "JSEngineDefines/SupportWidgets.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QGridLayout>
#include <QMetaObject>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QThread>
#include <QTimer>

using QtNodes::NodeData;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Nodes;

QString JavaScriptDataModel::defaultScript()
{
    return QStringLiteral(R"(function initInterface() {
    // 在这里编写初始化界面的代码
    // 例如：创建按钮、文本框等
}
function inputEventHandler(index){
    console.log(index)
    console.log(Node.getInputValue(index)["default"]);
}
)");
}

JavaScriptDataModel::JavaScriptDataModel()
    : widget(new JavaScriptInterface())
    , script(defaultScript())
{
    InPortCount = 4;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("JS Script");
    WidgetEmbeddable = false;
    Resizable = true;
    PortEditable = true;

    if (widget && widget->codeWidget) {
        widget->codeWidget->loadCodeFromCode(script);
        connect(widget->codeWidget->importJS, &QPushButton::clicked,
                this, &JavaScriptDataModel::onRunButtonClicked);
    }

    reloadScript(script, true);
}

JavaScriptDataModel::~JavaScriptDataModel()
{
    m_shuttingDown = true;
    discardPendingWork();
    if (widget && widget->codeWidget) {
        disconnect(widget->codeWidget->importJS, nullptr, this, nullptr);
    }
    if (widget) {
        widget->closeEditorWindow();
    }
    destroyLayoutWidgets();
    destroyJsEngine();
    // 已被 Node 图形项接管的控件不要再删，避免双重释�?
    if (widget && widget->parent() == nullptr) {
        widget->deleteLater();
    }
    widget = nullptr;
}

QString JavaScriptDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        return QStringLiteral("IN %1").arg(portIndex);
    case PortType::Out:
        return QStringLiteral("OUT %1").arg(portIndex);
    default:
        break;
    }
    return {};
}

unsigned int JavaScriptDataModel::nPorts(PortType portType) const
{
    switch (portType) {
    case PortType::In:
        return InPortCount;
    case PortType::Out:
        return OutPortCount;
    default:
        break;
    }
    return 0;
}

NodeDataType JavaScriptDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType)
    Q_UNUSED(portIndex)
    return VariableData().type();
}

std::shared_ptr<NodeData> JavaScriptDataModel::outData(PortIndex const portIndex)
{
    QMutexLocker locker(&m_dataMutex);
    if (out_data.contains(portIndex)) {
        return std::make_shared<VariableData>(out_data[portIndex]);
    }
    return std::make_shared<VariableData>();
}

void JavaScriptDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
{
    auto variable = std::dynamic_pointer_cast<VariableData>(data);
    if (!variable) {
        return;
    }

    {
        QMutexLocker locker(&m_dataMutex);
        inputPortIndex = portIndex;
        in_data[portIndex] = variable->asMap();
    }

    if (m_shuttingDown || m_reloading) {
        return;
    }

    if (!isGuiThread()) {
        const int index = static_cast<int>(portIndex);
        QPointer<JavaScriptDataModel> self(this);
        QMetaObject::invokeMethod(
            this,
            [self, index]() {
                if (self) {
                    self->inputEventHandler(index);
                }
            },
            Qt::QueuedConnection);
        return;
    }

    inputEventHandler(static_cast<int>(portIndex));
}

QWidget *JavaScriptDataModel::embeddedWidget()
{
    return widget;
}

QVariantMap JavaScriptDataModel::collectWidgetState() const
{
    QVariantMap state;
    for (auto it = m_widgets.constBegin(); it != m_widgets.constEnd(); ++it) {
        QWidget *w = it.value();
        if (!w) {
            continue;
        }
        const QString key = w->objectName().isEmpty()
                                ? QStringLiteral("w%1").arg(it.key())
                                : w->objectName();

        if (auto *spin = qobject_cast<QSpinBox *>(w)) {
            state[key] = spin->value();
        } else if (auto *dspin = qobject_cast<QDoubleSpinBox *>(w)) {
            state[key] = dspin->value();
        } else if (auto *slider = qobject_cast<QSlider *>(w)) {
            state[key] = slider->value();
        } else if (auto *lineEdit = qobject_cast<QLineEdit *>(w)) {
            state[key] = lineEdit->text();
        } else if (auto *button = qobject_cast<QAbstractButton *>(w)) {
            if (button->isCheckable()) {
                state[key] = button->isChecked();
            }
        } else if (auto *combo = qobject_cast<QComboBox *>(w)) {
            QVariantMap comboState;
            comboState[QStringLiteral("index")] = combo->currentIndex();
            comboState[QStringLiteral("text")] = combo->currentText();
            QStringList items;
            for (int i = 0; i < combo->count(); ++i) {
                items << combo->itemText(i);
            }
            comboState[QStringLiteral("items")] = items;
            state[key] = comboState;
        }
    }
    return state;
}

void JavaScriptDataModel::restoreWidgetState()
{
    if (m_widgetState.isEmpty()) {
        return;
    }
    for (auto it = m_widgets.constBegin(); it != m_widgets.constEnd(); ++it) {
        QWidget *w = it.value();
        if (!w) {
            continue;
        }
        const QString key = w->objectName().isEmpty()
                                ? QStringLiteral("w%1").arg(it.key())
                                : w->objectName();
        if (!m_widgetState.contains(key)) {
            continue;
        }

        const QVariant value = m_widgetState.value(key);
        const QSignalBlocker blocker(w);
        if (auto *spin = qobject_cast<QSpinBox *>(w)) {
            spin->setValue(value.toInt(nullptr));
        } else if (auto *dspin = qobject_cast<QDoubleSpinBox *>(w)) {
            dspin->setValue(value.toDouble(nullptr));
        } else if (auto *slider = qobject_cast<QSlider *>(w)) {
            slider->setValue(value.toInt(nullptr));
        } else if (auto *lineEdit = qobject_cast<QLineEdit *>(w)) {
            lineEdit->setText(value.toString());
        } else if (auto *button = qobject_cast<QAbstractButton *>(w)) {
            if (button->isCheckable()) {
                button->setChecked(value.toBool());
            }
        } else if (auto *combo = qobject_cast<QComboBox *>(w)) {
            const QVariantMap comboState = value.toMap();
            const QStringList items = comboState.value(QStringLiteral("items")).toStringList();
            QStringList currentItems;
            for (int i = 0; i < combo->count(); ++i) {
                currentItems << combo->itemText(i);
            }
            if (!items.isEmpty() && items != currentItems) {
                combo->clear();
                combo->addItems(items);
            }
            const int index = comboState.value(QStringLiteral("index"), combo->currentIndex()).toInt(nullptr);
            if (index >= 0 && index < combo->count()) {
                combo->setCurrentIndex(index);
            } else {
                const QString text = comboState.value(QStringLiteral("text")).toString();
                if (!text.isEmpty()) {
                    combo->setCurrentText(text);
                }
            }
        }
    }
}

QJsonObject JavaScriptDataModel::save() const
{
    QString code = script;
    if (widget && widget->codeWidget) {
        code = widget->codeWidget->saveCode();
    }

    QJsonObject values;
    values[QStringLiteral("scripts")] = code;
    values[QStringLiteral("widgets")] = QJsonObject::fromVariantMap(collectWidgetState());

    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson[QStringLiteral("values")] = values;
    modelJson[QStringLiteral("InPortCount")] = static_cast<int>(InPortCount);
    modelJson[QStringLiteral("OutPortCount")] = static_cast<int>(OutPortCount);
    return modelJson;
}

void JavaScriptDataModel::load(const QJsonObject &p)
{
    const QJsonValue valuesNode = p.value(QStringLiteral("values"));
    if (!valuesNode.isObject()) {
        return;
    }

    InPortCount = static_cast<unsigned int>(p.value(QStringLiteral("InPortCount")).toInt(static_cast<int>(InPortCount)));
    OutPortCount = static_cast<unsigned int>(p.value(QStringLiteral("OutPortCount")).toInt(static_cast<int>(OutPortCount)));

    const QJsonObject valuesObject = valuesNode.toObject();
    script = valuesObject.value(QStringLiteral("scripts")).toString();
    if (script.isEmpty()) {
        script = defaultScript();
    }
    m_widgetState = valuesObject.value(QStringLiteral("widgets")).toObject().toVariantMap();

    if (widget && widget->codeWidget) {
        widget->codeWidget->loadCodeFromCode(script);
    }

    reloadScript(script, true);
}

void JavaScriptDataModel::onRunButtonClicked()
{
    if (!widget || !widget->codeWidget || m_reloading || m_shuttingDown) {
        return;
    }
    script = widget->codeWidget->saveCode();
    reloadScript(script, true);
}

unsigned int JavaScriptDataModel::getInputCount()
{
    return InPortCount;
}

unsigned int JavaScriptDataModel::getOutputCount()
{
    return OutPortCount;
}

unsigned int JavaScriptDataModel::inputIndex()
{
    return inputPortIndex;
}

bool JavaScriptDataModel::isGuiThread() const
{
    return QThread::currentThread() == this->thread();
}

void JavaScriptDataModel::discardPendingWork()
{
    if (QCoreApplication::instance()) {
        QCoreApplication::removePostedEvents(this);
    }
}

void JavaScriptDataModel::destroyLayoutWidgets()
{
    takeWidgetOwnership();
    if (widget && widget->top_layout) {
        for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
            QWidget *w = it.value();
            if (!w) {
                continue;
            }
            widget->top_layout->removeWidget(w);
        }
    }
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        QWidget *w = it.value();
        if (!w) {
            continue;
        }
        w->blockSignals(true);
        disconnect(w, nullptr, nullptr, nullptr);
        w->hide();
        w->setParent(nullptr);
        delete w;
    }
    m_widgets.clear();
    m_widgetCounter = 0;
}

void JavaScriptDataModel::logJsError(const char *context, const QJSValue &result) const
{
    qWarning() << "JavaScript" << context << "错误:"
               << result.property(QStringLiteral("lineNumber")).toInt()
               << result.toString();
}

void JavaScriptDataModel::takeWidgetOwnership()
{
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        QWidget *w = it.value();
        if (w) {
            QJSEngine::setObjectOwnership(w, QJSEngine::CppOwnership);
        }
    }
}

void JavaScriptDataModel::destroyJsEngine()
{
    takeWidgetOwnership();
    if (!m_jsEngine) {
        return;
    }
    m_jsEngine->globalObject().setProperty(QStringLiteral("Node"), QJSValue());
    delete m_jsEngine;
    m_jsEngine = nullptr;
}

void JavaScriptDataModel::createJsEngine()
{
    m_jsEngine = new QJSEngine(this);
    m_jsEngine->installExtensions(QJSEngine::AllExtensions);

    QJSValue nodeObject = m_jsEngine->newQObject(this);
    QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
    m_jsEngine->globalObject().setProperty(QStringLiteral("Node"), nodeObject);
    m_jsEngine->globalObject().setProperty(QStringLiteral("SpinBox"), m_jsEngine->newQMetaObject<SpinBox>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("VSlider"), m_jsEngine->newQMetaObject<VSlider>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("HSlider"), m_jsEngine->newQMetaObject<HSlider>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("CheckBox"), m_jsEngine->newQMetaObject<CheckBox>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("LineEdit"), m_jsEngine->newQMetaObject<LineEdit>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("ComboBox"), m_jsEngine->newQMetaObject<ComboBox>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("Label"), m_jsEngine->newQMetaObject<Label>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("Button"), m_jsEngine->newQMetaObject<Button>());
    m_jsEngine->globalObject().setProperty(QStringLiteral("DoubleSpinBox"), m_jsEngine->newQMetaObject<DoubleSpinBox>());
}

void JavaScriptDataModel::reloadScript(const QString &code, bool runInitInterface)
{
    if (m_shuttingDown) {
        return;
    }

    if (!isGuiThread()) {
        const QString copied = code;
        const bool runInit = runInitInterface;
        QPointer<JavaScriptDataModel> self(this);
        QMetaObject::invokeMethod(
            this,
            [self, copied, runInit]() {
                if (self) {
                    self->reloadScript(copied, runInit);
                }
            },
            Qt::QueuedConnection);
        return;
    }

    if (m_reloading) {
        return;
    }

    if (m_handlingInput) {
        const QString copied = code;
        const bool runInit = runInitInterface;
        QTimer::singleShot(0, this, [this, copied, runInit]() {
            reloadScript(copied, runInit);
        });
        return;
    }

    m_reloading = true;
    script = code;

    // 先同步销毁控件并断开信号，再销毁引擎，避免 JS 回调进入已删除的 QJSEngine
    destroyLayoutWidgets();
    destroyJsEngine();
    createJsEngine();

    if (!script.isEmpty() && m_jsEngine) {
        const QJSValue result = m_jsEngine->evaluate(script);
        if (result.isError()) {
            logJsError("evaluate", result);
            m_reloading = false;
            return;
        }
        m_reloading = false;
        if (runInitInterface) {
            initInterface();
            restoreWidgetState();
        }
        return;
    }

    m_reloading = false;
}

void JavaScriptDataModel::clearLayout()
{
    if (m_shuttingDown) {
        m_widgets.clear();
        m_widgetCounter = 0;
        return;
    }

    if (!isGuiThread()) {
        QPointer<JavaScriptDataModel> self(this);
        QMetaObject::invokeMethod(
            this,
            [self]() {
                if (self) {
                    self->clearLayout();
                }
            },
            Qt::QueuedConnection);
        return;
    }

    destroyLayoutWidgets();
}

int JavaScriptDataModel::addToLayout(QObject *widgetObj, int x, int y, int rowSpan, int columnSpan)
{
    if (m_shuttingDown || !m_jsEngine || !isGuiThread() || !widget || !widget->top_layout) {
        return -1;
    }

    QWidget *widget_obj = qobject_cast<QWidget *>(widgetObj);
    if (!widget_obj) {
        return -1;
    }

    QJSEngine::setObjectOwnership(widget_obj, QJSEngine::CppOwnership);
    widget_obj->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (x != -1 && y != -1) {
        widget->top_layout->addWidget(widget_obj, x, y, rowSpan, columnSpan);
    } else {
        widget->top_layout->addWidget(widget_obj);
    }

    for (int c = 0; c < widget->top_layout->columnCount(); ++c) {
        if (widget->top_layout->columnStretch(c) == 0) {
            widget->top_layout->setColumnStretch(c, 1);
            widget->top_layout->setColumnMinimumWidth(c, 1);
        }
    }
    for (int r = 0; r < widget->top_layout->rowCount(); ++r) {
        if (widget->top_layout->rowStretch(r) == 0) {
            widget->top_layout->setRowStretch(r, 1);
            widget->top_layout->setRowMinimumHeight(r, 1);
        }
    }

    const int id = m_widgetCounter++;
    if (widget_obj->objectName().isEmpty()) {
        widget_obj->setObjectName(QStringLiteral("w%1").arg(id));
    }
    m_widgets[id] = widget_obj;
    return id;
}

void JavaScriptDataModel::initInterface()
{
    if (m_shuttingDown) {
        return;
    }

    if (!isGuiThread()) {
        QPointer<JavaScriptDataModel> self(this);
        QMetaObject::invokeMethod(
            this,
            [self]() {
                if (self) {
                    self->initInterface();
                }
            },
            Qt::QueuedConnection);
        return;
    }

    if (!m_jsEngine) {
        qWarning() << "JavaScript引擎未初始化";
        return;
    }

    const QJSValue initInterfaceFunc = m_jsEngine->globalObject().property(QStringLiteral("initInterface"));
    if (!initInterfaceFunc.isCallable()) {
        return;
    }

    const QJSValue initResult = initInterfaceFunc.call();
    if (initResult.isError()) {
        logJsError("initInterface", initResult);
    }
}

void JavaScriptDataModel::inputEventHandler(int portIndex)
{
    if (m_shuttingDown || m_reloading || m_handlingInput || !m_jsEngine) {
        return;
    }

    if (!isGuiThread()) {
        QPointer<JavaScriptDataModel> self(this);
        QMetaObject::invokeMethod(
            this,
            [self, portIndex]() {
                if (self) {
                    self->inputEventHandler(portIndex);
                }
            },
            Qt::QueuedConnection);
        return;
    }

    const QJSValue handler = m_jsEngine->globalObject().property(QStringLiteral("inputEventHandler"));
    if (!handler.isCallable()) {
        return;
    }

    struct HandlingGuard {
        bool &flag;
        explicit HandlingGuard(bool &f) : flag(f) { flag = true; }
        ~HandlingGuard() { flag = false; }
    } guard(m_handlingInput);

    const QJSValue result = handler.call(QJSValueList() << portIndex);
    if (result.isError()) {
        logJsError("inputEventHandler", result);
    }
}

QJSValue JavaScriptDataModel::getInputValue(int portIndex)
{
    if (m_shuttingDown || !m_jsEngine) {
        return {};
    }

    QVariantMap map;
    {
        QMutexLocker locker(&m_dataMutex);
        if (portIndex >= 0 && portIndex < static_cast<int>(InPortCount) && in_data.contains(portIndex)) {
            map = in_data[portIndex];
        } else {
            return m_jsEngine->newObject();
        }
    }
    return JSEngineDefines::variantMapToJSValue(m_jsEngine, map);
}

QJSValue JavaScriptDataModel::getOutputValue(int portIndex)
{
    if (m_shuttingDown || !m_jsEngine) {
        return {};
    }

    QVariantMap map;
    {
        QMutexLocker locker(&m_dataMutex);
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount) && out_data.contains(portIndex)) {
            map = out_data[portIndex];
        } else {
            return m_jsEngine->newObject();
        }
    }
    return JSEngineDefines::variantMapToJSValue(m_jsEngine, map);
}

void JavaScriptDataModel::ui(const QJSValue &spec)
{
    if (m_shuttingDown || m_reloading || !m_jsEngine || !isGuiThread()
        || !widget || !widget->top_layout) {
        return;
    }

    QJSValue list = spec;
    if (!list.isArray()) {
        list = m_jsEngine->newArray(1);
        list.setProperty(0, spec);
    }

    const int count = list.property(QStringLiteral("length")).toInt();
    for (int i = 0; i < count; ++i) {
        const QJSValue item = list.property(i);
        if (!item.isObject()) {
            continue;
        }

        const QString type = item.property(QStringLiteral("type")).toString();
        QWidget *created = nullptr;
        if (type.compare(QStringLiteral("SpinBox"), Qt::CaseInsensitive) == 0) {
            created = new SpinBox();
        } else if (type.compare(QStringLiteral("DoubleSpinBox"), Qt::CaseInsensitive) == 0) {
            created = new DoubleSpinBox();
        } else if (type.compare(QStringLiteral("Button"), Qt::CaseInsensitive) == 0) {
            created = new Button();
        } else if (type.compare(QStringLiteral("Label"), Qt::CaseInsensitive) == 0) {
            created = new Label();
        } else if (type.compare(QStringLiteral("LineEdit"), Qt::CaseInsensitive) == 0) {
            created = new LineEdit();
        } else if (type.compare(QStringLiteral("CheckBox"), Qt::CaseInsensitive) == 0) {
            created = new CheckBox();
        } else if (type.compare(QStringLiteral("ComboBox"), Qt::CaseInsensitive) == 0) {
            created = new ComboBox();
        } else if (type.compare(QStringLiteral("HSlider"), Qt::CaseInsensitive) == 0) {
            created = new HSlider();
        } else if (type.compare(QStringLiteral("VSlider"), Qt::CaseInsensitive) == 0) {
            created = new VSlider();
        }
        if (!created) {
            qWarning() << "JavaScript ui(): unknown widget type:" << type;
            continue;
        }

        const QString key = item.property(QStringLiteral("key")).toString();
        if (!key.isEmpty()) {
            created->setObjectName(key);
        }

        const QVariantMap props = JSEngineDefines::jsValueToVariantMap(item);
        for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
            const QString name = it.key();
            if (name == QStringLiteral("type") || name == QStringLiteral("key")
                || name == QStringLiteral("row") || name == QStringLiteral("col")
                || name == QStringLiteral("rowSpan") || name == QStringLiteral("columnSpan")
                || name == QStringLiteral("onChange")) {
                continue;
            }
            QString propName = name;
            if (propName == QStringLiteral("min")) {
                propName = QStringLiteral("minimum");
            } else if (propName == QStringLiteral("max")) {
                propName = QStringLiteral("maximum");
            }
            created->setProperty(propName.toUtf8().constData(), it.value());
        }

        const QJSValue rowProp = item.property(QStringLiteral("row"));
        const QJSValue colProp = item.property(QStringLiteral("col"));
        const QJSValue rowSpanProp = item.property(QStringLiteral("rowSpan"));
        const QJSValue columnSpanProp = item.property(QStringLiteral("columnSpan"));
        const int row = rowProp.isUndefined() ? -1 : rowProp.toInt();
        const int col = colProp.isUndefined() ? -1 : colProp.toInt();
        const int rowSpan = rowSpanProp.isUndefined() ? 1 : rowSpanProp.toInt();
        const int columnSpan = columnSpanProp.isUndefined() ? 1 : columnSpanProp.toInt();
        addToLayout(created, row, col, rowSpan, columnSpan);

        const QJSValue onChange = item.property(QStringLiteral("onChange"));
        if (!onChange.isCallable()) {
            continue;
        }
        auto invokeChange = [this, onChange](const QVariant &value) {
            if (m_shuttingDown || !m_jsEngine) {
                return;
            }
            QJSValueList args;
            args << m_jsEngine->toScriptValue(value);
            const QJSValue result = onChange.call(args);
            if (result.isError()) {
                logJsError("ui.onChange", result);
            }
        };

        if (auto *spin = qobject_cast<QSpinBox *>(created)) {
            connect(spin, qOverload<int>(&QSpinBox::valueChanged), created,
                    [invokeChange](int v) { invokeChange(v); });
        } else if (auto *dspin = qobject_cast<QDoubleSpinBox *>(created)) {
            connect(dspin, &QDoubleSpinBox::valueChanged, created,
                    [invokeChange](double v) { invokeChange(v); });
        } else if (auto *slider = qobject_cast<QSlider *>(created)) {
            connect(slider, &QSlider::valueChanged, created,
                    [invokeChange](int v) { invokeChange(v); });
        } else if (auto *lineEdit = qobject_cast<QLineEdit *>(created)) {
            connect(lineEdit, &QLineEdit::textChanged, created,
                    [invokeChange](const QString &v) { invokeChange(v); });
        } else if (auto *check = qobject_cast<QCheckBox *>(created)) {
            connect(check, &QCheckBox::toggled, created,
                    [invokeChange](bool v) { invokeChange(v); });
        } else if (auto *combo = qobject_cast<QComboBox *>(created)) {
            connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), created,
                    [invokeChange](int v) { invokeChange(v); });
        } else if (auto *button = qobject_cast<QPushButton *>(created)) {
            connect(button, &QPushButton::clicked, created,
                    [invokeChange]() { invokeChange(true); });
        }
    }
}

void JavaScriptDataModel::setOutputValue(int portIndex, const QJSValue &value)
{
    if (m_shuttingDown || m_reloading || !m_jsEngine
        || portIndex < 0 || portIndex >= static_cast<int>(OutPortCount)) {
        return;
    }

    const QVariantMap map = JSEngineDefines::jsValueToVariantMap(value);
    auto apply = [this, portIndex, map]() {
        if (m_shuttingDown) {
            return;
        }
        {
            QMutexLocker locker(&m_dataMutex);
            out_data[portIndex] = map;
        }
        emit dataUpdated(portIndex);
    };

    if (isGuiThread()) {
        apply();
    } else {
        QTimer::singleShot(0, this, apply);
    }
}
