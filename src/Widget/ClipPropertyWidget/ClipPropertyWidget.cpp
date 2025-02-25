#include "ClipPropertyWidget.hpp"
#include <QGroupBox>
ClipPropertyWidget::ClipPropertyWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    
}

void ClipPropertyWidget::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    // 1. 时间属性组
    QGroupBox *timeGroupBox = new QGroupBox(tr("时间属性"), this);
    QVBoxLayout *timeLayout = new QVBoxLayout(timeGroupBox);
    
    m_startFrameSpinBox = new QSpinBox(timeGroupBox);
    m_endFrameSpinBox = new QSpinBox(timeGroupBox);
    
    auto startLabel = new QLabel(tr("Start Frame:"), timeGroupBox);
    timeLayout->addWidget(startLabel);
    m_startFrameSpinBox->setRange(0, 99999);
    timeLayout->addWidget(m_startFrameSpinBox);
    
    auto endLabel = new QLabel(tr("End Frame:"), timeGroupBox);
    timeLayout->addWidget(endLabel);
    m_endFrameSpinBox->setRange(0, 99999);
    timeLayout->addWidget(m_endFrameSpinBox);
    
    // 2. 代理控件容器（初始为空）
    m_delegateContainer = new QWidget(this);
    m_delegateContainer->setLayout(new QVBoxLayout);
    m_delegateContainer->hide(); // 初始隐藏
    
    // 3. 弹簧
    m_spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    
    // 按顺序添加到主布局
    m_layout->addWidget(timeGroupBox);
    m_layout->addWidget(m_delegateContainer);
    m_layout->addSpacerItem(m_spacer);
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
    // 断开与旧片段的连接
    if (m_currentClip) {
        disconnect(m_currentClip, &AbstractClipModel::dataChanged,
                  this, &ClipPropertyWidget::onClipDataChanged);
    }
    
    // 清理旧的代理控件
    if (m_delegateWidget) {
        m_delegateContainer->layout()->removeWidget(m_delegateWidget);
        delete m_delegateWidget;
        m_delegateWidget = nullptr;
    }
    if (m_delegate) {
        delete m_delegate;
        m_delegate = nullptr;
    }
    
    m_currentClip = clip;
    m_model = model;
    
    // 设置新的代理控件
    setupDelegate();
    
    if (m_currentClip) {
        connect(m_currentClip, &AbstractClipModel::dataChanged,
                this, &ClipPropertyWidget::onClipDataChanged);
        updateUI();
        m_delegateContainer->show();
    } else {
        m_delegateContainer->hide();
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
    m_startFrameSpinBox->blockSignals(false);
    m_endFrameSpinBox->blockSignals(false);

    // 如果有代理编辑器，更新代理数据
    if (m_delegateWidget && m_delegate) {
        m_delegate->setEditorData(m_delegateWidget, QModelIndex());
    }
}

void ClipPropertyWidget::onStartFrameChanged(int value)
{
    if (!m_currentClip || !m_model) return;
    
    m_currentClip->setStart(value);
    m_model->calculateLength();
    m_model->emit timelineUpdated();
    emit propertyChanged();
}
//** 结束帧改变 */
void ClipPropertyWidget::onEndFrameChanged(int value)
{
    if (!m_currentClip || !m_model) return;
    
    m_currentClip->setEnd(value);
    m_model->calculateLength();
    m_model->emit timelineUpdated();
    emit propertyChanged();
}

void ClipPropertyWidget::onClipDataChanged()
{
    updateUI();
    if (m_model) {
        m_model->emit timelineUpdated();
    }
    
}

void ClipPropertyWidget::setupDelegate()
{
    if (!m_model || !m_currentClip) return;

    bool embedWidget = m_currentClip->isEmbedWidget();
    if(embedWidget) return;

    QString clipType = m_currentClip->type();
    m_delegate = m_model->m_pluginLoader->createDelegateForType(clipType);
    
    if (m_delegate) {
        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, width(), 100);
        m_delegateWidget = m_delegate->createEditor(this, option, QModelIndex());
        
        if (m_delegateWidget) {
            m_delegateContainer->layout()->addWidget(m_delegateWidget);
            m_delegate->setEditorData(m_delegateWidget, QModelIndex());
        } else {
            delete m_delegate;
            m_delegate = nullptr;
        }
    }
}