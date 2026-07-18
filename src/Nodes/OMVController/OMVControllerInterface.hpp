#pragma once

#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTextEdit>

namespace Nodes
{
    /**
     * @brief OMV 安卓播放器控制节点界面
     */
    class OMVControllerInterface : public QGroupBox
    {
    public:
        explicit OMVControllerInterface(QWidget *parent = nullptr)
            : QGroupBox(parent)
        {
            auto *layout = new QGridLayout(this);

            layout->addWidget(new QLabel(QStringLiteral("设备 ID:")), 0, 0);
            layout->addWidget(deviceIdSpin, 0, 1);
            deviceIdSpin->setRange(0, 999);
            deviceIdSpin->setValue(1);

            listeningLabel->setFlat(true);
            listeningLabel->setCheckable(true);
            listeningLabel->setEnabled(false);
            listeningLabel->setText(QStringLiteral("状态: 未监听"));
            listeningLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
            layout->addWidget(listeningLabel, 1, 0, 1, 2);

            playingLabel->setFlat(true);
            playingLabel->setCheckable(true);
            playingLabel->setEnabled(false);
            playingLabel->setText(QStringLiteral("播放: 停止"));
            playingLabel->setStyleSheet(QStringLiteral("color: gray; font-weight: bold;"));
            layout->addWidget(playingLabel, 2, 0, 1, 2);

            jsonEdit->setAcceptRichText(false);
            jsonEdit->setPlaceholderText(QStringLiteral("{\"tp\":\"1\",\"com\":\"10\",\"pm\":\"10\"}"));
            jsonEdit->setPlainText(QStringLiteral("{\"tp\":\"1\",\"com\":\"10\",\"pm\":\"10\"}"));
            jsonEdit->setMinimumHeight(64);
            layout->addWidget(jsonEdit, 3, 0, 1, 2);

            layout->addWidget(sendButton, 4, 0, 1, 2);
            sendButton->setText(QStringLiteral("Send"));

            auto *playStopRow = new QHBoxLayout();
            playButton->setText(QStringLiteral("Play"));
            stopButton->setText(QStringLiteral("Stop"));
            playStopRow->addWidget(playButton);
            playStopRow->addWidget(stopButton);
            layout->addLayout(playStopRow, 5, 0, 1, 2);

            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 6, 0, 1, 2);
            layout->setRowStretch(6, 1);
            setMinimumSize(220, 240);
        }

        void updateListeningStatus(bool listening)
        {
            listeningLabel->setChecked(listening);
            listeningLabel->setText(listening
                                        ? QStringLiteral("状态: 监听中")
                                        : QStringLiteral("状态: 未监听"));
            listeningLabel->setStyleSheet(listening
                                              ? QStringLiteral("color: green; font-weight: bold;")
                                              : QStringLiteral("color: red; font-weight: bold;"));
        }

        void updatePlayingStatus(bool playing)
        {
            playingLabel->setChecked(playing);
            playingLabel->setText(playing
                                      ? QStringLiteral("播放: 播放中")
                                      : QStringLiteral("播放: 停止"));
            playingLabel->setStyleSheet(playing
                                            ? QStringLiteral("color: green; font-weight: bold;")
                                            : QStringLiteral("color: gray; font-weight: bold;"));
        }

        QSpinBox *deviceIdSpin = new QSpinBox(this);
        QPushButton *listeningLabel = new QPushButton(this);
        QPushButton *playingLabel = new QPushButton(this);
        QTextEdit *jsonEdit = new QTextEdit(this);
        QPushButton *sendButton = new QPushButton(this);
        QPushButton *playButton = new QPushButton(this);
        QPushButton *stopButton = new QPushButton(this);
    };
}
