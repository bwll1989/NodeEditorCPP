#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QtConcurrent/QtConcurrent>
#include <QAbstractScrollArea>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
struct GlobalEvent;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;
namespace Nodes
{
    class ImageSwitchDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)

        public:
        ImageSwitchDataModel()
        {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= true;
            widget->setMinimum(0);
            in_dictionary=std::unordered_map<unsigned int,  std::shared_ptr<ImageData>>();
            m_index = widget->value();
            widget->setFixedSize(100,24);
            connect(widget, QOverload<int>::of(&IntDragValueWidget::valueChanged),
                    this, &ImageSwitchDataModel::setIndex);
            AbstractDelegateModel::registerExternalControl("/index",widget);

        }

        virtual ~ImageSwitchDataModel() override{}

        /**
         * @brief 获取当前图像索引
         * @return 索引值
         */
        int index() const { return m_index; }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                if (portIndex==InPortCount-1){
                    return "INDEX";
                }
                return "IMAGE "+QString::number(portIndex);
            case PortType::Out:
                return "IMAGE "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            // Q_UNUSED(portIndex);
            // Q_UNUSED(portType);
            switch(portType){
            case PortType::In:
                if(portIndex==InPortCount-1){
                    return VariableData().type();
                }
                return  ImageData().type();
            case PortType::Out:
                return ImageData().type();
            default:
                return ImageData().type();
            }
        }



        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            auto it = in_dictionary.find(static_cast<unsigned int>(m_index));
            if (it != in_dictionary.end()) {
                return it->second;
            }
            return nullptr;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (portIndex==InPortCount-1){
                if(data== nullptr){
                    setIndex(0);
                }
                else{
                    auto var = std::dynamic_pointer_cast<VariableData>(data);
                    if (var) {
                        setIndex(var->value().toInt());
                    }
                }
                return;
            }

            if (data== nullptr){
                in_dictionary[static_cast<unsigned int>(portIndex)]=nullptr;
            }
            else{
                in_dictionary[static_cast<unsigned int>(portIndex)]=std::dynamic_pointer_cast<ImageData>(data);
            }
            if (m_index==portIndex){
                for(unsigned int i = 0; i < OutPortCount; ++i){
                    Q_EMIT dataUpdated(i);
                }
            }
        }


        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson1["index"] = m_index;
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {

                setIndex(v.toObject()["index"].toInt());
            }
        }

    public slots:
        /**
         * @brief 设置当前输出图像索引
         * @param idx 索引值
         */
        void setIndex(int idx)
        {
            if (m_index == idx) return;
            m_index = idx;
            if (widget) {
                const QSignalBlocker blocker(widget);
                widget->setValue(idx);
            }
            for(unsigned int i=0;i<OutPortCount;i++){
                Q_EMIT dataUpdated(i);
            }
            Q_EMIT indexChanged(idx);
            AbstractDelegateModel::stateFeedBack("/index", idx);
        }

        /**
         * @brief 处理事件总线命令
         * @param ev 全局事件
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            const QString addrIndex = makeFullOscAddress("/index");
            if (ev.address == addrIndex) {
                setIndex(ev.payload.toInt());
            }
        }

    Q_SIGNALS:
        /**
         * @brief 索引变化信号
         * @param index 新索引
         */
        void indexChanged(int index);

    protected:
        /**
         * @brief 模型准备就绪后订阅事件总线
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/index"),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
        }
    
    private:
        QFutureWatcher<double>* m_watcher = nullptr;
        IntDragValueWidget *widget=new IntDragValueWidget();
        std::unordered_map<unsigned int,  std::shared_ptr<ImageData>> in_dictionary;
        int m_index = 0;
    };
}
