#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include "QTimer"
#include <iostream>
#include <QtCore/qglobal.h>
#include "DelayInterface.hpp"
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    /// The model dictates the number of inputs and outputs for the Node.
    /// In this example it has no logic.
    class DelayDataModel : public NodeDelegateModel
    {
        Q_OBJECT


    public:
        DelayDataModel()
        {
            InPortCount =1;
            OutPortCount=2;
            CaptionVisible=true;
            Caption="Delay";
            WidgetEmbeddable= false;
            Resizable=true;
            PortEditable= true;
            connect(timer, SIGNAL(timeout()), this, SLOT(onSingleTimerTimeout()));

        }

        ~DelayDataModel() override{
            // 函数级注释：析构时停止并清理单定时器的剩余排队任务，防止悬挂回调。
            stopAndClearSchedule();
            deleteLater();
        }

        // 函数级注释：返回端口数据类型（输入/输出统一为 VariableData）。
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        // 函数级注释：设置端口标题；输出端口标注为“OUTPUT <index>”便于识别。
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                        return "TRIGGER";
                default:
                        return "TRIGGER";
                }
            case PortType::Out:
                return QString("OUTPUT %1").arg(portIndex);
            default:
                return "";
            }

        }

        // 函数级注释：返回输出数据。该节点用于“触发”，输出数据采用最近一次触发时的输入数据副本。
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {

            return  inData;
        }

        // 函数级注释：输入事件处理。
        // - TRIGGER(端口0)：按界面列表构造延迟队列，仅使用一个定时器按队列顺序触发；遵循“忽略重复信号”。
        // - 其他输入端口：更新载荷数据，不启动定时器。
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) return;

            switch(portIndex)
            {
            case 0: {


                const bool ignoreRepeat = widget && widget->ignoreRepeatCheckBox && widget->ignoreRepeatCheckBox->isChecked();

                // 忽略重复：若已有活动队列或定时器在运行，则直接返回
                if (ignoreRepeat && (timer->isActive() || !m_schedule.isEmpty())) {

                    return;
                }
                inData = var;
                // 非忽略：停止当前队列并重建
                if (!ignoreRepeat) {
                    stopAndClearSchedule();
                }

                // 读取并排序队列
                QVector<delay_item> messages = widget->delay_message_list_widget->getDelayMessages();
                if (messages.isEmpty()) {
                    // 无配置时，立即触发端口0
                    Q_EMIT dataUpdated(0);
                    return;
                }
                std::sort(messages.begin(), messages.end(), [](const delay_item& a, const delay_item& b){
                    return a.time < b.time;
                });
                m_schedule = messages;
                m_cursor = 0;

                // 启动第一个间隔：从0到第一个延迟的绝对时间
                const int firstDelay = std::max(0, m_schedule.first().time);
                timer->setInterval(firstDelay);

                timer->start();
                break;
            }
            default:
                inData = var;
                break;
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1= widget->delay_message_list_widget->save();
            modelJson1["ignoreRepeat"]=widget->ignoreRepeatCheckBox->isChecked();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->delay_message_list_widget->load(v.toObject());
                widget->ignoreRepeatCheckBox->setChecked(v["ignoreRepeat"].toBool());
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::Out:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::None:
                    break;
            }

            return result;
        }

    public slots:
        // 函数级注释：将节点状态以 OSC 反馈出去，便于远程监控。
        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }

        // 函数级注释：单定时器超时回调。
        // 触发当前时间批次内的所有端口，计算到下一批次的时间差并继续启动定时器；队列结束则清理。
        void onSingleTimerTimeout()
        {
            if (m_schedule.isEmpty() || m_cursor >= m_schedule.size()) {
                stopAndClearSchedule();
                return;
            }

            const int currentTime = std::max(0, m_schedule[m_cursor].time);

            // 发出当前时刻的所有端口触发
            int i = m_cursor;
            while (i < m_schedule.size() && std::max(0, m_schedule[i].time) == currentTime) {
                const int outPort = std::max(0, m_schedule[i].port);
                Q_EMIT dataUpdated(outPort);
                ++i;
            }
            m_cursor = i;

            // 还有后续批次：按时间差继续单次定时
            if (m_cursor < m_schedule.size()) {
                const int nextTime = std::max(0, m_schedule[m_cursor].time);
                const int delta = std::max(0, nextTime - currentTime);
                timer->setInterval(delta);
                timer->start();
            } else {
                // 队列完成
                stopAndClearSchedule();
            }
        }

    public:
        DelayInterface *widget=new DelayInterface();
        std::shared_ptr<VariableData> inData;
        QTimer *timer=new QTimer();

    private:
        // 函数级注释：停止定时器并清空队列状态。
        void stopAndClearSchedule()
        {
            if (timer->isActive()) {
                timer->stop();
            }
            m_schedule.clear();
            m_cursor = 0;
        }

        // 单定时器的顺序执行队列与游标
        QVector<delay_item> m_schedule;
        int m_cursor = 0;
    };
}