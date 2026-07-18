#include "VectorDragValueWidget.hpp"

#include <QColor>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPointF>
#include <QSignalBlocker>
#include <QSizePolicy>

namespace {
constexpr const char *kLabels[4] = {"X", "Y", "Z", "W"};
const char *kLabelColors[4] = {"#e74c3c", "#2ecc71", "#3498db", "#f1c40f"};
}

VectorDragValueWidget::VectorDragValueWidget(int components, QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setComponents(components);
    rebuildUI();
}

VectorDragValueWidget::~VectorDragValueWidget() = default;

int VectorDragValueWidget::components() const
{
    return m_components;
}

void VectorDragValueWidget::setComponents(int count)
{
    const int clamped = qBound(count, 2, 4);
    if (m_components == clamped) {
        applyComponentsVisibility();
        return;
    }
    m_components = clamped;
    applyComponentsVisibility();
    emit componentsChanged(m_components);
}

QVariant VectorDragValueWidget::value() const
{
    return packValue(readValues());
}

QVector2D VectorDragValueWidget::vector2D() const
{
    const QVector4D v = readValues();
    return QVector2D(v.x(), v.y());
}

QVector3D VectorDragValueWidget::vector3D() const
{
    const QVector4D v = readValues();
    return QVector3D(v.x(), v.y(), v.z());
}

QVector4D VectorDragValueWidget::vector4D() const
{
    return readValues();
}

double VectorDragValueWidget::minimum() const
{
    return m_minimum;
}

double VectorDragValueWidget::maximum() const
{
    return m_maximum;
}

double VectorDragValueWidget::singleStep() const
{
    return m_singleStep;
}

int VectorDragValueWidget::decimals() const
{
    return m_decimals;
}

bool VectorDragValueWidget::labelsVisible() const
{
    return m_labelsVisible;
}

FloatDragValueWidget *VectorDragValueWidget::componentWidget(int index) const
{
    if (index < 0 || index >= 4) {
        return nullptr;
    }
    return m_fields[static_cast<size_t>(index)];
}

void VectorDragValueWidget::setValue(const QVariant &val)
{
    if (!val.isValid()) {
        return;
    }

    const int typeId = val.typeId();
    if (typeId == QMetaType::QVector2D) {
        setVector2D(val.value<QVector2D>());
        return;
    }
    if (typeId == QMetaType::QVector3D) {
        setVector3D(val.value<QVector3D>());
        return;
    }
    if (typeId == QMetaType::QVector4D) {
        setVector4D(val.value<QVector4D>());
        return;
    }
    if (typeId == QMetaType::QPointF) {
        const QPointF p = val.toPointF();
        setVector2D(QVector2D(float(p.x()), float(p.y())));
        return;
    }
    if (typeId == QMetaType::QColor) {
        const QColor c = val.value<QColor>();
        setVector4D(QVector4D(c.redF(), c.greenF(), c.blueF(), c.alphaF()));
        return;
    }
    if (val.canConvert<QVariantList>()) {
        const QVariantList list = val.toList();
        QVector4D v(0, 0, 0, 0);
        if (list.size() > 0) v.setX(float(list[0].toDouble()));
        if (list.size() > 1) v.setY(float(list[1].toDouble()));
        if (list.size() > 2) v.setZ(float(list[2].toDouble()));
        if (list.size() > 3) v.setW(float(list[3].toDouble()));
        if (list.size() >= 2 && list.size() <= 4) {
            setComponents(list.size());
        }
        setValuesInternal(v, true);
        return;
    }

    bool ok = false;
    const double d = val.toDouble(&ok);
    if (ok) {
        setValuesInternal(QVector4D(float(d), float(d), float(d), float(d)), true);
    }
}

void VectorDragValueWidget::setVector2D(const QVector2D &v)
{
    setComponents(Vec2);
    setValuesInternal(QVector4D(v, 0.0f, 0.0f), true);
}

void VectorDragValueWidget::setVector3D(const QVector3D &v)
{
    setComponents(Vec3);
    setValuesInternal(QVector4D(v, 0.0f), true);
}

void VectorDragValueWidget::setVector4D(const QVector4D &v)
{
    setComponents(Vec4);
    setValuesInternal(v, true);
}

void VectorDragValueWidget::setMinimum(double min)
{
    m_minimum = min;
    for (auto *field : m_fields) {
        if (field) field->setMinimum(min);
    }
}

void VectorDragValueWidget::setMaximum(double max)
{
    m_maximum = max;
    for (auto *field : m_fields) {
        if (field) field->setMaximum(max);
    }
}

void VectorDragValueWidget::setRange(double min, double max)
{
    setMinimum(min);
    setMaximum(max);
}

void VectorDragValueWidget::setSingleStep(double step)
{
    m_singleStep = step;
    for (auto *field : m_fields) {
        if (field) field->setSingleStep(step);
    }
}

void VectorDragValueWidget::setDecimals(int prec)
{
    m_decimals = prec;
    for (auto *field : m_fields) {
        if (field) field->setDecimals(prec);
    }
}

void VectorDragValueWidget::setLabelsVisible(bool visible)
{
    if (m_labelsVisible == visible) return;
    m_labelsVisible = visible;
    applyComponentsVisibility();
}

void VectorDragValueWidget::onComponentValueChanged(double)
{
    if (m_blockEmit) return;
    emit valueChanged(value());
}

void VectorDragValueWidget::onComponentEditingFinished()
{
    if (m_blockEmit) return;
    emit editingFinished();
}

void VectorDragValueWidget::rebuildUI()
{
    if (m_layout) {
        return;
    }

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(2);

    for (int i = 0; i < 4; ++i) {
        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(4);

        auto *label = new QLabel(kLabels[i], row);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedWidth(14);
        label->setStyleSheet(QStringLiteral("color:%1;font-weight:600;").arg(kLabelColors[i]));

        auto *field = new FloatDragValueWidget(row);
        field->setMinimum(m_minimum);
        field->setMaximum(m_maximum);
        field->setSingleStep(m_singleStep);
        field->setDecimals(m_decimals);
        field->setValue(0.0);

        rowLayout->addWidget(label, 0);
        rowLayout->addWidget(field, 1);

        m_layout->addWidget(row);

        connect(field, &FloatDragValueWidget::valueChanged,
                this, &VectorDragValueWidget::onComponentValueChanged);
        connect(field, &FloatDragValueWidget::editingFinished,
                this, &VectorDragValueWidget::onComponentEditingFinished);

        m_rows[static_cast<size_t>(i)] = row;
        m_labels[static_cast<size_t>(i)] = label;
        m_fields[static_cast<size_t>(i)] = field;
    }

    applyComponentsVisibility();
}

void VectorDragValueWidget::applyComponentsVisibility()
{
    if (!m_layout) return;
    for (int i = 0; i < 4; ++i) {
        const bool visible = i < m_components;
        if (m_rows[static_cast<size_t>(i)]) {
            m_rows[static_cast<size_t>(i)]->setVisible(visible);
        }
        if (m_labels[static_cast<size_t>(i)]) {
            m_labels[static_cast<size_t>(i)]->setVisible(visible && m_labelsVisible);
        }
        if (m_fields[static_cast<size_t>(i)]) {
            m_fields[static_cast<size_t>(i)]->setVisible(visible);
        }
    }
}

void VectorDragValueWidget::setValuesInternal(const QVector4D &v, bool emitSignal)
{
    if (!m_layout) {
        rebuildUI();
    }

    const QVector4D old = readValues();
    const bool same =
        qFuzzyCompare(old.x() + 1.0f, v.x() + 1.0f) &&
        qFuzzyCompare(old.y() + 1.0f, v.y() + 1.0f) &&
        qFuzzyCompare(old.z() + 1.0f, v.z() + 1.0f) &&
        qFuzzyCompare(old.w() + 1.0f, v.w() + 1.0f);
    if (same) return;

    m_blockEmit = true;
    const float comps[4] = {v.x(), v.y(), v.z(), v.w()};
    for (int i = 0; i < 4; ++i) {
        if (!m_fields[static_cast<size_t>(i)]) continue;
        QSignalBlocker blocker(m_fields[static_cast<size_t>(i)]);
        m_fields[static_cast<size_t>(i)]->setValue(comps[i]);
    }
    m_blockEmit = false;

    if (emitSignal) {
        emit valueChanged(value());
    }
}

QVector4D VectorDragValueWidget::readValues() const
{
    QVector4D v(0, 0, 0, 0);
    if (m_fields[0]) v.setX(float(m_fields[0]->value()));
    if (m_fields[1]) v.setY(float(m_fields[1]->value()));
    if (m_fields[2]) v.setZ(float(m_fields[2]->value()));
    if (m_fields[3]) v.setW(float(m_fields[3]->value()));
    return v;
}

QVariant VectorDragValueWidget::packValue(const QVector4D &v) const
{
    switch (m_components) {
    case Vec2:
        return QVariant::fromValue(QVector2D(v.x(), v.y()));
    case Vec3:
        return QVariant::fromValue(QVector3D(v.x(), v.y(), v.z()));
    case Vec4:
    default:
        return QVariant::fromValue(v);
    }
}
