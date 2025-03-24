#ifndef TRIGGERCLIPMODEL_H
#define TRIGGERCLIPMODEL_H
#include <QPushButton>
#include "TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include "../../Common/GUI/OscListWidget/OSCMessageListWidget.hpp"
class TriggerClipModel : public AbstractClipModel {
    Q_OBJECT
public:


    TriggerClipModel(int start, int end): AbstractClipModel(start, end, "Trigger"), 
    m_editor(nullptr), 
    m_listWidget(new OSCMessageListWidget(m_editor))
    {
        RESIZEABLE = false;
        EMBEDWIDGET = false;
        SHOWBORDER = true;
        initPropertyWidget();
    }
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
            painter->setBrush(ClipSelectedColor);
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

#endif // TRIGGERCLIPMODEL_H 