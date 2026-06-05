#pragma once

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QLineEdit"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

using namespace NodeDataTypes;

namespace Nodes
{
    class MetronomeInterface : public QFrame
    {
    public:
        explicit MetronomeInterface(QWidget* parent = nullptr)
            : QFrame(parent)
        {
            interval = new IntDragValueWidget(this);
            signalValue = new QLineEdit(this);
            start = new QPushButton(this);

            interval->setValue(1000);
            interval->setRange(1, 360000);

            signalValue->setText(QStringLiteral("tick"));

            start->setCheckable(true);
            start->setText("Start");

            auto labelInterval = new QLabel("间隔 (ms)", this);
            main_layout->addWidget(labelInterval, 0, 0, 1, 1);
            main_layout->addWidget(interval, 0, 1, 1, 2);

            auto labelSignal = new QLabel("信号值", this);
            main_layout->addWidget(labelSignal, 1, 0, 1, 1);
            main_layout->addWidget(signalValue, 1, 1, 1, 2);

            main_layout->setContentsMargins(4, 2, 4, 4);
            main_layout->addWidget(start, 2, 0, 1, 3);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 2);
            main_layout->setRowStretch(3, 1);
            this->setLayout(main_layout);
            this->setMinimumSize(200, 100);
        }

    private:
        QGridLayout* main_layout = new QGridLayout();

    public:
        IntDragValueWidget* interval = nullptr;
        QLineEdit* signalValue = nullptr;
        QPushButton* start = nullptr;
    };
}
