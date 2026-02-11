#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QVariant>
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif
class GUI_ELEMENTS_EXPORT FloatDragValueWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix)

public:
    explicit FloatDragValueWidget(QWidget *parent = nullptr);
    ~FloatDragValueWidget() override;

    double value() const;
    double minimum() const;
    double maximum() const;
    double singleStep() const;
    int decimals() const;
    QString suffix() const;

public slots:
    void setValue(double val);
    void setMinimum(double min);
    void setMaximum(double max);
    void setSingleStep(double step);
    void setDecimals(int prec);
    void setRange(double min, double max);
    void setSuffix(const QString &s);

signals:
    void valueChanged(double value);
    void editingFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onEditingFinished();

private:
    void updateText();

    double m_value = 0.0;
    double m_minimum = -std::numeric_limits<double>::infinity();
    double m_maximum = std::numeric_limits<double>::infinity();
    double m_singleStep = 0.1;
    int m_decimals = 2;

    bool m_isDragging = false;
    QPoint m_lastMousePos;
    double m_dragStartValue = 0.0;

    QLineEdit *m_lineEdit;
    QString m_suffix;
};
