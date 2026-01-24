#pragma once

#include <QPushButton>
#include <QTimer>

#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

class GUI_ELEMENTS_EXPORT TriggerWidget : public QPushButton
{
    Q_OBJECT

public:
    explicit TriggerWidget(const QString &text = "", QWidget *parent = nullptr);
    ~TriggerWidget() override;

public slots:
    void trigger();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onTimeout();

private:
    QTimer *m_timer;
    bool m_isActive;
};
