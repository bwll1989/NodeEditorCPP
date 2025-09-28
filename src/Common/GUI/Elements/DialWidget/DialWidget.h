#ifndef NODEEDITORCPP_DIALWIDGET_H
#define NODEEDITORCPP_DIALWIDGET_H

#include <qdial.h>
#include <QPushButton>
#include <QPainter>
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif
class GUI_ELEMENTS_EXPORT DialWidget : public QDial {
    Q_OBJECT

public:
    DialWidget(float val=-40,float min=-40,float max=10,bool drag=false,QWidget *parent = nullptr);

    float getLinearValue();
    float getValue();
signals:
    void valueChanged(float newValue); // 声明值变化的信号



private:
    float value; // 存储柱形高度对应的浮点数值
    float maxValue; //最大值
    float minValue; //最小值
    bool dragging;  // 是否正在拖拽

};
#endif