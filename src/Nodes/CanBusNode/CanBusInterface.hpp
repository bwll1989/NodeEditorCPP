#pragma once

#include <QWidget>
#include <QCoreApplication>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>

#include "Common/Devices/CanBus/CanBus.h"
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    class CanBusInterface : public QWidget
    {
        Q_OBJECT

    public:
        explicit CanBusInterface(QWidget *parent = nullptr)
        {
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(6);
            mainLayout->setColumnStretch(0, 1);
            mainLayout->setColumnStretch(1, 2);

            bitrateCombo->addItem(QStringLiteral("125000"), 125000);
            bitrateCombo->addItem(QStringLiteral("250000"), 250000);
            bitrateCombo->addItem(QStringLiteral("500000"), 500000);
            bitrateCombo->addItem(QStringLiteral("800000"), 800000);
            bitrateCombo->addItem(QStringLiteral("1000000"), 1000000);
            bitrateCombo->setCurrentIndex(2);

            modeCombo->addItem(QCoreApplication::translate("Nodes::CanBusInterface", "正常"), 0);
            modeCombo->addItem(QCoreApplication::translate("Nodes::CanBusInterface", "只听"), 1);
            modeCombo->addItem(QCoreApplication::translate("Nodes::CanBusInterface", "回环"), 2);

            channelSpin->setRange(0, 7);
            channelSpin->setValue(0);

            canIdEdit->setPlaceholderText(QStringLiteral("0x123"));
            canIdEdit->setText(QStringLiteral("0x123"));
            dataEdit->setPlaceholderText(QStringLiteral("01 02 03 04"));

            mainLayout->addWidget(deviceLabel, 0, 0);
            mainLayout->addWidget(deviceCombo, 0, 1);
            mainLayout->addWidget(refreshButton, 1, 0, 1, 2);
            mainLayout->addWidget(channelLabel, 2, 0);
            mainLayout->addWidget(channelSpin, 2, 1);
            mainLayout->addWidget(bitrateLabel, 3, 0);
            mainLayout->addWidget(bitrateCombo, 3, 1);
            mainLayout->addWidget(modeLabel, 4, 0);
            mainLayout->addWidget(modeCombo, 4, 1);
            mainLayout->addWidget(canIdLabel, 5, 0);
            mainLayout->addWidget(canIdEdit, 5, 1);
            mainLayout->addWidget(dataLabel, 6, 0);
            mainLayout->addWidget(dataEdit, 6, 1);
            mainLayout->addWidget(extendedCheck, 7, 0, 1, 2);
            mainLayout->addWidget(rtrCheck, 8, 0, 1, 2);
            mainLayout->addWidget(statusButton, 9, 0, 1, 2);
            mainLayout->addWidget(sendButton, 10, 0, 1, 2);

            statusButton->setFlat(true);
            statusButton->setCheckable(true);
            statusButton->setEnabled(false);
            statusButton->setText(QCoreApplication::translate("Nodes::CanBusInterface", "已断开"));
            statusButton->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
            sendButton->setEnabled(false);

            refreshDevices();
            connect(refreshButton, &QPushButton::clicked, this, &CanBusInterface::refreshDevices);
            setLayout(mainLayout);
        }

        void refreshDevices()
        {
            const QVariant current = deviceCombo->currentData();
            deviceCombo->clear();

            const auto devices = CanBus::availableDevices();
            for (const CanBusDeviceInfo &info : devices) {
                deviceCombo->addItem(info.displayName, info.index);
                deviceCombo->setItemData(deviceCombo->count() - 1, info.path, Qt::ToolTipRole);
            }

            if (deviceCombo->count() == 0) {
                deviceCombo->addItem(QCoreApplication::translate("Nodes::CanBusInterface", "（无 candleLight 设备）"), -1);
            } else if (current.isValid()) {
                const int idx = deviceCombo->findData(current);
                if (idx >= 0) {
                    deviceCombo->setCurrentIndex(idx);
                }
            }
        }

        QGridLayout *mainLayout = new QGridLayout(this);
        QComboBox *deviceCombo = new QComboBox();
        QPushButton *refreshButton = new QPushButton(QCoreApplication::translate("Nodes::CanBusInterface", "刷新设备"));
        IntDragValueWidget *channelSpin = new IntDragValueWidget();
        QComboBox *bitrateCombo = new QComboBox();
        QComboBox *modeCombo = new QComboBox();
        QLineEdit *canIdEdit = new QLineEdit();
        QLineEdit *dataEdit = new QLineEdit();
        QCheckBox *extendedCheck = new QCheckBox(QCoreApplication::translate("Nodes::CanBusInterface", "扩展帧"));
        QCheckBox *rtrCheck = new QCheckBox(QCoreApplication::translate("Nodes::CanBusInterface", "远程帧"));
        QPushButton *statusButton = new QPushButton();
        QPushButton *sendButton = new QPushButton(QCoreApplication::translate("Nodes::CanBusInterface", "发送"));

    private:
        QLabel *deviceLabel = new QLabel(QCoreApplication::translate("Nodes::CanBusInterface", "设备"));
        QLabel *channelLabel = new QLabel(QCoreApplication::translate("Nodes::CanBusInterface", "通道"));
        QLabel *bitrateLabel = new QLabel(QCoreApplication::translate("Nodes::CanBusInterface", "比特率"));
        QLabel *modeLabel = new QLabel(QCoreApplication::translate("Nodes::CanBusInterface", "模式"));
        QLabel *canIdLabel = new QLabel(QStringLiteral("CAN ID"));
        QLabel *dataLabel = new QLabel(QCoreApplication::translate("Nodes::CanBusInterface", "数据 (HEX)"));
    };
}
