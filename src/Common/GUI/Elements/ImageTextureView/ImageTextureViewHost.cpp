/**
 * @file ImageTextureViewHost.cpp
 * @brief QGraphicsView 场景下 GL 纹理预览的刷新桥接实现
 *
 * 刷新链（必须按此顺序才在节点画布内稳定显示）：
 *   setTexture / clearTexture
 *     → Host::update()
 *     → paintEvent
 *     → ImageTextureView::update()
 *     → paintGL
 */

#include "ImageTextureViewHost.hpp"

#include <QPaintEvent>
#include <QResizeEvent>
#include <QVBoxLayout>

ImageTextureViewHost::ImageTextureViewHost(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_view = new ImageTextureView(this);
    layout->addWidget(m_view);

    setMinimumSize(0, 0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ImageTextureViewHost::setTexture(const NodeDataTypes::GpuTextureHandle& texture)
{
    if (!m_view) {
        return;
    }
    m_view->setTexture(texture, true);
    update();
}

void ImageTextureViewHost::clearTexture()
{
    if (!m_view) {
        return;
    }
    m_view->clearTexture(false);
    update();
}

QSize ImageTextureViewHost::sizeHint() const
{
    return QSize(0, 0);
}

QSize ImageTextureViewHost::minimumSizeHint() const
{
    return QSize(0, 0);
}

void ImageTextureViewHost::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (m_view) {
        m_view->update();
    }
}

void ImageTextureViewHost::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    update();
}
