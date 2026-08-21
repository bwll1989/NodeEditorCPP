#include "CustomScriptDataModel.hpp"

#include "JSEngineDefines/SupportWidgets.hpp"

#include <QDebug>
#include <QGridLayout>
#include <QMetaObject>
#include <QSizePolicy>
#include <QThread>
#include <QTimer>

using QtNodes::NodeData;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Nodes;

CustomScriptDataModel::CustomScriptDataModel()
    : widget(new CustomScriptInterface())
{
    InPortCount = 4;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("Custom Script");
    WidgetEmbeddable = false;
    Resizable = true;
    PortEditable = true;
}

CustomScriptDataModel::CustomScriptDataModel(const JSPluginInfo &pluginInfo)
    : widget(new CustomScriptInterface())
{
    CaptionVisible = true;
    applyPluginInfo(pluginInfo);
    reloadScript(script, true);
}

CustomScriptDataModel::~CustomScriptDataModel()
{
    m_shuttingDown = true;
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (it.value()) {
            it.value()->blockSignals(true);
        }
    }
    destroyJsEngine();
    m_widgets.clear();
    if (widget && widget->parent() == nullptr) {
        widget->deleteLater();
    }
    widget = nullptr;
}

void CustomScriptDataModel::applyPluginInfo(const JSPluginInfo &pluginInfo)
{
    m_pluginInfo = pluginInfo;
    const QJsonObject metadata = pluginInfo.metadata;

    InPortCount = static_cast<unsigned int>(metadata.value(QStringLiteral("inputs")).toInt(pluginInfo.inputs));
    OutPortCount = static_cast<unsigned int>(metadata.value(QStringLiteral("outputs")).toInt(pluginInfo.outputs));
    WidgetEmbeddable = metadata.value(QStringLiteral("embeddable")).toBool(pluginInfo.embeddable);
    Resizable = metadata.value(QStringLiteral("resizable")).toBool(pluginInfo.resizable);
    PortEditable = metadata.value(QStringLiteral("portEditable")).toBool(pluginInfo.portEditable);
    Caption = pluginInfo.name.isEmpty() ? QStringLiteral("JS Script") : pluginInfo.name;

    if (!pluginInfo.code.isEmpty()) {
        script = pluginInfo.code;
    }
}

void CustomScriptDataModel::setPluginInfo(const JSPluginInfo &pluginInfo)
{
    applyPluginInfo(pluginInfo);
    if (!script.isEmpty()) {
        reloadScript(script, true);
    }
}

QString CustomScriptDataModel::portCaption(PortType portType, PortIndex portIndex) const
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

unsigned int CustomScriptDataModel::nPorts(PortType portType) const
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

NodeDataType CustomScriptDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType)
    Q_UNUSED(portIndex)
    return VariableData().type();
}

std::shared_ptr<NodeData> CustomScriptDataModel::outData(PortIndex const portIndex)
{
    QMutexLocker locker(&m_dataMutex);
    if (out_data.contains(portIndex)) {
        return std::make_shared<VariableData>(out_data[portIndex]);
    }
    return std::make_shared<VariableData>();
}

void CustomScriptDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
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
        QMetaObject::invokeMethod(
            this,
            [this, index]() { inputEventHandler(index); },
            Qt::QueuedConnection);
        return;
    }

    inputEventHandler(static_cast<int>(portIndex));
}

QWidget *CustomScriptDataModel::embeddedWidget()
{
    return widget;
}

QJsonObject CustomScriptDataModel::save() const
{
    return {};
}

void CustomScriptDataModel::load(const QJsonObject &p)
{
    Q_UNUSED(p)
}

unsigned int CustomScriptDataModel::getInputCount()
{
    return InPortCount;
}

unsigned int CustomScriptDataModel::getOutputCount()
{
    return OutPortCount;
}

unsigned int CustomScriptDataModel::inputIndex()
{
    return inputPortIndex;
}

bool CustomScriptDataModel::isGuiThread() const
{
    return QThread::currentThread() == this->thread();
}

void CustomScriptDataModel::logJsError(const char *context, const QJSValue &result) const
{
    qWarning() << "CustomScript" << context << "错误:"
               << result.property(QStringLiteral("lineNumber")).toInt()
               << result.toString();
}

void CustomScriptDataModel::takeWidgetOwnership()
{
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        QWidget *w = it.value();
        if (w) {
            QJSEngine::setObjectOwnership(w, QJSEngine::CppOwnership);
        }
    }
}

void CustomScriptDataModel::destroyJsEngine()
{
    takeWidgetOwnership();
    if (!m_jsEngine) {
        return;
    }
    m_jsEngine->globalObject().setProperty(QStringLiteral("Node"), QJSValue());
    delete m_jsEngine;
    m_jsEngine = nullptr;
}

void CustomScriptDataModel::createJsEngine()
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

void CustomScriptDataModel::reloadScript(const QString &code, bool runInitInterface)
{
    if (m_shuttingDown) {
        return;
    }

    if (!isGuiThread()) {
        const QString copied = code;
        const bool runInit = runInitInterface;
        QMetaObject::invokeMethod(
            this,
            [this, copied, runInit]() { reloadScript(copied, runInit); },
            Qt::QueuedConnection);
        return;
    }

    if (m_reloading) {
        return;
    }

    m_reloading = true;
    script = code;

    // 先断开并释放控件，再销毁引擎，避免残留信号进入已删除的 QJSEngine
    clearLayout();
    destroyJsEngine();
    createJsEngine();

    if (!script.isEmpty() && m_jsEngine) {
        const QJSValue result = m_jsEngine->evaluate(script);
        if (result.isError()) {
            logJsError("evaluate", result);
            m_reloading = false;
            return;
        }
        if (runInitInterface) {
            initInterface();
        }
    }

    m_reloading = false;
}

void CustomScriptDataModel::clearLayout()
{
    if (m_shuttingDown) {
        m_widgets.clear();
        m_widgetCounter = 0;
        return;
    }

    if (!isGuiThread()) {
        QMetaObject::invokeMethod(this, &CustomScriptDataModel::clearLayout, Qt::QueuedConnection);
        return;
    }

    takeWidgetOwnership();
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        QWidget *w = it.value();
        if (!w) {
            continue;
        }
        w->blockSignals(true);
        w->hide();
        w->setParent(nullptr);
        w->deleteLater();
    }
    m_widgets.clear();
    m_widgetCounter = 0;
}

int CustomScriptDataModel::addToLayout(QObject *widgetObj, int x, int y, int rowSpan, int columnSpan)
{
    if (m_shuttingDown || !isGuiThread() || !widget || !widget->controlLayout) {
        return -1;
    }

    QWidget *widget_obj = qobject_cast<QWidget *>(widgetObj);
    if (!widget_obj) {
        return -1;
    }

    QJSEngine::setObjectOwnership(widget_obj, QJSEngine::CppOwnership);
    widget_obj->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (x != -1 && y != -1) {
        widget->controlLayout->addWidget(widget_obj, x, y, rowSpan, columnSpan);
    } else {
        widget->controlLayout->addWidget(widget_obj);
    }

    for (int c = 0; c < widget->controlLayout->columnCount(); ++c) {
        if (widget->controlLayout->columnStretch(c) == 0) {
            widget->controlLayout->setColumnStretch(c, 1);
            widget->controlLayout->setColumnMinimumWidth(c, 1);
        }
    }
    for (int r = 0; r < widget->controlLayout->rowCount(); ++r) {
        if (widget->controlLayout->rowStretch(r) == 0) {
            widget->controlLayout->setRowStretch(r, 1);
            widget->controlLayout->setRowMinimumHeight(r, 1);
        }
    }

    const int id = m_widgetCounter++;
    m_widgets[id] = widget_obj;
    return id;
}

void CustomScriptDataModel::initInterface()
{
    if (m_shuttingDown) {
        return;
    }

    if (!isGuiThread()) {
        QMetaObject::invokeMethod(this, &CustomScriptDataModel::initInterface, Qt::QueuedConnection);
        return;
    }

    if (!m_jsEngine) {
        qWarning() << "CustomScript 引擎未初始化";
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

void CustomScriptDataModel::inputEventHandler(int portIndex)
{
    if (m_shuttingDown || m_reloading || m_handlingInput || !m_jsEngine) {
        return;
    }

    if (!isGuiThread()) {
        QMetaObject::invokeMethod(
            this,
            [this, portIndex]() { inputEventHandler(portIndex); },
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

QJSValue CustomScriptDataModel::getInputValue(int portIndex)
{
    if (!m_jsEngine) {
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

QJSValue CustomScriptDataModel::getOutputValue(int portIndex)
{
    if (!m_jsEngine) {
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

void CustomScriptDataModel::setOutputValue(int portIndex, const QJSValue &value)
{
    if (m_shuttingDown || portIndex < 0 || portIndex >= static_cast<int>(OutPortCount)) {
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
