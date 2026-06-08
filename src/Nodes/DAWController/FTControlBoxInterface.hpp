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
 * - 薄膜按钮：6 路
 * - 遥控器按钮：8 路
 */
class FTControlBoxInterface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kFilmButtonCount = 6;
    static constexpr int kRemoteButtonCount = 8;

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

        auto filmGroup = new QGroupBox("薄膜按钮 ($BB^)", this);
        auto filmLayout = new QGridLayout(filmGroup);
        filmLayout->setContentsMargins(6, 8, 6, 6);
        filmLayout->setSpacing(4);

        for (int i = 0; i < kFilmButtonCount; ++i) {
            _filmButtons[i] = new QPushButton(QString("BB%1").arg(i + 1), this);
            _filmButtons[i]->setAutoDefault(false);
            _filmButtons[i]->setStyleSheet(_filmNormalStyle);
            filmLayout->addWidget(_filmButtons[i], i / 3, i % 3);
        }

        mainLayout->addWidget(filmGroup);

        auto remoteGroup = new QGroupBox("遥控器按钮 ($YY^)", this);
        auto remoteLayout = new QGridLayout(remoteGroup);
        remoteLayout->setContentsMargins(6, 8, 6, 6);
        remoteLayout->setSpacing(4);

        for (int i = 0; i < kRemoteButtonCount; ++i) {
            _remoteButtons[i] = new QPushButton(QString("YY%1").arg(i + 1), this);
            _remoteButtons[i]->setAutoDefault(false);
            _remoteButtons[i]->setStyleSheet(_remoteNormalStyle);
            remoteLayout->addWidget(_remoteButtons[i], i / 4, i % 4);
        }

        mainLayout->addWidget(remoteGroup);
        mainLayout->addStretch();

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumSize(240, 360);
    }

    void flashFilmButton(int index)
    {
        flashButton(_filmButtons[index], _filmFlashStyle, _filmNormalStyle);
    }

    void flashRemoteButton(int index)
    {
        flashButton(_remoteButtons[index], _remoteFlashStyle, _remoteNormalStyle);
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
    QPushButton *_filmButtons[kFilmButtonCount] = {};
    QPushButton *_remoteButtons[kRemoteButtonCount] = {};

private:
    static void flashButton(QPushButton *button, const QString &flashStyle, const QString &normalStyle)
    {
        if (!button) return;
        button->setStyleSheet(flashStyle);
        QTimer::singleShot(200, button, [button, normalStyle]() {
            button->setStyleSheet(normalStyle);
        });
    }

    static constexpr const char *_filmNormalStyle = "";
    static constexpr const char *_filmFlashStyle =
        "background-color: #4CAF50; color: white; font-weight: bold;";
    static constexpr const char *_remoteNormalStyle = "";
    static constexpr const char *_remoteFlashStyle =
        "background-color: #2196F3; color: white; font-weight: bold;";
};
