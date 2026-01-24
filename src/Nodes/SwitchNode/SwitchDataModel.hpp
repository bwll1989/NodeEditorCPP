#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QtWidgets/QSpinBox>
#include <QtCore/qglobal.h>

#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
class QPushButton;
using namespace NodeDataTypes;
namespace Nodes
{
    class SwitchDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        Q_PROPERTY(int index READ getIndex WRITE setIndex NOTIFY indexChanged)

    public:

        /**
         * @brief 构造函数：初始化端口与UI，并连接属性系统
         */
        SwitchDataModel(){
            InPortCount =3;
            OutPortCount=1;
            CaptionVisible=true;
            Caption= PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= true;
            widget->setFixedSize(100,24);
            connect(widget,&IntDragValueWidget::valueChanged, this, [this](int v){ setIndex(v); });
            AbstractDelegateModel::registerExternalControl("switch",widget);
        }
        ~SwitchDataModel(){    }

    public:

        /**
         * @brief 在模型准备就绪后订阅事件总线
         */
        void afterModelReady() override {
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/index"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();

        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return in_dictionary[m_index];
        }
        /**
         * @brief 设置输入数据：端口2为索引，其他端口为数据
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{

            if (data== nullptr){
                return;
            }

            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if(portIndex==InPortCount-1){
                setIndex(textData->value().toInt());
            }
            else{
                in_dictionary[portIndex]=std::dynamic_pointer_cast<VariableData>(textData);
                for(unsigned int i = 0; i < OutPortCount; ++i){
                    Q_EMIT dataUpdated(i);
                }
            }

        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                if (portIndex==InPortCount-1){
                    return "INDEX";
                }
                return "INPUT "+QString::number(portIndex);
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = m_index;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                setIndex(v["val"].toInt());
            }
        }

        QWidget *embeddedWidget() override{return widget;}

    private Q_SLOTS:

        /**
         * @brief 全局事件回调：处理/index
         */
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "index") {
                setIndex(ev.payload.toInt());
            }
        }

        /**
         * @brief 索引编辑回调（保留）：触发输出更新
         */
        void onIndexEdited(int const &string)
        {
            for(unsigned int i = 0; i < OutPortCount; ++i){
                Q_EMIT dataUpdated(i);
            }
        }

    private:

        /**
         * @brief 获取索引属性
         */
        int getIndex() const { return m_index; }
        /**
         * @brief 设置索引属性并同步UI与反馈
         */
        void setIndex(int value) {
            if (m_index == value) return;
            m_index = value;
            if (widget) {
                QSignalBlocker blocker(widget);
                widget->setValue(value);
            }
            emit indexChanged(value);
            AbstractDelegateModel::stateFeedBack("/index", value);
            for(unsigned int i = 0; i < OutPortCount; ++i){
                Q_EMIT dataUpdated(i);
            }
        }

        IntDragValueWidget *widget=new IntDragValueWidget();
        std::unordered_map<unsigned int,  std::shared_ptr<VariableData>> in_dictionary;
        int m_index{0};

    Q_SIGNALS:
        void indexChanged(int value);

    };
}
