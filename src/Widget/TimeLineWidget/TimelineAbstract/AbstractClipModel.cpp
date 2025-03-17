#include "AbstractClipModel.hpp"
// 实现虚析构函数
AbstractClipModel::~AbstractClipModel(){
    if (m_standardPropertyWidget) {
            m_standardPropertyWidget->close();
            m_standardPropertyWidget->deleteLater();
        }
};
// Getters
int AbstractClipModel::start() const { return m_start; }
int AbstractClipModel::end() const { return m_end; }
QString AbstractClipModel::type() const { return m_type; }
int AbstractClipModel::length() const { return m_end - m_start; }
int AbstractClipModel::trackIndex() const { return m_trackIndex; }
bool AbstractClipModel::isResizable() const { return RESIZEABLE; }
bool AbstractClipModel::isEmbedWidget() const { return EMBEDWIDGET; }
bool AbstractClipModel::isShowBorder() const { return SHOWBORDER; }
Timecode AbstractClipModel::getStartTimeCode() const {return m_startTimeCode;}
Timecode AbstractClipModel::getEndTimeCode() const {return m_endTimeCode;}
TimecodeType AbstractClipModel::getTimecodeType() const {return m_timecodeType;}
void AbstractClipModel::setTimecodeType(TimecodeType timecodeType) 
{
    m_timecodeType = timecodeType;
    m_startTimeCode = Timecode::fromFrames(m_start, m_timecodeType);
    m_endTimeCode = Timecode::fromFrames(m_end, m_timecodeType);
}
// Setters
void AbstractClipModel::setStart(int start) { 
    if (m_start != start) {
        m_start = start; 
        m_startTimeCode = Timecode::fromFrames(m_start, m_timecodeType);
        emit timelinePositionChanged(m_start);

    }
}
void AbstractClipModel::setEnd(int end) { 
    if (m_end != end) {
        m_end = end;
        m_endTimeCode = Timecode::fromFrames(m_end, m_timecodeType);
        emit lengthChanged();

    }
}
void AbstractClipModel::setTrackIndex(int index) { m_trackIndex = index; }
void AbstractClipModel::setResizable(bool resizable) { RESIZEABLE = resizable; }
void AbstractClipModel::setEmbedWidget(bool embedWidget) { 
    if (EMBEDWIDGET != embedWidget) {
        EMBEDWIDGET = embedWidget;
    }
}
void AbstractClipModel::setShowBorder(bool showBorder) { SHOWBORDER = showBorder; }
// Save/Load
QJsonObject AbstractClipModel::save() const {
    QJsonObject clipJson;
    clipJson["start"] = m_start;
    clipJson["end"] = m_end;
    clipJson["type"] = m_type;
    clipJson["track"] = m_trackIndex;

    return clipJson;
}

void AbstractClipModel::load(const QJsonObject& json) {
    m_start = json["start"].toInt();
    m_end = json["end"].toInt();
    m_type = json["type"].toString();
    m_trackIndex = json["track"].toInt();
}

QVariantMap AbstractClipModel::currentVideoData(int currentFrame) const {

    return QVariantMap();
}

QVariantMap AbstractClipModel::currentAudioData(int currentFrame) const {
    emit audioDataUpdate();
    return QVariantMap();
}

QVariantMap AbstractClipModel::currentControlData(int currentFrame) const {
    emit controlDataUpdate();
    return QVariantMap();
}

QWidget* AbstractClipModel::standardPropertyWidget(){
    if(!m_standardPropertyWidget){
        m_standardPropertyWidget = new QWidget();
        m_layout = new QVBoxLayout(m_standardPropertyWidget);
        m_layout->setContentsMargins(4, 4, 4, 4);
        m_layout->setSpacing(4);
    
        // 创建一个容器 widget 来放置主要内容
        auto contentWidget = new QWidget(m_standardPropertyWidget);
        auto contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(4);
    
        // 1. 时间属性组
        QGroupBox *timeGroupBox = new QGroupBox(tr("时间属性"), m_standardPropertyWidget);
        QGridLayout *timeLayout = new QGridLayout(timeGroupBox);
        
        // 开始帧显示
        auto startLabel = new QLabel(tr("开始帧:"), m_standardPropertyWidget);
        timeLayout->addWidget(startLabel, 0, 0);
        m_startFrameSpinBox=new QSpinBox(m_standardPropertyWidget);
        m_startFrameSpinBox->setRange(0, 9999999);
        m_startFrameSpinBox->setValue(start());
        connect(m_startFrameSpinBox, &QSpinBox::valueChanged, this, &AbstractClipModel::setStart);
        connect(this, &AbstractClipModel::timelinePositionChanged, this, [this](){
            m_startFrameSpinBox->blockSignals(true);
            m_startFrameSpinBox->setValue(start());
            m_startTimeCodeLineEdit->setText(getStartTimeCode().toString());
            m_startFrameSpinBox->blockSignals(false);
        });
        timeLayout->addWidget(m_startFrameSpinBox, 0, 1);
        
        // 开始时间码显示
        auto startTimeCodeLabel = new QLabel(tr("开始时间码:"), m_standardPropertyWidget);
        timeLayout->addWidget(startTimeCodeLabel, 1, 0);
        // m_startTimeCodeLineEdit = new QLineEdit(this);  // 确保在头文件中声明
        m_startTimeCodeLineEdit=new QLineEdit(m_standardPropertyWidget);
        m_startTimeCodeLineEdit->setReadOnly(true);  // 设置为只读
        m_startTimeCodeLineEdit->setText(getStartTimeCode().toString());
        timeLayout->addWidget(m_startTimeCodeLineEdit, 1, 1);
        
        // 结束帧显示
        auto endLabel = new QLabel(tr("结束帧:"), m_standardPropertyWidget);
        timeLayout->addWidget(endLabel, 2, 0);
        m_endFrameSpinBox=new QSpinBox(m_standardPropertyWidget);
        m_endFrameSpinBox->setRange(0, 9999999);
        m_endFrameSpinBox->setValue(end());
        connect(m_endFrameSpinBox, &QSpinBox::valueChanged, this, &AbstractClipModel::setEnd);
        connect(this, &AbstractClipModel::lengthChanged, this, [this](){
            m_endFrameSpinBox->blockSignals(true);
            m_endFrameSpinBox->setValue(end());
            m_endTimeCodeLineEdit->setText(getEndTimeCode().toString());
            m_endFrameSpinBox->blockSignals(false);
        });
        timeLayout->addWidget(m_endFrameSpinBox, 2, 1);
        
        // 结束时间码显示
        auto endTimeCodeLabel = new QLabel(tr("结束时间码:"), m_standardPropertyWidget);
        timeLayout->addWidget(endTimeCodeLabel, 3, 0);
        // m_endTimeCodeLineEdit = new QLineEdit(this);  // 确保在头文件中声明
        m_endTimeCodeLineEdit=new QLineEdit(m_standardPropertyWidget);
        m_endTimeCodeLineEdit->setReadOnly(true);  // 设置为只读
        m_endTimeCodeLineEdit->setText(getEndTimeCode().toString());
        timeLayout->addWidget(m_endTimeCodeLineEdit, 3, 1);

        m_layout->addWidget(timeGroupBox);
       
        // 添加代理编辑器的占位符
        if (!m_clipPropertyWidget) {
            m_clipPropertyWidget = clipPropertyWidget();
            contentLayout->addWidget(m_clipPropertyWidget);
        }

        // 添加弹簧以确保内容在顶部
        contentLayout->addStretch();

        // 将内容 widget 添加到主布局
        m_layout->addWidget(contentWidget);
        m_standardPropertyWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
    }
    
    return m_standardPropertyWidget;
}
// 如果有任何其他非内联成员函数，也可以在这里实现 