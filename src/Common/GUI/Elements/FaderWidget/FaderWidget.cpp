//
// Created by bwll1 on 2024/10/9.
//

#include "FaderWidget.h"
#include <QMouseEvent>
FaderWidget::FaderWidget(float val,float min,float max,bool drag,QWidget *parent )
    : QPushButton(parent),
    value(val),
    minValue(min),
    maxValue(max),
    dragging(drag) {
    setMinimumSize(25, 25);
    // 最小尺寸25*25
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setFlat(true);
    // setToolTip(QString("%1Db").arg(getLinearValue()));

}

/**
 *
 * @param newValue db值
 */
void FaderWidget::setValue(float newValue) {
    if (newValue < minValue) newValue =minValue; // 确保不低于0
    if (newValue > maxValue) newValue =maxValue; // 确保不超过1
    value = newValue;
    emit valueChanged(getLinearValue());
    update(); // 触发重绘
}

float FaderWidget::getLinearValue() {
    if (value==minValue)
        return 0;
    return std::pow(10.0, value / 20.0);
}

float FaderWidget::getValue() {
    return  value ;
}

void FaderWidget::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event); // 调用基类的绘制方法

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // 设置背景颜色为黑色
    painter.fillRect(rect(), QColor(53, 53, 53));
    painter.drawRect(rect());

    // 将value值归一化到0-1范围，然后计算柱形高度
    float normalizedValue = (value - minValue) / (maxValue - minValue);
    int barHeight = static_cast<int>(normalizedValue * height());
    QRect barRect(0, height() - barHeight, width(), barHeight);//柱形区域
    painter.setBrush(QColor(0, 120, 215)); // 设置柱形颜色
    painter.drawRect(barRect); // 绘制柱形
    // 绘制边框
    painter.setPen(QPen(Qt::white, 1)); // 设置边框颜色和宽度
    painter.drawText(rect(), Qt::AlignCenter, QString::number(value,'f',2));

}


void FaderWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging = true;
        // 计算新值并设置
        setValue(minValue + (maxValue - minValue) * (1 - static_cast<float>(event->pos().y()) / height()));

    }

}

void FaderWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging = false;
        setValue(0);
    }
}

void FaderWidget::mouseMoveEvent(QMouseEvent *event) {
    if (dragging) {
        // 计算新值并设置
        setValue(minValue + (maxValue - minValue) * (1 - static_cast<float>(event->pos().y()) / height()));
        update();  // 触发重绘
    }
}

void FaderWidget::mouseReleaseEvent(QMouseEvent *event){
    if (event->button() == Qt::LeftButton) {
        dragging = false;
    }
}