#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QPushButton>
#include <QtCore/QTimer>

#include "DataTypes/NodeDataList.hpp"
#include "DataTypes/VariableData.h"
#include <QtNodes/NodeDelegateModel>
#include "AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "Common/GUI/Elements/TriggerWidget/TriggerWidget.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::ConnectionPolicy;
using namespace NodeDataTypes;

namespace Nodes {
    /**
     * @brief Trigger触发器节点
     * 
     * 实现Trigger功能：
     * - 只有一个按钮，点击即触发
     * - 输出端口发送信号（通常为true）
     * - 支持输入端口触发
     * - 支持全局事件总线控制 (/trigger)
     * - 支持自定义按钮标签 (label属性)
     */
    class TriggerSourceDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        TriggerSourceDataModel() : button(new TriggerWidget("Trigger")) {
            InPortCount = 1;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Trigger Source";
            WidgetEmbeddable = true;
            Resizable = false;
            button->setMinimumWidth(80);
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "value";
            AbstractDelegateModel::registerExternalBinding("/trigger", this, binding);
            // AbstractDelegateModel::registerExternalControl("/trigger", button);
            
            // 点击按钮触发
            connect(button, &TriggerWidget::clicked, this, [this]() { setTrigger(); });
            connect(this, &TriggerSourceDataModel::triggerChanged, [=](bool trigger){
                    QSignalBlocker blocker(button);
                    stateFeedBack("/trigger", trigger);
                    button->trigger();
                    stateFeedBack("/trigger", !trigger);
            });
        }

        ~TriggerSourceDataModel() override {}

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (m_trigger){
                m_trigger = false;
                return std::make_shared<VariableData>(!m_trigger);
            }
                return std::make_shared<VariableData>(m_trigger);


        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (!data) return;
            
            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (varData) {
                // 输入端口收到true时触发
                if (varData->value().toBool()) {
                    setTrigger();
                }
            }
        }

        QWidget *embeddedWidget() override { return button; }


    signals:
        void triggerChanged(bool trigger);

    protected:
        void afterModelReady() override
        {
            // 订阅 /trigger 命令
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/trigger"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private slots:
        /**
         * @brief 执行触发操作
         */

        void setTrigger()
        {
            m_trigger=true;
            emit triggerChanged(true);
            Q_EMIT dataUpdated(0);

            // QTimer::singleShot(10, this, [this](){
            //     m_trigger = false;
            //     emit triggerChanged(m_trigger);
            //     Q_EMIT dataUpdated(0);
            // });
        }

        /**
         * @brief 处理全局事件
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind == GlobalEventKind::Command) {
                QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
                if (localPath == "trigger" && ev.payload.toBool()==true)  {
                    setTrigger();
                }
            }
        }

    private:
        TriggerWidget *button;
        bool m_trigger = false;
    };
}
