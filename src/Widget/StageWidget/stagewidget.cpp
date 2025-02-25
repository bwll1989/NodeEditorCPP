#include "stagewidget.hpp"
#include <QPainter>
#include <QVBoxLayout>
#include <QQuickItem>
#include <QQmlContext>

StageWidget::StageWidget(QWidget *parent)
    : QWidget(parent)
    , m_quickWidget(new QQuickWidget(this))
    , m_layout(new QVBoxLayout(this))
    , m_stage(nullptr)
{
    setupUI();
    createConnections();
}

StageWidget::~StageWidget()
{
}

void StageWidget::setupUI()
{
    // Set minimum size
    setMinimumSize(200, 150);

    // Configure QQuickWidget
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 注册QML类型
    TimelineStage::registerType();
    TimelineScreen::registerType();
    
    m_quickWidget->setSource(QUrl("qrc:/qml/controls/TimelineStage.qml"));
    
    // Setup layout
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_quickWidget);
    setLayout(m_layout);
}

void StageWidget::createConnections()
{
    // Add any signal connections here
}

void StageWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    painter.fillRect(rect(), QColor(53, 53, 53));  // 使用与 TimelineWidget 相同的背景色
}

void StageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void StageWidget::updateFrame(int frame)
{
    if (QQuickItem *rootObject = m_quickWidget->rootObject()) {
        QMetaObject::invokeMethod(rootObject, "updateFrame",
                                Q_ARG(QVariant, frame));
    }
}

void StageWidget::setPlaying(bool playing)
{
    if (QQuickItem *rootObject = m_quickWidget->rootObject()) {
        QMetaObject::invokeMethod(rootObject, "setPlaying",
                                Q_ARG(QVariant, playing));
    }
}

void StageWidget::setTotalFrames(int frames)
{
    if (QQuickItem *rootObject = m_quickWidget->rootObject()) {
        rootObject->setProperty("totalFrames", frames);
    }
}

void StageWidget::setStage(TimelineStage* stage)
{
    if (m_stage != stage) {
        m_stage = stage;
        // 将stage暴露给QML
        m_quickWidget->rootContext()->setContextProperty("stage", m_stage);
    }
} 