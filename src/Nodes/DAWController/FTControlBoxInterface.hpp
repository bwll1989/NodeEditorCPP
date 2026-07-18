#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QTimer>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief FT-ControlBox 控制盒界面
 *
 * 设备三组 IO（面板 $BB^ / 遥控 $YY^ / IO 线 $KK^）统一映射为 8 路输出，
 * 界面提供 8 个联调按钮。
 */
class FTControlBoxInterface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kIoCount = 8;

    FTControlBoxInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto connectionGroup = new QGroupBox("连接设置", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        connectionLayout->setContentsMargins(6, 8, 6, 6);
        connectionLayout->setSpacing(4);

        connectionLayout->addWidget(new QLabel("主机:", this), 0, 0);
        _hostEdit = new QLineEdit("127.0.0.1", this);
        connectionLayout->addWidget(_hostEdit, 0, 1);

        connectionLayout->addWidget(new QLabel("端口:", this), 1, 0);
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(2001);
        connectionLayout->addWidget(_portEdit, 1, 1);

        connectionLayout->addWidget(new QLabel("485地址:", this), 2, 0);
        _addr485Edit = new IntDragValueWidget(this);
        _addr485Edit->setRange(0, 255);
        _addr485Edit->setValue(1);
        connectionLayout->addWidget(_addr485Edit, 2, 1);

        _statusLabel = new QPushButton("状态: 未连接", this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        connectionLayout->addWidget(_statusLabel, 3, 0, 1, 2);

        mainLayout->addWidget(connectionGroup);

        auto ioGroup = new QGroupBox("IO 输出 ($BB^/$YY^/$KK^)", this);
        auto ioLayout = new QGridLayout(ioGroup);
        ioLayout->setContentsMargins(6, 8, 6, 6);
        ioLayout->setSpacing(4);

        for (int i = 0; i < kIoCount; ++i) {
            _ioButtons[i] = new QPushButton(QString("IO%1").arg(i + 1), this);
            _ioButtons[i]->setAutoDefault(false);
            _ioButtons[i]->setStyleSheet(_ioNormalStyle);
            ioLayout->addWidget(_ioButtons[i], i / 4, i % 4);
        }

        mainLayout->addWidget(ioGroup);
        mainLayout->addStretch();

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumSize(240, 220);
    }

    void flashIoButton(int index)
    {
        if (index < 0 || index >= kIoCount) {
            return;
        }
        flashButton(_ioButtons[index], _ioFlashStyle, _ioNormalStyle);
    }

    void setConnectionStatus(bool connected)
    {
        _statusLabel->setChecked(connected);
        _statusLabel->setText(connected ? "状态: 已连接" : "状态: 未连接");
        _statusLabel->setStyleSheet(
            connected ? "color: green; font-weight: bold;" : "color: red; font-weight: bold;");
    }

    QLineEdit *_hostEdit = nullptr;
    IntDragValueWidget *_portEdit = nullptr;
    IntDragValueWidget *_addr485Edit = nullptr;
    QPushButton *_statusLabel = nullptr;
    QPushButton *_ioButtons[kIoCount] = {};

private:
    static void flashButton(QPushButton *button, const QString &flashStyle, const QString &normalStyle)
    {
        if (!button) return;
        button->setStyleSheet(flashStyle);
        QTimer::singleShot(200, button, [button, normalStyle]() {
            button->setStyleSheet(normalStyle);
        });
    }

    static constexpr const char *_ioNormalStyle = "";
    static constexpr const char *_ioFlashStyle =
        "background-color: #4CAF50; color: white; font-weight: bold;";
};
