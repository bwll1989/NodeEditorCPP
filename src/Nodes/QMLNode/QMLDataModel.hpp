#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "z_libpd.h"
#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include "QTimer"
#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#include "QSpinBox"
#include "QMLInterface.hpp"
#include "QtMath"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.

class QMLDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    QMLDataModel()
    {
        InPortCount =3;
        OutPortCount=3;
        CaptionVisible=true;
        Caption=PLUGIN_NAME;
        WidgetEmbeddable= false;
        Resizable=false;
        PortEditable= false;
//        libpd_init();
//        libpd_init_audio(2, 2, 44100);
//        libpd_set_printhook(QMLDataModel::pdPrint);
////        patch=libpd_openfile("test_libpd.pd", ".");

    }

    virtual ~QMLDataModel() override{
//        if (patch) {
//            libpd_closefile(patch);
//            patch = nullptr;
//        }
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
    QMLInterface *widget=new QMLInterface();
    shared_ptr<VariantData> inData;
    void *patch;
private:
    static void pdPrint(const char *message)
    {
        // Use qDebug to output the message
        qDebug() << "[libpd]:" << message;
    }
};
