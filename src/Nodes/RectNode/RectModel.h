//
// Created by pablo on 3/9/24.
//

#ifndef RECTMODEL_H
#define RECTMODEL_H


#include <opencv2/core/types.hpp>
#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include "ui_RectForm.h"
#include <QPointF>
#include <QRectF>
namespace Ui {
    class RectForm;
}
using namespace NodeDataTypes;
namespace Nodes
{
    class RectModel final : public QtNodes::NodeDelegateModel {
        Q_OBJECT

    public:
        RectModel(){
            m_outRect = QRectF(0, 0, 0, 0);
            InPortCount =4;
            OutPortCount=4;
            Caption="Rect";
            CaptionVisible=true;
            WidgetEmbeddable= false;
            Resizable=false;
        }

        ~RectModel() override{}


        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
            return VariableData().type();
        }

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
            // if the input data is a rect, update the m_outRectValue and the ui
            switch (portIndex)
            {
            case 0:
                if (auto rectData = std::dynamic_pointer_cast<VariableData>(nodeData))
                     m_ui->sb_x->setText(QString::number(rectData->value().toFloat()));
                break;
            case 1:
                if (auto rectData = std::dynamic_pointer_cast<VariableData>(nodeData))
                    m_ui->sb_y->setText(QString::number(rectData->value().toFloat()));
                break;
            case 2:
                if (auto rectData = std::dynamic_pointer_cast<VariableData>(nodeData))
                    m_ui->sb_width->setText(QString::number(rectData->value().toFloat()));
                break;
            case 3:
                if (auto rectData = std::dynamic_pointer_cast<VariableData>(nodeData))
                    m_ui->sb_height->setText(QString::number(rectData->value().toFloat()));
                break;
            }
        }

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override{
            switch (port) {
            case 0:
                return std::make_shared<VariableData>(m_outRect);
            case 1:
                return std::make_shared<VariableData>(m_outRect.size());
            case 2:
                return std::make_shared<VariableData>(m_outRect.center());
            case 3:
                return std::make_shared<VariableData>(m_outRect.bottomRight());
            default:
                return std::make_shared<VariableData>();
            }
        }

        QWidget* embeddedWidget() override
        {
            if (!m_widget) {
                m_ui = std::make_unique<Ui::RectForm>();
                m_widget = new QWidget();
                m_ui->setupUi(m_widget);
                connect(m_ui->sb_x, &QLineEdit::textChanged, this, &RectModel::updateRectFromUI);
                connect(m_ui->sb_y, &QLineEdit::textChanged, this, &RectModel::updateRectFromUI);
                connect(m_ui->sb_width, &QLineEdit::textChanged, this, &RectModel::updateRectFromUI);
                connect(m_ui->sb_height, &QLineEdit::textChanged, this, &RectModel::updateRectFromUI);
                NodeDelegateModel::registerOSCControl("/x",m_ui->sb_x);
                NodeDelegateModel::registerOSCControl("/y",m_ui->sb_y);
                NodeDelegateModel::registerOSCControl("/width",m_ui->sb_width);
                NodeDelegateModel::registerOSCControl("/height",m_ui->sb_height);
            }
            return m_widget;
        }


        QString portCaption(QtNodes::PortType port, QtNodes::PortIndex port_index) const override{
            switch (port) {
            case QtNodes::PortType::In:
                switch (port_index) {
                    case 0:
                            return "POS_X";
                    case 1:
                            return "POS_Y";
                    case 2:
                            return "WIDTH";
                    case 3:
                            return "HEIGHT";
                    default:
                            return QString();
                }
            case QtNodes::PortType::Out:
                switch (port_index) {
                    case 0:
                            return "Rect";
                    case 1:
                            return "SIZE";
                    case 2:
                            return "CENTER";
                    case 3:
                            return  "BOTTOM_RIGHT";
                    default:
                        return QString();
                }
            default:
                return NodeDelegateModel::portCaption(port, port_index);
            }
        }
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["x"] = m_ui->sb_x->text();
            modelJson1["y"] = m_ui->sb_y->text();
            modelJson1["width"] = m_ui->sb_width->text();
            modelJson1["height"] = m_ui->sb_height->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["rect"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["rect"];
            if (!v.isUndefined()&&v.isObject()) {
                m_ui->sb_x->blockSignals(true);
                m_ui->sb_y->blockSignals(true);
                m_ui->sb_width->blockSignals(true);
                m_ui->sb_height->blockSignals(true);
                m_ui->sb_x->setText(v["x"].toString());
                m_ui->sb_y->setText(v["y"].toString());
                m_ui->sb_width->setText(v["width"].toString());
                m_ui->sb_height->setText(v["height"].toString());
                m_ui->sb_x->blockSignals(false);
                m_ui->sb_y->blockSignals(false);
                m_ui->sb_width->blockSignals(false);
                m_ui->sb_height->blockSignals(false);
                updateRectFromUI();
            }
        }

    private slots:
        void updateRectFromUI(){
            // take the values from the ui and update the out data
            const float x = m_ui->sb_x->text().toFloat();
            const float y = m_ui->sb_y->text().toFloat();
            const float width = m_ui->sb_width->text().toFloat();
            const float height = m_ui->sb_height->text().toFloat();
            m_outRect = QRectF(x, y, width, height);
            emit dataUpdated(0);
            emit dataUpdated(1);
            emit dataUpdated(2);
            emit dataUpdated(3);
        }



    private:
        QWidget* m_widget = nullptr;
        std::unique_ptr<Ui::RectForm> m_ui;
        // in
        // 0
        std::weak_ptr<VariableData> m_inData;
        //out
        // 0
        QRectF m_outRect;


    };
}
#endif //RECTMODEL_H
