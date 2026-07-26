/**
 * @file MpvControllerInterface.hpp
 * @brief MPV Controller 嵌入式界面（布局对齐 SlideShow / VLC Remote）
 *
 * Base URL → Index/播放/停止 → 连接与状态 → 媒体列表。
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
    class MpvControllerInterface : public QWidget
    {
    public:
        explicit MpvControllerInterface(QWidget *parent = nullptr)
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

            baseUrlEdit->setPlaceholderText(QStringLiteral("http://127.0.0.1:8995"));
            baseUrlEdit->setText(QStringLiteral("http://127.0.0.1:8995"));

            indexSpinBox->setMinimum(0);
            indexSpinBox->setMaximum(9999);
            indexSpinBox->setValue(0);
            indexSpinBox->setToolTip(QStringLiteral("媒体库排序号（从 0 起）"));

            playButton->setText(QStringLiteral("播放"));
            playButton->setToolTip(QStringLiteral("按当前 Index 播放媒体"));
            stopButton->setText(QStringLiteral("停止"));
            stopButton->setToolTip(QStringLiteral("停止当前播放（不影响 Index）"));

            connectionLabel->setFlat(true);
            connectionLabel->setCheckable(true);
            connectionLabel->setEnabled(false);
            connectionLabel->setText(QStringLiteral("连接: 未连接"));
            connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            statusLabel->setWordWrap(true);
            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            statusLabel->setText(QStringLiteral("状态: —"));

            addRow(QStringLiteral("Base URL:"), baseUrlEdit);
            addRow(QStringLiteral("Index:"), indexSpinBox);

            auto *actionRow = new QWidget();
            auto *actionLayout = new QHBoxLayout(actionRow);
            actionLayout->setContentsMargins(0, 0, 0, 0);
            actionLayout->setSpacing(8);
            actionLayout->addWidget(playButton, 1);
            actionLayout->addWidget(stopButton, 1);
            layout->addWidget(actionRow);

            layout->addWidget(connectionLabel);
            layout->addWidget(statusLabel);

            layout->addWidget(new QLabel(QStringLiteral("媒体列表:")));
            mediaListWidget->setMinimumHeight(120);
            layout->addWidget(mediaListWidget, 1);

            setMinimumSize(320, 400);
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

        QLineEdit *baseUrlEdit = new QLineEdit();
        QSpinBox *indexSpinBox = new QSpinBox();
        QPushButton *playButton = new QPushButton();
        QPushButton *stopButton = new QPushButton();
        QPushButton *connectionLabel = new QPushButton();
        QLabel *statusLabel = new QLabel();
        QListWidget *mediaListWidget = new QListWidget();
    };
}
