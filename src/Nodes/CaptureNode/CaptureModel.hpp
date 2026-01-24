//
// Created by pablo on 3/9/24.
//

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QFutureWatcher>
#include <QPushButton>
#include "DataTypes/NodeDataList.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
class QPushButton;
using namespace QtNodes;
using namespace NodeDataTypes;
struct GlobalEvent;
namespace Nodes
{
    class CaptureModel final : public AbstractDelegateModel {
        Q_OBJECT
        Q_PROPERTY(bool trigger READ triggerProperty WRITE setTriggerProperty NOTIFY triggerChanged)

    public:
        /**
         * 函数级注释：构造函数，初始化图像捕获节点及触发属性
         */
        CaptureModel() {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Capture";
            WidgetEmbeddable=false;
            Resizable=false;
        }

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex) {
            case 0:
                    return ImageData().type();
            case 1:
                    return VariableData().type();
                }
                break;
            case QtNodes::PortType::Out:
                return  ImageData().type();
                break;
            case QtNodes::PortType::None:
                break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return ImageData().type();
        }
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case QtNodes::PortType::In:
                switch(portIndex)
                {
            case 0:
                    return "IMAGE";
            case 1:
                    return "TRIGGER";
            default:
                    return "";
                }
            case QtNodes::PortType::Out:
                return "IMAGE";
            default:
                return "";
            }

        }
        /**
         * 函数级注释：设置输入端口数据
         * - 端口0：图像输入
         * - 端口1：布尔触发信号，为 true 时触发捕获
         */
        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
            switch (portIndex) {
            case 0:
                m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
                break;
            case 1:
                m_inData=std::dynamic_pointer_cast<VariableData>(nodeData);
                if (const auto lock = m_inData.lock()) {
                    if (lock->value().toBool()) {
                        setTriggerProperty(true);
                    }
                }
                break;
            }
        }

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override{
            return m_outImageData;
        }

        QWidget* embeddedWidget() override{
            if (!m_button) {
                m_button = new QPushButton("Capture");
                AbstractDelegateModel::registerExternalControl("/capture",m_button);
                connect(m_button, &QPushButton::clicked, this, [this](){
                    setTriggerProperty(true);
                });
            }
            return m_button;
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

        /**
         * 函数级注释：获取当前触发属性值
         * @return true 表示请求捕获，false 表示空闲
         */
        bool triggerProperty() const
        {
            return m_trigger;
        }

        /**
         * 函数级注释：设置触发属性，当置为 true 时执行一次图像捕获并自动复位为 false
         */
        void setTriggerProperty(bool trigger)
        {
            if (!trigger) {
                return;
            }
            if (!m_trigger) {
                m_trigger = true;
                Q_EMIT triggerChanged(m_trigger);
                captureOnce();
                m_trigger = false;
                Q_EMIT triggerChanged(m_trigger);
            } else {
                captureOnce();
            }
        }

    Q_SIGNALS:
        /**
         * 函数级注释：触发属性变化信号，用于状态反馈到事件总线
         */
        void triggerChanged(bool trigger);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，实现外部触发控制
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/capture"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        /**
         * 函数级注释：执行一次图像捕获并输出到下游
         */
        void captureOnce(){
            if (const auto lock = m_inImageData.lock()) {
                m_outImageData = std::make_shared<ImageData>(lock->imgMat());
            } else {
                m_outImageData.reset();
            }
            emit dataUpdated(0);
            AbstractDelegateModel::stateFeedBack("/capture", true);
        }

        /**
         * 函数级注释：处理来自全局事件总线的捕获触发命令
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/capture")) {
                return;
            }
            if (ev.payload.toBool()) {
                setTriggerProperty(true);
            }
        }

    private:
        QPushButton* m_button = nullptr;
        // in
        // 0
        std::weak_ptr<ImageData> m_inImageData;
        std::weak_ptr<VariableData> m_inData;
        // out
        // 0
        std::shared_ptr<ImageData> m_outImageData;
        bool m_trigger = false;
    };
}
