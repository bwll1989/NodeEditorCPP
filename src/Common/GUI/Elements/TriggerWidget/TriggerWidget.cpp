#include "TriggerWidget.hpp"
#include <QPainter>
#include <QStyleOptionButton>

TriggerWidget::TriggerWidget(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
    , m_isActive(false)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(500);

    connect(m_timer, &QTimer::timeout, this, &TriggerWidget::onTimeout);
    connect(this, &QPushButton::clicked, this, &TriggerWidget::trigger);
}

TriggerWidget::~TriggerWidget()
{
}

void TriggerWidget::trigger()
{
    m_isActive = true;
    update();
    m_timer->start();
}

void TriggerWidget::onTimeout()
{
    m_isActive = false;
    update();
}

void TriggerWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    
    QStyleOptionButton option;
    option.initFrom(this);
    option.text = text();
    
    if (m_isActive) {
        option.state |= QStyle::State_On; // 设置为 On 状态，对应 QSS 中的 :checked
    }
    
    style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);
}
