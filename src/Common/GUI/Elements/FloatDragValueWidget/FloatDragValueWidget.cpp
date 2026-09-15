#include "FloatDragValueWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QStyleOption>
#include <QApplication>
#include <QDoubleValidator>
#include <QColor>
#include <QPen>
#include <cmath>

FloatDragValueWidget::FloatDragValueWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(25);

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

bool FloatDragValueWidget::compactMode() const
{
    return m_compactMode;
}

void FloatDragValueWidget::setCompactMode(bool enabled)
{
    if (m_compactMode == enabled) {
        return;
    }
    m_compactMode = enabled;
    if (m_compactMode) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(12, 12);
    } else {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(25);
        setMinimumWidth(0);
    }
    updateGeometry();
    update();
}

QSize FloatDragValueWidget::sizeHint() const
{
    return m_compactMode ? QSize(24, 24) : QSize(60, 25);
}

QSize FloatDragValueWidget::minimumSizeHint() const
{
    return m_compactMode ? QSize(12, 12) : QSize(40, 25);
}

void FloatDragValueWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    if (!m_compactMode) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        QStyleOptionFrame option;
        option.initFrom(this);
        option.rect = rect();
        option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
        style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, this);
        painter.setPen(option.palette.text().color());
        QString text = QString::number(m_value, 'f', m_decimals);
        if (!m_suffix.isEmpty()) {
            text += m_suffix;
        }
        painter.drawText(rect(), Qt::AlignCenter, text);
        return;
    }

    // 热力格：浅紫 → 靛蓝；静音 = 浅底描边
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF cell = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
    const qreal radius = qMin(4.0, qMin(cell.width(), cell.height()) * 0.18);

    const bool finiteRange = std::isfinite(m_minimum) && std::isfinite(m_maximum) && m_maximum > m_minimum;
    const double t = finiteRange
                         ? qBound(0.0, (m_value - m_minimum) / (m_maximum - m_minimum), 1.0)
                         : 0.0;
    const bool muted = !finiteRange || m_value <= m_minimum + 0.05;

    QColor fill;
    QColor border;
    QColor textColor;
    if (muted) {
        fill = QColor(245, 242, 250);
        border = QColor(180, 175, 195);
        textColor = QColor(120, 115, 140);
    } else {
        const QColor cLow(232, 224, 245);
        const QColor cMid(120, 100, 210);
        const QColor cHigh(45, 40, 150);
        if (t < 0.5) {
            const double u = t * 2.0;
            fill.setRgbF(cLow.redF() + (cMid.redF() - cLow.redF()) * u,
                         cLow.greenF() + (cMid.greenF() - cLow.greenF()) * u,
                         cLow.blueF() + (cMid.blueF() - cLow.blueF()) * u);
        } else {
            const double u = (t - 0.5) * 2.0;
            fill.setRgbF(cMid.redF() + (cHigh.redF() - cMid.redF()) * u,
                         cMid.greenF() + (cHigh.greenF() - cMid.greenF()) * u,
                         cMid.blueF() + (cHigh.blueF() - cMid.blueF()) * u);
        }
        border = fill.darker(110);
        textColor = (t > 0.38) ? QColor(255, 255, 255) : QColor(50, 40, 90);
    }

    painter.setPen(QPen(border, 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(cell, radius, radius);

    if (width() >= 28 && height() >= 16) {
        painter.setPen(textColor);
        QFont f = font();
        f.setPointSizeF(qMax(7.0, qMin(cell.height() * 0.36, 10.0)));
        painter.setFont(f);
        QString text = QString::number(m_value, 'f', m_decimals);
        if (!m_suffix.isEmpty() && width() >= 42) {
            text += m_suffix;
        }
        painter.drawText(cell, Qt::AlignCenter, text);
    }
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
