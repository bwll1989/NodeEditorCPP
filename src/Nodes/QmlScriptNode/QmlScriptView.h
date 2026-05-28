#pragma once

#include "JsCodeEditor.h"

#include <QQuickWidget>
#include <QWidget>

namespace Nodes {

class QmlScriptView : public QWidget
{
    Q_OBJECT

public:
    explicit QmlScriptView(QWidget *parent = nullptr);

    void setBackend(QObject *backend);
    void openScriptEditor();
    JsCodeEditor *editor() const { return m_editor; }

signals:
    void qmlReady();

private:
    void onQmlStatusChanged(QQuickWidget::Status status);

    QQuickWidget *m_qmlWidget = nullptr;
    JsCodeEditor *m_editor = nullptr;
    bool m_statusHooked = false;
};

} // namespace Nodes
