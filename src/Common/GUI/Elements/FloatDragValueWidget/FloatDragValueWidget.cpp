#include "FloatDragValueWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QStyleOption>
#include <QApplication>
#include <QDoubleValidator>
#include <cmath>

FloatDragValueWidget::FloatDragValueWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(28);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->hide();
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->setFrame(false);
    m_lineEdit->setValidator(new QDoubleValidator(this));
    
    // Connect line edit signals
    connect(m_lineEdit, &QLineEdit::editingFinished, this, &FloatDragValueWidget::onEditingFinished);
}

FloatDragValueWidget::~FloatDragValueWidget()
{
}

double FloatDragValueWidget::value() const
{
    return m_value;
}

double FloatDragValueWidget::minimum() const
{
    return m_minimum;
}

double FloatDragValueWidget::maximum() const
{
    return m_maximum;
}

double FloatDragValueWidget::singleStep() const
{
    return m_singleStep;
}

int FloatDragValueWidget::decimals() const
{
    return m_decimals;
}

void FloatDragValueWidget::setValue(double val)
{
    val = std::max(m_minimum, std::min(m_maximum, val));
    if (qAbs(m_value - val) > std::pow(10, -m_decimals - 1)) {
        m_value = val;
        update();
        emit valueChanged(m_value);
    }
}

void FloatDragValueWidget::setMinimum(double min)
{
    m_minimum = min;
    if (m_value < min) setValue(min);
}

void FloatDragValueWidget::setMaximum(double max)
{
    m_maximum = max;
    if (m_value > max) setValue(max);
}

void FloatDragValueWidget::setSingleStep(double step)
{
    m_singleStep = step;
}

void FloatDragValueWidget::setDecimals(int prec)
{
    m_decimals = prec;
    update();
}

void FloatDragValueWidget::setRange(double min, double max)
{
    setMinimum(min);
    setMaximum(max);
}

QString FloatDragValueWidget::suffix() const
{
    return m_suffix;
}

void FloatDragValueWidget::setSuffix(const QString &s)
{
    if (m_suffix == s) return;
    m_suffix = s;
    update();
}

void FloatDragValueWidget::paintEvent(QPaintEvent *event)
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
    QString text = QString::number(m_value, 'f', m_decimals);
    if (!m_suffix.isEmpty()) {
        text += m_suffix;
    }
    painter.drawText(rect(), Qt::AlignCenter, text);
    
    // Draw active indicator if focused or hovering? 
    // For now simple style is enough.
}

void FloatDragValueWidget::mousePressEvent(QMouseEvent *event)
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

void FloatDragValueWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPoint currentPos = event->globalPosition().toPoint();
        int deltaX = currentPos.x() - m_lastMousePos.x();
        
        // Calculate step based on modifiers (Shift for fine control)
        double step = m_singleStep;
        if (event->modifiers() & Qt::ShiftModifier) {
            step *= 0.1;
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

void FloatDragValueWidget::mouseReleaseEvent(QMouseEvent *event)
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

void FloatDragValueWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lineEdit->setText(QString::number(m_value, 'f', m_decimals));
        m_lineEdit->setGeometry(rect());
        m_lineEdit->show();
        m_lineEdit->setFocus();
        m_lineEdit->selectAll();
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void FloatDragValueWidget::wheelEvent(QWheelEvent *event)
{
    double steps = event->angleDelta().y() / 120.0;
    double step = m_singleStep;
    if (event->modifiers() & Qt::ShiftModifier) {
        step *= 0.1;
    }
    setValue(m_value + steps * step);
    event->accept();
    emit editingFinished();
}

void FloatDragValueWidget::onEditingFinished()
{
    bool ok;
    double val = m_lineEdit->text().toDouble(&ok);
    if (ok) {
        setValue(val);
    }
    m_lineEdit->hide();
    emit editingFinished();
}
