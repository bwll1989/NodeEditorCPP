//
// Created by pablo on 3/9/24.
// Modified to use OpenCV for better performance with threading
//
#pragma once
#include <QtNodes/NodeDelegateModel>
#include <QFutureWatcher>
#include <QTimer>
#include <QVariant>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <QMediaDevices>
#include <QCameraDevice>
#include <QList>
#include "DataTypes/NodeDataList.hpp"
#include "ui_CameraForm.h"

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <vector>

namespace Ui {
    class CameraForm;
}

class QLabel;
class QComboBox;
using namespace NodeDataTypes;

/**
 * @brief 摄像头捕获线程类
 * 在单独的线程中运行摄像头捕获，避免阻塞主UI线程
 */
class CameraCaptureThread : public QThread {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CameraCaptureThread(QObject* parent = nullptr)
        : QThread(parent), m_running(false), m_deviceIndex(-1) {}

    /**
     * @brief 析构函数，确保线程安全退出
     */
    ~CameraCaptureThread() override {
        stop();
        wait(); // 等待线程结束
    }

    /**
     * @brief 启动摄像头捕获线程
     * @param deviceIndex 摄像头设备索引
     */
    void startCapture(int deviceIndex) {
        QMutexLocker locker(&m_mutex);
        
        // 如果线程已经在运行，先停止它
        if (isRunning()) {
            stop();
            wait();
        }
        
        m_deviceIndex = deviceIndex;
        m_running = true;
        
        // 启动线程
        start(QThread::HighPriority); // 以高优先级启动线程
    }

    /**
     * @brief 停止摄像头捕获线程
     */
    void stop() {
        QMutexLocker locker(&m_mutex);
        m_running = false;
        m_condition.wakeOne(); // 唤醒线程，使其能够检查m_running并退出
    }

signals:
    /**
     * @brief 当新帧可用时发出信号
     * @param frame 捕获的帧
     */
    void frameAvailable(const cv::Mat& frame);
    
    /**
     * @brief 当捕获状态改变时发出信号
     * @param isCapturing 是否正在捕获
     */
    void captureStateChanged(bool isCapturing);

protected:
    /**
     * @brief 线程执行函数
     */
    void run() override {
        cv::VideoCapture capture;
        bool captureOpened = false;
        
        // 尝试打开摄像头
        {
            QMutexLocker locker(&m_mutex);
            try {
                capture.open(m_deviceIndex);
                captureOpened = capture.isOpened();
                emit captureStateChanged(captureOpened);
            } catch (const cv::Exception& e) {
                qWarning() << "OpenCV异常(打开摄像头): " << e.what();
                emit captureStateChanged(false);
                return;
            } catch (const std::exception& e) {
                qWarning() << "标准异常(打开摄像头): " << e.what();
                emit captureStateChanged(false);
                return;
            } catch (...) {
                qWarning() << "未知异常(打开摄像头)";
                emit captureStateChanged(false);
                return;
            }
        }
        
        if (!captureOpened) {
            qWarning() << "无法在线程中打开摄像头设备 " << m_deviceIndex;
            return;
        }
        
        // 获取摄像头实际参数
        double actualWidth = capture.get(cv::CAP_PROP_FRAME_WIDTH);
        double actualHeight = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
        double actualFps = capture.get(cv::CAP_PROP_FPS);
        
        qInfo() << "摄像头线程初始化，分辨率: " << actualWidth << "x" << actualHeight 
                << " @ " << actualFps << " fps";
        
        // 主捕获循环
        while (m_running) {
            try {
                cv::Mat frame;
                bool success = capture.read(frame);
                
                if (success && !frame.empty()) {
                    emit frameAvailable(frame);
                } else if (!success || frame.empty()) {
                    qWarning() << "捕获帧失败";
                }
                
                // 根据帧率控制捕获频率，避免CPU占用过高
                int delayMs = actualFps > 0 ? 1000 / static_cast<int>(actualFps) : 33;
                msleep(delayMs / 2); // 使用一半的延迟，确保不会错过帧
                
                // 检查是否应该停止
                QMutexLocker locker(&m_mutex);
                if (!m_running) break;
                
                // 如果需要暂停，等待条件变量
                if (!m_running) {
                    m_condition.wait(&m_mutex);
                }
            } catch (const cv::Exception& e) {
                qWarning() << "OpenCV异常(捕获线程): " << e.what();
                msleep(1000); // 出错时等待一段时间再重试
            } catch (const std::exception& e) {
                qWarning() << "标准异常(捕获线程): " << e.what();
                msleep(1000);
            } catch (...) {
                qWarning() << "未知异常(捕获线程)";
                msleep(1000);
            }
        }
        
        // 释放摄像头资源
        try {
            if (capture.isOpened()) {
                capture.release();
                qInfo() << "摄像头资源已在线程中释放";
            }
            emit captureStateChanged(false);
        } catch (...) {
            qWarning() << "释放摄像头资源时发生异常";
        }
    }

private:
    QMutex m_mutex;              // 互斥锁，保护共享数据
    QWaitCondition m_condition;  // 条件变量，用于线程同步
    QAtomicInt m_running;        // 原子变量，控制线程运行状态
    int m_deviceIndex;           // 摄像头设备索引
};

namespace Nodes
{
    class CameraModel final : public QtNodes::NodeDelegateModel {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化摄像头节点
         */
        CameraModel() {
            InPortCount = 1;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Camera";
            WidgetEmbeddable = true;
            Resizable = false;
            
            // 创建摄像头捕获线程
            m_captureThread = new CameraCaptureThread(this);
            connect(m_captureThread, &CameraCaptureThread::frameAvailable, 
                    this, &CameraModel::onFrameAvailable, Qt::QueuedConnection);
            connect(m_captureThread, &CameraCaptureThread::captureStateChanged,
                    this, &CameraModel::onCaptureStateChanged, Qt::QueuedConnection);
        }

        /**
         * @brief 析构函数
         */
        ~CameraModel() override {
            try {
                // 停止摄像头
                stopCamera();
                
                // 停止摄像头线程
                if (m_captureThread) {
                    m_captureThread->stop();
                    m_captureThread->wait(); // 等待线程结束
                }
                
                // 清除图像数据
                m_outImageData.reset();
                
                qInfo() << "CameraModel已销毁并释放资源";
            } catch (const std::exception& e) {
                qWarning() << "CameraModel析构函数中的异常: " << e.what();
            } catch (...) {
                qWarning() << "CameraModel析构函数中的未知异常";
            }
        }


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
                
                // 如果有可用摄像头，自动选择第一个
                if (m_ui->cb_devices->count() > 0 && 
                    m_ui->cb_devices->itemText(0) != "No video input devices available") {
                    m_ui->cb_devices->setCurrentIndex(0);
                    initializeCamera(0);
                }
            }
            return m_widget;
        }


    private slots:
        /**
         * @brief 处理设备选择变更
         * @param index 下拉列表中的索引
         */
        void onDeviceChanged(int index) {
            if (index < 0 || !m_ui) {
                return;
            }
            
            QString deviceName = m_ui->cb_devices->itemText(index);
            if (deviceName == "无可用视频输入设备" || 
                deviceName == "枚举摄像头时出错") {
                stopCamera();
                return;
            }
            
            // 获取设备索引（从QComboBox的userData中获取）
            QVariant userData = m_ui->cb_devices->itemData(index);
            int deviceIndex = userData.isValid() ? userData.toInt() : index;
            
            qInfo() << "切换到摄像头设备: " << deviceName << " (索引: " << deviceIndex << ")";
            
            // 停止当前摄像头
            stopCamera();
            
            // 初始化新选择的摄像头
            initializeCamera(deviceIndex);
        }
        
        /**
         * @brief 处理线程中捕获的新帧
         * @param frame 捕获的帧
         */
        void onFrameAvailable(const cv::Mat& frame) {
            try {
                if (!frame.empty()) {
                    // 创建ImageData对象并更新输出
                    m_outImageData = std::make_shared<ImageData>(frame);
                    emit dataUpdated(0);
                } else {
                    qWarning() << "接收到空帧";
                }
            } catch (const cv::Exception& e) {
                qWarning() << "OpenCV异常(处理帧): " << e.what();
            } catch (const std::exception& e) {
                qWarning() << "标准异常(处理帧): " << e.what();
            } catch (...) {
                qWarning() << "未知异常(处理帧)";
            }
        }
        
        /**
         * @brief 处理摄像头捕获状态变更
         * @param isCapturing 是否正在捕获
         */
        void onCaptureStateChanged(bool isCapturing) {
            if (m_ui) {
                m_ui->cb_takingFrame->setChecked(isCapturing);
            }
        }

        /**
         * @brief 更新可用摄像头列表
         * 使用Qt的QMediaDevices API获取摄像头列表和名称
         */
        void updateCameras() {
            if (!m_ui) {
                return;
            }
            
            // 断开之前的连接，避免重复连接
            disconnect(m_ui->cb_devices, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &CameraModel::onDeviceChanged);
            
            m_ui->cb_devices->clear();
            
            try {
                // 使用Qt的QMediaDevices API获取摄像头列表
                QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
                
                if (cameras.isEmpty()) {
                    m_ui->cb_devices->addItem("无可用视频输入设备");
                    qWarning() << "未找到视频输入设备";
                    return;
                }
                
                // 添加可用摄像头到下拉列表
                for (int i = 0; i < cameras.size(); ++i) {
                    const QCameraDevice& camera = cameras.at(i);
                    QString deviceName = camera.description();
                    // 构建完整的摄像头名称
                    QString cameraName = deviceName;
                    // 添加到下拉列表，使用索引作为用户数据
                    m_ui->cb_devices->addItem(cameraName, i);
                }
            } catch (const std::exception& e) {
                qWarning() << "标准异常(updateCameras): " << e.what();
                m_ui->cb_devices->addItem("枚举摄像头时出错");
            } catch (...) {
                qWarning() << "未知异常(updateCameras)";
                m_ui->cb_devices->addItem("枚举摄像头时出错");
            }
            
            // 连接设备选择变更信号
            connect(m_ui->cb_devices, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &CameraModel::onDeviceChanged);
        }

        /**
         * @brief 初始化摄像头
         * @param deviceIndex 摄像头设备索引
         */
        void initializeCamera(int deviceIndex) {
            try {
                // 先停止当前摄像头
                stopCamera();
                
                // 在线程中启动摄像头捕获
                m_captureThread->startCapture(deviceIndex);
                
                qInfo() << "摄像头初始化请求，设备索引: " << deviceIndex;
                
            } catch (const std::exception& e) {
                qWarning() << "初始化摄像头异常: " << e.what();
                if (m_ui) {
                    m_ui->cb_takingFrame->setChecked(false);
                }
            } catch (...) {
                qWarning() << "初始化摄像头时发生未知异常";
                if (m_ui) {
                    m_ui->cb_takingFrame->setChecked(false);
                }
            }
        }
        
        /**
         * @brief 停止摄像头
         */
        void stopCamera() {
            try {
                // 停止捕获线程
                if (m_captureThread) {
                    m_captureThread->stop();
                }
                
                // 更新UI状态
                if (m_ui) {
                    m_ui->cb_takingFrame->setChecked(false);
                }
                
                // 清除输出数据
                m_outImageData.reset();
                emit dataUpdated(0);
            } catch (const std::exception& e) {
                qWarning() << "停止摄像头异常: " << e.what();
            } catch (...) {
                qWarning() << "停止摄像头时发生未知异常";
            }
        }
        



    private:
        QWidget* m_widget = nullptr;
        QScopedPointer<Ui::CameraForm> m_ui;
        CameraCaptureThread* m_captureThread = nullptr; // 摄像头捕获线程

        // 输出图像数据
        std::shared_ptr<ImageData> m_outImageData;
    };
}
