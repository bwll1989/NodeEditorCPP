#include "QmlScriptDataModel.hpp"

#include <QApplication>
#include <QJsonArray>
#include <QFile>
#include <QJSValueIterator>
#include <QPointer>
#include <QPushButton>
#include <QThread>

using QtNodes::NodeData;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes {

namespace {

bool isGuiThread()
{
    return QThread::currentThread() == QApplication::instance()->thread();
}

} // namespace

QString QmlScriptDataModel::loadDefaultScript()
{
    QFile file(QStringLiteral(":/resources/defaultScript.js"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    qWarning() << "QmlScript: 无法加载默认脚本";
    return QString();
}

QmlScriptDataModel::QmlScriptDataModel()
    : m_view(new QmlScriptView())
    , m_runtime(new QmlScriptRuntime(this, this))
    , m_script(loadDefaultScript())
{
    InPortCount = 4;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("QML JS Script");
    WidgetEmbeddable = false;
    Resizable = true;
    PortEditable = true;

    connect(m_view, &QmlScriptView::qmlReady, this, &QmlScriptDataModel::onQmlReady);
    m_view->setBackend(this);

    connect(m_view->editor()->importJS, &QPushButton::clicked, this, &QmlScriptDataModel::onEditorSave);
    connect(m_view->editor()->updateUI, &QPushButton::clicked, this, &QmlScriptDataModel::onEditorRefreshUi);
}

QVariantMap QmlScriptDataModel::settings() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_settings;
}

QVariantList QmlScriptDataModel::uiSchema() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_uiSchema;
}

void QmlScriptDataModel::onQmlReady()
{
    if (m_qmlReady) {
        return;
    }
    m_qmlReady = true;
    if (m_pendingReload || !m_script.isEmpty()) {
        reloadScript();
    }
}

void QmlScriptDataModel::scheduleScriptReload()
{
    if (!m_qmlReady) {
        m_pendingReload = true;
        return;
    }
    if (m_reloading) {
        m_pendingReload = true;
        return;
    }
    m_pendingReload = false;
    reloadScript();
}

void QmlScriptDataModel::beginReload()
{
    m_reloading = true;
    m_initializing = true;
    emit initializingChanged();

    {
        QMutexLocker locker(&m_dataMutex);
        m_uiSchema.clear();
    }
    emit uiSchemaChanged();
    setScriptError(QString());
}

void QmlScriptDataModel::continueReloadAfterUiCleared()
{
    if (!m_reloading) {
        return;
    }

    m_runtime->ensureEngine();

    QString error;
    if (!m_runtime->evaluateScript(m_script, &error)) {
        setScriptError(error);
        completeReload();
        return;
    }

    m_runtime->registerUiSchemaBuilders();

    QPointer<QmlScriptDataModel> guard(this);
    QTimer::singleShot(0, this, [guard]() {
        if (!guard || !guard->m_reloading) {
            return;
        }

        QString initError;
        if (!guard->m_runtime->callInitInterface(&initError)) {
            guard->setScriptError(initError);
        }

        QTimer::singleShot(0, guard.data(), [guard]() {
            if (!guard || !guard->m_reloading) {
                return;
            }
            guard->completeReload();
        });
    });
}

void QmlScriptDataModel::completeReload()
{
    if (!m_reloading) {
        return;
    }
    m_reloading = false;
    m_initializing = false;
    emit initializingChanged();

    if (m_pendingReload) {
        m_pendingReload = false;
        scheduleScriptReload();
    }
}

void QmlScriptDataModel::reloadScript()
{
    if (!ensureGuiThread(__func__)) {
        QMetaObject::invokeMethod(this, &QmlScriptDataModel::reloadScript, Qt::QueuedConnection);
        return;
    }

    if (m_reloading) {
        return;
    }

    beginReload();

    QPointer<QmlScriptDataModel> guard(this);
    QTimer::singleShot(0, this, [guard]() {
        if (guard) {
            guard->continueReloadAfterUiCleared();
        }
    });
}

void QmlScriptDataModel::setScriptError(const QString &message)
{
    if (m_scriptError == message) {
        return;
    }
    m_scriptError = message;
    emit scriptErrorChanged();
}

bool QmlScriptDataModel::ensureGuiThread(const char *caller) const
{
    if (isGuiThread()) {
        return true;
    }
    qWarning() << "QmlScript:" << caller << "必须在 GUI 线程调用";
    return false;
}

void QmlScriptDataModel::runScript()
{
    onEditorSave();
}

void QmlScriptDataModel::refreshUi()
{
    if (m_reloading) {
        return;
    }
    if (m_view && m_view->editor()) {
        m_script = m_view->editor()->saveCode();
    }
    scheduleScriptReload();
}

void QmlScriptDataModel::openEditor()
{
    if (m_view) {
        m_view->openScriptEditor();
    }
}

void QmlScriptDataModel::onEditorSave()
{
    if (m_reloading || !m_view || !m_view->editor()) {
        return;
    }
    m_script = m_view->editor()->saveCode();

    m_runtime->ensureEngine();

    m_executing = true;
    emit executingChanged();

    QString error;
    if (!m_runtime->evaluateScript(m_script, &error)) {
        setScriptError(error);
    } else {
        setScriptError(QString());
    }

    m_executing = false;
    emit executingChanged();
}

void QmlScriptDataModel::onEditorRefreshUi()
{
    refreshUi();
}

void QmlScriptDataModel::applySettingDefault(const QString &key, const QVariant &value)
{
    {
        QMutexLocker locker(&m_dataMutex);
        if (m_settings.contains(key)) {
            return;
        }
        m_settings.insert(key, value);
    }
    emit settingsChanged();
}

void QmlScriptDataModel::setSettingValue(const QString &key, const QVariant &value)
{
    {
        QMutexLocker locker(&m_dataMutex);
        m_settings.insert(key, value);
    }
    emit settingsChanged();
    dispatchSettingChanged(key, value);
}

void QmlScriptDataModel::removeSetting(const QString &key)
{
    {
        QMutexLocker locker(&m_dataMutex);
        m_settings.remove(key);
    }
    emit settingsChanged();
    dispatchSettingRemoved(key);
}

void QmlScriptDataModel::dispatchSettingChanged(const QString &key, const QVariant &value)
{
    if (m_initializing || m_reloading || !m_runtime->engine()) {
        return;
    }
    QString error;
    QVariantList args{key, value};
    m_runtime->callGlobalFunction(QStringLiteral("onSettingChanged"), args, nullptr, &error);
    if (!error.isEmpty()) {
        setScriptError(error);
    }
}

void QmlScriptDataModel::dispatchSettingRemoved(const QString &key)
{
    if (m_initializing || m_reloading || !m_runtime->engine()) {
        return;
    }
    QString error;
    m_runtime->callGlobalFunction(QStringLiteral("onSettingRemoved"), {key}, nullptr, &error);
    if (!error.isEmpty()) {
        setScriptError(error);
    }
}

void QmlScriptDataModel::setUiSchema(const QJSValue &schema)
{
    QVariantList list;

    if (schema.isArray()) {
        const int length = schema.property(QStringLiteral("length")).toInt();
        list.reserve(length);
        for (int i = 0; i < length; ++i) {
            const QJSValue item = schema.property(static_cast<quint32>(i));
            list.append(item.isObject() || item.isArray() ? item.toVariant() : QVariant());
        }
    } else if (schema.isObject()) {
        list.append(schema.toVariant().toMap());
    } else {
        return;
    }

    {
        QMutexLocker locker(&m_dataMutex);
        m_uiSchema = list;
    }
    emit uiSchemaChanged();
}

QVariant QmlScriptDataModel::callJsFunction(const QString &functionName, const QVariantList &args)
{
    if (m_initializing || m_reloading || !m_runtime->engine()) {
        return {};
    }

    if (!ensureGuiThread(__func__)) {
        QVariant result;
        const QString fn = functionName;
        const QVariantList fnArgs = args;
        QMetaObject::invokeMethod(
            this,
            [this, &result, fn, fnArgs]() { result = callJsFunction(fn, fnArgs); },
            Qt::BlockingQueuedConnection);
        return result;
    }

    QString error;
    QVariant result;
    if (!m_runtime->callGlobalFunction(functionName, args, &result, &error)) {
        setScriptError(error);
        return {};
    }
    return result;
}

unsigned int QmlScriptDataModel::getInputCount()
{
    return InPortCount;
}

unsigned int QmlScriptDataModel::getOutputCount()
{
    return OutPortCount;
}

unsigned int QmlScriptDataModel::inputIndex()
{
    return m_inputPortIndex;
}

QJSValue QmlScriptDataModel::getInputValue(int portIndex)
{
    auto *engine = m_runtime->engine();
    if (!engine) {
        return {};
    }

    QMutexLocker locker(&m_dataMutex);
    if (portIndex >= 0 && portIndex < static_cast<int>(InPortCount) && m_inData.contains(portIndex)) {
        return JSEngineDefines::variantMapToJSValue(engine, m_inData[portIndex]);
    }
    return engine->newObject();
}

QJSValue QmlScriptDataModel::getOutputValue(int portIndex)
{
    auto *engine = m_runtime->engine();
    if (!engine) {
        return {};
    }

    QMutexLocker locker(&m_dataMutex);
    if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount) && m_outData.contains(portIndex)) {
        return JSEngineDefines::variantMapToJSValue(engine, m_outData[portIndex]);
    }
    return engine->newObject();
}

void QmlScriptDataModel::setOutputValue(int portIndex, const QJSValue &value)
{
    if (portIndex < 0 || portIndex >= static_cast<int>(OutPortCount) || !m_runtime->engine()) {
        return;
    }

    const QVariantMap map = JSEngineDefines::jsValueToVariantMap(value);
    auto apply = [this, portIndex, map]() {
        {
            QMutexLocker locker(&m_dataMutex);
            m_outData[portIndex] = map;
        }
        emit dataUpdated(portIndex);
    };

    if (isGuiThread()) {
        apply();
    } else {
        QTimer::singleShot(0, this, apply);
    }
}

void QmlScriptDataModel::setSetting(const QString &key, const QJSValue &value)
{
    setSettingValue(key, value.toVariant());
}

void QmlScriptDataModel::setSettings(const QJSValue &obj)
{
    if (!obj.isObject()) {
        return;
    }

    {
        QMutexLocker locker(&m_dataMutex);
        QJSValueIterator it(obj);
        while (it.hasNext()) {
            it.next();
            m_settings.insert(it.name(), it.value().toVariant());
        }
    }
    emit settingsChanged();
}

QJSValue QmlScriptDataModel::getSetting(const QString &key)
{
    auto *engine = m_runtime->engine();
    if (!engine) {
        return {};
    }

    QMutexLocker locker(&m_dataMutex);
    if (!m_settings.contains(key)) {
        return {};
    }
    return engine->toScriptValue(m_settings.value(key));
}

bool QmlScriptDataModel::hasSetting(const QString &key) const
{
    QMutexLocker locker(&m_dataMutex);
    return m_settings.contains(key);
}

QJSValue QmlScriptDataModel::getSettings()
{
    auto *engine = m_runtime->engine();
    if (!engine) {
        return {};
    }

    QJSValue obj = engine->newObject();
    QMutexLocker locker(&m_dataMutex);
    for (auto it = m_settings.constBegin(); it != m_settings.constEnd(); ++it) {
        obj.setProperty(it.key(), engine->toScriptValue(it.value()));
    }
    return obj;
}

QString QmlScriptDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

unsigned int QmlScriptDataModel::nPorts(PortType portType) const
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

QtNodes::NodeDataType QmlScriptDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType)
    Q_UNUSED(portIndex)
    return VariableData().type();
}

std::shared_ptr<NodeData> QmlScriptDataModel::outData(PortIndex portIndex)
{
    QMutexLocker locker(&m_dataMutex);
    if (m_outData.contains(portIndex)) {
        return std::make_shared<VariableData>(m_outData[portIndex]);
    }
    return std::make_shared<VariableData>();
}

void QmlScriptDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    auto variable = std::dynamic_pointer_cast<VariableData>(data);
    if (!variable) {
        return;
    }

    {
        QMutexLocker locker(&m_dataMutex);
        m_inputPortIndex = portIndex;
        m_inData[portIndex] = variable->getMap();
    }

    if (!m_runtime->engine() || m_initializing || m_reloading) {
        return;
    }

    if (!ensureGuiThread(__func__)) {
        const PortIndex idx = portIndex;
        QMetaObject::invokeMethod(
            this,
            [this, idx]() {
                QString error;
                if (!m_runtime->callInputEventHandler(static_cast<int>(idx), &error)) {
                    setScriptError(error);
                }
            },
            Qt::QueuedConnection);
        return;
    }

    QString error;
    if (!m_runtime->callInputEventHandler(static_cast<int>(portIndex), &error)) {
        setScriptError(error);
    }
}

QWidget *QmlScriptDataModel::embeddedWidget()
{
    if (m_view && m_view->editor()) {
        m_view->editor()->loadCodeFromCode(m_script);
    }

    if (m_qmlReady && !m_script.isEmpty()) {
        bool schemaEmpty = false;
        {
            QMutexLocker locker(&m_dataMutex);
            schemaEmpty = m_uiSchema.isEmpty();
        }
        if (schemaEmpty) {
            scheduleScriptReload();
        }
    }

    return m_view;
}

QJsonObject QmlScriptDataModel::save() const
{
    QString scriptOnDisk = m_script;
    if (m_view && m_view->editor()) {
        scriptOnDisk = const_cast<JsCodeEditor *>(m_view->editor())->saveCode();
    }

    QJsonObject values;
    values[QStringLiteral("scripts")] = scriptOnDisk;
    {
        QMutexLocker locker(&m_dataMutex);
        values[QStringLiteral("settings")] = QJsonObject::fromVariantMap(m_settings);

        QJsonArray schemaArr;
        for (const QVariant &item : m_uiSchema) {
            if (item.metaType().id() == QMetaType::QVariantMap) {
                schemaArr.append(QJsonObject::fromVariantMap(item.toMap()));
            }
        }
        values[QStringLiteral("uiSchema")] = schemaArr;
    }

    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson[QStringLiteral("values")] = values;
    modelJson[QStringLiteral("InPortCount")] = static_cast<int>(InPortCount);
    modelJson[QStringLiteral("OutPortCount")] = static_cast<int>(OutPortCount);
    return modelJson;
}

void QmlScriptDataModel::load(const QJsonObject &p)
{
    const QJsonValue valuesNode = p.value(QStringLiteral("values"));
    if (!valuesNode.isObject()) {
        return;
    }

    const QJsonObject valuesObj = valuesNode.toObject();
    m_script = valuesObj.value(QStringLiteral("scripts")).toString();
    if (m_script.isEmpty()) {
        m_script = loadDefaultScript();
    }

    {
        QMutexLocker locker(&m_dataMutex);
        m_settings = valuesObj.value(QStringLiteral("settings")).toObject().toVariantMap();

        m_uiSchema.clear();
        const QJsonValue schemaNode = valuesObj.value(QStringLiteral("uiSchema"));
        if (schemaNode.isArray()) {
            const QJsonArray schemaArr = schemaNode.toArray();
            m_uiSchema.reserve(schemaArr.size());
            for (const QJsonValue &item : schemaArr) {
                if (item.isObject()) {
                    m_uiSchema.append(item.toObject().toVariantMap());
                }
            }
        }
    }

    InPortCount = static_cast<unsigned int>(p.value(QStringLiteral("InPortCount")).toInt(static_cast<int>(InPortCount)));
    OutPortCount = static_cast<unsigned int>(p.value(QStringLiteral("OutPortCount")).toInt(static_cast<int>(OutPortCount)));

    emit settingsChanged();
    emit uiSchemaChanged();

    if (m_view && m_view->editor()) {
        m_view->editor()->loadCodeFromCode(m_script);
    }

    scheduleScriptReload();
}

} // namespace Nodes
