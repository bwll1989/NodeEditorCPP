#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "DataTypes/AudioTimestampRingQueue.h"
#include "TimeCodeDefines.h"
#include "Eigen/Core"
namespace Nodes
{
    /**
     * @brief LTC解码工作线程类
     * 负责在独立线程中处理音频数据解码
     */
    class AudioMatrixWorker : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit AudioMatrixWorker(QObject *parent = nullptr);
        
        /**
         * @brief 析构函数
         */
        ~AudioMatrixWorker();

    public slots:
        /**
         * @brief 更新矩阵数据
         * @param matrix 新的矩阵数据
         */
        void updateMatrix(const Eigen::MatrixXd& matrix);
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

        void performMatrixOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp);
    signals:
        /**
         * @brief 处理状态变化信号
         * @param isProcessing 是否正在处理
         */
        void processingStatusChanged(bool isProcessing);
        
        /**
         * @brief 音频处理完成信号
         * @param outputBuffers 处理后的输出音频缓冲区
         */
        void audioProcessed(const std::vector<std::shared_ptr<AudioTimestampRingQueue>>& outputBuffers);

    public slots:
        /**
         * @brief 初始化缓冲区
         * @param inputCount 输入端口数量
         * @param outputCount 输出端口数量
         * @param matrix 矩阵数据
         */
        void initializeBuffers(int inputCount, int outputCount, const Eigen::MatrixXd& matrix);
        
        /**
         * @brief 设置指定末端口的缓冲区
         * @param port 端口索引
         * @param buffer 音频缓冲区
         */
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);
        
        /**
         * @brief 获取指定输出端口的缓冲区
         * @param port 端口索引
         * @return 输出音频缓冲区
         */
        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);
        
    private:
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;    ///< 输入音频缓冲区数组
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;   ///< 输出音频缓冲区数组
        Eigen::MatrixXd _matrix;                                               ///< 矩阵数据
        QTimer* _processingTimer;                                              ///< 处理定时器
        QMutex _mutex;                                                         ///< 互斥锁
        bool _isProcessing;                                                    ///< 处理状态标志
        qint64 _lastProcessedTimestamp;                                        ///< 上次处理的时间戳
    };
}