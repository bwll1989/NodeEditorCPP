/**
 * @file PingInterface.hpp
 * @brief Ping 节点界面：Host + Enable + 可达状态
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCoreApplication>

namespace Nodes
{
    class PingInterface : public QWidget
    {
    public:
        explicit PingInterface(QWidget *parent = nullptr)
        {
            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(6);

            auto *hostRow = new QWidget();
            auto *hostLayout = new QHBoxLayout(hostRow);
            hostLayout->setContentsMargins(0, 0, 0, 0);
            hostLayout->setSpacing(8);
            hostLayout->addWidget(new QLabel(QCoreApplication::translate("Nodes::PingInterface", "主机：")));
            hostEdit->setPlaceholderText(QStringLiteral("192.168.1.1"));
            hostEdit->setText(QStringLiteral("127.0.0.1"));
            hostLayout->addWidget(hostEdit, 1);
            layout->addWidget(hostRow);

            enableCheckBox->setText(QCoreApplication::translate("Nodes::PingInterface", "启用"));
            enableCheckBox->setChecked(false);
            enableCheckBox->setToolTip(QCoreApplication::translate("Nodes::PingInterface", "开启后每 10 秒 ping 一次（与 ENABLE 端口相同）"));
            layout->addWidget(enableCheckBox);

            connectionLabel->setFlat(true);
            connectionLabel->setCheckable(true);
            connectionLabel->setEnabled(false);
            connectionLabel->setText(QCoreApplication::translate("Nodes::PingInterface", "状态: 不可达"));
            connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
            layout->addWidget(connectionLabel);

            setMinimumWidth(240);
        }

        void updateConnectionStatus(bool connected)
        {
            connectionLabel->setChecked(connected);
            connectionLabel->setText(connected
                                         ? QCoreApplication::translate("Nodes::PingInterface", "状态: 可达")
                                         : QCoreApplication::translate("Nodes::PingInterface", "状态: 不可达"));
            connectionLabel->setStyleSheet(connected
                                               ? QStringLiteral("color: green; font-weight: bold;")
                                               : QStringLiteral("color: red; font-weight: bold;"));
        }

        QLineEdit *hostEdit = new QLineEdit();
        QCheckBox *enableCheckBox = new QCheckBox();
        QPushButton *connectionLabel = new QPushButton();
    };
}
