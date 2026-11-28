#pragma once

#include <QGroupBox>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QSignalBlocker>
#include <QCoreApplication>

namespace Nodes
{
    class RTSPInInterface : public QGroupBox
    {
        Q_OBJECT

    public:
        explicit RTSPInInterface(QWidget* parent = nullptr)
            : QGroupBox(QCoreApplication::translate("Nodes::RTSPInInterface", "RTSP 拉流"), parent)
        {
            m_urlEdit = new QLineEdit(this);
            m_urlEdit->setPlaceholderText(QStringLiteral("rtsp://user:pass@host:554/stream"));
            m_urlEdit->setText(QStringLiteral("rtsp://127.0.0.1:8554/live"));

            m_startStopButton = new QPushButton(QCoreApplication::translate("Nodes::RTSPInInterface", "开始"), this);
            m_startStopButton->setCheckable(true);

            m_connectionStatusLabel = new QLabel(QCoreApplication::translate("Nodes::RTSPInInterface", "状态: 未连接"), this);
            m_connectionStatusLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            m_mainLayout = new QVBoxLayout(this);
            m_mainLayout->setContentsMargins(5, 5, 5, 5);
            m_mainLayout->setSpacing(6);
            m_mainLayout->addWidget(m_urlEdit);
            m_mainLayout->addWidget(m_startStopButton);
            m_mainLayout->addWidget(m_connectionStatusLabel);
            m_mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

            setMinimumSize(280, 130);
            setMaximumWidth(360);

            connect(m_startStopButton, &QPushButton::toggled, this, [this](bool checked) {
                if (checked) {
                    emit startReceiving();
                } else {
                    emit stopReceiving();
                }
            });
            connect(m_urlEdit, &QLineEdit::editingFinished, this, [this]() {
                emit urlChanged(m_urlEdit->text());
            });
        }

    signals:
        void startReceiving();
        void stopReceiving();
        void urlChanged(const QString& url);

    public slots:
        void updateConnectionStatus(bool connected)
        {
            QSignalBlocker blocker(m_startStopButton);
            m_startStopButton->setChecked(connected);
            if (connected) {
                m_connectionStatusLabel->setText(QCoreApplication::translate("Nodes::RTSPInInterface", "状态: 已连接"));
                m_connectionStatusLabel->setStyleSheet(QStringLiteral("color: green; font-weight: bold;"));
                m_startStopButton->setText(QCoreApplication::translate("Nodes::RTSPInInterface", "停止"));
            } else {
                m_connectionStatusLabel->setText(QCoreApplication::translate("Nodes::RTSPInInterface", "状态: 未连接"));
                m_connectionStatusLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
                m_startStopButton->setText(QCoreApplication::translate("Nodes::RTSPInInterface", "开始"));
            }
        }

        void showError(const QString& message)
        {
            m_connectionStatusLabel->setText(message);
            m_connectionStatusLabel->setStyleSheet(QStringLiteral("color: orange; font-weight: bold;"));
        }

    public:
        QLineEdit* m_urlEdit = nullptr;
        QPushButton* m_startStopButton = nullptr;

    private:
        QVBoxLayout* m_mainLayout = nullptr;
        QLabel* m_connectionStatusLabel = nullptr;
    };
}
