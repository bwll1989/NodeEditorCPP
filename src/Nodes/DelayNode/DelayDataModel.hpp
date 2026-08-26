#pragma once

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include "QTimer"
#include <QElapsedTimer>
#include <iostream>
#include <QtCore/qglobal.h>
#include "DelayInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
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
    class DelayDataModel : public AbstractDelegateModel
    {
        Q_OBJECT


    public:
        // 函数级注释：初始化 Delay 节点模型，配置单定时器触发与进度条刷新。
        DelayDataModel()
        {
            InPortCount =1;
            OutPortCount=2;
            CaptionVisible=true;
            Caption="Delay";
            WidgetEmbeddable= false;
            Resizable=true;
            PortEditable= true;

            timer->setParent(this);
            timer->setSingleShot(true);
            // 默认 CoarseTimer 在 Windows 上约 15ms 粒度，长序列会明显抖动
            timer->setTimerType(Qt::PreciseTimer);

            progressTimer->setParent(this);
            progressTimer->setInterval(100);

            connect(timer, &QTimer::timeout, this, &DelayDataModel::onSingleTimerTimeout);
            connect(progressTimer, &QTimer::timeout, this, &DelayDataModel::updateProgress);

            updateProgress();
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
                    stopAndClearSchedule();
                    Q_EMIT dataUpdated(0);
                    return;
                }
                std::sort(messages.begin(), messages.end(), [](const delay_item& a, const delay_item& b){
                    return a.time < b.time;
                });
                m_schedule = messages;
                m_cursor = 0;

                m_totalDurationMs = std::max(0, m_schedule.last().time);
                m_elapsed.restart();

                if (!progressTimer->isActive()) {
                    progressTimer->start();
                }
                // 按墙钟绝对时间调度；0ms 批次会在 arm 时立即发出
                armNextFromElapsed();
                updateProgress();
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

        // 函数级注释：按“已运行时间/总延时”更新界面进度条（0-100）；停止或队列结束时重置为空闲。
        void updateProgress()
        {
            if (!widget) {
                return;
            }
            if (m_schedule.isEmpty() || m_cursor >= m_schedule.size()) {
                widget->setProgressIdle();
                return;
            }

            const int totalMs = std::max(0, m_totalDurationMs);
            const int nowMs = static_cast<int>(elapsedMs());

            if (totalMs <= 0) {
                widget->setProgressPercent(100);
                return;
            }

            const int percent = std::max(0, std::min(100, (nowMs * 100) / totalMs));
            widget->setProgressPercent(percent);
        }

        // 函数级注释：单定时器超时回调。
        // 以 QElapsedTimer 墙钟为准触发所有已到期批次，再按「目标绝对时间 - 已耗时」补偿下一次间隔，避免相对链式计时漂移。
        void onSingleTimerTimeout()
        {
            if (m_schedule.isEmpty() || m_cursor >= m_schedule.size()) {
                stopAndClearSchedule();
                return;
            }

            fireDueItems();

            if (m_cursor < m_schedule.size()) {
                armNextFromElapsed();
                updateProgress();
            } else {
                stopAndClearSchedule();
            }
        }

    public:
        DelayInterface *widget=new DelayInterface();
        std::shared_ptr<VariableData> inData;
        QTimer *timer=new QTimer();
        QTimer *progressTimer = new QTimer();

    private:
        QElapsedTimer m_elapsed;
        int m_totalDurationMs = 0;

        // 函数级注释：停止定时器并清空队列状态。
        void stopAndClearSchedule()
        {
            if (timer->isActive()) {
                timer->stop();
            }
            if (progressTimer->isActive()) {
                progressTimer->stop();
            }
            m_schedule.clear();
            m_cursor = 0;
            m_totalDurationMs = 0;
            m_elapsed.invalidate();
            if (widget) {
                widget->setProgressIdle();
            }
        }

        qint64 elapsedMs() const
        {
            return m_elapsed.isValid() ? m_elapsed.elapsed() : 0;
        }

        // 发出所有「绝对时间 <= 当前墙钟」的批次（含事件循环卡顿时的追赶）
        void fireDueItems()
        {
            const qint64 now = elapsedMs();
            while (m_cursor < m_schedule.size()) {
                const int dueTime = std::max(0, m_schedule[m_cursor].time);
                if (dueTime > now) {
                    break;
                }
                while (m_cursor < m_schedule.size()
                       && std::max(0, m_schedule[m_cursor].time) == dueTime) {
                    Q_EMIT dataUpdated(std::max(0, m_schedule[m_cursor].port));
                    ++m_cursor;
                }
            }
        }

        // 按墙钟剩余时间武装下一次单次定时；已到期则 interval=0 尽快触发
        void armNextFromElapsed()
        {
            fireDueItems();

            if (m_cursor >= m_schedule.size()) {
                stopAndClearSchedule();
                return;
            }

            const int nextAbs = std::max(0, m_schedule[m_cursor].time);
            const int waitMs = std::max(0, static_cast<int>(nextAbs - elapsedMs()));
            timer->setInterval(waitMs);
            timer->start();
        }

        // 单定时器的顺序执行队列与游标（time 为相对触发起点的绝对毫秒）
        QVector<delay_item> m_schedule;
        int m_cursor = 0;
    };
}