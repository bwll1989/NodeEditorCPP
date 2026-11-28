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
#include <QCoreApplication>

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

            addRow(QCoreApplication::translate("Nodes::NDVPlayerInterface", "播放器 ID："), PlayerID);
            PlayerID->setRange(1, 99);
            PlayerID->setValue(1);

            addRow(QCoreApplication::translate("Nodes::NDVPlayerInterface", "文件序号："), FileIndex);
            FileIndex->setRange(0, 100);
            FileIndex->setValue(0);

            connectionButton->setFlat(true);
            connectionButton->setCheckable(true);
            connectionButton->setEnabled(false);
            connectionButton->setText(QCoreApplication::translate("Nodes::NDVPlayerInterface", "连接: 离线"));
            connectionButton->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            hostButton->setFlat(true);
            hostButton->setCheckable(false);
            hostButton->setEnabled(false);
            hostButton->setText(QCoreApplication::translate("Nodes::NDVPlayerInterface", "主机: —"));
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
                                          ? QCoreApplication::translate("Nodes::NDVPlayerInterface", "连接: 在线")
                                          : QCoreApplication::translate("Nodes::NDVPlayerInterface", "连接: 离线"));
            connectionButton->setStyleSheet(online
                                                ? QStringLiteral("color: green; font-weight: bold;")
                                                : QStringLiteral("color: red; font-weight: bold;"));

            const QString ipText = ip.isEmpty() ? QStringLiteral("—") : ip;
            hostButton->setText(QCoreApplication::translate("Nodes::NDVPlayerInterface", "主机: %1").arg(ipText));
            hostButton->setStyleSheet(online
                                          ? QStringLiteral("color: green;")
                                          : QStringLiteral("color: gray;"));
        }

        IntDragValueWidget *PlayerID = new IntDragValueWidget(this);
        IntDragValueWidget *FileIndex = new IntDragValueWidget(this);
        QPushButton *connectionButton = new QPushButton(this);
        QPushButton *hostButton = new QPushButton(this);
        QPushButton *Play = new QPushButton(QCoreApplication::translate("Nodes::NDVPlayerInterface", "播放"), this);
        QPushButton *LoopPlay = new QPushButton(QCoreApplication::translate("Nodes::NDVPlayerInterface", "循环播放"), this);
        QPushButton *Stop = new QPushButton(QCoreApplication::translate("Nodes::NDVPlayerInterface", "停止"), this);
    };
}
