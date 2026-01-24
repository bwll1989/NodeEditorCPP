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
#include <opencv2/imgproc.hpp>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>
namespace Ui {
    class ScaleImageForm;
}
using namespace NodeDataTypes;
namespace Nodes
{
    class ScaleImageModel final : public AbstractDelegateModel {
        Q_OBJECT
        Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
        Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

    public:
        ScaleImageModel() {
            InPortCount =4;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Scale Image";
            WidgetEmbeddable=false;
            Resizable=false;
            PortEditable=false;

            m_widget = new QWidget();
            m_ui->setupUi(m_widget);

            connect(m_ui->widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                               this,
                               [this](int w) {
                                   setWidth(w);
                               });

            // 保持对称性，对heightSpinBox也添加相同逻辑
            connect(m_ui->heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this,
                [this](int h) {
                    setHeight(h);
                });
            AbstractDelegateModel::registerExternalControl("/Height",m_ui->heightSpinBox);
            AbstractDelegateModel::registerExternalControl("/Width",m_ui->widthSpinBox);

            m_width = m_ui->widthSpinBox->value();
            m_height = m_ui->heightSpinBox->value();
            m_inScaleFactor = QSize(m_width, m_height);
        };

        ~ScaleImageModel() override {
            delete m_ui;
            delete m_widget;
        }

        int width() const { return m_width; }
        int height() const { return m_height; }

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
                case 3:
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
                    return "SIZE";
            case 2:
                    return "Width";
            case 3:
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

                if (auto data=std::dynamic_pointer_cast<VariableData>(nodeData))
                {
                    m_inScaleFactor=data->value().toSize();
                    setWidth(m_inScaleFactor.width());
                    setHeight(m_inScaleFactor.height());
                }
                break;
            case 2:
                if (auto data=std::dynamic_pointer_cast<VariableData>(nodeData))
                {
                    setWidth(data->value().toInt());
                }
                break;
            case 3:
                if (auto data=std::dynamic_pointer_cast<VariableData>(nodeData))
                {
                    setHeight(data->value().toInt());
                }
                break;
            default:
                break;
            }


        }

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override {
            return m_outImageData;
        }

        QWidget* embeddedWidget() override {

            return m_widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["width"] = m_width;
            modelJson1["height"] = m_height;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["size"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["size"];
            if (!v.isUndefined()&&v.isObject()) {
                setWidth(v["width"].toInt());
                setHeight(v["height"].toInt());
            }
        }

    public slots:
        void setWidth(int w)
        {
            if (m_width == w) return;
            m_width = w;
            m_inScaleFactor.setWidth(w);
            if (m_ui && m_ui->widthSpinBox) {
                const QSignalBlocker blocker(m_ui->widthSpinBox);
                m_ui->widthSpinBox->setValue(w);
            }
            requestProcess();
            Q_EMIT widthChanged(w);
            AbstractDelegateModel::stateFeedBack("/Width", w);
        }

        void setHeight(int h)
        {
            if (m_height == h) return;
            m_height = h;
            m_inScaleFactor.setHeight(h);
            if (m_ui && m_ui->heightSpinBox) {
                const QSignalBlocker blocker(m_ui->heightSpinBox);
                m_ui->heightSpinBox->setValue(h);
            }
            requestProcess();
            Q_EMIT heightChanged(h);
            AbstractDelegateModel::stateFeedBack("/Height", h);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            const QString addrWidth = makeFullOscAddress("/Width");
            const QString addrHeight = makeFullOscAddress("/Height");
            if (ev.address == addrWidth) {
                setWidth(ev.payload.toInt());
            } else if (ev.address == addrHeight) {
                setHeight(ev.payload.toInt());
            }
        }

    signals:
        void widthChanged(int width);
        void heightChanged(int height);
    private:
         static QPair<cv::Mat, quint64> processImage(const cv::Mat& inputImage, const QSize& scaleFactor) {
            QElapsedTimer timer;
            timer.start();

            if (inputImage.empty() || scaleFactor.width() <= 0 || scaleFactor.height() <= 0) {
                return {cv::Mat(), 0};
            }

            // 转换Qt的缩放模式到OpenCV插值方式
            int interpolation = cv::INTER_LINEAR;
            cv::Mat outputImage;
            try {
                cv::resize(inputImage, outputImage,
                          cv::Size(scaleFactor.width(), scaleFactor.height()),
                          0, 0, interpolation);
            } catch (const cv::Exception& e) {
                qWarning() << "OpenCV resize error:" << e.what();
                return {cv::Mat(), 0};
            }

            return {outputImage, static_cast<quint64>(timer.elapsed())};
        }

        void requestProcess() {
            const auto lockImage = m_inImageData.lock();
            if (lockImage) {
                const auto cvImage = lockImage->imgMat(); // 获取OpenCV矩阵

                if (cvImage.empty()) {
                    m_outImageData.reset();
                    return;
                }


                const auto [processedMat, elapsedTime] = processImage(cvImage, m_inScaleFactor);

                if (!processedMat.empty()) {
                    m_outImageData = std::make_shared<ImageData>(processedMat);
                } else {
                    m_outImageData.reset();
                }

                if (m_ui) {
                    m_ui->sb_time->setValue(elapsedTime);
                }
            } else {
                m_outImageData.reset();
            }
            emit dataUpdated(0);
        }

    private:
        QWidget* m_widget = nullptr;
        Ui::ScaleImageForm* m_ui =new Ui::ScaleImageForm();
        // in
        // 0
        std::weak_ptr<ImageData> m_inImageData;
        // 1
        QSize m_inScaleFactor;
        // out
        // 0
        std::shared_ptr<ImageData> m_outImageData;
        int m_width = 0;
        int m_height = 0;

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/Width"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/Height"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }
    };
}
#endif //SCALEIMAGEMODEL_H
