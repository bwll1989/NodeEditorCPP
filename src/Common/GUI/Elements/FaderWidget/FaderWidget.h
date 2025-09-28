#ifndef NODEEDITORCPP_FADERWIDGET_H
#define NODEEDITORCPP_FADERWIDGET_H

#include <QPushButton>
#include <QPainter>

#include "opencv2/core/mat.hpp"
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif
class GUI_ELEMENTS_EXPORT FaderWidget : public QPushButton {
    Q_OBJECT

public:
    FaderWidget(float val=-40,float min=-40,float max=10,bool drag=false,QWidget *parent = nullptr);

    float getLinearValue();
    float getValue();
    float getMinValue(){return minValue;}
    float getMaxValue(){return maxValue;}
signals:
    void valueChanged(float newValue); // 声明值变化的信号
public slots:
    // 设置浮点值，范围应在0.0到1.0之间
    void setValue(float newValue);

protected:
    void paintEvent(QPaintEvent *event) override ;

    void mousePressEvent(QMouseEvent *event) override ;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override ;

    void mouseDoubleClickEvent(QMouseEvent *event) override ;
private:
    float value; // 存储柱形高度对应的浮点数值
    float maxValue; //最大值
    float minValue; //最小值
    bool dragging;  // 是否正在拖拽

};
#endif