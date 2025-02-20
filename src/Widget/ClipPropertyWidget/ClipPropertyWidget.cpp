#include "ClipPropertyWidget.hpp"

ClipPropertyWidget::ClipPropertyWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
}

void ClipPropertyWidget::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_startFrameSpinBox = new QSpinBox(this);
    m_endFrameSpinBox = new QSpinBox(this);
    
    setLayout(m_layout);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);
    
    auto startLabel = new QLabel(tr("Start Frame:"), this);
    m_layout->addWidget(startLabel);
    m_startFrameSpinBox->setRange(0, 99999);
    m_layout->addWidget(m_startFrameSpinBox);
    
    auto endLabel = new QLabel(tr("End Frame:"), this);
    m_layout->addWidget(endLabel);
    m_endFrameSpinBox->setRange(0, 99999);
    m_layout->addWidget(m_endFrameSpinBox);
    
    // 添加分隔线
    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(line);
    
    // 添加弹簧以确保编辑器不会过度拉伸
    m_layout->addStretch();
}

void ClipPropertyWidget::connectSignals()
{
    connect(m_startFrameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ClipPropertyWidget::onStartFrameChanged);
    connect(m_endFrameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ClipPropertyWidget::onEndFrameChanged);
}

void ClipPropertyWidget::setClip(AbstractClipModel* clip, TimelineModel* model)
{
    if (m_currentClip) {
        disconnect(m_currentClip, &AbstractClipModel::dataChanged,
                  this, &ClipPropertyWidget::onClipDataChanged);
    }
    
    m_currentClip = clip;
    m_model = model;
    
    if (m_currentClip) {
        connect(m_currentClip, &AbstractClipModel::dataChanged,
                this, &ClipPropertyWidget::onClipDataChanged);
        updateUI();
    }
}

void ClipPropertyWidget::updateUI()
{
    if (!m_currentClip) {
        m_startFrameSpinBox->setEnabled(false);
        m_endFrameSpinBox->setEnabled(false);
        return;
    }
    
    m_startFrameSpinBox->setEnabled(true);
    m_endFrameSpinBox->setEnabled(true);
    
    m_startFrameSpinBox->setValue(m_currentClip->start());
    m_endFrameSpinBox->setValue(m_currentClip->end());
}

void ClipPropertyWidget::onStartFrameChanged(int value)
{
    if (!m_currentClip || !m_model) return;
    
    m_currentClip->setStart(value);
    m_model->calculateLength();
    emit propertyChanged();
}

void ClipPropertyWidget::onEndFrameChanged(int value)
{
    if (!m_currentClip || !m_model) return;
    
    m_currentClip->setEnd(value);
    m_model->calculateLength();
    emit propertyChanged();
}

void ClipPropertyWidget::onClipDataChanged()
{
    updateUI();
}

