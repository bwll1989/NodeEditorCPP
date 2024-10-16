//
// Created by bwll1 on 2024/10/8.
//

#ifndef NODEEDITORCPP_XYPAD_H
#define NODEEDITORCPP_XYPAD_H
#include <QPushButton>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QCursor>

class XYPad:public QPushButton {
Q_OBJECT
public:
    explicit XYPad(QPointF Point=QPointF(0.5,0.5),bool display_value= false,QWidget *parent = nullptr);

public slots:
    // 外部接口，用于设置十字线的坐标
    void setCrossPoint(float x, float y);
signals:
    void pointChanged(QPointF point);
    void pointXChanged(float x);
    void pointYChanged(float y);
    void pressed(QPointF point);
    void released();
protected:
    // 保证按钮始终为正方形
    void resizeEvent(QResizeEvent *event) override;

    // 处理鼠标点击事件
    void mousePressEvent(QMouseEvent *event) override ;

    // 处理鼠标滑动事件
    void mouseMoveEvent(QMouseEvent *event) override ;

    // 处理鼠标释放事件
    void mouseReleaseEvent(QMouseEvent *event) override ;

    // 处理鼠标离开事件
    void leaveEvent(QEvent *event) override ;
    // 按钮绘制事件
    void paintEvent(QPaintEvent *event) override;
    // 显示数值
    void setValueDisplay(bool val);
private:
    bool m_isDragging;
    QPointF m_crossPoint;  // 归一化的交点位置，使用浮点类型存储
    bool isValueDisplay;
};


#endif //NODEEDITORCPP_XYPAD_H
