#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QCheckBox>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{
    class RangeMapInterface : public QWidget
    {
    public:
        explicit RangeMapInterface(QWidget *parent = nullptr)
        {
            main_layout->setContentsMargins(0, 0, 0, 0);
            setLayout(main_layout);

            int row = 0;
            main_layout->addWidget(new QLabel(QStringLiteral("输入最小:")), row, 0);
            inMin->setDecimals(4);
            inMin->setValue(0.0);
            main_layout->addWidget(inMin, row++, 1, 1, 3);

            main_layout->addWidget(new QLabel(QStringLiteral("输入最大:")), row, 0);
            inMax->setDecimals(4);
            inMax->setValue(1.0);
            main_layout->addWidget(inMax, row++, 1, 1, 3);

            main_layout->addWidget(new QLabel(QStringLiteral("输出最小:")), row, 0);
            outMin->setDecimals(4);
            outMin->setValue(0.0);
            main_layout->addWidget(outMin, row++, 1, 1, 3);

            main_layout->addWidget(new QLabel(QStringLiteral("输出最大:")), row, 0);
            outMax->setDecimals(4);
            outMax->setValue(255.0);
            main_layout->addWidget(outMax, row++, 1, 1, 3);

            main_layout->addWidget(new QLabel(QStringLiteral("输入值:")), row, 0);
            expression->setPlaceholderText(QStringLiteral("留空使用 default 键"));
            main_layout->addWidget(expression, row++, 1, 1, 3);

            clampCheck->setChecked(true);
            main_layout->addWidget(clampCheck, row++, 0, 1, 4);

            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), row, 0);
            main_layout->setRowStretch(row, 1);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

    public:
        QGridLayout *main_layout = new QGridLayout(this);
        FloatDragValueWidget *inMin = new FloatDragValueWidget(this);
        FloatDragValueWidget *inMax = new FloatDragValueWidget(this);
        FloatDragValueWidget *outMin = new FloatDragValueWidget(this);
        FloatDragValueWidget *outMax = new FloatDragValueWidget(this);
        QLineEdit *expression = new QLineEdit();
        QCheckBox *clampCheck = new QCheckBox(QStringLiteral("限制在输出范围内"));
    };
}
