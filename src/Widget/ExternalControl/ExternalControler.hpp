//
// Created by bwll1 on 2024/9/1.
//
#pragma once
#include <QObject>
#include <QColor>
#include <QJsonObject>
#include <unordered_map>
#include <QString>
#include <memory>
#include "Common/Devices/OSCReceiver/OSCReceiver.h"
#include "../NodeWidget/CustomDataFlowGraphModel.h"
#include "../TimeLineWidget/TimeLineModel.h"
#include "OSCMessage.h"
#include <QWidget>

#include "OSCSender/OSCSender.h"
#include <QCoreApplication>

enum class AddressType { Dataflow, Timeline, Unknown };

static AddressType getAddressType(const QString& type) {
    if (type == "dataflow") return AddressType::Dataflow;
    else if (type == "timeline") return AddressType::Timeline;
    else return AddressType::Unknown;
}

class ExternalControler:public QObject {
        Q_OBJECT
public:
    explicit ExternalControler();
    ~ExternalControler();
    static ExternalControler* instance() {
        if (!s_instance) {
            s_instance = new ExternalControler(); // 需要传入父窗口参数
            if (QCoreApplication::instance()) {
                s_instance->moveToThread(QCoreApplication::instance()->thread());
            }
        }
        return s_instance;
    }
    /**
     * 设置数据流程模型映射
     * @param models 指向数据流程模型映射的指针
     */
    void setDataflowModels(std::map<QString, std::unique_ptr<CustomDataFlowGraphModel>> *models);
    /**
     * 设置时间线模型
     * @param model 指向时间线模型的指针
     */
    void setTimelineModel(TimeLineModel *model);
    /**
     * 设置时间线工具栏映射
     * @param oscMapping 指向时间线工具栏映射的共享指针
     */
    void setTimelineToolBarMap(std::shared_ptr<std::unordered_map<QString, QAction*>> oscMapping);

public slots:
    /**
     * 处理接收到的UDP数据
     * @param const QVariantMap &data 接收到的UDP数据
     */
    void parseOSC(const OSCMessage &message);  // 处理接收到的UDP数据
    // void handleJsonMessage(const QJsonObject &json);  // 处理解析后的JSON消息
    // void stateFeedback(const OSCMessage &message);

private:
    // 禁用拷贝和赋值
    ExternalControler(const ExternalControler&) = delete;
    ExternalControler& operator=(const ExternalControler&) = delete;

private:
    static ExternalControler* s_instance;
    // 数据流程模型映射
    std::map<QString, std::unique_ptr<CustomDataFlowGraphModel>>  *m_dataflowmodels;
    // 时间线模型
    TimeLineModel *m_timelinemodel;
    // 时间线工具栏映射
    std::shared_ptr<std::unordered_map<QString, QAction*>> m_TimelineToolbarMapping;
    //OSC接收器
    OSCReceiver *OSC_Receiver;

};
