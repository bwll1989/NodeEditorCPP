/**
 * @file SlideShowInterface.hpp
 * @brief SlideShow 节点嵌入式界面
 *
 * 布局：连接配置 → Index/播放 → 连接与状态 → 播放列表。
 * 用户名、密码仅本地配置，不参与属性系统对外发布。
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace Nodes
{
    class SlideShowInterface : public QWidget
    {
    public:
        explicit SlideShowInterface(QWidget *parent = nullptr)
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

            baseUrlEdit->setPlaceholderText(QStringLiteral("http://192.168.204.37:8081"));
            baseUrlEdit->setText(QStringLiteral("http://192.168.204.37:8081"));
            usernameEdit->setText(QStringLiteral("admin"));
            passwordEdit->setText(QStringLiteral("admin"));
            passwordEdit->setEchoMode(QLineEdit::Password);
            zoneNameEdit->setPlaceholderText(QStringLiteral("留空=主分区"));

            indexSpinBox->setMinimum(0);
            indexSpinBox->setMaximum(9999);
            indexSpinBox->setValue(0);
            indexSpinBox->setToolTip(QStringLiteral("播放列表排序号（从 0 起）"));

            playButton->setText(QStringLiteral("播放"));
            playButton->setToolTip(QStringLiteral("按当前 Index 切换播放列表"));

            // 连接指示：只读展示
            connectionLabel->setFlat(true);
            connectionLabel->setCheckable(true);
            connectionLabel->setEnabled(false);
            connectionLabel->setText(QStringLiteral("连接: 未连接"));
            connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            statusLabel->setWordWrap(true);
            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            statusLabel->setText(QStringLiteral("状态: —"));

            addRow(QStringLiteral("Base URL:"), baseUrlEdit);
            addRow(QStringLiteral("Username:"), usernameEdit);
            addRow(QStringLiteral("Password:"), passwordEdit);
            addRow(QStringLiteral("Zone:"), zoneNameEdit);
            addRow(QStringLiteral("Index:"), indexSpinBox);
            layout->addWidget(playButton);
            layout->addWidget(connectionLabel);
            layout->addWidget(statusLabel);

            layout->addWidget(new QLabel(QStringLiteral("播放列表:")));
            playlistWidget->setMinimumHeight(120);
            layout->addWidget(playlistWidget, 1);

            setMinimumSize(300, 400);
        }

        /** 更新连接指示颜色与文案 */
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

        QLineEdit *baseUrlEdit = new QLineEdit();
        QLineEdit *usernameEdit = new QLineEdit();
        QLineEdit *passwordEdit = new QLineEdit();
        QLineEdit *zoneNameEdit = new QLineEdit();
        QSpinBox *indexSpinBox = new QSpinBox();
        QPushButton *playButton = new QPushButton();
        QPushButton *connectionLabel = new QPushButton();
        QLabel *statusLabel = new QLabel();
        QListWidget *playlistWidget = new QListWidget();
    };
}
