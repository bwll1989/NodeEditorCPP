#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtWidgets/QComboBox>

#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>
#if defined(UNTITLED_LIBRARY)
#  define UNTITLED_EXPORT Q_DECL_EXPORT
#else
#  define UNTITLED_EXPORT Q_DECL_IMPORT
#endif
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{

    class LogicOperationDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int logicMethod READ logicMethod WRITE setLogicMethod NOTIFY logicMethodChanged)

    public:
        LogicOperationDataModel()
        {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Logic Operation";
            WidgetEmbeddable= true;
            Resizable=false;
            PortEditable= false;
            //        method->setStyleSheet("QComboBox{background-color: transparent;}");
            widget->setFixedWidth(80);
            widget->addItems(*methods);
            //        method->setFlat(true);
            val=QVariant(false);
            connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LogicOperationDataModel::setLogicMethod);
            AbstractDelegateModel::registerExternalControl("/method",widget);
        }

        virtual ~LogicOperationDataModel() override{}

        int logicMethod() const { return m_logicMethod; }

    public Q_SLOTS:
        void setLogicMethod(int value)
        {
            if (m_logicMethod == value) return;
            m_logicMethod = value;
            methodChanged();
            Q_EMIT logicMethodChanged(value);
            AbstractDelegateModel::stateFeedBack("/method", m_logicMethod);
            {
                QSignalBlocker blocker(widget);
                widget->setCurrentIndex(value);
            }

        }

    Q_SIGNALS:
        void logicMethodChanged(int value);

    public:
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
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
            return std::make_shared<VariableData>(val);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            if (auto textData = std::dynamic_pointer_cast<VariableData>(data)) {
                in_dictionary[portIndex]=textData->value();
                methodChanged();

            }
        }
        void methodChanged()
        {
            bool tempVal=false;

                switch (m_logicMethod) {
                case 0:
                    tempVal=in_dictionary[0].toString()==in_dictionary[1].toString();
                    break;
                case 1:
                    tempVal=in_dictionary[0].toBool()||in_dictionary[1].toBool();
                    break;
                case 2:
                    tempVal=in_dictionary[0].toString()!=in_dictionary[1].toString();
                    break;
                case 3:
                    tempVal=qMax(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                    break;
                case 4:
                    tempVal=qMin(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                    break;
                case 5:
                    tempVal=in_dictionary[0].toFloat()<in_dictionary[1].toFloat();
                    break;
                case 6:
                    tempVal=in_dictionary[0].toFloat()<=in_dictionary[1].toFloat();
                    break;
                case 7:
                    tempVal=in_dictionary[0].toFloat()>in_dictionary[1].toFloat();
                    break;
                case 8:
                    tempVal=in_dictionary[0].toFloat()>=in_dictionary[1].toFloat();
                    break;
                }
            // qDebug()<<"tempVal:"<<tempVal;
            val = tempVal;
            Q_EMIT dataUpdated(0);
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/method"), this,SLOT(onGlobalEvent(GlobalEvent)));
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson;
            modelJson["method"] = m_logicMethod;
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            if (p.contains("method")) {
                setLogicMethod(p["method"].toInt());
            }
        }
        QWidget *embeddedWidget() override {
            return widget; }
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

    public Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            const QString addr = makeFullOscAddress("/method");
            if (ev.address == addr) {
                setLogicMethod(ev.payload.toInt());
            }
        }
    private:

        QComboBox *widget=new QComboBox();
        QStringList *methods=new QStringList {"AND","OR","NOT","MAX","MIN","<","<=",">",">="};
        std::unordered_map<unsigned int, QVariant> in_dictionary;
        QVariant val;
        int m_logicMethod = 0;
    };
}