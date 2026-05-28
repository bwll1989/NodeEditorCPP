#pragma once

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QLineEdit"
#include <QSerialPort>
#include "QSerialPortInfo"
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    class SerialPortInterface : public QWidget
    {
        Q_OBJECT

    public:
        explicit SerialPortInterface(QWidget *parent = nullptr)
        {
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(6);
            mainLayout->setColumnStretch(0, 1);
            mainLayout->setColumnStretch(1, 2);

            baudRateSpinBox->setRange(1200, 921600);
            baudRateSpinBox->setValue(9600);

            dataBitsCombo->addItem("5", QSerialPort::Data5);
            dataBitsCombo->addItem("6", QSerialPort::Data6);
            dataBitsCombo->addItem("7", QSerialPort::Data7);
            dataBitsCombo->addItem("8", QSerialPort::Data8);
            dataBitsCombo->setCurrentIndex(3);

            parityCombo->addItem("None", QSerialPort::NoParity);
            parityCombo->addItem("Even", QSerialPort::EvenParity);
            parityCombo->addItem("Odd", QSerialPort::OddParity);
            parityCombo->addItem("Space", QSerialPort::SpaceParity);
            parityCombo->addItem("Mark", QSerialPort::MarkParity);

            stopBitsCombo->addItem("1", QSerialPort::OneStop);
            stopBitsCombo->addItem("1.5", QSerialPort::OneAndHalfStop);
            stopBitsCombo->addItem("2", QSerialPort::TwoStop);

            formatCombo->addItem("HEX");
            formatCombo->addItem("UTF-8");
            formatCombo->addItem("ASCII");

            mainLayout->addWidget(portLabel, 0, 0);
            mainLayout->addWidget(portCombo, 0, 1);
            mainLayout->addWidget(refreshButton, 1, 0, 1, 2);
            mainLayout->addWidget(baudRateLabel, 2, 0);
            mainLayout->addWidget(baudRateSpinBox, 2, 1);
            mainLayout->addWidget(dataBitsLabel, 3, 0);
            mainLayout->addWidget(dataBitsCombo, 3, 1);
            mainLayout->addWidget(parityLabel, 4, 0);
            mainLayout->addWidget(parityCombo, 4, 1);
            mainLayout->addWidget(stopBitsLabel, 5, 0);
            mainLayout->addWidget(stopBitsCombo, 5, 1);
            mainLayout->addWidget(valueLabel, 6, 0);
            mainLayout->addWidget(valueEdit, 6, 1);
            mainLayout->addWidget(formatCombo, 7, 0, 1, 2);
            mainLayout->addWidget(statusButton, 8, 0, 1, 2);
            mainLayout->addWidget(sendButton, 9, 0, 1, 2);

            statusButton->setFlat(true);
            statusButton->setCheckable(true);
            statusButton->setEnabled(false);
            statusButton->setText("Disconnect");
            statusButton->setStyleSheet("color: red; font-weight: bold;");

            refreshPorts();
            connect(refreshButton, &QPushButton::clicked, this, &SerialPortInterface::refreshPorts);
            setLayout(mainLayout);
        }

        void refreshPorts()
        {
            const QString current = portCombo->currentText();
            portCombo->clear();
            const auto ports = QSerialPortInfo::availablePorts();
            for (const auto &info : ports) {
                portCombo->addItem(info.portName());
            }
            if (!current.isEmpty()) {
                const int index = portCombo->findText(current);
                if (index >= 0) {
                    portCombo->setCurrentIndex(index);
                }
            }
            if (portCombo->count() == 0) {
                portCombo->addItem("COM1");
            }
        }

        QGridLayout *mainLayout = new QGridLayout(this);
        QComboBox *portCombo = new QComboBox();
        QPushButton *refreshButton = new QPushButton("Refresh Ports");
        IntDragValueWidget *baudRateSpinBox = new IntDragValueWidget();
        QComboBox *dataBitsCombo = new QComboBox();
        QComboBox *parityCombo = new QComboBox();
        QComboBox *stopBitsCombo = new QComboBox();
        QLineEdit *valueEdit = new QLineEdit();
        QComboBox *formatCombo = new QComboBox();
        QPushButton *statusButton = new QPushButton();
        QPushButton *sendButton = new QPushButton("Send");

    private:
        QLabel *portLabel = new QLabel("Port");
        QLabel *baudRateLabel = new QLabel("Baud Rate");
        QLabel *dataBitsLabel = new QLabel("Data Bits");
        QLabel *parityLabel = new QLabel("Parity");
        QLabel *stopBitsLabel = new QLabel("Stop Bits");
        QLabel *valueLabel = new QLabel("Value");
    };
}
