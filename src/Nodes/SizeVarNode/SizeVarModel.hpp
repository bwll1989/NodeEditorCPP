//
// Created by pablo on 2/29/24.
//

#ifndef SIZEVARMODEL_H
#define SIZEVARMODEL_H

#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

using namespace NodeDataTypes;
namespace Nodes
{
    class SizeVarModel final : public AbstractDelegateModel {
        Q_OBJECT
        Q_PROPERTY(double width READ getWidth WRITE setWidth NOTIFY widthChanged)
        Q_PROPERTY(double height READ getHeight WRITE setHeight NOTIFY heightChanged)

    public:
        SizeVarModel() {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Size Source";
            WidgetEmbeddable= false;
            PortEditable= false;
            Resizable= true;

            // UI Setup
            // QGridLayout* gridLayout = new QGridLayout(m_widget);
            // gridLayout->setContentsMargins(0, 0, 0, 0);
            // gridLayout->setSpacing(6);
            // gridLayout->setColumnStretch(0, 1);
            // gridLayout->setColumnStretch(1, 2);
            //
            // QLabel* labelWidth = new QLabel("Width:", m_widget);
            // gridLayout->addWidget(labelWidth, 0, 0);
            //
            // sb_width = new IntDragValueWidget(m_widget);
            // gridLayout->addWidget(sb_width, 0, 1);
            //
            // QLabel* labelHeight = new QLabel("Height:", m_widget);
            // gridLayout->addWidget(labelHeight, 1, 0);
            //
            // sb_height = new IntDragValueWidget(m_widget);
            // gridLayout->addWidget(sb_height, 1, 1);
            //
            // gridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 0, 1, 2);
            // gridLayout->setRowStretch(2, 1);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "width";
                AbstractDelegateModel::registerExternalBinding("/width", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "height";
                AbstractDelegateModel::registerExternalBinding("/height", this, b);
            }
            // AbstractDelegateModel::registerExternalControl("/width", sb_width);
            // AbstractDelegateModel::registerExternalControl("/height", sb_height);

            // // UI -> Property connections
            // connect(sb_width, &IntDragValueWidget::valueChanged, this, [this](){
            //     setWidth(sb_width->value());
            // });
            // connect(sb_height, &IntDragValueWidget::valueChanged, this, [this](){
            //     setHeight(sb_height->value());
            // });

            // // Initial sync
            // sb_width->setValue(m_outSize.width());
            // sb_height->setValue(m_outSize.height());
        };

        ~SizeVarModel() override{}

        void afterModelReady() override {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/width"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/height"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

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
                bool ok = false;
                float val = sizeData->value().toFloat(&ok);
                if (!ok) return;

                switch (portIndex)
                {
                case 0: setWidth(val); break;
                case 1: setHeight(val); break;
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

        // QWidget* embeddedWidget() override{
        //     return m_widget;
        // }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["width"] = m_outSize.width();
            modelJson1["height"] = m_outSize.height();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["size"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["size"];
            if (!v.isUndefined()&&v.isObject()) {
                if (v["width"].isDouble()) setWidth(v["width"].toDouble());
                else if (v["width"].isString()) setWidth(v["width"].toString().toDouble());

                if (v["height"].isDouble()) setHeight(v["height"].toDouble());
                else if (v["height"].isString()) setHeight(v["height"].toString().toDouble());
            }
        }

    public: // Getters and Setters
        qreal getWidth() const { return m_outSize.width(); }
        void setWidth(qreal value) {
            if (qFuzzyCompare(m_outSize.width(), value)) return;
            m_outSize.setWidth(value);
            
            // if (m_widget && sb_width && sb_width->value()!= value) {
            //     QSignalBlocker blocker(sb_width);
            //     sb_width->setValue(value);
            // }
            emit widthChanged(value);
            // AbstractDelegateModel::stateFeedBack("/width", value);
            Q_EMIT dataUpdated(0);
        }

        qreal getHeight() const { return m_outSize.height(); }
        void setHeight(qreal value) {
            if (qFuzzyCompare(m_outSize.height(), value)) return;
            m_outSize.setHeight(value);
            
            // if (m_widget && sb_height && sb_height->value() != value) {
            //     QSignalBlocker blocker(sb_height);
            //     sb_height->setValue(value);
            // }
            emit heightChanged(value);
            // AbstractDelegateModel::stateFeedBack("/height", value);
            Q_EMIT dataUpdated(0);
        }

    signals:
        void widthChanged(qreal value);
        void heightChanged(qreal value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "width") {
                setWidth(ev.payload.toDouble());
            } else if (localPath == "height") {
                setHeight(ev.payload.toDouble());
            }
        }

    private:
        IntDragValueWidget* sb_width = nullptr;
        IntDragValueWidget* sb_height = nullptr;
        // QWidget* m_widget = new QWidget();
        std::weak_ptr<VariableData> m_inSizeData;
        // 0
        QSizeF m_outSize;
        // 1
        std::shared_ptr<VariableData> m_outData;

    };
}

#endif //SIZEVARMODEL_H
