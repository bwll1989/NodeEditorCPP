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
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);
    
    // Host
    portIndex = new QSpinBox(this);

    // Value
    timeEdit = new QLineEdit(this);
    timeEdit->setPlaceholderText("value");
    timeEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    QLabel* moveLabel = new QLabel();
    moveLabel->setFixedWidth(10);
    moveLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    moveLabel->setPixmap(QPixmap(":/icons/icons/move.png").scaled(moveLabel->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    moveLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(new QLabel("Index:"));
    layout->addWidget(portIndex);
    layout->addWidget(new QLabel("Delay:"));
    layout->addWidget(timeEdit);
    layout->addWidget(new QLabel("ms"));
    layout->addWidget(moveLabel);
    
    // 设置弹性布局比例
    layout->setStretch(0, 1);  // port text
    layout->setStretch(1, 2);  // port
    layout->setStretch(2, 1);  // value text
    layout->setStretch(3, 2);  // value
    layout->setStretch(4, 1);  // ms
    layout->setStretch(5, 2);  // icon
    layout->setStretch(7, 1);  // command

}

delay_item DelayMessageItemWidget::getMessage() const
{
    delay_item message;

    message.port = portIndex->value();
    message.time = timeEdit->text().toInt();

    return message;
}

void DelayMessageItemWidget::setMessage(const delay_item& message)
{
    portIndex->setValue(message.port);
    timeEdit->setText(QString::number(message.time));

}




