#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "DataTypes/AudioTimestampRingQueue.h"
#include "../../Common/Devices/LtcDecoder/ltcdecoder.h"
#include "TimeCodeDefines.h"

namespace Nodes
{
    /**
     * @brief LTC解码工作线程类
     * 负责在独立线程中处理音频数据解码
     */
    class LTCDecoderWorker : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit LTCDecoderWorker(QObject *parent = nullptr);
        
        /**
         * @brief 析构函数
         */
        ~LTCDecoderWorker();

    public slots:
        /**
         * @brief 设置音频缓冲区
         * @param audioBuffer 共享音频缓冲区
         */
        void setAudioBuffer(std::shared_ptr<AudioTimestampRingQueue> audioBuffer);
        /**
         * @brief 开始处理音频数据
         */
        void startProcessing();
        
        /**
         * @brief 停止处理音频数据
         */
        void stopProcessing();
        
        /**
         * @brief 处理音频数据的主循环
         */
        void processAudioData();

    signals:
        /**
         * @brief 解码到新的时间码帧信号
         * @param frame 时间码帧数据
         */
        void timeCodeFrameDecoded(TimeCodeFrame frame);
        
        /**
         * @brief 处理状态变化信号
         * @param isProcessing 是否正在处理
         */
        void processingStatusChanged(bool isProcessing);

    private slots:
        /**
         * @brief LTC解码器新帧槽函数
         * @param frame 解码得到的时间码帧
         */
        void onLtcDecoderNewFrame(TimeCodeFrame frame);

    private:
        LtcDecoder* _ltcDecoder;                                        ///< LTC解码器
        std::shared_ptr<AudioTimestampRingQueue> _audioBuffer;          ///< 音频缓冲区
        QTimer* _processingTimer;                                       ///< 处理定时器
        QMutex _mutex;                                                  ///< 互斥锁
        bool _isProcessing;                                             ///< 处理状态标志
        qint64 _lastProcessedTimestamp;                                 ///< 上次处理的时间戳
    };
}