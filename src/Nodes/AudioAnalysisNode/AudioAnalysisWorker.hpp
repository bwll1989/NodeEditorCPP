#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "DataTypes/AudioTimestampRingQueue.h"
#include "TimeCodeDefines.h"
#include "Eigen/Core"
#include "Gist.h"  // 添加GIST库头文件

namespace Nodes
{
    /**
     * @brief 音频分析工作线程类
     * 负责在独立线程中处理音频数据分析
     */
    class AudioAnalysisWorker : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit AudioAnalysisWorker(QObject *parent = nullptr);
        
        /**
         * @brief 析构函数
         */
        ~AudioAnalysisWorker();

    public slots:
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

        /**
         * @brief 执行音频分析操作
         * @param inputFrames 输入音频帧向量
         * @param timestamp 时间戳
         */
        void performAnalysisOperation(const AudioFrame& inputFrames, qint64 timestamp);
        
    signals:
        /**
         * @brief 处理状态变化信号
         * @param isProcessing 是否正在处理
         */
        void processingStatusChanged(bool isProcessing);
        
        /**
         * @brief RMS值变化信号
         * @param rmsValue RMS值
         * @param channel 通道索引
         */
        void analysisValueChanged(const QVariantMap &values);

    public slots:
        /**
         * @brief 设置指定端口的缓冲区
         * @param port 端口索引
         * @param buffer 音频缓冲区
         */
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);
        
    private:
        std::shared_ptr<AudioTimestampRingQueue> _inputBuffers;    ///< 输入音频缓冲区数组
        QTimer* _processingTimer;                                              ///< 处理定时器
        QMutex _mutex;                                                         ///< 互斥锁
        bool _isProcessing;                                                    ///< 处理状态标志
        qint64 _lastProcessedTimestamp;                                        ///< 上次处理的时间戳
        
        // GIST相关成员变量
        std::unique_ptr<Gist<float>> _gistAnalyzer;                           ///< GIST分析器
        int _frameSize;                                                        ///< 分析帧大小
        int _sampleRate;                                                       ///< 采样率
        
        /**
         * @brief 初始化GIST分析器
         * @param frameSize 帧大小
         * @param sampleRate 采样率
         */
        void initializeGist(int frameSize, int sampleRate);
    };
}