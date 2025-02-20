#include "clipproperty.hpp"

ClipProperty::ClipProperty(AbstractClipModel* model, TimelineModel* timelineModel, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
    , m_timelineModel(timelineModel)
    , m_layout(new QVBoxLayout(this))
    , m_startFrameSpinBox(new QSpinBox(this))
    , m_endFrameSpinBox(new QSpinBox(this))
{
    setupUI();
    connectSignals();
    setupDelegate();
}

void ClipProperty::setupUI()
{
    setLayout(m_layout);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);
    // 开始帧
    auto startLabel = new QLabel(tr("Start Frame:"), this);
    m_layout->addWidget(startLabel);
    
    m_startFrameSpinBox->setRange(0, 99999);
    m_startFrameSpinBox->setValue(m_model->start());
    m_layout->addWidget(m_startFrameSpinBox);

    // 结束帧
    auto endLabel = new QLabel(tr("End Frame:"), this);
    m_layout->addWidget(endLabel);
    
    m_endFrameSpinBox->setRange(0, 99999);
    m_endFrameSpinBox->setValue(m_model->end());
    m_layout->addWidget(m_endFrameSpinBox);

    // 添加分隔线
    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(line);
}

void ClipProperty::connectSignals()
{
    if (!m_model) return;

    connect(m_startFrameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ClipProperty::onStartFrameChanged);
    connect(m_endFrameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ClipProperty::onEndFrameChanged);
            
    // 连接模型的数据变化信号
    connect(m_model, &AbstractClipModel::dataChanged,
            this, &ClipProperty::onClipDataChanged);
}

void ClipProperty::updateUI()
{
    if (!m_model) return;

    // 更新开始帧和结束帧
    m_startFrameSpinBox->blockSignals(true);
    m_endFrameSpinBox->blockSignals(true);

    m_startFrameSpinBox->setValue(m_model->start());
    m_endFrameSpinBox->setValue(m_model->end());

    m_startFrameSpinBox->blockSignals(false);
    m_endFrameSpinBox->blockSignals(false);

    // 如果有代理编辑器，更新代理数据
    if (m_delegateWidget && m_delegate) {
        m_delegate->setEditorData(m_delegateWidget, QModelIndex());
    }
}

void ClipProperty::onClipDataChanged()
{
    updateUI();
}

void ClipProperty::setupDelegate()
{
    if (!m_model || !m_timelineModel) return;

    // 检查是否需要显示代理编辑器
    bool showWidget = m_model->isEmbedWidget();
    if (showWidget) {
        return;  // 如果不需要显示，直接返回
    }

    // 获取片段类型
    QString clipType = m_model->type();
    
    // 通过插件加载器获取对应的代理
    m_delegate = m_timelineModel->m_pluginLoader->createDelegateForType(clipType);
    if (m_delegate) {
        // 创建代理的编辑器
        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, width(), 100);  // 给一个合理的初始大小
        m_delegateWidget = m_delegate->createEditor(this, option, QModelIndex());
        
        if (m_delegateWidget) {
            m_layout->addWidget(m_delegateWidget);
            
            // 设置编辑器的数据
            m_delegate->setEditorData(m_delegateWidget, QModelIndex());
        } else {
            delete m_delegate;  // 只有在创建编辑器失败时才删除代理
            m_delegate = nullptr;
        }
    }
    
    // 添加弹簧以确保编辑器不会过度拉伸
    m_layout->addStretch();
}

void ClipProperty::onStartFrameChanged(int value)
{
    if (m_model) {
        m_model->setStart(value);
        emit propertyChanged();
    }
}

void ClipProperty::onEndFrameChanged(int value)
{
    if (m_model) {
        m_model->setEnd(value);
        emit propertyChanged();
    }
}

