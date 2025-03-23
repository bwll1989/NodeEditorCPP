//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_EXTERNALCONTROLER_HPP
#define NODEEDITORCPP_EXTERNALCONTROLER_HPP
#include <QObject>
#include <QColor>
#include <QJsonObject>
#include <unordered_map>
#include <QString>
#include <memory>
#include "Common/Devices/OSCReceiver/OSCReceiver.h"
#include "../NodeWidget/CustomDataFlowGraphModel.h"
// #include "../TimelineWidget/TimelineMVC/timelinemodel.hpp"
#include "OSCMessage.h"
#include <QWidget>
class ExternalControler:public QObject {
        Q_OBJECT
public:
    explicit ExternalControler();
    ~ExternalControler();
    static ExternalControler* instance() {
        static ExternalControler* instance = nullptr;
        if (!instance) {
            instance = new ExternalControler(); // 需要传入父窗口参数
        }
        return instance;
    }
    void setDataFlowModel(CustomDataFlowGraphModel *model);
    // void setTimelineModel(TimelineModel *model);
private slots:
    /**
     * 处理接收到的UDP数据
     * @param const QVariantMap &data 接收到的UDP数据
     */
    void hasOSC(const OSCMessage &message);  // 处理接收到的UDP数据
    // void handleJsonMessage(const QJsonObject &json);  // 处理解析后的JSON消息
 
private:
    // 禁用拷贝和赋值
    ExternalControler(const ExternalControler&) = delete;
    ExternalControler& operator=(const ExternalControler&) = delete;
private:
    CustomDataFlowGraphModel *m_dataflowmodel;
    // TimelineModel *m_timelinemodel;
    //OSC接收器
    OSCReceiver *OSC_Receiver;
    //布局

    // QtVariantPropertyManager *variantManager;

};


#endif //NODEEDITORCPP_EXTERNALCONTROLER_HPP
