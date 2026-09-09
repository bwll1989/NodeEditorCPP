#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QComboBox>

#include "RangerMiniController.h"
#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{
    class RangerMiniInterface : public QWidget
    {
        Q_OBJECT

    public:
        explicit RangerMiniInterface(QWidget *parent = nullptr)
        {
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(6);
            mainLayout->setColumnStretch(0, 1);
            mainLayout->setColumnStretch(1, 2);

            motionModeCombo->addItem(QStringLiteral("阿克曼"), RangerMiniController::Ackermann);
            motionModeCombo->addItem(QStringLiteral("斜移"), RangerMiniController::Diagonal);
            motionModeCombo->addItem(QStringLiteral("自旋"), RangerMiniController::Spin);
            motionModeCombo->addItem(QStringLiteral("驻车"), RangerMiniController::Park);

            linearSpin->setRange(-2.0, 2.0);
            linearSpin->setDecimals(3);
            linearSpin->setSingleStep(0.05);
            linearSpin->setSuffix(QStringLiteral(" m/s"));
            linearSpin->setValue(0.0);

            steerSpin->setRange(-1.571, 1.571);
            steerSpin->setDecimals(3);
            steerSpin->setSingleStep(0.01);
            steerSpin->setSuffix(QStringLiteral(" rad"));
            steerSpin->setValue(0.0);

            spinRateSpin->setRange(-3.259, 3.259);
            spinRateSpin->setDecimals(3);
            spinRateSpin->setSingleStep(0.1);
            spinRateSpin->setSuffix(QStringLiteral(" rad/s"));
            spinRateSpin->setValue(0.0);

            int row = 0;
            mainLayout->addWidget(deviceLabel, row, 0);
            mainLayout->addWidget(deviceCombo, row++, 1);
            mainLayout->addWidget(refreshButton, row++, 0, 1, 2);
            mainLayout->addWidget(connectButton, row++, 0, 1, 2);
            mainLayout->addWidget(motionModeLabel, row, 0);
            mainLayout->addWidget(motionModeCombo, row++, 1);
            mainLayout->addWidget(linearLabel, row, 0);
            mainLayout->addWidget(linearSpin, row++, 1);
            mainLayout->addWidget(steerLabel, row, 0);
            mainLayout->addWidget(steerSpin, row++, 1);
            mainLayout->addWidget(spinLabel, row, 0);
            mainLayout->addWidget(spinRateSpin, row++, 1);
            mainLayout->addWidget(clearFaultButton, row++, 0, 1, 2);
            mainLayout->addWidget(enableButton, row++, 0, 1, 2);
            mainLayout->addWidget(statusLabel, row++, 0, 1, 2);

            connectButton->setCheckable(true);
            connectButton->setText(QStringLiteral("未连接"));
            connectButton->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            enableButton->setCheckable(true);
            enableButton->setEnabled(false);
            enableButton->setText(QStringLiteral("控制关闭"));
            enableButton->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            statusLabel->setWordWrap(true);
            statusLabel->setText(QStringLiteral("状态：-"));

            refreshDevices();
            connect(refreshButton, &QPushButton::clicked, this, &RangerMiniInterface::refreshDevices);
            setLayout(mainLayout);
        }

        void refreshDevices()
        {
            const QVariant current = deviceCombo->currentData();
            deviceCombo->clear();
            for (const CanBusDeviceInfo &info : RangerMiniController::availableDevices()) {
                deviceCombo->addItem(info.displayName, info.index);
                deviceCombo->setItemData(deviceCombo->count() - 1, info.path, Qt::ToolTipRole);
            }
            if (deviceCombo->count() == 0) {
                deviceCombo->addItem(QStringLiteral("（未检测到 candleLight 设备）"), -1);
            } else if (current.isValid()) {
                const int idx = deviceCombo->findData(current);
                if (idx >= 0) {
                    deviceCombo->setCurrentIndex(idx);
                }
            }
        }

        void setBusConnected(bool ready)
        {
            QSignalBlocker blocker(connectButton);
            connectButton->setChecked(ready);
            connectButton->setText(ready ? QStringLiteral("已连接 @500k") : QStringLiteral("未连接"));
            connectButton->setStyleSheet(ready ? QStringLiteral("color: green; font-weight: bold;")
                                               : QStringLiteral("color: red; font-weight: bold;"));
            enableButton->setEnabled(ready);
        }

        void setControlArmed(bool armed)
        {
            QSignalBlocker blocker(enableButton);
            enableButton->setChecked(armed);
            enableButton->setText(armed ? QStringLiteral("控制开启") : QStringLiteral("控制关闭"));
            enableButton->setStyleSheet(armed ? QStringLiteral("color: green; font-weight: bold;")
                                              : QStringLiteral("color: red; font-weight: bold;"));
        }

        QGridLayout *mainLayout = new QGridLayout(this);
        QComboBox *deviceCombo = new QComboBox();
        QPushButton *refreshButton = new QPushButton(QStringLiteral("刷新设备"));
        QPushButton *connectButton = new QPushButton();
        QComboBox *motionModeCombo = new QComboBox();
        FloatDragValueWidget *linearSpin = new FloatDragValueWidget();
        FloatDragValueWidget *steerSpin = new FloatDragValueWidget();
        FloatDragValueWidget *spinRateSpin = new FloatDragValueWidget();
        QPushButton *clearFaultButton = new QPushButton(QStringLiteral("清除故障"));
        QPushButton *enableButton = new QPushButton();
        QLabel *statusLabel = new QLabel();

    private:
        QLabel *deviceLabel = new QLabel(QStringLiteral("设备"));
        QLabel *motionModeLabel = new QLabel(QStringLiteral("运动模式"));
        QLabel *linearLabel = new QLabel(QStringLiteral("线速度 Vx"));
        QLabel *steerLabel = new QLabel(QStringLiteral("转角"));
        QLabel *spinLabel = new QLabel(QStringLiteral("自旋"));
    };
}
