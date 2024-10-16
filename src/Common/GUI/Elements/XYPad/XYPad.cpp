//
// Created by bwll1 on 2024/10/8.
//

#include "XYPad.h"
#include "QPainter"

XYPad::XYPad(QPointF Point,bool display_value,QWidget *parent) : QPushButton(parent), m_crossPoint(Point),isValueDisplay(display_value), m_isDragging(false) {
        setMouseTracking(true); // 开启鼠标追踪，以便捕捉不按下的鼠标移动
        setMinimumSize(50, 50);
// 最小尺寸25*25
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


// 保证按钮始终为正方形
void XYPad::resizeEvent(QResizeEvent *event){
//    int side = qMin(event->size().width(), event->size().height());
//    setFixedSize(side, side);  // 强制将按钮的宽高设置为相等
//    qDebug()<<side;
    QPushButton::resizeEvent(event);
}

// 重写 paintEvent 画横竖线
void XYPad::paintEvent(QPaintEvent *event){
    // 调用基类的paintEvent以确保按钮其他部分正常绘制
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // 开启抗锯齿
    painter.fillRect(rect(), QColor(53, 53, 53));


    if (m_crossPoint != QPointF(-1, -1)) {
    // 将归一化的坐标转化回实际像素坐标
    int x = m_crossPoint.x() * width();
    int y = m_crossPoint.y() * height();
    painter.setPen(QPen(QColor(0, 120, 215), 2));  // 设置红色线条和线条宽度
    // 画横线
    painter.drawLine(0, y, width(), y);
    // 画竖线
    painter.drawLine(x, 0, x, height());
    }
    if(isValueDisplay){
        QFont font = painter.font();
        font.setPointSize(5); // 设置字体大小
        painter.setFont(font);
        painter.setPen(QPen(Qt::white, 2)); // 设置边框颜色和宽度
        painter.drawText(rect(), Qt::AlignCenter, QString::number(m_crossPoint.x(),'f',2)+" "+QString::number(m_crossPoint.y(),'f',2));
    }

    // 设置线条颜色为白色
//    painter.setPen(QPen(QColor(0, 120, 215), 2));  // 设置边框颜色和线条宽度
////    painter.setPen(Qt::black);  // 设置线条颜色为黑色
//    painter.drawRect(rect().adjusted(1, 1, -1, -1));  // 画普通矩形边框
}
// 处理鼠标点击事件
void XYPad::mousePressEvent(QMouseEvent *event){
    if (event->button() == Qt::LeftButton) {
        // 归一化坐标计算
        m_crossPoint.setX(static_cast<float>(event->pos().x()) / width());
        m_crossPoint.setY(static_cast<float>(event->pos().y()) / height());

        // 隐藏光标
        setCursor(Qt::BlankCursor);

        // 开始拖动
        m_isDragging = true;

        emit pressed(m_crossPoint);
        update();  // 触发重绘以显示新的横竖线
    }
    QPushButton::mousePressEvent(event);  // 保持按钮默认行为
}

// 处理鼠标滑动事件
void XYPad::mouseMoveEvent(QMouseEvent *event){
    if (m_isDragging) {
        // 归一化坐标计算
        m_crossPoint.setX(static_cast<float>(event->pos().x()) / width());
        m_crossPoint.setY(static_cast<float>(event->pos().y()) / height());
//        setCursor(Qt::BlankCursor);
        emit pointChanged(m_crossPoint);
        emit pointXChanged( m_crossPoint.x());
        emit pointYChanged( m_crossPoint.y());
//        qDebug() << "Normalized Mouse Moved at: (" << m_crossPoint.x() << "," << m_crossPoint.y() << ")";
        update();  // 触发重绘以显示新的横竖线
    }
    QPushButton::mouseMoveEvent(event);  // 保持按钮默认行为
}

// 处理鼠标释放事件
void XYPad::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 停止拖动
        m_isDragging = false;
        emit released();
        // 恢复默认光标
        unsetCursor();

//        qDebug() << "Normalized Mouse Released at: (" << m_crossPoint.x() << "," << m_crossPoint.y() << ")";
    }
    QPushButton::mouseReleaseEvent(event);  // 保持按钮默认行为
}

// 处理鼠标离开事件
void XYPad::leaveEvent(QEvent *event)  {
    unsetCursor();  // 鼠标离开按钮时恢复默认光标
    QPushButton::leaveEvent(event);  // 保持默认行为
}

// 外部接口，用于设置十字线的坐标
void XYPad::setCrossPoint(float x, float y) {
    if (x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f) {
        m_crossPoint.setX(x);
        m_crossPoint.setY(y);
        update();  // 更新界面以显示新的十字线
    } else {
//        qDebug() << "Invalid cross point. Coordinates should be in the range [0, 1].";
    }
}

void XYPad::setValueDisplay(bool val){
    isValueDisplay=val;
    update();
}
