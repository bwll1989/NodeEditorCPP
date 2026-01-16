#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "DAWControllerInterface.hpp"
#include <iostream>
#include <qcryptographichash.h>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>

#include "ConstantDefines.h"

#include "Common/Devices/FTDAWController/FTDAWController.h"
#include "QMutex"
#include "PluginDefinition.hpp"
#include  "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class DAWControllerNode : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化PJLink节点
         */
        DAWControllerNode(){
            // 设置端口数量和基本属性
            InPortCount = 2;  //
            OutPortCount = 1; // 输出端口：STATUS (包含连接状态、最后消息、时间戳)
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化数据
            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();

            m_controller = FTDAWController::acquire();
            AbstractDelegateModel::registerOSCControl("/send",widget->customCommandButton);
            AbstractDelegateModel::registerOSCControl("/host",widget->hostEdit);
            AbstractDelegateModel::registerOSCControl("/command",widget->customCommandLineEdit);

            connect(widget->hostEdit, &QLineEdit::editingFinished, this, &DAWControllerNode::hostChange);
            
            connect(widget->customCommandButton, &QPushButton::clicked, [=](){
                if (m_controller) {
                    QString cmd = widget->customCommandLineEdit->text();
                    sendToController(cmd);
                }
            });


            if (m_controller) {
                connect(m_controller, &FTDAWController::isReady, this, [=](bool ready){
                    widget->updateConnectionStatus(ready);
                    m_outData->insert("connected", ready);
                    Q_EMIT dataUpdated(0);
                });
                
                connect(m_controller, &FTDAWController::onHostChanged, this, [=](const QString &host){
                    if(widget->hostEdit->text() != host){
                        widget->hostEdit->setText(host);
                    }
                });
            }
        }
        
        /**
         * @brief 析构函数，清理资源
         */
        ~DAWControllerNode(){
            if (m_controller) {
                FTDAWController::release();
                m_controller = nullptr;
            }
        }

    public:
        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                    switch (portIndex) {
                        case 0:
                            return "Command";
                                break;
                        case 1:
                            return "Trigger";
                                break;
                    }
                break;
            case PortType::Out:
                return "STATUS";
                break;
            default:
                break;
            }
            return "";
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
         * @brief 获取输出数据
         * @param portIndex 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex)
            {
            case 0:
                return m_outData;  // 包含所有状态信息的Map
            default:
                return nullptr;
            }
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (!varData) return;

            if (portIndex == 0) {
                // 设置命令
                widget->customCommandLineEdit->setText(varData->value().toString());
                sendToController(varData->value().toString());
            }
            else if (portIndex == 1) {
                // 触发发送 (这里简化逻辑，只要收到非空数据就触发，或者根据具体需求判断)
                // 只有当有命令时才发送

                if (varData->value().toBool()&&!widget->customCommandLineEdit->text().isEmpty()) {
                     sendToController(widget->customCommandLineEdit->text());
                }
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

        /**
     * @brief 保存节点配置
     * @return JSON对象
     */
    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["IP"] = widget->hostEdit->text();
        modelJson1["Command"] = widget->customCommandLineEdit->text();
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }
    
    /**
     * @brief 加载节点配置
     * @param p JSON对象
     */
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            widget->hostEdit->setText(v["IP"].toString());
            widget->customCommandLineEdit->setText(v["Command"].toString());
            hostChange();
        }
    }

    public slots:

        /**
         * @brief 主机或端口改变时重新连接
         */
        void hostChange()
        {
            if (m_controller) {
                m_controller->connectToServer(widget->hostEdit->text());
            }
        }

    private:
        void sendToController(const QString& cmd)
        {
            if (m_controller) {
                m_controller->sendMessage(cmd);
                m_outData->insert("connected", true);
                m_outData->insert("last_message", cmd);
                m_outData->insert("timestamp", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"));
                Q_EMIT dataUpdated(0);
            }
        }

    private:
        DAWControllerInterface *widget = new DAWControllerInterface();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;           // 输出数据 (Map类型)
        FTDAWController* m_controller = nullptr;

    };

}

