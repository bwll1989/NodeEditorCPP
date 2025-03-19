#ifndef TRIGGERCLIPMODEL_H
#define TRIGGERCLIPMODEL_H
#include <QPushButton>
#include "TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include "OSCMessageListWidget.hpp"
class TriggerClipModel : public AbstractClipModel {
    Q_OBJECT
public:


    TriggerClipModel(int start, int end): AbstractClipModel(start, end, "Trigger"), 
    m_editor(new QWidget()), 
    m_listWidget(new OSCMessageListWidget(m_editor))
    {
        RESIZEABLE = false;
        EMBEDWIDGET = false;
        SHOWBORDER = true;
        m_oscMessage.host = "127.0.0.1";
        m_oscMessage.port = 8993;
        m_oscMessage.address = "/all/stop";
        m_oscMessage.value = 1;
    }

    ~TriggerClipModel() override{
        delete m_editor;
    };
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
        // m_editor = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(4);
        
         // OSC消息设置
        auto* playGroup = new QGroupBox("触发设置", m_editor);
        auto* playLayout = new QVBoxLayout(playGroup);
        playLayout->setContentsMargins(0, 0, 0, 0);
        // // Host设置
        // auto* hostLabel = new QLabel("Host:", playGroup);
        // auto* hostLineEdit = new QLineEdit(playGroup);
        // hostLineEdit->setText(m_oscMessage.host);
        // connect(hostLineEdit, &QLineEdit::editingFinished, [=]() {
        //     m_oscMessage.host = hostLineEdit->text();
        // });
        // playLayout->addWidget(hostLabel, 0, 0);
        // playLayout->addWidget(hostLineEdit, 0, 1);

        // // Port设置 
        // auto* portLabel = new QLabel("Port:", playGroup);
        // auto* portLineEdit = new QLineEdit(playGroup);
        // portLineEdit->setText(QString::number(m_oscMessage.port));
        // connect(portLineEdit, &QLineEdit::editingFinished, [=]() {
        //     m_oscMessage.port = portLineEdit->text().toInt();
        // });
        // playLayout->addWidget(portLabel, 0, 2);
        // playLayout->addWidget(portLineEdit, 0, 3);

        // // Address设置
        // auto* addressLabel = new QLabel("地址:", playGroup);
        // auto* addressLineEdit = new QLineEdit(playGroup);
        // addressLineEdit->setText(m_oscMessage.address);
        // connect(addressLineEdit, &QLineEdit::editingFinished, [=]() {
        //     m_oscMessage.address = addressLineEdit->text();
        // });
        // playLayout->addWidget(addressLabel, 1, 0);
        // playLayout->addWidget(addressLineEdit, 1, 1);

        // // Value设置
        // auto* valueLabel = new QLabel("值:", playGroup);
        // auto* valueLineEdit = new QLineEdit(playGroup);
        // valueLineEdit->setText(m_oscMessage.value.toString());
        // connect(valueLineEdit, &QLineEdit::editingFinished, [=]() {
        //     m_oscMessage.value = valueLineEdit->text();
        // });
        // playLayout->addWidget(valueLabel, 1, 2);
        // playLayout->addWidget(valueLineEdit, 1, 3);

        
        // m_listWidget = new OSCMessageListWidget(m_editor);
        
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
        // qDebug() << "messages: " << messages.size();
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
    OSCMessage m_oscMessage;
    // QString m_oscHost;
    // QString m_oscPort;
    QWidget* m_editor;
    OSCMessageListWidget* m_listWidget;
};

#endif // TRIGGERCLIPMODEL_H 