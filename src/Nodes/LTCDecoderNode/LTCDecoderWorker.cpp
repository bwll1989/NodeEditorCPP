#include "LTCDecoderWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>

namespace Nodes
{
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    LTCDecoderWorker::LTCDecoderWorker(QObject *parent)
        : QObject(parent)
        , _ltcDecoder(nullptr)
        , _audioBuffer(nullptr)
        , _processingTimer(new QTimer(this))
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
    {
        // 创建LTC解码器
        _ltcDecoder = new LtcDecoder(this);
        
        // 连接LTC解码器的信号
        connect(_ltcDecoder, &LtcDecoder::newFrame, this, &LTCDecoderWorker::onLtcDecoderNewFrame);
        
        // 设置处理定时器
        _processingTimer->setInterval(15); // 10ms间隔处理音频数据
        connect(_processingTimer, &QTimer::timeout, this, &LTCDecoderWorker::processAudioData);

    }
    
    /**
     * @brief 析构函数
     */
    LTCDecoderWorker::~LTCDecoderWorker()
    {
        stopProcessing();
        
        if (_ltcDecoder) {
            _ltcDecoder->deleteLater();
            _ltcDecoder = nullptr;
        }

    }
    
    /**
     * @brief 设置音频缓冲区
     * @param audioBuffer 共享音频缓冲区
     */
    void LTCDecoderWorker::setAudioBuffer(std::shared_ptr<AudioTimestampRingQueue> audioBuffer)
    {
         {
        QMutexLocker locker(&_mutex);
        _audioBuffer = audioBuffer;
        }
        
        if (_audioBuffer) {
            // 如果当前正在处理，重新开始
            if (_isProcessing) {
                stopProcessing();
            }
            startProcessing();
        } else {
            stopProcessing();
        }
    }
    
    /**
     * @brief 开始处理音频数据
     */
    void LTCDecoderWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);
        
        if (!_isProcessing && _audioBuffer) {
            _isProcessing = true;
            _lastProcessedTimestamp = 0;
            
            // 启动处理定时器
            _processingTimer->start();
            emit processingStatusChanged(true);

        }
    }
    
    /**
     * @brief 停止处理音频数据
     */
    void LTCDecoderWorker::stopProcessing()
    {
        QMutexLocker locker(&_mutex);
        
        if (_isProcessing) {
            _isProcessing = false;
            
            // 停止处理定时器
            if (_processingTimer->isActive()) {
                _processingTimer->stop();
            }
            
            emit processingStatusChanged(false);

        }
    }
    
    /**
     * @brief 处理音频数据的主循环
     */
    void LTCDecoderWorker::processAudioData()
    {

        // 不需要加锁，因为这个方法在定时器线程中调用，频率很高
        if (!_isProcessing || !_audioBuffer || !_audioBuffer->isActive()) {
            return;
        }
        
        // 获取当前时间戳
        auto currentTime = TimestampGenerator::getInstance()->getCurrentFrameCount();

        // 避免重复处理相同的时间戳
        if (currentTime == _lastProcessedTimestamp) {

            return;
        }
        
        // 尝试获取新的音频帧
        AudioFrame frame;
        if (_audioBuffer->getFrameByTimestamp(currentTime, frame)) {
            _lastProcessedTimestamp = frame.timestamp;
            
            // 将音频数据写入LTC解码器
            if (!frame.data.isEmpty() && _ltcDecoder) {
                // 使用修正后的LTC解码方法
                // 注意：这里假设已经修复了ltcdecoder.cpp中的writeData方法
                _ltcDecoder->writeData(frame.data.constData(), frame.data.size());
            }
        }
    }
    
    /**
     * @brief LTC解码器新帧槽函数
     * @param frame 解码得到的时间码帧
     */
    void LTCDecoderWorker::onLtcDecoderNewFrame(TimeCodeFrame frame)
    {
        // 直接转发解码结果到主线程
        emit timeCodeFrameDecoded(frame);
        
        // 可选：输出调试信息
        // qDebug() << QString("LTCDecoderWorker decoded: %1:%2:%3.%4")
        //             .arg(frame.hours, 2, 10, QLatin1Char('0'))
        //             .arg(frame.minutes, 2, 10, QLatin1Char('0'))
        //             .arg(frame.seconds, 2, 10, QLatin1Char('0'))
        //             .arg(frame.frames, 2, 10, QLatin1Char('0'));
    }
}

