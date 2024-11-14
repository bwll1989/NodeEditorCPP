//
// Created by pablo on 3/5/24.
//

#ifndef SCALEIMAGEMODEL_H
#define SCALEIMAGEMODEL_H


#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include "ui_ScaleImageForm.h"
#include <QElapsedTimer>
#include <QFileDialog>
#include <QtConcurrent/QtConcurrent>
namespace Ui {
    class ScaleImageForm;
}

class ScaleImageModel final : public QtNodes::NodeDelegateModel {
    Q_OBJECT

public:
    ScaleImageModel() {
        InPortCount =2;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Scale Image";
        WidgetEmbeddable=true;
        Resizable=false;
        PortEditable=false;
        this->installEventFilter(this);

    };

    ~ScaleImageModel() override {}

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex) {
                    case 0:
                        return ImageData().type();
                    case 1:
                        return VariableData().type();
                    default:
                        return ImageData().type();
                }
            case QtNodes::PortType::Out:
                switch (portIndex) {
                    case 0:
                        return ImageData().type();
                    default:
                        return ImageData().type();
                }
            default:
                return VariableData().type();
        }
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override {

        switch (portIndex) {
            case 0:
                m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
            break;
            case 1:
                m_inScaleFactor = std::dynamic_pointer_cast<VariableData>(nodeData);
            break;
            default:
                break;
        }
        requestProcess();

    }

    std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override {
        return m_outImageData;
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            m_ui.reset(new Ui::ScaleImageForm);
            m_widget = new QWidget();
            m_ui->setupUi(m_widget);
            // connect combobox cb_aspectRatio to requestProcess
            connect(m_ui->cb_aspectRatio, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScaleImageModel::requestProcess);

        }
        return m_widget;
    }
private:
    static QPair<QImage, quint64> processImage(const QImage& image, const QSize& scaleFactor, Qt::AspectRatioMode mode) {
        QElapsedTimer timer;
        timer.start();
        const auto scaledImage = image.scaled(scaleFactor, mode);
        return {scaledImage, static_cast<quint64>(timer.elapsed())};
    }

    void requestProcess() {
        const auto lockImage = m_inImageData.lock();
        const auto lockScaleFactor = m_inScaleFactor.lock();
        if (lockImage && lockScaleFactor) {
            const auto image = lockImage->image();
            const auto scaleFactor = lockScaleFactor->value().toSize();
            if (image.isNull() || scaleFactor.isEmpty()) {
                m_outImageData.reset();
                return;
            }
            // get aspect ratio mode from the current index of the combobox
            const auto mode = static_cast<Qt::AspectRatioMode>(m_ui->cb_aspectRatio->currentIndex());
            const auto [fst, snd] = processImage(image, scaleFactor, mode);
            m_outImageData = std::make_shared<ImageData>(fst);
            if (m_ui) {
                m_ui->sb_time->setValue(snd);
            }
        } else {
            m_outImageData.reset();
        }
        emit dataUpdated(0);
    }


private:
    QWidget* m_widget = nullptr;
    QScopedPointer<Ui::ScaleImageForm> m_ui;
    // in
    // 0
    std::weak_ptr<ImageData> m_inImageData;
    // 1
    std::weak_ptr<VariableData> m_inScaleFactor;
    // out
    // 0
    std::shared_ptr<ImageData> m_outImageData;
};


#endif //SCALEIMAGEMODEL_H
