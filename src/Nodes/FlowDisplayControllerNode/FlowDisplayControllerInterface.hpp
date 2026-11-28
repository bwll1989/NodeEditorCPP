/**
 * @file FlowDisplayControllerInterface.hpp
 * @brief FlowDisplay Controller 嵌入式界面（布局对齐 MpvController / VLC Remote）
 *
 * Base URL → Index/播放/停止/暂停 → 连接与状态 → 时间轴列表。
 */
#pragma once

#include <QWidget>
#include <QCoreApplication>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace Nodes
{
    class FlowDisplayControllerInterface : public QWidget
    {
    public:
        explicit FlowDisplayControllerInterface(QWidget *parent = nullptr)
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
            indexSpinBox->setToolTip(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "时间轴排序号（从 0 起）"));

            playButton->setText(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "播放"));
            playButton->setToolTip(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "按当前 Index 播放时间轴"));
            stopButton->setText(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "停止"));
            stopButton->setToolTip(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "停止播放并回待机（播放头归零）"));

            loopCheckBox->setText(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "循环"));
            loopCheckBox->setChecked(false);
            loopCheckBox->setToolTip(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "勾选 = 当前 Index 指向的时间轴循环播放；取消 = 单次播放（写回时间轴文档 loop 字段）"));

            connectionLabel->setFlat(true);
            connectionLabel->setCheckable(true);
            connectionLabel->setEnabled(false);
            connectionLabel->setText(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "连接: 未连接"));
            connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            statusLabel->setWordWrap(true);
            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            statusLabel->setText(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "状态: —"));

            addRow(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "基础 URL："), baseUrlEdit);
            addRow(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "序号："), indexSpinBox);

            auto *actionRow = new QWidget();
            auto *actionLayout = new QHBoxLayout(actionRow);
            actionLayout->setContentsMargins(0, 0, 0, 0);
            actionLayout->setSpacing(8);
            actionLayout->addWidget(playButton, 1);
            actionLayout->addWidget(stopButton, 1);
            actionLayout->addWidget(loopCheckBox);
            layout->addWidget(actionRow);

            layout->addWidget(connectionLabel);
            layout->addWidget(statusLabel);

            layout->addWidget(new QLabel(QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "时间轴列表:")));
            timelineListWidget->setMinimumHeight(120);
            layout->addWidget(timelineListWidget, 1);

            setMinimumSize(320, 400);
        }

        void updateConnectionStatus(bool connected)
        {
            connectionLabel->setChecked(connected);
            connectionLabel->setText(connected
                                         ? QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "连接: 已连接")
                                         : QCoreApplication::translate("Nodes::FlowDisplayControllerInterface", "连接: 未连接"));
            connectionLabel->setStyleSheet(connected
                                               ? QStringLiteral("color: green; font-weight: bold;")
                                               : QStringLiteral("color: red; font-weight: bold;"));
        }

        QLineEdit *baseUrlEdit = new QLineEdit();
        QSpinBox *indexSpinBox = new QSpinBox();
        QPushButton *playButton = new QPushButton();
        QPushButton *stopButton = new QPushButton();
        QCheckBox *loopCheckBox = new QCheckBox();
        QPushButton *connectionLabel = new QPushButton();
        QLabel *statusLabel = new QLabel();
        QListWidget *timelineListWidget = new QListWidget();
    };
}
