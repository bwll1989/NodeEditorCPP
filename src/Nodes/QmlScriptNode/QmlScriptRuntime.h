#pragma once

#include <QJSEngine>
#include <QJSValue>
#include <QObject>
#include <QString>
#include <QVariant>

namespace Nodes {

class QmlScriptRuntime : public QObject
{
    Q_OBJECT

public:
    explicit QmlScriptRuntime(QObject *backend, QObject *parent = nullptr);

    void ensureEngine();
    bool evaluateScript(const QString &code, QString *errorOut = nullptr);
    void registerUiSchemaBuilders();
    bool callInitInterface(QString *errorOut = nullptr);
    bool callInputEventHandler(int portIndex, QString *errorOut = nullptr);
    bool callGlobalFunction(const QString &name,
                            const QVariantList &args,
                            QVariant *resultOut,
                            QString *errorOut = nullptr);

    QJSEngine *engine() const { return m_engine; }

private:
    void createEngine();
    bool callGlobal(const QString &name, const QJSValueList &args, QJSValue *resultOut, QString *errorOut);

    QObject *m_backend = nullptr;
    QJSEngine *m_engine = nullptr;
};

} // namespace Nodes
