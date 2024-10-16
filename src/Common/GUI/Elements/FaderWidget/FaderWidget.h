#ifndef NODEEDITORCPP_FADERWIDGET_H
#define NODEEDITORCPP_FADERWIDGET_H

#include <QPushButton>
#include <QPainter>
class FaderWidget : public QPushButton {
    Q_OBJECT

public:
    FaderWidget(float val=0,bool drag=true,QWidget *parent = nullptr);

    float getValue();
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
private:
    float value; // 存储柱形高度对应的浮点数值

    bool dragging;  // 是否正在拖拽

};
#endif