#include "clipproperty.hpp"
#include <QGroupBox>
ClipProperty::ClipProperty(AbstractClipModel* model, TimelineModel* timelineModel, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
    , m_timelineModel(timelineModel)
    , m_layout(new QVBoxLayout(this))
    , m_startFrameSpinBox(new QSpinBox(this))
    , m_endFrameSpinBox(new QSpinBox(this))
{
    // 设置窗口标题和属性
    setWindowTitle(tr("Clip Properties"));
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);  // 设置窗口始终在顶端
    
    setupUI();
    connectSignals();
    setupDelegate();
    
    // 设置合适的窗口大小
    resize(300, 400);
}

void ClipProperty::setupUI()
{
    setLayout(m_layout);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);
    
    // 创建一个容器 widget 来放置主要内容
    auto contentWidget = new QWidget(this);
    auto contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(4);
    
    // 1. 时间属性组
    QGroupBox *timeGroupBox = new QGroupBox(tr("时间属性"), this);
    QGridLayout *timeLayout = new QGridLayout(timeGroupBox);
    // 开始帧
    auto startLabel = new QLabel(tr("Start Frame:"), this);
    timeLayout->addWidget(startLabel,0,0);
    
    m_startFrameSpinBox->setRange(0, 99999);
    m_startFrameSpinBox->setValue(m_model->start());
    timeLayout->addWidget(m_startFrameSpinBox,0,1);

    // 结束帧
    auto endLabel = new QLabel(tr("End Frame:"), this);
    timeLayout->addWidget(endLabel,1,0);
    
    m_endFrameSpinBox->setRange(0, 99999);
    m_endFrameSpinBox->setValue(m_model->end());
    timeLayout->addWidget(m_endFrameSpinBox,1,1);

    m_layout->addWidget(timeGroupBox);

    // 添加代理编辑器的占位符
    if (m_delegateWidget) {
        contentLayout->addWidget(m_delegateWidget);
    }

    // 添加弹簧以确保内容在顶部
    contentLayout->addStretch();

    // 将内容 widget 添加到主布局
    m_layout->addWidget(contentWidget);

    // 添加按钮组到主布局底部
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        this
    );
    m_layout->addWidget(m_buttonBox);
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

    // 连接按钮信号
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &ClipProperty::onApplyClicked);
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
            // 将代理编辑器添加到内容布局中
            if (auto* contentWidget = findChild<QWidget*>()) {
                if (auto* contentLayout = qobject_cast<QVBoxLayout*>(contentWidget->layout())) {
                    // 在弹簧之前插入代理编辑器
                    contentLayout->insertWidget(contentLayout->count() - 1, m_delegateWidget);
                }
            }
            
            // 设置编辑器的数据
            m_delegate->setEditorData(m_delegateWidget, QModelIndex());
        } else {
            delete m_delegate;  // 只有在创建编辑器失败时才删除代理
            m_delegate = nullptr;
        }
    }
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

void ClipProperty::onApplyClicked()
{
    // 应用当前的更改
    onStartFrameChanged(m_startFrameSpinBox->value());
    onEndFrameChanged(m_endFrameSpinBox->value());
    
    // 如果有代理编辑器，也应用其更改
    if (m_delegateWidget && m_delegate) {
        m_delegate->setModelData(m_delegateWidget, nullptr, QModelIndex());
    }
}

