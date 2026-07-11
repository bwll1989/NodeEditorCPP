#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>

namespace Nodes
{
    class EdgeTriggerInterface : public QWidget
    {
    public:
        explicit EdgeTriggerInterface(QWidget *parent = nullptr)
        {
            main_layout->setContentsMargins(0, 0, 0, 0);
            setLayout(main_layout);

            expression->setPlaceholderText(QStringLiteral("JS Expression (e.g., \"$input['key']\")"));
            edgeMode->addItem(QStringLiteral("0 → 1 (上升沿)"), 0);
            edgeMode->addItem(QStringLiteral("1 → 0 (下降沿)"), 1);

            main_layout->addWidget(new QLabel(QStringLiteral("触发沿:")), 0, 0);
            main_layout->addWidget(edgeMode, 0, 1);
            main_layout->addWidget(new QLabel(QStringLiteral("输入值:")), 1, 0);
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
