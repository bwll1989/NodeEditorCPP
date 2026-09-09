/**
 * @file SyncOutInterface.hpp
 * @brief Sync Out：TCP 服务端 — Listen Port / Active / 客户端数 / Push
 */
#pragma once

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes {

class SyncOutInterface : public QWidget
{
public:
    explicit SyncOutInterface(QWidget *parent = nullptr)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);

        auto *portRow = new QWidget();
        auto *portLayout = new QHBoxLayout(portRow);
        portLayout->setContentsMargins(0, 0, 0, 0);
        portLayout->setSpacing(8);
        portLayout->addWidget(new QLabel(QStringLiteral("Listen Port:")));
        portSpin->setRange(1, 65535);
        portSpin->setValue(9100);
        portLayout->addWidget(portSpin, 1);
        layout->addWidget(portRow);

        activeCheck->setText(QStringLiteral("启用"));
        activeCheck->setChecked(true);
        activeCheck->setToolTip(QStringLiteral("关闭后停止 TCP 监听"));
        layout->addWidget(activeCheck);

        connectionLabel->setFlat(true);
        connectionLabel->setCheckable(true);
        connectionLabel->setEnabled(false);
        connectionLabel->setText(QStringLiteral("状态: 未监听"));
        connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
        layout->addWidget(connectionLabel);

        pushButton->setText(QStringLiteral("Push"));
        pushButton->setToolTip(QStringLiteral("向所有已连接的 Sync In 重发口数与缓存值"));
        layout->addWidget(pushButton);

        setMinimumWidth(240);
    }

    void updateStatus(bool listening, int clientCount)
    {
        if (!listening) {
            connectionLabel->setChecked(false);
            connectionLabel->setText(QStringLiteral("状态: 未监听"));
            connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
            return;
        }
        if (clientCount <= 0) {
            connectionLabel->setChecked(false);
            connectionLabel->setText(QStringLiteral("状态: 等待连接"));
            connectionLabel->setStyleSheet(QStringLiteral("color: orange; font-weight: bold;"));
            return;
        }
        connectionLabel->setChecked(true);
        connectionLabel->setText(QStringLiteral("状态: 已连接 (%1)").arg(clientCount));
        connectionLabel->setStyleSheet(QStringLiteral("color: green; font-weight: bold;"));
    }

    IntDragValueWidget *portSpin = new IntDragValueWidget();
    QCheckBox *activeCheck = new QCheckBox();
    QPushButton *connectionLabel = new QPushButton();
    QPushButton *pushButton = new QPushButton();
};

} // namespace Nodes
