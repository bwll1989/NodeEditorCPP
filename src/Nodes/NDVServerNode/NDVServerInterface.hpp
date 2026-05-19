//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QLineEdit>
#include "QWidget"
#include "QLabel"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QPushButton>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief NDV控制节点界面 - 简化版，只包含网络配置
     */
    class NDVServerInterface: public QWidget{
        Q_OBJECT
    public:
        explicit NDVServerInterface(QWidget *parent = nullptr){
            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            const auto addRow = [this, layout](const QString& labelText, QWidget* editor) {
                auto *row = new QWidget(this);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);

                auto *label = new QLabel(labelText, row);
                rowLayout->addWidget(label, 0);
                rowLayout->addWidget(editor, 1);

                layout->addWidget(row);
            };

            addRow("IP:", IP);
            addRow("Port:", Port);

            Port->setRange(1000, 65535);
            Port->setValue(9008);
            
            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }

    signals:
        void AddressChanged(const int &port);

    public:
        QLineEdit *IP = new QLineEdit("0.0.0.0", this);
        IntDragValueWidget *Port = new IntDragValueWidget(this);

    };
}
