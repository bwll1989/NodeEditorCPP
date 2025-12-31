#pragma once

#include "DataTypes/NodeDataList.hpp"

#include "Common/BuildInNodes/AbstractDelegateModel.h"

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include "QTimer"
#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#include "QSpinBox"
#include "LFOInterface.hpp"
#include "QtMath"
#include "QDebug"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;

using namespace NodeDataTypes;
namespace Nodes
{
    class LFODataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        LFODataModel()
        {
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="LFO";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            connect(timer, &QTimer::timeout, this, &LFODataModel::generateWave);
            connect(widget->method,&QComboBox::currentIndexChanged,this,&LFODataModel::reStart);
            connect(widget->start,&QPushButton::clicked,this,&LFODataModel::reStart);
            // timer->start(1000 / widget->sampleRate->value());
        }

         ~LFODataModel() override{
            if(timer->isActive())
            {
                timer->stop();
            }
            deleteLater();

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
            //        Q_UNUSED(port);
            auto result=std::make_shared<VariableData>(value);
            result->insert("method",widget->method->itemData(widget->method->currentIndex()));
            return  result;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {

            if (data== nullptr){
                return;
            }
            if ((inData = std::dynamic_pointer_cast<VariableData>(data))) {
                if(timer->isActive())
                {
                    timer->stop();
                }
                timer->setInterval(100);
                //            connect(timer,&QTimer::timeout,this,&LFODataModel::delayFinished);
                timer->setSingleShot(true);
                //            定时器单次执行
                timer->start();
            }
        }

        QWidget *embeddedWidget() override{

            return widget;
        }

        double generateSineWave(double m_amplitude,double m_frequency,double m_time,double m_phase) {
            return m_amplitude * qSin(2 * M_PI * m_frequency * m_time + m_phase);
        }

        double generateSquareWave(double m_amplitude,double m_frequency,double m_time,double m_phase) {
            return m_amplitude * (qSin(2 * M_PI * m_frequency * m_time + m_phase) >= 0 ? 1 : -1);
        }

        double generateTriangleWave(double m_amplitude,double m_frequency,double m_time,double m_phase) {
            double period = 1.0 / m_frequency;
            double t = fmod(m_time + m_phase / (2 * M_PI * m_frequency), period);
            double value = (4 * m_amplitude / period) * (t - period / 2.0);
            return (value >= 0 ? 1 : -1) * qAbs(value);
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
    public slots:
        void generateWave() {
            m_amplitude=widget->amplitude->value();
            m_frequency= widget->frequency->value();
            //        double m_time=widget->sampleRate->value();
            m_phase=widget->phase->value();
            WaveType waveType = static_cast<WaveType>(widget->method->itemData(widget->method->currentIndex()).toInt());
            switch (waveType) {
            case SineWave:
                value = generateSineWave(m_amplitude,m_frequency,m_time,m_phase);
                break;
            case SquareWave:
                value = generateSquareWave(m_amplitude,m_frequency,m_time,m_phase);

                break;
            case TriangleWave:
                value = generateTriangleWave(m_amplitude,m_frequency,m_time,m_phase);
                //                qInfo() << "Selected Triangle Wave"<<value;

                break;
            }

            m_time += 1.0 / widget->sampleRate->value();
            emit dataUpdated(0);

        }
        void reStart()
        {
            timer->stop();
            timer->start(1000 / widget->sampleRate->value());
            m_time=0.0;
        }

    public:
        LFOInterface *widget=new LFOInterface();
        shared_ptr<VariableData> inData;
        QTimer *timer=new QTimer();
        double m_frequency;
        double m_amplitude;
        double m_phase;
        double m_sampleRate;
        double m_time=0.0;
        double value;
    };
}