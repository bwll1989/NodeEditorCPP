#ifndef TRIGGERCLIPMODEL_H
#define TRIGGERCLIPMODEL_H
#include <QPushButton>
#include "TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"
class TriggerClipModel : public AbstractClipModel {
    Q_OBJECT
public:


    TriggerClipModel(int start, int end): AbstractClipModel(start, end, "Trigger"), m_editor(nullptr)
    {
        RESIZEABLE = false;
        EMBEDWIDGET = false;
        SHOWBORDER = true;
        m_oscSender = std::make_unique<OSCSender>("127.0.0.1", 6002);
    }

    ~TriggerClipModel() override{
        delete m_editor;
    };
    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();

        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
       
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
            qDebug() << "currentFrame: " << currentFrame;
        }
        return QVariantMap();
    }

    QWidget* clipPropertyWidget() override{
        m_editor = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(4);
        
         // 文件名显示
        auto* playGroup = new QGroupBox("设置", m_editor);
        auto* playLayout = new QGridLayout(playGroup);
        QLineEdit* commandLineEdit = new QLineEdit(playGroup);
        // playerName->setText(oscHost());
        // connect(playerName, &QLineEdit::editingFinished, [=]() {
        //     setOscHost(playerName->text());
        // });
        playLayout->addWidget(commandLineEdit, 0, 0,1,2);
        auto* valueLineEdit = new QLineEdit(playGroup);
        // playerID->setText(this->playerID());
        // connect(playerID, &QLineEdit::editingFinished, [=]() {
        //     setPlayerID(playerID->text());
        // });
        playLayout->addWidget(valueLineEdit, 0, 2,1,2);
        auto* sendButton = new QPushButton("test", playGroup);
        playLayout->addWidget(sendButton, 1, 0,1,4);
        // connect(sendButton, &QPushButton::clicked, [=]() {
        //     m_oscSender->sendMessage(commandLineEdit->text(), valueLineEdit->text());
        // });
  
        mainLayout->addWidget(playGroup);
        return m_editor;
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
    std::unique_ptr<OSCSender> m_oscSender;
    QString m_oscHost;
    QString m_oscPort;
    QWidget* m_editor;
};

#endif // TRIGGERCLIPMODEL_H 