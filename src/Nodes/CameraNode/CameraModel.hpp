//
// CameraModel.hpp — OpenCV 摄像头采集 → ImageTimestampRingQueue
//
// 数据流：
//   CameraCaptureThread (OpenCV VideoCapture::read)
//     → frameAvailable(cv::Mat)  [QueuedConnection → GUI 线程]
//     → pending 合并 → ImageFrame::fromMat → pushFrame
//   m_outImageData ──共享句柄──→ 下游 Display / ImageOperates
//   下游按 TimestampGenerator tick 调用 getLatestFrame()，无需每帧 dataUpdated
//

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QComboBox>
#include <QDebug>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QMutex>
#include <QPointer>
#include <QScopedPointer>
#include <QThread>
#include <QVariant>

#include <atomic>

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "ui_CameraForm.h"

namespace Ui {
class CameraForm;
}

using namespace NodeDataTypes;

/**
 * @brief 摄像头捕获线程 — OpenCV VideoCapture 在独立线程中 read()
 */
class CameraCaptureThread : public QThread {
    Q_OBJECT

public:
    explicit CameraCaptureThread(QObject* parent = nullptr)
        : QThread(parent)
        , m_deviceIndex(-1)
    {}

    ~CameraCaptureThread() override
    {
        stop();
        wait();
    }

    void startCapture(int deviceIndex)
    {
        if (isRunning()) {
            stop();
            wait();
        }

        QMutexLocker locker(&m_mutex);
        m_deviceIndex = deviceIndex;
        m_running.store(true);
        start(QThread::HighPriority);
    }

    void stop()
    {
        QMutexLocker locker(&m_mutex);
        m_running.store(false);
    }

signals:
    void frameAvailable(const cv::Mat& frame);
    void captureStateChanged(bool isCapturing);

protected:
    void run() override
    {
        cv::VideoCapture capture;
        bool captureOpened = false;

        {
            QMutexLocker locker(&m_mutex);
            try {
                capture.open(m_deviceIndex, cv::CAP_ANY);
                if (capture.isOpened()) {
                    // 请求常见分辨率；实际以设备支持为准
                    capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
                    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
                }
                captureOpened = capture.isOpened();
                emit captureStateChanged(captureOpened);
            } catch (const cv::Exception& e) {
                qWarning() << "OpenCV exception (open camera):" << e.what();
                emit captureStateChanged(false);
                return;
            }
        }

        if (!captureOpened) {
            qWarning() << "Failed to open camera device" << m_deviceIndex;
            return;
        }

        const double actualFps = capture.get(cv::CAP_PROP_FPS);
        qInfo() << "Camera thread started, resolution:"
                << capture.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
                << capture.get(cv::CAP_PROP_FRAME_HEIGHT)
                << "@" << actualFps << "fps";

        while (m_running.load()) {
            cv::Mat frame;
            const bool success = capture.read(frame);

            if (!m_running.load()) {
                break;
            }

            if (success && !frame.empty()) {
                emit frameAvailable(frame.clone());
            } else if (m_running.load()) {
                qWarning() << "Camera read failed";
            }

            const int delayMs = actualFps > 1.0 ? static_cast<int>(500.0 / actualFps) : 16;
            msleep(static_cast<unsigned long>(delayMs));
        }

        if (capture.isOpened()) {
            capture.release();
        }
        emit captureStateChanged(false);
    }

private:
    QMutex m_mutex;
    std::atomic<bool> m_running{false};
    int m_deviceIndex = -1;
};

namespace Nodes
{
class CameraModel final : public AbstractDelegateModel {
    Q_OBJECT

public:
    CameraModel()
    {
        InPortCount = 1;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Camera";
        WidgetEmbeddable = false;
        Resizable = false;

        qRegisterMetaType<cv::Mat>("cv::Mat");

        ensureImageDataBuffer(m_outImageData, m_outputBuffer);
        ImageGpuUpload::instance().warmup();

        m_captureThread = new CameraCaptureThread(this);
        connect(m_captureThread,
                &CameraCaptureThread::frameAvailable,
                this,
                &CameraModel::onFrameAvailable,
                Qt::QueuedConnection);
        connect(m_captureThread,
                &CameraCaptureThread::captureStateChanged,
                this,
                &CameraModel::onCaptureStateChanged,
                Qt::QueuedConnection);
    }

    ~CameraModel() override
    {
        stopCamera(true);
        if (m_captureThread) {
            m_captureThread->stop();
            m_captureThread->wait();
        }
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex);
        return ImageData().type();
    }

    void setInData(std::shared_ptr<QtNodes::NodeData>, const QtNodes::PortIndex portIndex) override
    {
        Q_UNUSED(portIndex);
    }

    std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override
    {
        Q_UNUSED(port);
        return m_outImageData;
    }

    QWidget* embeddedWidget() override
    {
        if (!m_widget) {
            m_ui.reset(new Ui::CameraForm);
            m_widget = new QWidget();
            m_ui->setupUi(m_widget);
            updateCameras();

            if (m_ui->cb_devices->count() > 0
                && m_ui->cb_devices->itemText(0) != "No video input devices available") {
                m_ui->cb_devices->setCurrentIndex(0);
                initializeCamera(0);
            }
        }
        return m_widget;
    }

private slots:
    void onDeviceChanged(int index)
    {
        if (index < 0 || !m_ui) {
            return;
        }

        const QString deviceName = m_ui->cb_devices->itemText(index);
        if (deviceName == "无可用视频输入设备" || deviceName == "枚举摄像头时出错") {
            stopCamera();
            return;
        }

        const QVariant userData = m_ui->cb_devices->itemData(index);
        const int deviceIndex = userData.isValid() ? userData.toInt() : index;

        qInfo() << "Switch camera:" << deviceName << "index" << deviceIndex;
        stopCamera();
        initializeCamera(deviceIndex);
    }

    /** 捕获线程回调：仅缓存最新 Mat，在 GUI 线程合并上传 */
    void onFrameAvailable(const cv::Mat& frame)
    {
        if (frame.empty()) {
            return;
        }

        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingFrame = frame;
        }
        scheduleUploadIfNeeded();
    }

    void onCaptureStateChanged(bool isCapturing)
    {
        if (m_ui && m_widget) {
            m_ui->cb_takingFrame->setChecked(isCapturing);
        }
    }

    void updateCameras()
    {
        if (!m_ui || !m_widget) {
            return;
        }

        disconnect(m_ui->cb_devices,
                   QOverload<int>::of(&QComboBox::currentIndexChanged),
                   this,
                   &CameraModel::onDeviceChanged);

        m_ui->cb_devices->clear();

        const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
        if (cameras.isEmpty()) {
            m_ui->cb_devices->addItem("无可用视频输入设备");
            return;
        }

        for (int i = 0; i < cameras.size(); ++i) {
            m_ui->cb_devices->addItem(cameras.at(i).description(), i);
        }

        connect(m_ui->cb_devices,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                &CameraModel::onDeviceChanged);
    }

    void initializeCamera(int deviceIndex)
    {
        stopCamera();
        m_captureThread->startCapture(deviceIndex);
    }

    void stopCamera(bool fromDestructor = false)
    {
        if (m_captureThread) {
            m_captureThread->stop();
        }

        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingFrame.release();
        }
        m_uploadScheduled.store(false);

        if (m_outputBuffer) {
            m_outputBuffer->clear();
        }

        if (!fromDestructor && m_ui && m_widget) {
            m_ui->cb_takingFrame->setChecked(false);
        }
    }

    void scheduleUploadIfNeeded()
    {
        if (!m_uploadScheduled.exchange(true)) {
            QMetaObject::invokeMethod(this, "publishPendingFrame", Qt::QueuedConnection);
        }
    }

    /** GUI 线程：cv::Mat → GPU 纹理 → push 到 ring buffer */
    void publishPendingFrame()
    {
        m_uploadScheduled.store(false);

        cv::Mat mat;
        {
            QMutexLocker locker(&m_pendingMutex);
            mat = std::move(m_pendingFrame);
        }

        if (!m_outputBuffer || mat.empty()) {
            return;
        }

        ensureImageDataBuffer(m_outImageData, m_outputBuffer);

        const qint64 timestamp = TimestampGenerator::getInstance()->getCurrentFrameCount();
        ImageFrame imageFrame = ImageFrame::fromMat(mat, timestamp);
        if (!imageFrame.texture.valid()) {
            qWarning() << "Camera frame GPU upload failed";
            return;
        }

        m_outputBuffer->pushFrame(std::move(imageFrame));

        bool hasPending = false;
        {
            QMutexLocker locker(&m_pendingMutex);
            hasPending = !m_pendingFrame.empty();
        }
        if (hasPending) {
            scheduleUploadIfNeeded();
        }
    }

private:
    QPointer<QWidget> m_widget;
    QScopedPointer<Ui::CameraForm> m_ui;
    CameraCaptureThread* m_captureThread = nullptr;

    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;

    QMutex m_pendingMutex;
    cv::Mat m_pendingFrame;
    std::atomic<bool> m_uploadScheduled{false};
};
} // namespace Nodes
