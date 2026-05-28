#include "QmlScriptRuntime.h"

#include <QDebug>

namespace Nodes {

QmlScriptRuntime::QmlScriptRuntime(QObject *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
{
}

void QmlScriptRuntime::ensureEngine()
{
    if (!m_engine) {
        createEngine();
    }
}

void QmlScriptRuntime::createEngine()
{
    m_engine = new QJSEngine(this);
    m_engine->installExtensions(QJSEngine::AllExtensions);

    if (m_backend) {
        QJSValue nodeObj = m_engine->newQObject(m_backend);
        m_engine->globalObject().setProperty(QStringLiteral("Node"), nodeObj);
    }

    registerUiSchemaBuilders();
}

void QmlScriptRuntime::registerUiSchemaBuilders()
{
    if (!m_engine) {
        return;
    }

    const struct {
        const char *name;
        const char *type;
    } builders[] = {
        {"Slider", "slider"},
        {"VSlider", "slider"},
        {"HSlider", "slider"},
        {"SpinBox", "spinbox"},
        {"CheckBox", "checkbox"},
        {"LineEdit", "lineedit"},
        {"ComboBox", "combobox"},
        {"Label", "label"},
        {"Button", "button"},
    };

    for (const auto &b : builders) {
        const QString code = QStringLiteral(
            "(function(spec){ spec = spec || {}; spec.type = '%1'; return spec; })")
                                 .arg(QString::fromLatin1(b.type));
        m_engine->globalObject().setProperty(QString::fromLatin1(b.name), m_engine->evaluate(code));
    }
}

bool QmlScriptRuntime::evaluateScript(const QString &code, QString *errorOut)
{
    if (!m_engine || code.isEmpty()) {
        return true;
    }

    const QJSValue result = m_engine->evaluate(code);
    if (result.isError()) {
        const QString msg = QStringLiteral("脚本错误 (行 %1): %2")
                                .arg(result.property(QStringLiteral("lineNumber")).toInt())
                                .arg(result.toString());
        if (errorOut) {
            *errorOut = msg;
        }
        qWarning() << msg;
        return false;
    }
    return true;
}

bool QmlScriptRuntime::callInitInterface(QString *errorOut)
{
    return callGlobal(QStringLiteral("initInterface"), {}, nullptr, errorOut);
}

bool QmlScriptRuntime::callInputEventHandler(int portIndex, QString *errorOut)
{
    QJSValueList args;
    args << portIndex;
    return callGlobal(QStringLiteral("inputEventHandler"), args, nullptr, errorOut);
}

bool QmlScriptRuntime::callGlobalFunction(const QString &name,
                                          const QVariantList &args,
                                          QVariant *resultOut,
                                          QString *errorOut)
{
    if (!m_engine) {
        if (errorOut) {
            *errorOut = QStringLiteral("JS 引擎未初始化");
        }
        return false;
    }

    QJSValueList jsArgs;
    jsArgs.reserve(args.size());
    for (const QVariant &v : args) {
        jsArgs << m_engine->toScriptValue(v);
    }

    QJSValue result;
    if (!callGlobal(name, jsArgs, &result, errorOut)) {
        return false;
    }
    if (resultOut) {
        *resultOut = result.toVariant();
    }
    return true;
}

bool QmlScriptRuntime::callGlobal(const QString &name,
                                  const QJSValueList &args,
                                  QJSValue *resultOut,
                                  QString *errorOut)
{
    if (!m_engine) {
        if (errorOut) {
            *errorOut = QStringLiteral("JS 引擎未初始化");
        }
        return false;
    }

    QJSValue func = m_engine->globalObject().property(name);
    if (!func.isCallable()) {
        return true;
    }

    const QJSValue result = func.call(args);
    if (result.isError()) {
        const QString msg = QStringLiteral("%1 错误 (行 %2): %3")
                                .arg(name)
                                .arg(result.property(QStringLiteral("lineNumber")).toInt())
                                .arg(result.toString());
        if (errorOut) {
            *errorOut = msg;
        }
        qWarning() << msg;
        return false;
    }

    if (resultOut) {
        *resultOut = result;
    }
    return true;
}

} // namespace Nodes
