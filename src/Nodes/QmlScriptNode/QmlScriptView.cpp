#include "QmlScriptView.h"

#include <QQmlContext>
#include <QQmlError>
#include <QVBoxLayout>

namespace Nodes {

QmlScriptView::QmlScriptView(QWidget *parent)
    : QWidget(parent)
    , m_editor(new JsCodeEditor(QString(), this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_qmlWidget = new QQuickWidget(this);
    m_qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_qmlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_qmlWidget);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void QmlScriptView::setBackend(QObject *backend)
{
    if (!m_qmlWidget || !backend) {
        return;
    }

    m_qmlWidget->rootContext()->setContextProperty(QStringLiteral("node"), backend);

    if (!m_statusHooked) {
        m_statusHooked = true;
        connect(m_qmlWidget, &QQuickWidget::statusChanged, this, &QmlScriptView::onQmlStatusChanged);
    }

    if (m_qmlWidget->source().isEmpty()) {
        m_qmlWidget->setSource(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    }

    if (m_qmlWidget->status() == QQuickWidget::Ready) {
        emit qmlReady();
    }
}

void QmlScriptView::onQmlStatusChanged(QQuickWidget::Status status)
{
    if (status == QQuickWidget::Ready) {
        emit qmlReady();
        return;
    }

    if (status == QQuickWidget::Error) {
        for (const QQmlError &err : m_qmlWidget->errors()) {
            qWarning() << "QmlScript QML:" << err.toString();
        }
    }
}

void QmlScriptView::openScriptEditor()
{
    m_editor->setParent(nullptr);
    m_editor->setWindowTitle(QStringLiteral("JS 脚本编辑器"));
    m_editor->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    m_editor->setAttribute(Qt::WA_DeleteOnClose, false);
    m_editor->setAttribute(Qt::WA_QuitOnClose, false);
    m_editor->resize(900, 640);
    m_editor->setReadOnly(false);
    m_editor->show();
    m_editor->raise();
    m_editor->activateWindow();
}

} // namespace Nodes
