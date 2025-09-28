//
// Created by bwll1 on 2024/10/9.
//

#include "DialWidget.h"
#include <QMouseEvent>
DialWidget::DialWidget(float val,float min,float max,bool drag,QWidget *parent )
    : QDial(parent),
    value(val),
    minValue(min),
    maxValue(max),
    dragging(drag) {
    setMinimumSize(25, 25);
    // 最小尺寸25*25
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // this->setFlat(true);
    setToolTip(QString("%1Db").arg(minValue));

}

// 设置浮点值，范围应在0.0到1.0之间
void DialWidget::setValue(float newValue) {
    if (newValue < minValue) newValue =minValue; // 确保不低于0
    if (newValue > maxValue) newValue =maxValue; // 确保不超过1
    value = newValue;
    emit valueChanged(getLinearValue());
    update(); // 触发重绘
}

float DialWidget::getLinearValue() {
    return std::pow(10.0, value / 20.0);
}

float DialWidget::getValue() {
    return  value ;
}

