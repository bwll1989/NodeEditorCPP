#pragma once

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "JavaScriptInterface.hpp"
#include "NodeDataList.hpp"

#include <QJSEngine>
#include <QJSValue>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QPointer>
#include <QString>
#include <QVariantMap>

#include <QtNodes/NodeDelegateModel>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;

namespace Nodes {

/**
 * JavaScript 脚本节点：在 GUI 线程内运行 QJSEngine，避免跨线程崩溃。
 * JS 创建的 QWidget 在加入布局时转为 C++ 所有权，防止引擎与父控件双重释放。
 */
class JavaScriptDataModel : public AbstractDelegateModel
{
    Q_OBJECT

public:
    JavaScriptDataModel();
    ~JavaScriptDataModel() override;

    QString portCaption(PortType portType, PortIndex portIndex) const override;
    unsigned int nPorts(PortType portType) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override;

    QWidget *embeddedWidget() override;
    QJsonObject save() const override;
    void load(const QJsonObject &p) override;

    Q_INVOKABLE unsigned int getInputCount();
    Q_INVOKABLE unsigned int getOutputCount();
    Q_INVOKABLE unsigned int inputIndex();

    Q_INVOKABLE int addToLayout(QObject *widgetObj, int x = -1, int y = -1, int rowSpan = 1, int columnSpan = 1);
    Q_INVOKABLE void clearLayout();
    Q_INVOKABLE void initInterface();
    Q_INVOKABLE void inputEventHandler(int portIndex);

    Q_INVOKABLE QJSValue getInputValue(int portIndex);
    Q_INVOKABLE QJSValue getOutputValue(int portIndex);
    Q_INVOKABLE void setOutputValue(int portIndex, const QJSValue &value);

private slots:
    void onRunButtonClicked();

private:
    bool isGuiThread() const;
    void createJsEngine();
    void destroyJsEngine();
    void takeWidgetOwnership();
    void reloadScript(const QString &code, bool runInitInterface);
    void logJsError(const char *context, const QJSValue &result) const;
    static QString defaultScript();

    QPointer<JavaScriptInterface> widget;
    QJSEngine *m_jsEngine = nullptr;

    QMap<unsigned int, QVariantMap> in_data;
    QMap<unsigned int, QVariantMap> out_data;
    QMap<int, QPointer<QWidget>> m_widgets;

    unsigned inputPortIndex = 0;
    int m_widgetCounter = 0;
    bool m_shuttingDown = false;
    bool m_reloading = false;
    bool m_handlingInput = false;

    QString script;
    mutable QMutex m_dataMutex;
};

} // namespace Nodes
