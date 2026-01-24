#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QAbstractScrollArea>
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
    class MathOperationDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int mathMethod READ mathMethod WRITE setMathMethod NOTIFY mathMethodChanged)

    public:
        MathOperationDataModel()
        {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Math Operation";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            widget->setFixedWidth(80);
            widget->addItems(*methods);

            val=QVariant(0.0);
            connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MathOperationDataModel::setMathMethod);
            // AbstractDelegateModel::registerExternalControl("/math",widget);
            AbstractDelegateModel::registerExternalControl("/math",widget);
        }

        virtual ~MathOperationDataModel() override{}

        int mathMethod() const { return m_mathMethod; }

    public Q_SLOTS:
        void setMathMethod(int value)
        {
            if (m_mathMethod == value) return;
            m_mathMethod = value;
            methodChanged();
            Q_EMIT mathMethodChanged(value);
            {
                QSignalBlocker blocker(widget);
                widget->setCurrentIndex(value);
            }
            AbstractDelegateModel::stateFeedBack("/math", m_mathMethod);
        }

    Q_SIGNALS:
        void mathMethodChanged(int value);

    public:
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                return "INPUT "+QString::number(portIndex);
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
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



                switch (m_mathMethod) {
                case 0:
                    val=in_dictionary[0].toDouble()+in_dictionary[1].toDouble();
                    break;
                case 1:
                    val=in_dictionary[0].toDouble()-in_dictionary[1].toDouble();
                    break;
                case 2:
                    val=in_dictionary[0].toDouble()*in_dictionary[1].toDouble();
                    break;
                case 3:
                    if (in_dictionary[1].toDouble()==0){
                        val=0;
                        break;
                    }
                    val=in_dictionary[0].toDouble()/in_dictionary[1].toDouble();
                    break;
                case 4:
                    val=fmod(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                    break;
                case 5:
                    val=pow(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                    break;
                }


            Q_EMIT dataUpdated(0);
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/math"), this,SLOT(onGlobalEvent(GlobalEvent)));

        }

        QJsonObject save() const override
        {
            QJsonObject modelJson;
            modelJson["math"] = m_mathMethod;
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            if (p.contains("math")) {
                setMathMethod(p["math"].toInt());
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
    public Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            const QString addr = makeFullOscAddress("/math");
            if (ev.address == addr) {
                setMathMethod(ev.payload.toInt());
            }
        }
    private:

        QComboBox *widget=new QComboBox();
        QStringList *methods=new QStringList {"+","-","*","/","%","^"};
        std::unordered_map<unsigned int, QVariant> in_dictionary;
        QVariant val;
        int m_mathMethod = 0;
    };
}
