#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCoreApplication>

namespace Nodes
{
    class EdgeTriggerInterface : public QWidget
    {
    public:
        explicit EdgeTriggerInterface(QWidget *parent = nullptr)
        {
            main_layout->setContentsMargins(0, 0, 0, 0);
            setLayout(main_layout);

            expression->setPlaceholderText(QCoreApplication::translate("Nodes::EdgeTriggerInterface", "JS 表达式（如 \"$input['key']\"）"));
            edgeMode->addItem(QCoreApplication::translate("Nodes::EdgeTriggerInterface", "0 → 1 (上升沿)"), 0);
            edgeMode->addItem(QCoreApplication::translate("Nodes::EdgeTriggerInterface", "1 → 0 (下降沿)"), 1);

            main_layout->addWidget(new QLabel(QCoreApplication::translate("Nodes::EdgeTriggerInterface", "触发沿:")), 0, 0);
            main_layout->addWidget(edgeMode, 0, 1);
            main_layout->addWidget(new QLabel(QCoreApplication::translate("Nodes::EdgeTriggerInterface", "输入值:")), 1, 0);
            main_layout->addWidget(expression, 1, 1);
            main_layout->setColumnStretch(1, 1);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }

    public:
        QGridLayout *main_layout = new QGridLayout(this);
        QLineEdit *expression = new QLineEdit();
        QComboBox *edgeMode = new QComboBox();
    };
}
