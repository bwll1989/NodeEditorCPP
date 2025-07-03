#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include "QTimer"
#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#include "QSpinBox"
#include "HotKeyInterface.hpp"
#include "HotKeyItem.hpp"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class HotKeyDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        HotKeyDataModel()
        {
            InPortCount =0;
            OutPortCount=5;
            CaptionVisible=true;
            Caption="HotKey";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;

            //        connect(this->hotkey_1, &QHotkey::activated,
            //                this->widget, &HotKeyInterface::increase_1);

            connect(this->widget->item_1->hotkey, &QHotkey::activated,
                [this]() { hotkeyChanged(0); });
            connect(this->widget->item_2->hotkey, &QHotkey::activated,
                    [this]() { hotkeyChanged(1); });
            connect(this->widget->item_3->hotkey, &QHotkey::activated,
                    [this]() { hotkeyChanged(2); });
            connect(this->widget->item_4->hotkey, &QHotkey::activated,
                    [this]() { hotkeyChanged(3); });
            connect(this->widget->item_5->hotkey, &QHotkey::activated,
                    [this]() { hotkeyChanged(4); });

        }

        virtual ~HotKeyDataModel() override=default;

    private slots:
        void hotkeyChanged(int port) {
        Q_EMIT dataUpdated(port);
    }

    public:

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
            //        Q_UNUSED(port);
            //
            // da->insert("count",)
            auto item = this->widget->findChild<HotKeyItem*>(QString("item_%1").arg(port + 1));

            auto da=std::make_shared<VariableData>(QVariant(item->boolDisplay->isChecked()));
            da->insert("count",item->boolDisplay->text().toInt());
            return da;
            // auto ptr = std::make_shared<VariableData>(QVariant(this->widget->item_1->boolDisplay->isChecked()));




        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {

            if (data== nullptr){
                return;
            }
            // if ((inData = std::dynamic_pointer_cast<VariableData>(data))) {
            //
            // }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson  = NodeDelegateModel::save();

            QJsonObject modelJson1;
            modelJson1["enable"] =widget->item_1->EnableButton->isChecked();
            modelJson1["value"] = widget->item_1->Editor->keySequence().toString();
            modelJson["hotkey1"]=modelJson1;

            QJsonObject modelJson2;
            modelJson2["enable"] =widget->item_2->EnableButton->isChecked();
            modelJson2["value"] = widget->item_2->Editor->keySequence().toString();
            modelJson["hotkey2"]=modelJson2;

            QJsonObject modelJson3;

            modelJson3["enable"] =widget->item_3->EnableButton->isChecked();
            modelJson3["value"] = widget->item_3->Editor->keySequence().toString();
            modelJson["hotkey3"]=modelJson3;

            QJsonObject modelJson4;
            modelJson4["enable"] =widget->item_4->EnableButton->isChecked();
            modelJson4["value"] = widget->item_4->Editor->keySequence().toString();
            modelJson["hotkey4"]=modelJson4;

            QJsonObject modelJson5;
            modelJson5["enable"] =widget->item_5->EnableButton->isChecked();
            modelJson5["value"] = widget->item_5->Editor->keySequence().toString();
            modelJson["hotkey5"]=modelJson5;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {

            QJsonValue v1 = p["hotkey1"];
            if (!v1.isUndefined()&&v1.isObject()) {
                widget->item_1->Editor->setKeySequence(QKeySequence(v1["value"].toString()));
                widget->item_1->EnableButton->setChecked(v1["enable"].toBool());

            }
            QJsonValue v2 = p["hotkey2"];
            if (!v2.isUndefined()&&v2.isObject()) {
                widget->item_2->Editor->setKeySequence(QKeySequence(v2["value"].toString()));
                widget->item_2->EnableButton->setChecked(v2["enable"].toBool());
            }

            QJsonValue v3 = p["hotkey3"];
            if (!v3.isUndefined()&&v3.isObject()) {
                widget->item_3->Editor->setKeySequence(QKeySequence(v3["value"].toString()));
                widget->item_3->EnableButton->setChecked(v3["enable"].toBool());
            }

            QJsonValue v4 = p["hotkey4"];
            if (!v4.isUndefined()&&v4.isObject()) {
                widget->item_4->Editor->setKeySequence(QKeySequence(v4["value"].toString()));
                widget->item_4->EnableButton->setChecked(v4["enable"].toBool());
            }

            QJsonValue v5 = p["hotkey5"];
            if (!v5.isUndefined()&&v5.isObject()) {
                widget->item_5->Editor->setKeySequence(QKeySequence(v5["value"].toString()));
                widget->item_5->EnableButton->setChecked(v5["enable"].toBool());
            }

        }

        QWidget *embeddedWidget() override{

            return widget;
        }

    private:
        HotKeyInterface *widget=new HotKeyInterface();
    };
}