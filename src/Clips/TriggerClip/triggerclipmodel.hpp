#ifndef TRIGGERCLIPMODEL_H
#define TRIGGERCLIPMODEL_H
#include <QPushButton>
#include "AbstractClipModel.hpp"
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include "OscListWidget/OSCMessageListWidget.hpp"
namespace Clips
{
    class TriggerClipModel : public AbstractClipModel {
        Q_OBJECT
    public:


        TriggerClipModel(int start): AbstractClipModel(start,"Trigger"),
        m_editor(nullptr),
        m_listWidget(new OSCMessageListWidget(m_editor))
        {
            setEnd(start);
            RESIZEABLE = false;
            EMBEDWIDGET = false;
            SHOWBORDER = false;
            // 片段正常颜色
            ClipColor=QColor("#f08a5d");
            initPropertyWidget();
        }
        ~TriggerClipModel() override =default;
        // 重写保存和加载函数
        QJsonObject save() const override {
            QJsonObject json = AbstractClipModel::save();
            json["messages"] = m_listWidget->save();
            return json;
        }

        void load(const QJsonObject& json) override {
            AbstractClipModel::load(json);
            m_listWidget->load(json["messages"].toObject());

        }

        QVariant data(int role) const override {
            switch (role) {
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<TriggerClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
            }
        }


        QVariantMap currentData(int currentFrame) const override {
            if(currentFrame >= m_start && currentFrame <= m_end){
                const_cast<TriggerClipModel*>(this)->trigger();
            }
            return QVariantMap();
        }

        void initPropertyWidget() override{

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
            auto startLabel = new QLabel(tr("触发帧:"), m_standardPropertyWidget);
            timeLayout->addWidget(startLabel, 0, 0);
            m_startFrameSpinBox=new QSpinBox(m_standardPropertyWidget);
            m_startFrameSpinBox->setRange(0, 9999999);
            m_startFrameSpinBox->setValue(start());
            connect(m_startFrameSpinBox, &QSpinBox::valueChanged, this, &AbstractClipModel::setStart);
            connect(this, &AbstractClipModel::timelinePositionChanged, this, [this](){
                m_startFrameSpinBox->blockSignals(true);
                m_startFrameSpinBox->setValue(start());
                auto startTimeCode=frames_to_timecode_frame(start(),getTimeCodeType());
                m_startTimeCodeLineEdit->setText(QString("%1:%2:%3.%4").arg(startTimeCode.hours)
                                                                        .arg(startTimeCode.minutes)
                                                                        .arg(startTimeCode.seconds)
                                                                        .arg(startTimeCode.frames));
                m_startFrameSpinBox->blockSignals(false);
            });
            timeLayout->addWidget(m_startFrameSpinBox, 0, 1);
            m_startTimeCodeLineEdit=new QLineEdit(m_standardPropertyWidget);
            m_startTimeCodeLineEdit->setReadOnly(true);  // 设置为只读
            auto startTimeCode=frames_to_timecode_frame(start(),getTimeCodeType());
            m_startTimeCodeLineEdit->setText(QString("%1:%2:%3.%4").arg(startTimeCode.hours)
                                                                       .arg(startTimeCode.minutes)
                                                                       .arg(startTimeCode.seconds)
                                                                       .arg(startTimeCode.frames));
            timeLayout->addWidget(m_startTimeCodeLineEdit, 0, 2);
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
        QWidget* clipPropertyWidget() override{
            m_editor = new QWidget();
            QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            mainLayout->setSpacing(4);
            // OSC消息设置
            auto* playGroup = new QGroupBox("触发设置", m_editor);
            auto* playLayout = new QVBoxLayout(playGroup);
            playLayout->setContentsMargins(0, 0, 0, 0);
            playLayout->addWidget(m_listWidget);
            // 测试按钮
            auto* sendButton = new QPushButton("测试发送", m_editor);
            connect(sendButton, &QPushButton::clicked, [=]() {
                trigger();
            });
            playLayout->addWidget(sendButton);
            mainLayout->addWidget(playGroup);
            return m_editor;
        }

        void trigger(){
            auto messages = m_listWidget->getOSCMessages();
            for(auto message : messages){
                OSCSender::instance()->sendOSCMessageWithQueue(message);
            }
        }

        void paint(QPainter* painter, const QRect& rect, bool selected) const override{
            painter->save();  // 保存状态
            painter->setRenderHint(QPainter::Antialiasing);
            // 设置画笔和画刷
            if (selected) {
                painter->setBrush(ClipColor.darker(150));
            } else {
                painter->setBrush(ClipColor);
            }

            // 设置细边框
            if(SHOWBORDER){
                painter->setPen(QPen(ClipBorderColour, 1));
            }else{
                painter->setPen(QPen(ClipColor, 0));
            }
            // 绘制文本
            auto textRect = rect.adjusted(4,10,40,-10);
            painter->drawRect(textRect);
            painter->setPen(Qt::white);
            QFont font = painter->font();
            font.setPointSize(8);  // 设置较小的字体大小
            painter->setFont(font);

            painter->drawText(textRect, Qt::AlignCenter, "Trigger");
            // 绘制左侧竖线
            painter->setPen(QPen(ClipBorderColour, 4));  // 稍微粗一点的线
            painter->drawLine(rect.left() + 2, rect.top(),
                             rect.left() + 2, rect.bottom());

            // 设置回细边框
            painter->setPen(QPen(ClipBorderColour, 1));
            painter->restore();  // 恢复状态
        }

    private:
        QWidget* m_editor;
        OSCMessageListWidget* m_listWidget;
    };
}
#endif // TRIGGERCLIPMODEL_H 