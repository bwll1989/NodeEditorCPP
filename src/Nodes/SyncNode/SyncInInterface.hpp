/**
 * @file SyncInInterface.hpp
 * @brief Sync In：TCP 客户端 — Host / Port / Active / 连接状态
 */
#pragma once

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include <QCoreApplication>

namespace Nodes {

class SyncInInterface : public QWidget
{
public:
    explicit SyncInInterface(QWidget *parent = nullptr)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);

        auto *hostRow = new QWidget();
        auto *hostLayout = new QHBoxLayout(hostRow);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->setSpacing(8);
        hostLayout->addWidget(new QLabel(QCoreApplication::translate("Nodes::SyncInInterface", "主机：")));
        hostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
        hostEdit->setText(QStringLiteral("127.0.0.1"));
        hostLayout->addWidget(hostEdit, 1);
        layout->addWidget(hostRow);

        auto *portRow = new QWidget();
        auto *portLayout = new QHBoxLayout(portRow);
        portLayout->setContentsMargins(0, 0, 0, 0);
        portLayout->setSpacing(8);
        portLayout->addWidget(new QLabel(QCoreApplication::translate("Nodes::SyncInInterface", "端口：")));
        portSpin->setRange(1, 65535);
        portSpin->setValue(9100);
        portLayout->addWidget(portSpin, 1);
        layout->addWidget(portRow);

        activeCheck->setText(QCoreApplication::translate("Nodes::SyncInInterface", "启用"));
        activeCheck->setChecked(true);
        activeCheck->setToolTip(QCoreApplication::translate("Nodes::SyncInInterface", "关闭后断开 TCP 并停止重连"));
        layout->addWidget(activeCheck);

        connectionLabel->setFlat(true);
        connectionLabel->setCheckable(true);
        connectionLabel->setEnabled(false);
        connectionLabel->setText(QCoreApplication::translate("Nodes::SyncInInterface", "状态: 未连接"));
        connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
        layout->addWidget(connectionLabel);

        sourceLabel->setText(QCoreApplication::translate("Nodes::SyncInInterface", "对端: —"));
        sourceLabel->setWordWrap(true);
        sourceLabel->setStyleSheet(QStringLiteral("color: gray;"));
        layout->addWidget(sourceLabel);

        setMinimumWidth(240);
    }

    void updateConnectionStatus(bool connected)
    {
        connectionLabel->setChecked(connected);
        connectionLabel->setText(connected
                                     ? QCoreApplication::translate("Nodes::SyncInInterface", "状态: 已连接")
                                     : QCoreApplication::translate("Nodes::SyncInInterface", "状态: 未连接"));
        connectionLabel->setStyleSheet(connected
                                           ? QStringLiteral("color: green; font-weight: bold;")
                                           : QStringLiteral("color: red; font-weight: bold;"));
    }

    void updateSource(const QString &source)
    {
        sourceLabel->setText(source.isEmpty()
                                 ? QCoreApplication::translate("Nodes::SyncInInterface", "对端: —")
                                 : QCoreApplication::translate("Nodes::SyncInInterface", "对端: %1").arg(source));
    }

    QLineEdit *hostEdit = new QLineEdit();
    IntDragValueWidget *portSpin = new IntDragValueWidget();
    QCheckBox *activeCheck = new QCheckBox();
    QPushButton *connectionLabel = new QPushButton();
    QLabel *sourceLabel = new QLabel();
};

} // namespace Nodes
