/**
 * @file QSysQrcInterface.hpp
 * @brief Q-SYS ECP 节点界面：媒体库 XML + 连接状态指示 + Named Control 列表
 */
#pragma once

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWidget>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"

namespace Nodes
{
    class QSysQrcInterface : public QWidget
    {
    public:
        explicit QSysQrcInterface(QWidget *parent = nullptr)
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

            hostEdit->setText(QStringLiteral("192.168.1.10"));
            hostEdit->setPlaceholderText(QStringLiteral("Q-SYS Core IP"));

            userEdit->setPlaceholderText(QStringLiteral("可选 login NAME"));
            pinEdit->setEchoMode(QLineEdit::Password);
            pinEdit->setPlaceholderText(QStringLiteral("可选 login PIN"));

            xmlSelector->setPlaceholderText(
                QStringLiteral("媒体库中的 Named Controls XML"));
            xmlSelector->setAllowCustomInput(true);

            // 仅状态显示 + OSC drag，不可手动点按连接
            connectButton->setCheckable(true);
            connectButton->setFlat(true);
            connectButton->setEnabled(false);
            connectButton->setText(QStringLiteral("未连接"));
            connectButton->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            designLabel->setWordWrap(true);
            designLabel->setStyleSheet(QStringLiteral("color: gray;"));
            designLabel->setText(QStringLiteral("Design: —"));

            statusLabel->setWordWrap(true);
            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            statusLabel->setText(QStringLiteral("状态: —"));

            controlList->setSelectionMode(QAbstractItemView::SingleSelection);
            controlList->setMinimumHeight(200);
            controlList->setAlternatingRowColors(false);
            controlList->setStyleSheet(QStringLiteral(
                "QListWidget { outline: none; }"
                "QListWidget::item { padding: 4px 6px; }"
                "QListWidget::item:hover {"
                "  background-color: rgba(74, 163, 255, 0.28);"
                "}"
                "QListWidget::item:selected {"
                "  background-color: rgba(74, 163, 255, 0.50);"
                "  color: palette(text);"
                "}"
                "QListWidget::item:selected:!active {"
                "  background-color: rgba(74, 163, 255, 0.40);"
                "  color: palette(text);"
                "}"));

            addRow(QStringLiteral("主机:"), hostEdit);
            addRow(QStringLiteral("用户:"), userEdit);
            addRow(QStringLiteral("PIN:"), pinEdit);
            addRow(QStringLiteral("Named Controls XML:"), xmlSelector);

            layout->addWidget(connectButton);
            layout->addWidget(designLabel);
            layout->addWidget(statusLabel);
            layout->addWidget(new QLabel(
                QStringLiteral("Named Controls（端口数随 XML 自动同步）:")));
            layout->addWidget(controlList, 1);

            setMinimumSize(360, 480);
        }

        void updateConnectionStatus(bool connected)
        {
            QSignalBlocker blocker(connectButton);
            connectButton->setChecked(connected);
            connectButton->setText(connected
                                       ? QStringLiteral("已连接")
                                       : QStringLiteral("未连接"));
            connectButton->setStyleSheet(connected
                                             ? QStringLiteral("color: green; font-weight: bold;")
                                             : QStringLiteral("color: red; font-weight: bold;"));
        }

        QLineEdit *hostEdit = new QLineEdit();
        QLineEdit *userEdit = new QLineEdit();
        QLineEdit *pinEdit = new QLineEdit();
        SelectorComboBox *xmlSelector =
            new SelectorComboBox(MediaLibrary::Category::Document, this);
        QPushButton *connectButton = new QPushButton();
        QLabel *designLabel = new QLabel();
        QLabel *statusLabel = new QLabel();
        QListWidget *controlList = new QListWidget();
    };
}
