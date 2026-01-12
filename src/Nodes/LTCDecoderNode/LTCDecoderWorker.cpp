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
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
    {
        // 创建LTC解码器
        _ltcDecoder = new LtcDecoder(this);
        
        // 连接LTC解码器的信号
        connect(_ltcDecoder, &LtcDecoder::newFrame, this, &LTCDecoderWorker::onLtcDecoderNewFrame);

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
            
            QObject::connect(TimestampGenerator::getInstance(),
                             &TimestampGenerator::frameCountUpdated,
                             this,
                             &LTCDecoderWorker::onFrameTick,
                             Qt::QueuedConnection);
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
            
            QObject::disconnect(TimestampGenerator::getInstance(),
                                &TimestampGenerator::frameCountUpdated,
                                this,
                                &LTCDecoderWorker::onFrameTick);
            
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

    /**
     * 按全局帧计数驱动的解码回调
     * - 由 TimestampGenerator::frameCountUpdated 信号触发
     * - 避免固定周期轮询，确保与系统统一时钟对齐
     * @param frameCount 当前全局帧计数
     */
    void LTCDecoderWorker::onFrameTick(qint64 frameCount)
    {
        if (!_isProcessing || !_audioBuffer || !_audioBuffer->isActive()) {
            return;
        }
        if (frameCount == _lastProcessedTimestamp) {
            return;
        }
        AudioFrame frame;
        if (_audioBuffer->getFrameByTimestamp(frameCount, frame)) {
            _lastProcessedTimestamp = frame.timestamp;
            if (!frame.data.isEmpty() && _ltcDecoder) {
                _ltcDecoder->writeData(frame.data.constData(), frame.data.size());
            }
        }
    }
}

