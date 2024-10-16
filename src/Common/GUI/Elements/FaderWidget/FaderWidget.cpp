//
// Created by bwll1 on 2024/10/9.
//

#include "FaderWidget.h"
#include <QMouseEvent>
FaderWidget::FaderWidget(float val,bool drag,QWidget *parent ) : QPushButton(parent), value(val),dragging(drag) {
    setMinimumSize(50, 50);
    // 最小尺寸25*25
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setFlat(true);
}

// 设置浮点值，范围应在0.0到1.0之间
void FaderWidget::setValue(float newValue) {
    if (newValue < 0.0f) newValue = 0.0f; // 确保不低于0
    if (newValue > 1.0f) newValue = 1.0f; // 确保不超过1
    value = newValue;
    emit valueChanged(value);
    update(); // 触发重绘
}

float FaderWidget::getValue() {
    return value;
}

void FaderWidget::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event); // 调用基类的绘制方法

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // 设置背景颜色为黑色
    painter.fillRect(rect(), QColor(53, 53, 53));
    // 绘制柱形图
    int barHeight = static_cast<int>(value * (height())); // 根据浮点数设置柱形高度
    QRect barRect(0, height() - barHeight, width(), barHeight);//柱形区域
    painter.setBrush(QColor(0, 120, 215)); // 设置柱形颜色
    painter.drawRect(barRect); // 绘制柱形
    // 绘制边框
    painter.setPen(QPen(Qt::white, 2)); // 设置边框颜色和宽度
    painter.drawText(rect(), Qt::AlignCenter, QString::number(value,'f',2));
}


void FaderWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging = true;
        setValue(1-static_cast<float>(event->pos().y()) / height());
    }
}

void FaderWidget::mouseMoveEvent(QMouseEvent *event) {
    if (dragging) {
        setValue(1-static_cast<float>(event->pos().y()) / height());
        update();  // 触发重绘
    }
}

void FaderWidget::mouseReleaseEvent(QMouseEvent *event){
    if (event->button() == Qt::LeftButton) {
        dragging = false;
    }
}