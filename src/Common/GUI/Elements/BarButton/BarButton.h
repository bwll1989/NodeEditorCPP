//
// Created by bwll1 on 2024/10/8.
//

#ifndef NODEEDITORCPP_BARBUTTON_H
#define NODEEDITORCPP_BARBUTTON_H
#include <QPushButton>
#include <QPainter>
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif
class GUI_ELEMENTS_EXPORT BarButton : public QPushButton {
Q_OBJECT

public:
    BarButton(float val=0,QString label="",QWidget *parent = nullptr);
public slots:
    // 设置浮点值，范围应在0.0到1.0之间
    void setValue(float newValue);
protected:
    void paintEvent(QPaintEvent *event) override ;

private:
    float value; // 存储柱形高度对应的浮点数值
    QString text;
};

#endif //NODEEDITORCPP_BARBUTTON_H
