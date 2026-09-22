#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSignalBlocker>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief FT-AJT 六路调光控制器界面
 */
class FTAJTInterface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kChannelCount = 6;

    explicit FTAJTInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto connectionGroup = new QGroupBox("连接设置", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        connectionLayout->setContentsMargins(6, 8, 6, 6);
        connectionLayout->setSpacing(4);

        connectionLayout->addWidget(new QLabel("设备 IP:", this), 0, 0);
        _hostEdit = new QLineEdit("127.0.0.1", this);
        connectionLayout->addWidget(_hostEdit, 0, 1);

        connectionLayout->addWidget(new QLabel("端口:", this), 1, 0);
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(1001);
        connectionLayout->addWidget(_portEdit, 1, 1);

        connectionLayout->addWidget(new QLabel("源地址:", this), 2, 0);
        _srcAddrEdit = new IntDragValueWidget(this);
        _srcAddrEdit->setRange(0, 255);
        _srcAddrEdit->setValue(0x00);
        connectionLayout->addWidget(_srcAddrEdit, 2, 1);

        connectionLayout->addWidget(new QLabel("设备地址:", this), 3, 0);
        _dstAddrEdit = new IntDragValueWidget(this);
        _dstAddrEdit->setRange(0, 255);
        _dstAddrEdit->setValue(0x35);
        connectionLayout->addWidget(_dstAddrEdit, 3, 1);

        _statusLabel = new QPushButton("状态: 未连接", this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        connectionLayout->addWidget(_statusLabel, 4, 0, 1, 2);

        mainLayout->addWidget(connectionGroup);

        auto channelGroup = new QGroupBox("六路调光 (0～255)", this);
        auto channelLayout = new QGridLayout(channelGroup);
        channelLayout->setContentsMargins(6, 8, 6, 6);
        channelLayout->setSpacing(4);

        for (int i = 0; i < kChannelCount; ++i) {
            channelLayout->addWidget(new QLabel(QString("CH%1:").arg(i + 1), this), i, 0);
            _channelEdits[i] = new IntDragValueWidget(this);
            _channelEdits[i]->setRange(0, 255);
            _channelEdits[i]->setValue(0);
            channelLayout->addWidget(_channelEdits[i], i, 1);
        }

        mainLayout->addWidget(channelGroup);

        _enableButton = new QPushButton("全开/全关", this);
        _enableButton->setCheckable(true);
        _enableButton->setChecked(false);
        mainLayout->addWidget(_enableButton);

        mainLayout->addStretch();
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumSize(240, 340);
    }

    void setChannelLevel(int index, int level)
    {
        if (index < 0 || index >= kChannelCount) {
            return;
        }
        QSignalBlocker blocker(_channelEdits[index]);
        _channelEdits[index]->setValue(level);
    }

    void setEnableChecked(bool enabled)
    {
        QSignalBlocker blocker(_enableButton);
        _enableButton->setChecked(enabled);
    }

    void setConnectionStatus(bool connected)
    {
        if (connected) {
            _statusLabel->setChecked(true);
            _statusLabel->setText("状态: 已连接");
            _statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            _statusLabel->setChecked(false);
            _statusLabel->setText("状态: 未连接");
            _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }

    QLineEdit *_hostEdit = nullptr;
    IntDragValueWidget *_portEdit = nullptr;
    IntDragValueWidget *_srcAddrEdit = nullptr;
    IntDragValueWidget *_dstAddrEdit = nullptr;
    IntDragValueWidget *_channelEdits[kChannelCount]{};
    QPushButton *_statusLabel = nullptr;
    QPushButton *_enableButton = nullptr;
};
