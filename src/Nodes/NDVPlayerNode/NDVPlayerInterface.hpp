/**
 * @file NDVPlayerInterface.hpp
 * @brief NDV 播放器节点界面（连接状态 / host 使用 Button 以支持拖拽绑定）
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    class NDVPlayerInterface : public QWidget
    {
        Q_OBJECT
    public:
        explicit NDVPlayerInterface(QWidget *parent = nullptr)
        {
            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            const auto addRow = [this, layout](const QString &labelText, QWidget *editor) {
                auto *row = new QWidget(this);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(new QLabel(labelText, row), 0);
                rowLayout->addWidget(editor, 1);
                layout->addWidget(row);
            };

            addRow(QStringLiteral("Player ID:"), PlayerID);
            PlayerID->setRange(1, 99);
            PlayerID->setValue(1);

            addRow(QStringLiteral("File Index:"), FileIndex);
            FileIndex->setRange(0, 100);
            FileIndex->setValue(0);

            connectionButton->setFlat(true);
            connectionButton->setCheckable(true);
            connectionButton->setEnabled(false);
            connectionButton->setText(QStringLiteral("连接: 离线"));
            connectionButton->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            hostButton->setFlat(true);
            hostButton->setCheckable(false);
            hostButton->setEnabled(false);
            hostButton->setText(QStringLiteral("Host: —"));
            hostButton->setStyleSheet(QStringLiteral("color: gray;"));

            layout->addWidget(connectionButton);
            layout->addWidget(hostButton);

            layout->addWidget(Play);
            layout->addWidget(LoopPlay);
            layout->addWidget(Stop);

            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }

        void updateConnectionDisplay(bool online, const QString &ip)
        {
            connectionButton->setChecked(online);
            connectionButton->setText(online
                                          ? QStringLiteral("连接: 在线")
                                          : QStringLiteral("连接: 离线"));
            connectionButton->setStyleSheet(online
                                                ? QStringLiteral("color: green; font-weight: bold;")
                                                : QStringLiteral("color: red; font-weight: bold;"));

            const QString ipText = ip.isEmpty() ? QStringLiteral("—") : ip;
            hostButton->setText(QStringLiteral("Host: %1").arg(ipText));
            hostButton->setStyleSheet(online
                                          ? QStringLiteral("color: green;")
                                          : QStringLiteral("color: gray;"));
        }

        IntDragValueWidget *PlayerID = new IntDragValueWidget(this);
        IntDragValueWidget *FileIndex = new IntDragValueWidget(this);
        QPushButton *connectionButton = new QPushButton(this);
        QPushButton *hostButton = new QPushButton(this);
        QPushButton *Play = new QPushButton(QStringLiteral("Play"), this);
        QPushButton *LoopPlay = new QPushButton(QStringLiteral("Loop Play"), this);
        QPushButton *Stop = new QPushButton(QStringLiteral("Stop"), this);
    };
}
