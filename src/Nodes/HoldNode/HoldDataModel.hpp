#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include "QTimer"
#include <iostream>
#include <QtCore/qglobal.h>
#include "HoldInterface.hpp"
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    /**
     * @brief 信号保持节点模型
     * 输入值会保持设定时间，时间到达后信号清空
     * 支持两种重复信号处理模式：忽略模式和重置模式
     */
    class HoldDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化信号保持节点
         */
        HoldDataModel()
        {
            InPortCount = 2;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Hold";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            
            // 初始化空的输出数据
            outputData = std::make_shared<VariableData>();
            isHolding = false;
            
            // 设置定时器为单次触发
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, this, &HoldDataModel::holdTimeExpired);
            NodeDelegateModel::registerOSCControl("/time",widget->value);
            NodeDelegateModel::registerOSCControl("/ignoreRepeat",widget->ignoreRepeatCheckBox);
        }

        /**
         * @brief 析构函数，清理资源
         */
        ~HoldDataModel() override
        {
            if(timer->isActive())
            {
                timer->stop();
            }
            timer->deleteLater();
            widget->deleteLater();
        }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "INPUT";
                case 1:
                    return "HOLD_TIME";
                default:
                    return "";
                }
            case PortType::Out:
                return "OUTPUT";
            default:
                return "";
            }
        }

        /**
         * @brief 获取输出数据
         * @param port 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return outputData;
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            
            switch(portIndex)
            {
            case 0: // 输入信号
                handleInputSignal(std::dynamic_pointer_cast<VariableData>(data));
                break;
            case 1: // 保持时间
                {
                    auto timeData = std::dynamic_pointer_cast<VariableData>(data);
                    if (timeData) {
                        int holdTime = timeData->value().toInt();
                        widget->value->setText(QString::number(holdTime));
                    }
                }
                break;
            }
        }

        /**
         * @brief 保存节点状态
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["holdTime"] = widget->value->text().toInt();
            modelJson1["ignoreRepeat"] = widget->ignoreRepeatCheckBox->isChecked();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        /**
         * @brief 加载节点状态
         * @param p JSON对象
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                int holdTime = v["holdTime"].toInt();
                bool ignoreRepeat = v["ignoreRepeat"].toBool();
                widget->value->setText(QString::number(holdTime));
                widget->ignoreRepeatCheckBox->setChecked(ignoreRepeat);
            }
        }

        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
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

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    private slots:
        /**
         * @brief 保持时间到期处理函数
         */
        void holdTimeExpired()
        {
            // 清空输出信号
            outputData = std::make_shared<VariableData>();
            isHolding = false;
            
            // 通知输出数据更新
            Q_EMIT dataUpdated(0);
        }

    private:
        /**
         * @brief 处理输入信号
         * @param inputData 输入数据
         */
        void handleInputSignal(std::shared_ptr<VariableData> inputData)
        {
            if (!inputData) {
                return;
            }
            
            // 获取保持时间
            int holdTime = widget->value->text().toInt();
            if (holdTime <= 0) {
                // 如果保持时间为0或负数，直接输出信号
                outputData = inputData;
                Q_EMIT dataUpdated(0);
                return;
            }
            
            // 检查重复信号处理模式
            bool ignoreRepeat = widget->ignoreRepeatCheckBox->isChecked();
            
            if (isHolding) {
                if (ignoreRepeat) {
                    // 忽略模式：在保持期间忽略新的输入信号
                    return;
                } else {
                    // 重置模式：停止当前计时器，立即处理新信号
                    if (timer->isActive()) {

                        timer->stop();
                    }
                }
            }
            
            // 设置输出数据并开始保持
            outputData = inputData;
            isHolding = true;
            
            // 启动定时器
            timer->setInterval(holdTime);

            timer->start();
            
            // 立即输出信号
            Q_EMIT dataUpdated(0);
        }

    private:
        HoldInterface *widget = new HoldInterface();           // 界面控件
        std::shared_ptr<VariableData> outputData;              // 输出数据
        QTimer *timer = new QTimer(this);                      // 保持时间定时器
        bool isHolding = false;                                // 是否正在保持状态
    };
}