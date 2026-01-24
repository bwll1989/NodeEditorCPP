#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QAbstractScrollArea>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "ImageThresholdInterface.hpp"
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>
struct GlobalEvent;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;
namespace Nodes
{
    class ImageThresholdDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int thresh READ thresh WRITE setThresh NOTIFY threshChanged)
        Q_PROPERTY(int maxval READ maxval WRITE setMaxval NOTIFY maxvalChanged)
        Q_PROPERTY(int method READ method WRITE setMethod NOTIFY methodChanged)

        public:
        ImageThresholdDataModel()
        {
            InPortCount =4;
            OutPortCount=1;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_outImage=std::make_shared<ImageData>();
            connect(widget->threshEdit,&QSpinBox::valueChanged,this,[this](int v){ setThresh(v); });
            connect(widget->maxvalEdit,&QSpinBox::valueChanged,this,[this](int v){ setMaxval(v); });
            connect(widget->methodEdit,&QComboBox::currentIndexChanged,this,[this](int v){ setMethod(v); });

            AbstractDelegateModel::registerExternalControl("/method",widget->methodEdit);
            AbstractDelegateModel::registerExternalControl("/thresh",widget->threshEdit);
            AbstractDelegateModel::registerExternalControl("/maxval",widget->maxvalEdit);

            m_thresh = widget->threshEdit->value();
            m_maxval = widget->maxvalEdit->value();
            m_method = widget->methodEdit->currentIndex();
        }

        virtual ~ImageThresholdDataModel() override{}

        int thresh() const { return m_thresh; }
        int maxval() const { return m_maxval; }
        int method() const { return m_method; }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "IMAGE";
                case 1:
                    return "THRESHOLD";
                case 2:
                    return "MAXVAL";
                case 3:
                    return "METHOD";
                default:
                    return "INPUT "+QString::number(portIndex);
                }
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            // Q_UNUSED(portIndex);
            // Q_UNUSED(portType);
            switch(portType)
            {
            case PortType::In:
                switch (portIndex)
                {
            case 0:
                    return ImageData().type();
            case 1:
                    return VariableData().type();

            default:
                    return VariableData().type();
            };

            case PortType::Out:
                return ImageData().type();
            default:
                return ImageData().type();
            }
        }



        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return m_outImage;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            switch (portIndex)
            {
            case 0:
                m_inImage=std::dynamic_pointer_cast<ImageData>(data);
                updateImage();
                break;
            case 1:
                setThresh(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            case 2:
                setMaxval(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            case 3:
                setMethod(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            }
        }
        void updateImage()
        {
            if (m_inImage) {
                cv::Mat inputMat = m_inImage->imgMat();
                cv::Mat thresholdedMat;

                // 转换为灰度图像（如果输入是彩色）
                if (inputMat.channels() > 1) {
                    cv::cvtColor(inputMat, inputMat, cv::COLOR_BGR2GRAY);
                }

                double thresh = m_thresh;
                double maxval = m_maxval;
                int method = m_method;
                cv::threshold(inputMat, thresholdedMat, thresh, maxval, method);

                m_outImage = std::make_shared<ImageData>(thresholdedMat);
            }
            Q_EMIT dataUpdated(0);
        }
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson  = NodeDelegateModel::save();

            modelJson1["thresh"] = m_thresh;
            modelJson1["maxval"] = m_maxval;
            modelJson1["method"] = m_method;
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                setThresh(v.toObject()["thresh"].toInt());
                setMaxval(v.toObject()["maxval"].toInt());
                setMethod(v.toObject()["method"].toInt());
            }
        }

    private:

        ImageThresholdInterface *widget=new ImageThresholdInterface();
        std::shared_ptr<NodeData> m_outImage;
        std::shared_ptr<ImageData> m_inImage;
        std::shared_ptr<VariableData> m_inVariable;
        int m_thresh = 0;
        int m_maxval = 0;
        int m_method = 0;

    public Q_SLOTS:
        void setThresh(int v)
        {
            if (m_thresh == v) return;
            m_thresh = v;
            if (widget && widget->threshEdit) {
                const QSignalBlocker blocker(widget->threshEdit);
                widget->threshEdit->setValue(v);
            }
            updateImage();
            Q_EMIT threshChanged(v);
            AbstractDelegateModel::stateFeedBack("/thresh", v);
        }

        void setMaxval(int v)
        {
            if (m_maxval == v) return;
            m_maxval = v;
            if (widget && widget->maxvalEdit) {
                const QSignalBlocker blocker(widget->maxvalEdit);
                widget->maxvalEdit->setValue(v);
            }
            updateImage();
            Q_EMIT maxvalChanged(v);
            AbstractDelegateModel::stateFeedBack("/maxval", v);
        }

        void setMethod(int v)
        {
            if (m_method == v) return;
            m_method = v;
            if (widget && widget->methodEdit) {
                const QSignalBlocker blocker(widget->methodEdit);
                widget->methodEdit->setCurrentIndex(v);
            }
            updateImage();
            Q_EMIT methodChanged(v);
            AbstractDelegateModel::stateFeedBack("/method", v);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            const QString addrThresh = makeFullOscAddress("/thresh");
            const QString addrMaxval = makeFullOscAddress("/maxval");
            const QString addrMethod = makeFullOscAddress("/method");
            if (ev.address == addrThresh) {
                setThresh(ev.payload.toInt());
            } else if (ev.address == addrMaxval) {
                setMaxval(ev.payload.toInt());
            } else if (ev.address == addrMethod) {
                setMethod(ev.payload.toInt());
            }
        }

    Q_SIGNALS:
        void threshChanged(int v);
        void maxvalChanged(int v);
        void methodChanged(int v);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/thresh"),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/maxval"),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/method"),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
        }
    };
}
