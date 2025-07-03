#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "NDVControlInterface.hpp"
#include "Common/Devices/TcpServer/TcpServer.h"
#include <QVariantMap>
#include "QThread"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

const QByteArray COMMAND_PREFIX = QByteArray::fromHex("4A480E0010000000000000004A481000484A0100");
const QByteArray COMMAND_STOP =   QByteArray::fromHex("4A480E0010000000000000004A481000484A0100020000000000000000000000");
const QByteArray COMMAND_NEXT =   QByteArray::fromHex("4A480E0010000000000000004A481000484A0100040000000000000000000000");
const QByteArray COMMAND_PREV =   QByteArray::fromHex("4A480E0010000000000000004A481000484A0100030000000000000000000000");
const QByteArray COMMAND_CLOSE =  QByteArray::fromHex("4A480E0010000000000000004A481000484A0100090000000000000000000000");

namespace Nodes
{
    class NDVControlDataModel : public NodeDelegateModel
    {

    public:
        NDVControlDataModel()
        {
            InPortCount =1;
            OutPortCount=0;
            CaptionVisible=true;
            Caption="NDV Control";
            WidgetEmbeddable=true;
            Resizable=false;
            inData=std::make_shared<VariableData>();

            // 初始化 TcpServer
            mSender = new TcpServer("0.0.0.0", widget->Port->value());

            // 连接界面信号到 TcpServer
            connect(widget->IP, &QLineEdit::textChanged, this, [this](const QString &text) {
                mSender->setHost(text, widget->Port->value());
            });

            connect(widget->Port, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
                mSender->setHost(widget->IP->text(), value);
            });

            // 连接 TcpServer 的信号到数据更新
            connect(mSender, &TcpServer::recMsg, this, &NDVControlDataModel::getOsc);

            // 连接播放和停止按钮
            connect(widget->Play, &QPushButton::clicked, this, [this]() {
                // 构建16进制播放指令
                QByteArray playCommand;
                playCommand.append(COMMAND_PREFIX);
                playCommand.append(static_cast<char>(0x16));
                playCommand.append(static_cast<char>(0x00));
                playCommand.append(static_cast<char>(widget->FileIndex->value())); // 节目ID
                playCommand.append(static_cast<char>(0x00));
                playCommand.append(static_cast<char>(0x00));
                playCommand.append(static_cast<char>(0x00));
                playCommand.append(static_cast<char>(0x00));
                playCommand.append(static_cast<char>(0x00));
                playCommand.append(static_cast<char>(0x00));
                playCommand.append(static_cast<char>(0x00));
                mSender->sendMessage(playCommand);
            });

            connect(widget->Stop, &QPushButton::clicked, this, [this]() {

                mSender->sendMessage(COMMAND_STOP);
            });





            // 连接关闭客户端按钮
            connect(widget->ClosePC, &QPushButton::clicked, this, [this]() {
                mSender->sendMessage(COMMAND_CLOSE);
            });

            connect(widget->LoopPlay, &QPushButton::clicked, this, [this]() {
                QByteArray Command;
                Command.append(COMMAND_PREFIX);
                Command.append(static_cast<char>(0x10));
                Command.append(static_cast<char>(0x00));
                Command.append(static_cast<char>(widget->FileIndex->value())); // 节目ID
                Command.append(static_cast<char>(0x00));
                Command.append(static_cast<char>(0x00));
                Command.append(static_cast<char>(0x00));
                Command.append(static_cast<char>(0x00));
                Command.append(static_cast<char>(0x00));
                Command.append(static_cast<char>(0x00));
                Command.append(static_cast<char>(0x00));
                mSender->sendMessage(Command);
            });

            registerOSCControl("/loop",widget->LoopPlay);
            registerOSCControl("/play",widget->Play);
            registerOSCControl("/stop",widget->Stop);
            registerOSCControl("/close",widget->ClosePC);
            registerOSCControl("/file",widget->FileIndex);
        }

        virtual ~NDVControlDataModel() override {
            delete mSender;
            delete widget;
        }


        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return VariableData().type();
            case PortType::None:
                break;
            default:
                break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return inData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex);
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["IP"] = widget->IP->text();
            modelJson1["Port"] = widget->Port->value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["Address"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["Address"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->IP->setText(v.toObject()["IP"].toString());
                widget->Port->setValue(v.toObject()["Port"].toInt());
                NodeDelegateModel::load(p);
            }
        }
        QWidget *embeddedWidget() override {

            return widget;
        }
    private Q_SLOTS:
        void getOsc(const QVariantMap &data) {
            // inData = std::make_shared<VariableData>();
            // inData->insert("Host", data["Host"]);
            // inData->insert("Data", data["Default"]);
            Q_EMIT dataUpdated(0);
        }

    private:
        std::shared_ptr<VariableData> inData;
        NDVControlInterface * widget = new NDVControlInterface();
        TcpServer * mSender = nullptr;
    };
}