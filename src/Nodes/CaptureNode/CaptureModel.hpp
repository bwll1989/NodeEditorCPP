//
// Created by pablo on 3/9/24.
//

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QFutureWatcher>
#include <QPushButton>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
class QPushButton;
using namespace QtNodes;
using namespace NodeDataTypes;
struct GlobalEvent;
namespace Nodes
{
    class CaptureModel final : public AbstractDelegateModel {
        Q_OBJECT

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
            m_button = new QPushButton("Capture");
            connect(m_button, &QPushButton::clicked, this, [this](){
                    captureOnce();
                });
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control=m_button;
                AbstractDelegateModel::registerExternalBinding("/capture", this, b);
            }
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
                        captureOnce();
                    }
                }
                break;
            }
        }

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override{
            return m_outImageData;
        }

        QWidget* embeddedWidget() override{
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
            AbstractDelegateModel::stateFeedBack("/capture", true);
            emit dataUpdated(0);
            AbstractDelegateModel::stateFeedBack("/capture", false);

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
                captureOnce();
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

    };
}
