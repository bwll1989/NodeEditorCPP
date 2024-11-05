//
// Created by pablo on 3/9/24.
//

#ifndef CAMERAMODEL_H
#define CAMERAMODEL_H


#include <QtNodes/NodeDelegateModel>
#include <QFutureWatcher>

#include "DataTypes/NodeDataList.hpp"
#include "ui_CameraForm.h"

#include <QCamera>
#include <QCameraDevice>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>
#include <QMediaDevices>
namespace Ui {
    class CameraForm;
}

class QLabel;
class QCamera;
class QComboBox;
class QVideoFrame;
class QMediaCaptureSession;
class QVideoSink;
class QCameraDevice;

class CameraModel final : public QtNodes::NodeDelegateModel {
    Q_OBJECT

public:
    CameraModel() {
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Camera";
        WidgetEmbeddable=true;
        Resizable=false;
        setCamera(QMediaDevices::defaultVideoInput());
    }

    ~CameraModel() override{}


    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{  switch (portType) {
        case QtNodes::PortType::In:
            return ImageData().type();
        case QtNodes::PortType::Out:
            return ImageData().type();
        default:
            return ImageData().type();
    }}

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{}

    std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override {
        return m_outImageData;
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            m_ui.reset(new Ui::CameraForm);
            m_widget = new QWidget();
            m_ui->setupUi(m_widget);
            // cb_devices QComboBox
            updateCameras();
        }
        return m_widget;
    }


private slots:
    void onDeviceChanged(int index){
        const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
        if (index < devices.size()) {
            setCamera(devices[index]);
        }}

    void updateCameras() {
        if (m_ui) {
            m_ui->cb_devices->clear();
            const QList<QCameraDevice> devices = QMediaDevices::videoInputs();
            if (devices.isEmpty()) {
                m_ui->cb_devices->addItem("No video input devices available");
                return;
            }

            // 遍历所有可用的视频输入设备
            for (const QCameraDevice& device : devices) {
                m_ui->cb_devices->addItem(device.description());
            }
        }
        connect(m_ui->cb_devices, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    &CameraModel::onDeviceChanged);
    }

    void setCamera(const QCameraDevice& device) {
        // 创建摄像头实例
        m_camera.reset(new QCamera(device));

        // 创建捕获会话
        m_captureSession.reset(new QMediaCaptureSession(this));

        // 将摄像头设置到捕获会话
        m_captureSession->setCamera(m_camera.data());

        // 创建视频接收器
        m_videoSink.reset(new QVideoSink(m_captureSession.get()));

        // 将视频接收器设置到捕获会话
        m_captureSession->setVideoSink(m_videoSink.data());

        // 连接视频帧变化信号
        connect(m_videoSink.data(), &QVideoSink::videoFrameChanged, this, &CameraModel::onFrameAvailable);

        // 启动摄像头
        m_camera->start();
    }

    void onFrameAvailable(const QVideoFrame& frame) {
        QVideoFrame copy(frame);
        if (copy.map(QVideoFrame::ReadOnly)) {
            const QImage image = copy.toImage();
            m_outImageData = std::make_shared<ImageData>(image);
            emit dataUpdated(0);
            // update ui label with image
            if (m_ui) {
                m_ui->cb_takingFrame->setChecked(true);
            }
        }
        else {
            m_outImageData.reset();
            emit dataUpdated(0);
            if (m_ui) {
                m_ui->cb_takingFrame->setChecked(false);
            }
        }
    }

private:
    QWidget* m_widget = nullptr;
    QScopedPointer<Ui::CameraForm> m_ui;
    QScopedPointer<QCamera> m_camera;
    QScopedPointer<QMediaCaptureSession> m_captureSession;
    QScopedPointer<QVideoSink> m_videoSink;

    // out
    std::shared_ptr<ImageData> m_outImageData;
};


#endif //CAMERAMODEL_H
