#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "DataTypes/AudioTimestampRingQueue.h"
#include "TimeCodeDefines.h"
#include "porcupine_bridge.h" 

namespace Nodes
{
    /**
     * @brief 音频分析工作线程类
     * 负责在独立线程中处理音频数据分析，使用Porcupine进行唤醒词检测
     */
    class AudioIntentWorker : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief 处理状态枚举
         */
        enum ProcessingState {
            STATE_IDLE,              ///< 空闲状态
            STATE_WAKE_DETECTION     ///< 唤醒词检测状态
        };

        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit AudioIntentWorker(QObject *parent = nullptr);
        
        /**
         * @brief 析构函数
         */
        ~AudioIntentWorker();

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
         * @brief 唤醒词检测信号
         * @param keyword 检测到的唤醒词
         */
        void wakewordDetected(const QString& keyword);
        
        /**
         * @brief 状态变化信号
         * @param state 当前状态
         */
        void stateChanged(ProcessingState state);
        
        /**
         * @brief RMS值变化信号
         * @param rmsValue RMS值
         * @param channel 通道索引
         */
        void analysisValueChanged(const QVariantMap &values);

    public slots:
        /**
         * @brief 设置输入缓冲区
         * @param port 端口索引
         * @param buffer 音频缓冲区
         */
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);
        
        /**
         * @brief 设置Porcupine配置参数
         * @param accessKey 访问密钥
         * @param modelPath 模型文件路径
         * @param keywordPaths 关键词文件路径列表
         * @param sensitivities 敏感度列表
         */
        void setPorcupineConfig(const QString& accessKey, const QString& modelPath, 
                               const QStringList& keywordPaths, const QList<float>& sensitivities);
        
        /**
         * @brief 打印版本信息
         */
        void printVersionInfo();

    private:
        static const int BUFFER_SIZE = 8192;  // 缓冲区大小
        std::shared_ptr<AudioTimestampRingQueue> _inputBuffers;    ///< 输入音频缓冲区数组
        QTimer* _processingTimer;                                  ///< 处理定时器
        QMutex _mutex;                                             ///< 互斥锁
        bool _isProcessing;                                        ///< 处理状态标志
        qint64 _lastProcessedTimestamp;                            ///< 上次处理的时间戳

        int _frameSize;                                            ///< 分析帧大小
        int _sampleRate;                                           ///< 采样率
        
        // Porcupine相关变量
        void* _porcupineHandle;                                  ///< Porcupine实例句柄（使用void*替代pv_porcupine_t*）
        bool _porcupineInitialized;                              ///< Porcupine初始化状态
        QString _porcupineAccessKey;                             ///< Porcupine访问密钥
        QString _porcupineModelPath;                             ///< Porcupine模型文件路径
        QStringList _keywordPaths;                               ///< 关键词文件路径列表
        QList<float> _keywordSensitivities;                      ///< 关键词敏感度列表
        QStringList _keywords;                                    ///< 关键词名称列表
        
        // 处理状态相关
        ProcessingState _currentState;                            ///< 当前处理状态
        
        // 音频处理相关
        std::vector<float> _audioBuffer;                          ///< 累积输入音频数据的缓冲区（48kHz float格式）
        
        // 音频格式转换相关
        static const int INPUT_SAMPLE_RATE = 48000;              ///< 输入采样率
        static const int INPUT_FRAME_SIZE = 2048;                ///< 输入帧大小
        std::vector<int16_t> _porcupineBuffer;                   ///< Porcupine处理用的缓冲区（16kHz int16格式）
        
        // 重采样相关
        static const double RESAMPLE_RATIO;                      ///< 重采样比例
        
        /**
         * @brief 初始化Porcupine引擎
         */
        void initializePorcupine();
        
        /**
         * @brief 清理Porcupine资源
         */
        void cleanupPorcupine();
        
        /**
         * @brief 处理唤醒词检测
         * @param audioData 音频数据
         * @param dataSize 数据大小
         */
        void processWakewordDetection(const int16_t* audioData, int dataSize);
        
        /**
         * @brief 设置处理状态
         * @param state 新状态
         */
        void setState(ProcessingState state);
        
        /**
         * @brief 将音频数据转换为Picovoice格式
         * @param inputFrame 输入音频帧
         * @param outputBuffer 输出缓冲区
         * @return 转换后的样本数
         */
        int convertAudioForPicovoice(const AudioFrame& inputFrame, std::vector<int16_t>& outputBuffer);
    };
}