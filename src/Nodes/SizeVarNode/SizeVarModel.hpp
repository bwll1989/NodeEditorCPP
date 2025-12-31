//
// Created by pablo on 2/29/24.
//

#ifndef SIZEVARMODEL_H
#define SIZEVARMODEL_H

#include <QtNodes/NodeDelegateModel>

#include "ConstantDefines.h"
#include "OSCMessage.h"
#include "DataTypes/NodeDataList.hpp"
#include "ui_SizeVarForm.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
namespace Ui {
    class SizeVarForm;
}
using namespace NodeDataTypes;
namespace Nodes
{
    class SizeVarModel final : public AbstractDelegateModel {
        Q_OBJECT
    public:
        SizeVarModel() {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Size Source";
            WidgetEmbeddable= false;
            PortEditable= false;
            Resizable= true;
            m_ui->setupUi(m_widget);
            connect(m_ui->sb_width, &QLineEdit::textChanged, this ,&SizeVarModel::updateOutput);
            connect(m_ui->sb_height, &QLineEdit::textChanged, this ,&SizeVarModel::updateOutput);
            AbstractDelegateModel::registerOSCControl("/width",m_ui->sb_width);
            AbstractDelegateModel::registerOSCControl("/height",m_ui->sb_height);

        };

        ~SizeVarModel() override{}



        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
            return VariableData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {

            switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex)
                {
                    case 0:
                            return "Width";
                    case 1:
                            return "Height";
                }
            case QtNodes::PortType::Out:
                return "SIZE";
            default:
                break;
            }
            return "";
        }

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
            m_inSizeData = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (auto sizeData = m_inSizeData.lock())
            {
                switch (portIndex)
                {
                case 0:
                    m_ui->sb_width->setText(sizeData->value().toString());
                    break;
                case 1:
                    m_ui->sb_height->setText( sizeData->value().toString());
                    break;
                }
            }
        }

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override {
            switch (port) {
            case 0:
                m_outData=std::make_shared<VariableData>(QVariant(m_outSize));
                return m_outData;
            default:
                return nullptr;
            }
        }

        QWidget* embeddedWidget() override{
            return m_widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["width"] = m_ui->sb_width->text();
            modelJson1["height"] = m_ui->sb_height->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["size"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["size"];
            if (!v.isUndefined()&&v.isObject()) {
                //            button->setChecked(v["val"].toBool(false));
                m_ui->sb_width->setText(v["width"].toString());
                m_ui->sb_height->setText(v["height"].toString());
            }
        }

    private slots:
        void updateOutput() {
            // updates the size from incoming data
           m_outSize = QSizeF(m_ui->sb_width->text().toFloat(), m_ui->sb_height->text().toFloat());
            emit dataUpdated(0);
        }

    private:
        std::unique_ptr<Ui::SizeVarForm> m_ui=std::make_unique<Ui::SizeVarForm>();
        QWidget* m_widget = new QWidget();
        std::weak_ptr<VariableData> m_inSizeData;
        // 0
        QSizeF m_outSize;
        // 1
        std::shared_ptr<VariableData> m_outData;

    };
}

#endif //SIZEVARMODEL_H
