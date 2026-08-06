#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief FT-C040A Modbus TCP 从机界面
 *
 * - 监听地址 / 端口 / 从站 ID
 * - 16 路线圈状态（地址 0001～0016）
 */
class FTC040AInterface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kCoilCount = 16;

    explicit FTC040AInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto connectionGroup = new QGroupBox("从机监听", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        connectionLayout->setContentsMargins(6, 8, 6, 6);
        connectionLayout->setSpacing(4);

        connectionLayout->addWidget(new QLabel("监听地址:", this), 0, 0);
        _hostEdit = new QLineEdit("0.0.0.0", this);
        connectionLayout->addWidget(_hostEdit, 0, 1);

        connectionLayout->addWidget(new QLabel("端口:", this), 1, 0);
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(50001);
        connectionLayout->addWidget(_portEdit, 1, 1);

        connectionLayout->addWidget(new QLabel("从站ID:", this), 2, 0);
        _serverId = new IntDragValueWidget(this);
        _serverId->setRange(0, 255);
        _serverId->setValue(1);
        connectionLayout->addWidget(_serverId, 2, 1);

        _statusLabel = new QPushButton("状态: 未监听", this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        connectionLayout->addWidget(_statusLabel, 3, 0, 1, 2);

        mainLayout->addWidget(connectionGroup);

        auto coilGroup = new QGroupBox("线圈 (地址 0001～0016)", this);
        auto coilLayout = new QGridLayout(coilGroup);
        coilLayout->setContentsMargins(6, 8, 6, 6);
        coilLayout->setSpacing(4);

        for (int i = 0; i < kCoilCount; ++i) {
            const int addr = i + 1;
            _coilCheckBoxes[i] = new QCheckBox(QString("%1").arg(addr, 4, 10, QChar('0')), this);
            coilLayout->addWidget(_coilCheckBoxes[i], i / 4, i % 4);
        }

        mainLayout->addWidget(coilGroup);
        mainLayout->addStretch();

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumSize(260, 320);
    }

    void setCoilState(int index, bool state)
    {
        if (index < 0 || index >= kCoilCount) {
            return;
        }
        QSignalBlocker blocker(_coilCheckBoxes[index]);
        _coilCheckBoxes[index]->setChecked(state);
    }

    bool getCoilState(int index) const
    {
        if (index < 0 || index >= kCoilCount) {
            return false;
        }
        return _coilCheckBoxes[index]->isChecked();
    }

    void setListeningStatus(bool listening)
    {
        if (listening) {
            _statusLabel->setChecked(true);
            _statusLabel->setText("状态: 监听中");
            _statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            _statusLabel->setChecked(false);
            _statusLabel->setText("状态: 未监听");
            _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }

    QCheckBox *_coilCheckBoxes[kCoilCount];
    QLineEdit *_hostEdit;
    IntDragValueWidget *_portEdit;
    IntDragValueWidget *_serverId;
    QPushButton *_statusLabel;
};
