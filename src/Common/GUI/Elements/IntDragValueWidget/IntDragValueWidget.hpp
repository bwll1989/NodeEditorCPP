#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QVariant>
#include <limits>
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif
class GUI_ELEMENTS_EXPORT IntDragValueWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int singleStep READ singleStep WRITE setSingleStep)

public:
    explicit IntDragValueWidget(QWidget *parent = nullptr);
    ~IntDragValueWidget() override;

    int value() const;
    int minimum() const;
    int maximum() const;
    int singleStep() const;

public slots:
    void setValue(int val);
    void setMinimum(int min);
    void setMaximum(int max);
    void setSingleStep(int step);
    void setRange(int min, int max);

signals:
    void valueChanged(int value);
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

    int m_value = 0;
    int m_minimum = std::numeric_limits<int>::min();
    int m_maximum = std::numeric_limits<int>::max();
    int m_singleStep = 1;

    bool m_isDragging = false;
    QPoint m_lastMousePos;
    int m_dragStartValue = 0;

    QLineEdit *m_lineEdit;
};
