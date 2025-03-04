#include "clipproperty.hpp"
#include <QGroupBox>
ClipProperty::ClipProperty(TimelineModel* timelineModel, const QModelIndex& index ,QWidget *parent)
    : QDialog(parent)
    , m_timelineModel(timelineModel)
    , m_layout(new QVBoxLayout(this))
    , m_startFrameSpinBox(new QSpinBox(this))
    , m_endFrameSpinBox(new QSpinBox(this))
    , m_startTimeCodeLineEdit(new QLineEdit(this))
    , m_endTimeCodeLineEdit(new QLineEdit(this))
    , m_currentIndex(index)
{
    // 设置窗口标题和属性
    m_model = static_cast<AbstractClipModel*>(index.internalPointer());
    setWindowTitle(tr(m_model->type().toStdString().c_str()));
    setWindowFlags(Qt::Dialog);  // 设置窗口始终在顶端
    
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
    
    // 开始帧显示
    auto startLabel = new QLabel(tr("开始帧:"), this);
    timeLayout->addWidget(startLabel, 0, 0);
    m_startFrameSpinBox->setRange(0, 9999999);
    m_startFrameSpinBox->setValue(m_model->start());
    timeLayout->addWidget(m_startFrameSpinBox, 0, 1);
    
    // 开始时间码显示
    auto startTimeCodeLabel = new QLabel(tr("开始时间码:"), this);
    timeLayout->addWidget(startTimeCodeLabel, 1, 0);
    // m_startTimeCodeLineEdit = new QLineEdit(this);  // 确保在头文件中声明
    m_startTimeCodeLineEdit->setReadOnly(true);  // 设置为只读
    m_startTimeCodeLineEdit->setText(m_model->getStartTimeCode().toString());
    timeLayout->addWidget(m_startTimeCodeLineEdit, 1, 1);
    
    // 结束帧显示
    auto endLabel = new QLabel(tr("结束帧:"), this);
    timeLayout->addWidget(endLabel, 2, 0);
    m_endFrameSpinBox->setRange(0, 9999999);
    m_endFrameSpinBox->setValue(m_model->end());
    timeLayout->addWidget(m_endFrameSpinBox, 2, 1);
    
    // 结束时间码显示
    auto endTimeCodeLabel = new QLabel(tr("结束时间码:"), this);
    timeLayout->addWidget(endTimeCodeLabel, 3, 0);
    // m_endTimeCodeLineEdit = new QLineEdit(this);  // 确保在头文件中声明
    m_endTimeCodeLineEdit->setReadOnly(true);  // 设置为只读
    m_endTimeCodeLineEdit->setText(m_model->getEndTimeCode().toString());
    timeLayout->addWidget(m_endTimeCodeLineEdit, 3, 1);

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

    connect(m_model, &AbstractClipModel::lengthChanged,this, &ClipProperty::onClipDataChanged);
    connect(m_model, &AbstractClipModel::timelinePositionChanged,this, &ClipProperty::onClipDataChanged);
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
    
    // 更新时间码显示
    if (m_startTimeCodeLineEdit && m_endTimeCodeLineEdit) {
        m_startTimeCodeLineEdit->setText(m_model->getStartTimeCode().toString());
        m_endTimeCodeLineEdit->setText(m_model->getEndTimeCode().toString());
    }
    
    m_startFrameSpinBox->blockSignals(false);
    m_endFrameSpinBox->blockSignals(false);

    // 如果有代理编辑器，更新代理数据
    if (m_delegateWidget && m_delegate) {
        m_delegate->setEditorData(m_delegateWidget, m_currentIndex);
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
    m_delegate = m_timelineModel->getPluginLoader()->createDelegateForType(clipType);

    if (m_delegate) {
        // 创建代理的编辑器
        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, width(), 100);  // 给一个合理的初始大小
        m_delegateWidget = m_delegate->createEditor(this, option, m_currentIndex);
        
        if (m_delegateWidget) {
            // 将代理编辑器添加到内容布局中
            if (auto* contentWidget = findChild<QWidget*>()) {
                if (auto* contentLayout = qobject_cast<QVBoxLayout*>(contentWidget->layout())) {
                    // 在弹簧之前插入代理编辑器
                    contentLayout->insertWidget(contentLayout->count() - 1, m_delegateWidget);
                }
            }
            
            // 设置编辑器的数据
            m_delegate->setEditorData(m_delegateWidget, m_currentIndex);
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
        // 更新时间码显示
        if (m_startTimeCodeLineEdit) {
            m_startTimeCodeLineEdit->setText(m_model->getStartTimeCode().toString());
        }
        emit propertyChanged();
    }
}

void ClipProperty::onEndFrameChanged(int value)
{
    if (m_model) {
        m_model->setEnd(value);
        // 更新时间码显示
        if (m_endTimeCodeLineEdit) {
            m_endTimeCodeLineEdit->setText(m_model->getEndTimeCode().toString());
        }
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
        m_delegate->setModelData(m_delegateWidget, nullptr, m_currentIndex);
    }
}



