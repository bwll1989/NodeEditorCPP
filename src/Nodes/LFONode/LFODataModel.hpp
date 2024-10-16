#pragma once

#include "Nodes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

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
#include "spdlog/spdlog.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class LFODataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    LFODataModel()
    {
        InPortCount =3;
        OutPortCount=3;
        CaptionVisible=true;
        Caption="LFO";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= false;
        connect(timer, &QTimer::timeout, this, &LFODataModel::generateWave);
        connect(widget->method,&QComboBox::currentIndexChanged,this,&LFODataModel::reStart);
        timer->start(1000 / widget->sampleRate->value());
    }

    virtual ~LFODataModel() override{
        if(timer->isActive())
        {
            timer->stop();
        }
        deleteLater();

    }

public:

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "➩";
        QString out = "➩";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }

    unsigned int nPorts(PortType portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
        }
        return result;
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex)
        switch (portType) {
            case PortType::In:
                return VariantData().type();
            case PortType::Out:
                return VariantData().type();
            case PortType::None:
                break;
            default:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return VariantData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
//        Q_UNUSED(port);
        return  inData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {

        if (data== nullptr){
            return;
        }
        if ((inData = std::dynamic_pointer_cast<VariantData>(data))) {
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
                qDebug() << "Selected Sine Wave"<<m_time;
                break;
            case SquareWave:
                value = generateSquareWave(m_amplitude,m_frequency,m_time,m_phase);
                qDebug() << "Selected Square Wave"<<value;
                break;
            case TriangleWave:
                value = generateTriangleWave(m_amplitude,m_frequency,m_time,m_phase);
//                qInfo() << "Selected Triangle Wave"<<value;
                spdlog::debug("Testing debug");
                break;
        }

        m_time += 1.0 / widget->sampleRate->value();
        qWarning() << "Wave value:" << m_time;
    }
    void reStart()
    {
        timer->stop();
        timer->start(1000 / widget->sampleRate->value());
        m_time=0.0;
    }
//    void generateWave(int index) {
//        double value = 0.0;
//        WaveType waveType = static_cast<WaveType>(widget->method->itemData(index).toInt());
//        switch (waveType) {
//            case SineWave:
//                value = generateSineWave();
//                qDebug() << "Selected Sine Wave";
//                break;
//            case SquareWave:
//                value = generateSquareWave();
//                qDebug() << "Selected Square Wave";
//                break;
//            case TriangleWave:
//                value = generateTriangleWave();
//                qDebug() << "Selected Triangle Wave";
//                break;
//        }
//    }

//    void generateSineWave() {
//        double value = m_amplitude * qSin(2 * M_PI * m_frequency * m_time + m_phase);
//        qDebug() << "Sine wave value:" << value;
//        m_time += 1.0 / m_sampleRate;
//    }
public:
    LFOInterface *widget=new LFOInterface();
    shared_ptr<VariantData> inData;
    QTimer *timer=new QTimer();
    double m_frequency;
    double m_amplitude;
    double m_phase;
    double m_sampleRate;
    double m_time=0.0;
    double value;
};
