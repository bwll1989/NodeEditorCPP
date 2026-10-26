//
// CameraModel.hpp — FFmpeg dshow 摄像头采集 → ImageTimestampRingQueue
//
// 数据流：
//   CameraCaptureThread (项目 FFmpeg avdevice/dshow)
//     → frameAvailable(cv::Mat)  [QueuedConnection → GUI 线程]
//     → pending 合并 → ImageFrame::fromMat → pushFrame
//   m_outImageData ──共享句柄──→ 下游 Display / ImageOperates
//   下游按 TimestampGenerator tick 调用 getLatestFrame()，无需每帧 dataUpdated
//   设备名写入 values.device，load() 后自行 startCapture（FlowRuntime 无属性面板）
//

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QComboBox>
#include <QDebug>
#include <QJsonObject>
#include <QMutex>
#include <QPointer>
#include <QScopedPointer>
#include <QSignalBlocker>
#include <QTimer>
#include <QVariant>

#include <atomic>

#include <opencv2/core.hpp>

#include "CameraCapture.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "ui_CameraForm.h"

namespace Ui {
class CameraForm;
}

using namespace NodeDataTypes;

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
        connect(m_captureThread,
                &CameraCaptureThread::errorOccurred,
                this,
                &CameraModel::onCaptureError,
                Qt::QueuedConnection);
    }

    ~CameraModel() override
    {
        stopCamera(true);
        if (m_captureThread) {
            m_captureThread->stop();
            m_captureThread->wait(5000);
        }
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        Q_UNUSED(portType);
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

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        QJsonObject values;
        values[QStringLiteral("device")] = m_deviceOpenName;
        values[QStringLiteral("displayName")] = m_deviceDisplayName;
        modelJson[QStringLiteral("values")] = values;
        return modelJson;
    }

    void load(const QJsonObject& jsonObject) override
    {
        NodeDelegateModel::load(jsonObject);

        const QJsonObject values = jsonObject.value(QStringLiteral("values")).toObject();
        m_deviceOpenName = values.value(QStringLiteral("device")).toString().trimmed();
        m_deviceDisplayName = values.value(QStringLiteral("displayName")).toString().trimmed();

        // 采集不依赖属性面板。FlowRuntime 没有界面，加载后必须自己打开设备。
        QTimer::singleShot(0, this, [self = QPointer<CameraModel>(this)]() {
            if (self) {
                self->ensureCaptureStarted();
            }
        });
    }

    QWidget* embeddedWidget() override
    {
        if (!m_widget) {
            m_ui.reset(new Ui::CameraForm);
            m_widget = new QWidget();
            m_ui->setupUi(m_widget);
            updateCameras();
            ensureCaptureStarted();
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
        if (deviceName == QStringLiteral("无可用视频输入设备")
            || deviceName == QStringLiteral("枚举摄像头时出错")) {
            stopCamera();
            return;
        }

        const QString openName = m_ui->cb_devices->itemData(index).toString();
        m_deviceDisplayName = deviceName;
        m_deviceOpenName = openName.isEmpty() ? deviceName : openName;
        qInfo() << "Switch camera:" << m_deviceDisplayName << "open as" << m_deviceOpenName;
        initializeCamera(m_deviceOpenName);
    }

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

    void onCaptureError(const QString& message)
    {
        qWarning() << "Camera:" << message;
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

        const QList<CameraDeviceInfo> cameras = CameraCaptureThread::listVideoDevices();
        if (cameras.isEmpty()) {
            m_ui->cb_devices->addItem(QStringLiteral("无可用视频输入设备"));
            return;
        }

        for (const CameraDeviceInfo& cam : cameras) {
            // 界面显示友好名，itemData 存 openName 供 dshow 打开
            m_ui->cb_devices->addItem(cam.displayName, cam.openName);
        }

        connect(m_ui->cb_devices,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                &CameraModel::onDeviceChanged);
    }

    void scheduleUploadIfNeeded()
    {
        if (!m_uploadScheduled.exchange(true)) {
            QMetaObject::invokeMethod(this, "publishPendingFrame", Qt::QueuedConnection);
        }
    }

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

        const qint64 timestamp = mediaOutputTimestamp(
            TimestampGenerator::getInstance()->getCurrentFrameCount());
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
    static bool isPlaceholderDevice(const QString& name)
    {
        return name == QStringLiteral("无可用视频输入设备")
            || name == QStringLiteral("枚举摄像头时出错");
    }

    /** 没有已保存设备时选用列表第一项，和编辑器里第一次打开面板的行为一致 */
    bool pickFirstDevice()
    {
        if (m_ui && m_widget && m_ui->cb_devices->count() > 0) {
            const QString text = m_ui->cb_devices->itemText(0);
            if (!isPlaceholderDevice(text)) {
                m_deviceDisplayName = text;
                const QString openName = m_ui->cb_devices->itemData(0).toString().trimmed();
                m_deviceOpenName = openName.isEmpty() ? text : openName;
                return true;
            }
        }

        const QList<CameraDeviceInfo> cameras = CameraCaptureThread::listVideoDevices();
        if (cameras.isEmpty()) {
            qWarning() << "Camera: no video input device";
            return false;
        }

        m_deviceDisplayName = cameras.first().displayName;
        m_deviceOpenName = cameras.first().openName.trimmed().isEmpty()
            ? cameras.first().displayName
            : cameras.first().openName.trimmed();
        return !m_deviceOpenName.isEmpty();
    }

    void syncComboToSavedDevice()
    {
        if (!m_ui || !m_widget || m_deviceOpenName.isEmpty()) {
            return;
        }

        QComboBox* box = m_ui->cb_devices;
        int index = box->findData(m_deviceOpenName);
        if (index < 0 && !m_deviceDisplayName.isEmpty()) {
            index = box->findText(m_deviceDisplayName);
        }
        if (index < 0) {
            index = box->findText(m_deviceOpenName);
        }
        QSignalBlocker blocker(box);
        if (index < 0) {
            const QString label = m_deviceDisplayName.isEmpty() ? m_deviceOpenName : m_deviceDisplayName;
            box->addItem(label, m_deviceOpenName);
            index = box->count() - 1;
        }
        box->setCurrentIndex(index);
        m_deviceDisplayName = box->itemText(index);
        const QString openName = box->itemData(index).toString().trimmed();
        if (!openName.isEmpty()) {
            m_deviceOpenName = openName;
        }
    }

    void ensureCaptureStarted()
    {
        if (m_deviceOpenName.isEmpty() && !pickFirstDevice()) {
            return;
        }

        syncComboToSavedDevice();
        initializeCamera(m_deviceOpenName);
    }

    void initializeCamera(const QString& deviceName)
    {
        const QString trimmed = deviceName.trimmed();
        if (trimmed.isEmpty()) {
            stopCamera();
            return;
        }
        if (trimmed == m_activeOpenName && m_captureThread && m_captureThread->isRunning()) {
            return;
        }

        stopCamera();
        m_deviceOpenName = trimmed;
        m_activeOpenName = trimmed;
        m_captureThread->startCapture(trimmed);
    }

    void stopCamera(bool fromDestructor = false)
    {
        m_activeOpenName.clear();
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

private:
    QPointer<QWidget> m_widget;
    QScopedPointer<Ui::CameraForm> m_ui;
    CameraCaptureThread* m_captureThread = nullptr;
    QString m_deviceOpenName;
    QString m_deviceDisplayName;
    QString m_activeOpenName;

    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;

    QMutex m_pendingMutex;
    cv::Mat m_pendingFrame;
    std::atomic<bool> m_uploadScheduled{false};
};
} // namespace Nodes
