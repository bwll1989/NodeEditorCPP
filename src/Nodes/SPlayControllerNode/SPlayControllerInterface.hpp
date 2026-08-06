/**
 * @file SPlayControllerInterface.hpp
 * @brief S-Play 控制器界面（非嵌入：WidgetEmbeddable=false）
 *
 * 主机 → 全播/全停 → 连接与状态 → 播放列表（每行勾选 = 期望播/停）。
 * 节点 In/Out 端口数量由 PortEditable 手动编辑，与列表无关。
 * HTTP 使用默认 80，无需单独配置端口。
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>

namespace Nodes
{
    class SPlayControllerInterface : public QWidget
    {
    public:
        explicit SPlayControllerInterface(QWidget *parent = nullptr)
        {
            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(6);

            const auto addRow = [layout](const QString &labelText, QWidget *editor) {
                auto *row = new QWidget();
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(new QLabel(labelText));
                rowLayout->addWidget(editor, 1);
                layout->addWidget(row);
            };

            hostEdit->setText(QStringLiteral("192.168.1.13"));
            hostEdit->setPlaceholderText(QStringLiteral("IP 或 IP:端口，如 127.0.0.1:8080"));

            playAllButton->setText(QStringLiteral("全部播放"));
            playAllButton->setToolTip(QStringLiteral("PLAY_ALL_PLAYLISTS（command 5）"));
            stopAllButton->setText(QStringLiteral("全停"));
            stopAllButton->setToolTip(QStringLiteral("STOP_ALL_PLAYLISTS（command 7）"));

            connectionLabel->setFlat(true);
            connectionLabel->setCheckable(true);
            connectionLabel->setEnabled(false);
            connectionLabel->setText(QStringLiteral("连接: 未连接"));
            connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            statusLabel->setWordWrap(true);
            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            statusLabel->setText(QStringLiteral("状态: —"));

            addRow(QStringLiteral("主机:"), hostEdit);

            auto *actionRow = new QWidget();
            auto *actionLayout = new QHBoxLayout(actionRow);
            actionLayout->setContentsMargins(0, 0, 0, 0);
            actionLayout->setSpacing(8);
            actionLayout->addWidget(playAllButton, 1);
            actionLayout->addWidget(stopAllButton, 1);
            layout->addWidget(actionRow);

            layout->addWidget(connectionLabel);
            layout->addWidget(statusLabel);

            layout->addWidget(new QLabel(QStringLiteral("播放列表 (勾选跟设备状态，也可点选控制):")));
            playlistWidget->setMinimumHeight(140);
            playlistWidget->setSelectionMode(QAbstractItemView::NoSelection);
            layout->addWidget(playlistWidget, 1);

            setMinimumSize(320, 420);
        }

        void updateConnectionStatus(bool connected)
        {
            connectionLabel->setChecked(connected);
            connectionLabel->setText(connected
                                         ? QStringLiteral("连接: 已连接")
                                         : QStringLiteral("连接: 未连接"));
            connectionLabel->setStyleSheet(connected
                                               ? QStringLiteral("color: green; font-weight: bold;")
                                               : QStringLiteral("color: red; font-weight: bold;"));
        }

        QLineEdit *hostEdit = new QLineEdit();
        QPushButton *playAllButton = new QPushButton();
        QPushButton *stopAllButton = new QPushButton();
        QPushButton *connectionLabel = new QPushButton();
        QLabel *statusLabel = new QLabel();
        QListWidget *playlistWidget = new QListWidget();
    };
}
