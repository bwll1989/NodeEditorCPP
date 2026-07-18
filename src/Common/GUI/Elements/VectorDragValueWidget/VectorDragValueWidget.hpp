#pragma once

#include <QWidget>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QVariant>
#include <array>
#include <limits>

#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

class QVBoxLayout;
class QWidget;
class QLabel;

#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief 基于 FloatDragValueWidget 的 vec2 / vec3 / vec4 显示与编辑控件
 */
class GUI_ELEMENTS_EXPORT VectorDragValueWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int components READ components WRITE setComponents)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
    Q_PROPERTY(bool labelsVisible READ labelsVisible WRITE setLabelsVisible)

public:
    enum Components {
        Vec2 = 2,
        Vec3 = 3,
        Vec4 = 4
    };
    Q_ENUM(Components)

    explicit VectorDragValueWidget(int components = Vec3, QWidget *parent = nullptr);
    ~VectorDragValueWidget() override;

    int components() const;
    void setComponents(int count);

    QVariant value() const;
    QVector2D vector2D() const;
    QVector3D vector3D() const;
    QVector4D vector4D() const;

    double minimum() const;
    double maximum() const;
    double singleStep() const;
    int decimals() const;
    bool labelsVisible() const;

    FloatDragValueWidget *componentWidget(int index) const;

public slots:
    void setValue(const QVariant &val);
    void setVector2D(const QVector2D &v);
    void setVector3D(const QVector3D &v);
    void setVector4D(const QVector4D &v);

    void setMinimum(double min);
    void setMaximum(double max);
    void setRange(double min, double max);
    void setSingleStep(double step);
    void setDecimals(int prec);
    void setLabelsVisible(bool visible);

signals:
    void valueChanged(const QVariant &value);
    void editingFinished();
    void componentsChanged(int components);

private slots:
    void onComponentValueChanged(double);
    void onComponentEditingFinished();

private:
    void rebuildUI();
    void applyComponentsVisibility();
    void setValuesInternal(const QVector4D &v, bool emitSignal);
    QVector4D readValues() const;
    QVariant packValue(const QVector4D &v) const;

    int m_components = Vec3;
    double m_minimum = -std::numeric_limits<double>::infinity();
    double m_maximum = std::numeric_limits<double>::infinity();
    double m_singleStep = 0.1;
    int m_decimals = 2;
    bool m_labelsVisible = true;
    bool m_blockEmit = false;

    QVBoxLayout *m_layout = nullptr;
    std::array<QWidget *, 4> m_rows {};
    std::array<QLabel *, 4> m_labels {};
    std::array<FloatDragValueWidget *, 4> m_fields {};
};
