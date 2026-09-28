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
    setMinimumHeight(25);

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

    // 编辑态由 QLineEdit 负责显示，不再绘制底层文字，避免透明背景时重叠
    if (m_lineEdit && m_lineEdit->isVisible()) {
        return;
    }

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

double IntDragValueWidget::effectiveStep(Qt::KeyboardModifiers modifiers) const
{
    double step = m_singleStep;
    if (modifiers & Qt::ShiftModifier) {
        step *= 0.1;
    } else if (modifiers & (Qt::ControlModifier | Qt::AltModifier)) {
        step *= 10.0;
    }
    return step;
}

void IntDragValueWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        setCursor(Qt::SizeHorCursor);
        m_lastMousePos = event->globalPosition().toPoint();
        m_dragStartValue = m_value;
        m_dragAccum = 0.0;
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

        // 累加小数步进，避免 Shift×0.1 时被 int 截断导致不动
        m_dragAccum += deltaX * effectiveStep(event->modifiers());
        const int intChange = static_cast<int>(m_dragAccum);
        if (intChange != 0) {
            setValue(m_value + intChange);
            m_dragAccum -= intChange;
        }

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
        m_isDragging = false;
        unsetCursor();

        m_lineEdit->setText(QString::number(m_value));
        m_lineEdit->setGeometry(rect());
        // 不透明底，防止父级/全局透明样式透出底层绘制
        QPalette pal = m_lineEdit->palette();
        pal.setColor(QPalette::Base, palette().color(QPalette::Base));
        pal.setColor(QPalette::Text, palette().color(QPalette::Text));
        m_lineEdit->setPalette(pal);
        m_lineEdit->setAutoFillBackground(true);
        m_lineEdit->show();
        m_lineEdit->raise();
        m_lineEdit->setFocus();
        m_lineEdit->selectAll();
        update();
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void IntDragValueWidget::wheelEvent(QWheelEvent *event)
{
    const double steps = event->angleDelta().y() / 120.0;
    const double change = steps * effectiveStep(event->modifiers());
    int intChange = static_cast<int>(std::round(change));
    // 整数最小步进为 1：精调时若单次不足 1 仍至少动一格
    if (intChange == 0 && change != 0.0) {
        intChange = change > 0.0 ? 1 : -1;
    }
    if (intChange != 0) {
        setValue(m_value + intChange);
    }
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
    update();
    emit editingFinished();
}
