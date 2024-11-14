//
// Created by pablo on 3/9/24.
//

#ifndef CAPTUREMODEL_H
#define CAPTUREMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QFutureWatcher>
#include <QPushButton>
#include "DataTypes/NodeDataList.hpp"

class QPushButton;

class CaptureModel final : public QtNodes::NodeDelegateModel {
    Q_OBJECT

public:
    CaptureModel() {
        InPortCount =2;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Capture";
        WidgetEmbeddable=true;
        Resizable=false;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
        switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex) {
                    case 0:
                        return ImageData().type();
                    case 1:
                        return VariableData().type();
                    }
                    break;
            case QtNodes::PortType::Out:
                return  ImageData().type();
                 break;
            case QtNodes::PortType::None:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return ImageData().type();
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
        switch (portIndex) {
            case 0:
                m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
            case 1:
                m_inData=std::dynamic_pointer_cast<VariableData>(nodeData);
            if (const auto lock = m_inData.lock()) {
               if(m_inData.lock().get()->value().toBool())
                   captureClicked();
            }
            break;
        }
    }

    std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override{
        return m_outImageData;
    }

    QWidget* embeddedWidget() override{
        if (!m_button) {
            m_button = new QPushButton("Capture");
            connect(m_button, &QPushButton::clicked, this, &CaptureModel::captureClicked);
        }
        return m_button;
    }

private slots:
    void captureClicked(){
        if (const auto lock = m_inImageData.lock()) {
            m_outImageData = std::make_shared<ImageData>(lock->image());
        } else {
            m_outImageData.reset();
        }
        emit dataUpdated(0);
    }


private:
    QPushButton* m_button = nullptr;
    // in
    // 0
    std::weak_ptr<ImageData> m_inImageData;
    std::weak_ptr<VariableData> m_inData;
    // out
    // 0
    std::shared_ptr<ImageData> m_outImageData;
};


#endif //CAPTUREMODEL_H
