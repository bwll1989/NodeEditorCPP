#pragma once

#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "DataTypes/NodeDataList.hpp"
#include "AudioDeviceOutInterface.hpp"
#include <portaudio.h>
#include "QThread"
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include "PluginDefinition.hpp"
#include <QVector>  // 添加 QVector 头文件
#include <map>  // 添加 map 头文件
#include <QSet>  // 添加 QSet 头文件
#include <QMap>  // 添加 QMap 头文件
#include <QRegularExpression>  // 添加 QRegExp 头文件
#include <QTimer>
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;


static const int SAMPLE_RATE = 48000;
static const size_t BUFFER_SIZE = SAMPLE_RATE/TimestampGenerator::getInstance()->getFrameRate();
namespace Nodes
{
    class AudioDeviceOutDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        
    public:
        AudioDeviceOutDataModel() {
            InPortCount = 2;
            OutPortCount = 0;
            CaptionVisible = true;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;
            // 初始化 PortAudio
            initPortAudio();
            // 连接设备选择器信号
            connect(widget->deviceSelector, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
                    this, &AudioDeviceOutDataModel::onDeviceChanged);
            // 连接驱动选择器信号
            connect(widget->driverSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &AudioDeviceOutDataModel::onDriverChanged);
        }
        
        ~AudioDeviceOutDataModel() {
            if (paStream) {
                Pa_StopStream(paStream);
                Pa_CloseStream(paStream);
            }
        }
        
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                return AudioData().type();
            }
            return NodeDataType();
        }
        
        /**
         * @brief 设置输入数据（连接建立时调用）
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (portIndex < InPortCount) {
                auto audioData = std::dynamic_pointer_cast<AudioData>(data);
                
                // 检查是否为有效的音频数据连接
                if (audioData && audioData->isConnectedToSharedBuffer()) {
                    // 建立连接：保存AudioData指针
                    inputAudioData[portIndex] = audioData;
                    // 获取AudioTimestampQueue（新的队列模式）
                    auto timestampQueue = audioData->getSharedAudioBuffer();
                    if (timestampQueue) {
                        inputTimestampQueues[portIndex] = timestampQueue;
                    }
                    
                    // 自动开始播放
                    if (!isPlaying) {
                        startAudioOutput();
                    }
                } else {
                    // 从所有map中移除该端口的数据
                    inputAudioData.erase(portIndex);
                    inputTimestampQueues.erase(portIndex);

                }
            }
        }
        
        std::shared_ptr<NodeData> outData(PortIndex port) override
        {
            Q_UNUSED(port)
            return nullptr;
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        /**
         * @brief 保存节点配置
         * @return JSON对象，包含设备和驱动配置
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            
            // 保存当前选择的设备索引和名称
            modelJson1["selectedDeviceIndex"] = selectedDeviceIndex;
            if (widget->deviceSelector->currentIndex() >= 0) {
                modelJson1["selectedDeviceText"] = widget->deviceSelector->currentText();
                modelJson1["selectedDeviceData"] = widget->deviceSelector->currentData().toInt();
            }
            // 保存当前选择的驱动类型
            modelJson1["selectedDriverIndex"] = widget->driverSelector->currentIndex();
            modelJson1["selectedDriverText"] = widget->driverSelector->currentText();
            modelJson1["selectedDriverData"] = widget->driverSelector->currentData().toInt();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        /**
         * @brief 加载节点配置
         * @param p JSON对象，包含保存的配置
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject config = v.toObject();
                
                // 恢复驱动选择
                if (config.contains("selectedDriverIndex")) {
                    int driverIndex = config["selectedDriverIndex"].toInt();
                    if (driverIndex >= 0 && driverIndex < widget->driverSelector->count()) {
                        widget->driverSelector->setCurrentIndex(driverIndex);
                    }
                }
                
                // 等待设备列表更新后再恢复设备选择
                QTimer::singleShot(100, [this, config]() {
                    // 恢复设备选择
                    if (config.contains("selectedDeviceIndex")) {
                        int deviceIndex = config["selectedDeviceIndex"].toInt();
                        selectedDeviceIndex = deviceIndex;

                        // 尝试通过设备索引匹配
                        for (int i = 0; i < widget->deviceSelector->count(); ++i) {
                            if (widget->deviceSelector->itemData(i).toInt() == deviceIndex) {
                                widget->deviceSelector->setCurrentIndex(i);
                                break;
                            }
                        }
                    }

                });
            }
        }

    public slots:
        /**
         * @brief 初始化PortAudio
         */
        void initPortAudio() ;

        /**
         * @brief 更新设备列表（支持驱动筛选）
         */
        void updateDeviceList() ;

        /**
         * @brief 开始音频播放
         */
        bool startAudioOutput() ;

        /**
         * @brief 停止音频播放
         */
        void stopAudioOutput() ;

        /**
         * @brief 驱动类型改变处理
         * @param index 驱动选择器索引
         */
        void onDriverChanged(int index) ;

        /**
         * @brief 设备改变处理
         * @param deviceText 设备文本
         */
        void onDeviceChanged(const QString& deviceText) ;

        /**
         * @brief 音频处理回调，根据系统时间从AudioTimestampQueue读取对应时间戳的数据
         * @param outputBuffer 输出缓冲区指针
         * @param framesPerBuffer 每次处理的帧数（由PortAudio决定，通常固定）
         * @return PortAudio继续标志
         */
        /**
         * @brief 音频处理回调函数，严格按照FRAME_INTERVAL_MS控制处理间隔
         * @param outputBuffer 输出缓冲区
         * @param framesPerBuffer 每次处理的帧数
         * @return PortAudio继续标志
         */
        int processAudio(void* outputBuffer, unsigned long framesPerBuffer) {

            // 将输出缓冲区转换为16位整数指针
            int16_t* output = static_cast<int16_t*>(outputBuffer);
            // 获取音频设备的最大通道数
            const int deviceChannels = getDeviceMaxChannels(selectedDeviceIndex);
            const int frames = static_cast<int>(framesPerBuffer);
        
            // 清零输出缓冲区，确保没有杂音
            memset(output, 0, static_cast<size_t>(frames) * static_cast<size_t>(deviceChannels) * sizeof(int16_t));

            const qint64 targetTimestamp = TimestampGenerator::getInstance()->getCurrentFrameCount();
            // 遍历所有活跃的音频输入端口
            for (auto& [portIndex, timestampQueue] : inputTimestampQueues) {
                // 检查端口索引是否在设备通道范围内
                if (portIndex >= deviceChannels) {
                    continue;
                }
        
                // 检查队列有效性（避免在回调中额外加锁）
                if (!timestampQueue) {
                    continue;
                }


                AudioFrame frame;

                // 使用新的基于时间戳的读取方法
                if (timestampQueue->getFrameByTimestamp(targetTimestamp, frame)) {

                    const int16_t* input = reinterpret_cast<const int16_t*>(frame.data.constData());
                    const int totalSamples = static_cast<int>(frame.data.size() / sizeof(int16_t));
                    const int samplesToProcess = std::min(frames, totalSamples);
                    // qDebug()<<"find target"<<targetTimestamp<<TimestampGenerator::getInstance()->getCurrentFrameInfo().absoluteTimeMs;
                    // 指针步进写入，减少乘法
                    int16_t* outPtr = output + portIndex;
                    const int16_t* inPtr = input;
                    int remaining = samplesToProcess;
                    while (remaining-- > 0) {
                        *outPtr = *inPtr++;
                        outPtr += deviceChannels;
                    }
                    // 可选：调试同步
                    // qint64 timeDiff = frame.timestamp - targetTimestamp;
                }
                // else {
                //     // 没有找到合适的帧，可以记录调试信息
                //     qDebug() << "No suitable frame found for port" << portIndex << "at timestamp" << targetTimestamp;
                // }
            }
            return paContinue;
        }

        /**
         * @brief 获取设备最大声道数
         * @param deviceIndex 设备索引
         * @return 最大声道数
         */
        int getDeviceMaxChannels(PaDeviceIndex deviceIndex) {
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
            return deviceInfo ? deviceInfo->maxOutputChannels : 2;
        }
private:
        /**
         * @brief PortAudio 回调函数
         */
        static int paCallback(const void* inputBuffer, void* outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void* userData) {
            Q_UNUSED(inputBuffer)
            Q_UNUSED(timeInfo)
            Q_UNUSED(statusFlags)
            AudioDeviceOutDataModel* model = static_cast<AudioDeviceOutDataModel*>(userData);
            return model->processAudio(outputBuffer, framesPerBuffer);
        }

    private:
        // 成员变量
        bool isPlaying = false;
        AudioDeviceOutInterface* widget = new AudioDeviceOutInterface();
        PaStream* paStream = nullptr;
        PaDeviceIndex selectedDeviceIndex = Pa_GetDefaultOutputDevice();
        // 稀疏存储：只存储实际连接的通道
        std::map<int, std::shared_ptr<AudioData>> inputAudioData;  // 每个端口的音频数据
        std::map<int, std::shared_ptr<AudioTimestampRingQueue>> inputTimestampQueues;  // 每个端口对应的时间戳队列
    };
}

