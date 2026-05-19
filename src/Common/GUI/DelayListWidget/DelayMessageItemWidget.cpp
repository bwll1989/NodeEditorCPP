#include "DelayMessageItemWidget.hpp"
#include <QLabel>


DelayMessageItemWidget::DelayMessageItemWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

}

void DelayMessageItemWidget::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 0, 2, 0);
    layout->setSpacing(4);
    layout->setAlignment(Qt::AlignVCenter);

    portIndex = new IntDragValueWidget(this);

    timeEdit = new IntDragValueWidget(this);
    timeEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QLabel* moveLabel = new QLabel(this);
    moveLabel->setFixedSize(14, 14);
    moveLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    moveLabel->setPixmap(QPixmap(":/icons/icons/move.png").scaled(moveLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    moveLabel->setAlignment(Qt::AlignCenter);

    auto* indexLabel = new QLabel("Index:", this);
    auto* delayLabel = new QLabel("Delay:", this);
    auto* msLabel = new QLabel("ms", this);

    layout->addWidget(indexLabel, 0, Qt::AlignVCenter);
    layout->addWidget(portIndex, 1, Qt::AlignVCenter);
    portIndex->setMinimum(0);

    layout->addWidget(delayLabel, 0, Qt::AlignVCenter);
    layout->addWidget(timeEdit, 1, Qt::AlignVCenter);
    timeEdit->setMinimum(0);

    layout->addWidget(msLabel, 0, Qt::AlignVCenter);
    layout->addWidget(moveLabel, 0, Qt::AlignVCenter);

    layout->setStretch(1, 1);
    layout->setStretch(3, 1);
}

delay_item DelayMessageItemWidget::getMessage() const
{
    delay_item message;

    message.port = portIndex->value();
    message.time = timeEdit->value();

    return message;
}

void DelayMessageItemWidget::setMessage(const delay_item& message)
{
    portIndex->setValue(message.port);
    timeEdit->setValue(message.time);

}




