#include "stagewidget.hpp"
#include <QPainter>
#include <QVBoxLayout>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>

StageWidget::StageWidget(QWidget *parent)
    : QWidget(parent)
    , m_quickWidget(new QQuickWidget(this))
    , m_layout(new QVBoxLayout(this))
    , m_stage(nullptr)
{
    setupUI();
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
    
   
    
    // Setup layout
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_quickWidget);
    setLayout(m_layout);
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

void StageWidget::setStage(TimelineStage* stage)
{
    if (m_stage != stage) {
        m_stage = stage;
        
        // 先注册图像提供者，再设置上下文属性
        auto engine = m_quickWidget->engine();
        if (engine && !engine->imageProvider("timeline")) {
            qDebug() << "Registering timeline image provider";
            engine->addImageProvider("timeline", TimelineImageProducer::instance());
        }
        
        // 将 stage 对象暴露给 QML 引擎 
        m_quickWidget->rootContext()->setContextProperty("stage", m_stage);
        m_quickWidget->setSource(QUrl("qrc:/qml/controls/TimelineStage.qml"));
    }
} 