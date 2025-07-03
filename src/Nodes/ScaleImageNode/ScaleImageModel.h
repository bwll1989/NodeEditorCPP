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
using namespace NodeDataTypes;
namespace Nodes
{
    class ScaleImageModel final : public QtNodes::NodeDelegateModel {
        Q_OBJECT

    public:
        ScaleImageModel() {
            InPortCount =3;
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
            case 2:
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
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {

            switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex)
                {
            case 0:
                    return "Image";
            case 1:
                    return "Width";
            case 2:
                    return "Height";
                }
            case QtNodes::PortType::Out:
                return "Image";
            default:
                break;
            }
            return "";
        }
        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override {

            switch (portIndex) {
            case 0:
                m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
                if (m_inImageData.lock()) {
                    requestProcess();
                }
                break;
            case 1:

                m_ui->widthSpinBox->setValue(std::dynamic_pointer_cast<VariableData>(nodeData)->getMap()["default"].toInt());
                break;
            case 2:

                m_ui->heightSpinBox->setValue(std::dynamic_pointer_cast<VariableData>(nodeData)->getMap()["default"].toInt());
                break;
            default:
                break;
            }


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
                connect(m_ui->widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this,
                    [this](int w) {
                        m_inScaleFactor.setWidth(w);
                        requestProcess(); // 触发图像处理
                    });

                // 保持对称性，对heightSpinBox也添加相同逻辑
                connect(m_ui->heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this,
                    [this](int h) {
                        m_inScaleFactor.setHeight(h);
                        requestProcess();
                    });
                registerOSCControl("/aspectRatio",m_ui->cb_aspectRatio);
                registerOSCControl("/Height",m_ui->heightSpinBox);
                registerOSCControl("/Width",m_ui->widthSpinBox);
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
            if (lockImage ) {
                const auto image = lockImage->image();

                if (image.isNull() ) {
                    m_outImageData.reset();
                    return;
                }
                // get aspect ratio mode from the current index of the combobox
                const auto mode = static_cast<Qt::AspectRatioMode>(m_ui->cb_aspectRatio->currentIndex());
                const auto [fst, snd] = processImage(image, m_inScaleFactor, mode);
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
        QSize m_inScaleFactor;
        // out
        // 0
        std::shared_ptr<ImageData> m_outImageData;
    };
}
#endif //SCALEIMAGEMODEL_H
