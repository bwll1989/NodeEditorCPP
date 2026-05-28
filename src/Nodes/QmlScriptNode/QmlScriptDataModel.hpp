#pragma once

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "NodeDataList.hpp"
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "QmlScriptRuntime.h"
#include "QmlScriptView.h"

#include <QJSEngine>
#include <QJSValue>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <QtNodes/NodeDelegateModel>

namespace Nodes {

class QmlScriptDataModel : public AbstractDelegateModel
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList uiSchema READ uiSchema NOTIFY uiSchemaChanged)
    Q_PROPERTY(bool executing READ executing NOTIFY executingChanged)
    Q_PROPERTY(bool initializing READ initializing NOTIFY initializingChanged)
    Q_PROPERTY(QString scriptError READ scriptError NOTIFY scriptErrorChanged)

public:
    QmlScriptDataModel();
    ~QmlScriptDataModel() override = default;

    QVariantMap settings() const;
    QVariantList uiSchema() const;
    bool executing() const { return m_executing; }
    bool initializing() const { return m_initializing; }
    QString scriptError() const { return m_scriptError; }

    Q_INVOKABLE void runScript();
    Q_INVOKABLE void refreshUi();
    Q_INVOKABLE void openEditor();

    Q_INVOKABLE void setSettingValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void applySettingDefault(const QString &key, const QVariant &value);
    Q_INVOKABLE void removeSetting(const QString &key);
    Q_INVOKABLE void setUiSchema(const QJSValue &schema);
    Q_INVOKABLE QVariant callJsFunction(const QString &functionName, const QVariantList &args = {});

    Q_INVOKABLE unsigned int getInputCount();
    Q_INVOKABLE unsigned int getOutputCount();
    Q_INVOKABLE unsigned int inputIndex();

    Q_INVOKABLE QJSValue getInputValue(int portIndex);
    Q_INVOKABLE QJSValue getOutputValue(int portIndex);
    Q_INVOKABLE void setOutputValue(int portIndex, const QJSValue &value);

    Q_INVOKABLE void setSetting(const QString &key, const QJSValue &value);
    Q_INVOKABLE void setSettings(const QJSValue &obj);
    Q_INVOKABLE QJSValue getSetting(const QString &key);
    Q_INVOKABLE bool hasSetting(const QString &key) const;
    Q_INVOKABLE QJSValue getSettings();

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    unsigned int nPorts(QtNodes::PortType portType) const override;
    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex portIndex) override;
    void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

    QWidget *embeddedWidget() override;
    QJsonObject save() const override;
    void load(const QJsonObject &p) override;

signals:
    void settingsChanged();
    void uiSchemaChanged();
    void executingChanged();
    void initializingChanged();
    void scriptErrorChanged();

private slots:
    void onQmlReady();
    void onEditorSave();
    void onEditorRefreshUi();

private:
    void scheduleScriptReload();
    void reloadScript();
    void beginReload();
    void continueReloadAfterUiCleared();
    void completeReload();
    void setScriptError(const QString &message);
    void dispatchSettingChanged(const QString &key, const QVariant &value);
    void dispatchSettingRemoved(const QString &key);
    bool ensureGuiThread(const char *caller) const;

    static QString loadDefaultScript();

    QmlScriptView *m_view = nullptr;
    QmlScriptRuntime *m_runtime = nullptr;

    QString m_script;
    QVariantMap m_settings;
    QVariantList m_uiSchema;
    QMap<unsigned int, QVariantMap> m_inData;
    QMap<unsigned int, QVariantMap> m_outData;

    unsigned m_inputPortIndex = 0;
    bool m_qmlReady = false;
    bool m_pendingReload = false;
    bool m_reloading = false;
    bool m_initializing = false;
    bool m_executing = false;
    QString m_scriptError;

    mutable QMutex m_dataMutex;
};

} // namespace Nodes
