#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QSignalBlocker>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief AJT Gateway 界面：TCP 连接与源地址
 */
class AJTGatewayInterface : public QWidget
{
    Q_OBJECT

public:
    explicit AJTGatewayInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto connectionGroup = new QGroupBox(QStringLiteral("连接设置"), this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        connectionLayout->setContentsMargins(6, 8, 6, 6);
        connectionLayout->setSpacing(4);

        connectionLayout->addWidget(new QLabel(QStringLiteral("设备 IP:"), this), 0, 0);
        _hostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), this);
        connectionLayout->addWidget(_hostEdit, 0, 1);

        connectionLayout->addWidget(new QLabel(QStringLiteral("端口:"), this), 1, 0);
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(1001);
        connectionLayout->addWidget(_portEdit, 1, 1);

        connectionLayout->addWidget(new QLabel(QStringLiteral("源地址:"), this), 2, 0);
        _srcAddrEdit = new IntDragValueWidget(this);
        _srcAddrEdit->setRange(0, 255);
        _srcAddrEdit->setValue(0x46);
        connectionLayout->addWidget(_srcAddrEdit, 2, 1);

        _statusLabel = new QPushButton(QStringLiteral("状态: 未连接"), this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
        connectionLayout->addWidget(_statusLabel, 3, 0, 1, 2);

        _deviceCountLabel = new QLabel(QStringLiteral("设备数: 0"), this);
        connectionLayout->addWidget(_deviceCountLabel, 4, 0, 1, 2);

        mainLayout->addWidget(connectionGroup);
        mainLayout->addStretch();
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumSize(240, 200);
    }

    void setConnectionStatus(bool connected)
    {
        if (connected) {
            _statusLabel->setChecked(true);
            _statusLabel->setText(QStringLiteral("状态: 已连接"));
            _statusLabel->setStyleSheet(QStringLiteral("color: green; font-weight: bold;"));
        } else {
            _statusLabel->setChecked(false);
            _statusLabel->setText(QStringLiteral("状态: 未连接"));
            _statusLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
        }
    }

    void setDeviceCount(int count)
    {
        _deviceCountLabel->setText(QStringLiteral("设备数: %1").arg(count));
    }

    QLineEdit *_hostEdit = nullptr;
    IntDragValueWidget *_portEdit = nullptr;
    IntDragValueWidget *_srcAddrEdit = nullptr;
    QPushButton *_statusLabel = nullptr;
    QLabel *_deviceCountLabel = nullptr;
};
