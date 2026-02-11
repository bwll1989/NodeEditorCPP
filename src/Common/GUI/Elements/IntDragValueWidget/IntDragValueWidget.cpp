#include "IntDragValueWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QStyleOption>
#include <QApplication>
#include <QIntValidator>
#include <cmath>

IntDragValueWidget::IntDragValueWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(28);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->hide();
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->setFrame(false);
    m_lineEdit->setValidator(new QIntValidator(this));
    
    // Connect line edit signals
    connect(m_lineEdit, &QLineEdit::editingFinished, this, &IntDragValueWidget::onEditingFinished);
}

IntDragValueWidget::~IntDragValueWidget()
{
}

int IntDragValueWidget::value() const
{
    return m_value;
}

int IntDragValueWidget::minimum() const
{
    return m_minimum;
}

int IntDragValueWidget::maximum() const
{
    return m_maximum;
}

int IntDragValueWidget::singleStep() const
{
    return m_singleStep;
}

void IntDragValueWidget::setValue(int val)
{
    val = std::max(m_minimum, std::min(m_maximum, val));
    if (m_value != val) {
        m_value = val;
        update();
        emit valueChanged(m_value);
    }
}

void IntDragValueWidget::setMinimum(int min)
{
    m_minimum = min;
    if (m_value < min) setValue(min);
}

void IntDragValueWidget::setMaximum(int max)
{
    m_maximum = max;
    if (m_value > max) setValue(max);
}

void IntDragValueWidget::setSingleStep(int step)
{
    m_singleStep = step;
}

void IntDragValueWidget::setRange(int min, int max)
{
    setMinimum(min);
    setMaximum(max);
}

QString IntDragValueWidget::suffix() const
{
    return m_suffix;
}

void IntDragValueWidget::setSuffix(const QString &s)
{
    if (m_suffix == s) return;
    m_suffix = s;
    update();
}

void IntDragValueWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
      QPainter painter(this);
    
    QStyleOptionFrame option;
    option.initFrom(this); // 从当前控件获取状态、调色板等
    option.rect = rect();
    option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
    
    // 使用 LineEdit 的样式原语进行绘制
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, this);
    
    // 使用调色板中的文本颜色
    painter.setPen(option.palette.text().color());
    QString text = QString::number(m_value);
    if (!m_suffix.isEmpty()) {
        text += m_suffix;
    }
    painter.drawText(rect(), Qt::AlignCenter, text);
    
    // Draw active indicator if focused or hovering? 
    // For now simple style is enough.
}

void IntDragValueWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        setCursor(Qt::SizeHorCursor);
        m_lastMousePos = event->globalPosition().toPoint();
        m_dragStartValue = m_value;
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void IntDragValueWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPoint currentPos = event->globalPosition().toPoint();
        int deltaX = currentPos.x() - m_lastMousePos.x();
        
        // Calculate step based on modifiers (Shift for fine control)
        double step = m_singleStep;
        if (event->modifiers() & Qt::ShiftModifier) {
            step *= 10;
        }
        
        // Adjust value
        // Use incremental change
        double change = deltaX * step;
        setValue(m_value + change);
        
        m_lastMousePos = currentPos;
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void IntDragValueWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        unsetCursor();
        event->accept();
        emit editingFinished();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void IntDragValueWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lineEdit->setText(QString::number(m_value));
        m_lineEdit->setGeometry(rect());
        m_lineEdit->show();
        m_lineEdit->setFocus();
        m_lineEdit->selectAll();
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void IntDragValueWidget::wheelEvent(QWheelEvent *event)
{
    double steps = event->angleDelta().y() / 120.0;
    double step = m_singleStep;
    if (event->modifiers() & Qt::ShiftModifier) {
        step *= 10;
    }
    setValue(m_value + steps * step);
    event->accept();
    emit editingFinished();
}

void IntDragValueWidget::onEditingFinished()
{
    bool ok;
    int val = m_lineEdit->text().toInt(&ok);
    if (ok) {
        setValue(val);
    }
    m_lineEdit->hide();
    emit editingFinished();
}
