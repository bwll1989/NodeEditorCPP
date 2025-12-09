//
// Created by WuBin on 2025/8/21.
//
#include "UniversePlaybackDataModel.hpp"
using namespace Nodes;

UniversePlaybackDataModel::UniversePlaybackDataModel()
{
    InPortCount = 4;   // 无输入端口，从视频文件读取
    OutPortCount = 4;   // 初始1个输出端口，会根据视频动态调整
    PortEditable = true;
    CaptionVisible = true;
    Caption = "Universe Playback";
    WidgetEmbeddable = false;
    Resizable = false;
    isLooping=false;
    // 初始化FFmpeg
    initializeFFmpeg();
    playbackTimer = new QTimer(this);
    playbackTimer->setInterval(1000 / AppConstants::ARTNET_OUTPUT_FPS); // 50fps
    connect(playbackTimer, &QTimer::timeout, this, &UniversePlaybackDataModel::onPlaybackTimer);

    // 连接界面信号
    connect(widget->universeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &UniversePlaybackDataModel::onUniverseChanged);
    connect(widget->subnetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &UniversePlaybackDataModel::onSubnetChanged);
    connect(widget->netSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &UniversePlaybackDataModel::onNetChanged);

    // 连接控制按钮信号
    connect(widget, &Nodes::UniversePlaybackInterface::clearDataClicked,
        this, &UniversePlaybackDataModel::onClearDataClicked);
    connect(widget, &Nodes::UniversePlaybackInterface::selectFileClicked,
        this, &UniversePlaybackDataModel::onSelectFileClicked);
    connect(widget, &Nodes::UniversePlaybackInterface::playClicked,
        this, &UniversePlaybackDataModel::onPlayClicked);
    connect(widget, &Nodes::UniversePlaybackInterface::stopClicked,
        this, &UniversePlaybackDataModel::onStopClicked);
    
    // 连接循环播放信号
    connect(widget, &Nodes::UniversePlaybackInterface::loopStateChanged,
        this, &UniversePlaybackDataModel::onLoopStateChanged);  // 新增：连接循环播放信号

    // 注册OSC控制
    NodeDelegateModel::registerOSCControl("/universe", widget->universeSpinBox);
    NodeDelegateModel::registerOSCControl("/subnet", widget->subnetSpinBox);
    NodeDelegateModel::registerOSCControl("/net", widget->netSpinBox);
    NodeDelegateModel::registerOSCControl("/clear", widget->clearButton);
    NodeDelegateModel::registerOSCControl("/play", widget->playButton);
    NodeDelegateModel::registerOSCControl("/stop", widget->stopButton);
    NodeDelegateModel::registerOSCControl("/loop", widget->loopCheckBox);
}


void UniversePlaybackDataModel::onUniverseChanged(int universe) {
    Q_UNUSED(universe)
    updateAllUniverseData();
}

void UniversePlaybackDataModel::onSubnetChanged(int subnet) {
    Q_UNUSED(subnet)
    updateAllUniverseData();
}

void UniversePlaybackDataModel::onNetChanged(int net) {
    Q_UNUSED(net)
    updateAllUniverseData();
}

void UniversePlaybackDataModel::onClearDataClicked() {
    // 清空所有DMX数据
    for (int i = 0; i < dmxDataList.size(); i++) {
        dmxDataList[i].fill(0);
    }
    // 更新输出数据
    updateAllUniverseData();
}

void UniversePlaybackDataModel::onSelectFileClicked(QString fileName) {
    Q_UNUSED(fileName)
    if (!fileName.isEmpty()) {
        auto filePath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR+"/"+fileName;

        if (openVideo(filePath)) {
            widget->updateFileInfo(filePath, m_duration, m_fps,m_videoHeight);
            updateNodeState(QtNodes::NodeValidationState::State::Valid);
        }
    } else {
        updateNodeState(QtNodes::NodeValidationState::State::Error,"cannot open file");
        widget->showError("无法打开视频文件");
    }

}

void UniversePlaybackDataModel::onPlayClicked() {
    if (m_formatContext && !isPlaying) {
        startPlayback();
    }
}


void UniversePlaybackDataModel::onStopClicked() {
    stopPlayback();
}

/**
 * @brief 播放定时器回调，处理视频帧读取和循环播放逻辑
 */
void UniversePlaybackDataModel::onPlaybackTimer() {
    if (!isPlaying || !m_formatContext) {
        return;
    }

    // 读取下一帧
    if (readNextFrame()) {
        // 提取DMX数据并更新输出
        extractAndUpdateDmxData();

        /**
         * 函数级注释：用当前帧时间戳更新播放时间显示
         * 优先使用 best_effort_timestamp；若不可用则使用 pts。
         * 换算：毫秒 = timestamp * time_base * 1000
         */
        if (m_frame) {
            int64_t ts = (m_frame->best_effort_timestamp != AV_NOPTS_VALUE)
                           ? m_frame->best_effort_timestamp
                           : m_frame->pts;
            if (ts != AV_NOPTS_VALUE) {
                m_currentTimestampMs = static_cast<qint64>(ts * m_timeBase * 1000.0);
                widget->currentTimeEdit->setTime(QTime(0,0,0,0).addMSecs(static_cast<int>(m_currentTimestampMs)));
            }
        }
    } else {
        // 播放结束，检查是否需要循环播放
        if (isLooping) {
            // 循环播放：重新定位到文件开头
            if (av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD) >= 0) {
                // 清空解码器缓冲区
                avcodec_flush_buffers(m_codecContext);
                /**
                 * 函数级注释：循环重置后将时间归零
                 */
                m_currentTimestampMs = 0;
                widget->currentTimeEdit->setTime(QTime(0,0,0,0));
            } else {
                stopPlayback();
            }
        } else {
            stopPlayback();
        }
    }
}

void UniversePlaybackDataModel::initializeFFmpeg() {
    // 初始化FFmpeg（只需要调用一次）
    static bool initialized = false;
    if (!initialized) {
        avformat_network_init();
        initialized = true;
    }

    m_formatContext = nullptr;
    m_codecContext = nullptr;
    m_codec = nullptr;
    m_frame = nullptr;
    m_packet = nullptr;
    m_videoStreamIndex = -1;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_timeBase = 0.0;
    m_fps = 25.0;
    m_duration = 0.0;
    isPlaying = false;
    currentUniverseCount = 0;
}

void UniversePlaybackDataModel::cleanupFFmpegInternal() {
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
    }
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
    }
}

void UniversePlaybackDataModel::cleanupFFmpeg() {
    QMutexLocker locker(&m_mutex);
    cleanupFFmpegInternal();
}

void UniversePlaybackDataModel::adjustUniverseCount(int videoHeight)
{
    /* 函数级注释：
     * 根据视频高度调整 Universe 数量，并为每个 Universe 初始化 512 字节的 DMX 缓冲区。
     * 由于 dmxDataList 类型改为 QList<QByteArray>，此处使用 QByteArray.resize(512) + fill(0)
     * 来创建并清零每个 Universe 的数据。
     */
    // 计算Universe数量（每4行对应一个Universe）
    int newUniverseCount = videoHeight ;

    if (newUniverseCount != currentUniverseCount) {
        // 清空现有数据结构
        dmxDataList.clear();
        universeOutputs.clear();

        // 初始化新的 Universe 数据结构
        for (int i = 0; i < newUniverseCount; ++i) {
            QByteArray universeData;
            universeData.resize(512);
            universeData.fill(char(0x00));
            dmxDataList.append(universeData);

            auto outputData = std::make_shared<VariableData>();
            universeOutputs.append(outputData);
        }

        currentUniverseCount = newUniverseCount;
    }
}

bool UniversePlaybackDataModel::openVideo(const QString& filename)
{
    QMutexLocker locker(&m_mutex);

    // 清理之前的资源（使用内部函数，避免重复加锁）
    cleanupFFmpegInternal();
    initializeFFmpeg();

    // 打开输入文件
    if (avformat_open_input(&m_formatContext, filename.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred(QString("无法打开视频文件: %1").arg(filename));
        return false;
    }

    // 查找流信息
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        emit errorOccurred("无法获取流信息");
        return false;
    }

    // 查找视频流
    m_videoStreamIndex = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }

    if (m_videoStreamIndex == -1) {
        emit errorOccurred("未找到视频流");
        return false;
    }

    // 获取编解码器参数
    AVCodecParameters* codecpar = m_formatContext->streams[m_videoStreamIndex]->codecpar;

    // 检查视频宽度是否为128（RGBA格式：128像素 × 4通道 = 512 DMX通道）
    m_videoWidth = codecpar->width;
    m_videoHeight = codecpar->height;

    if (m_videoWidth != 512 || m_videoHeight < 1) {
        emit errorOccurred(QString("尺寸必须为512x1，当前尺寸: %1x%2").arg(m_videoWidth).arg(m_videoHeight));
        return false;
    }

    // 根据视频高度调整Universe数量
    adjustUniverseCount(m_videoHeight);

    // 查找编解码器
    m_codec = avcodec_find_decoder(codecpar->codec_id);
    if (!m_codec) {
        emit errorOccurred("未找到合适的解码器");
        return false;
    }

    // 检查是否为FFV1编码
    if (m_codec->id != AV_CODEC_ID_FFV1) {
        emit errorOccurred("视频必须使用FFV1编码");
        return false;
    }

    // 分配编解码器上下文
    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        emit errorOccurred("无法分配编解码器上下文");
        return false;
    }

    // 复制编解码器参数
    if (avcodec_parameters_to_context(m_codecContext, codecpar) < 0) {
        emit errorOccurred("无法复制编解码器参数");
        return false;
    }

    // 打开编解码器
    if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0) {
        emit errorOccurred("无法打开编解码器");
        return false;
    }

    // 分配帧和包
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();

    if (!m_frame || !m_packet) {
        emit errorOccurred("内存分配失败");
        return false;
    }

    // 获取视频信息
    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    m_timeBase = av_q2d(videoStream->time_base);
    m_fps = av_q2d(videoStream->r_frame_rate);
    m_duration = m_formatContext->duration / (double)AV_TIME_BASE;

    /**
     * 函数级注释：打开视频后重置时间显示
     */
    m_currentTimestampMs = 0;
    widget->currentTimeEdit->setTime(QTime(0,0,0,0));
    // qDebug() << QString("视频已打开: %1x%2, FPS: %3, 时长: %4秒, Universe数量: %5")
    //                 .arg(m_videoWidth).arg(m_videoHeight).arg(m_fps).arg(m_duration).arg(currentUniverseCount);
    m_fileName=filename;
    return true;
}

void UniversePlaybackDataModel::startPlayback() {
    if (!m_formatContext) {
        return;
    }

    isPlaying = true;
    widget->updatePlaybackState(true);
    /**
     * 函数级注释：开始播放时重置时间显示为 00:00:00.000
     */
    m_currentTimestampMs = 0;
    widget->currentTimeEdit->setTime(QTime(0,0,0,0));
    playbackTimer->start();
}

void UniversePlaybackDataModel::stopPlayback() {
    isPlaying = false;
    playbackTimer->stop();
    widget->updatePlaybackState(false);

    // 重置到文件开头
    if (m_formatContext) {
        av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    }
}

bool UniversePlaybackDataModel::readNextFrame() {
    if (!m_formatContext || !m_codecContext) {
        return false;
    }

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {
            // 发送包到解码器
            if (avcodec_send_packet(m_codecContext, m_packet) < 0) {
                av_packet_unref(m_packet);
                continue;
            }

            // 接收解码后的帧
            int ret = avcodec_receive_frame(m_codecContext, m_frame);
            av_packet_unref(m_packet);

            if (ret == 0) {
                return true; // 成功解码一帧
            } else if (ret == AVERROR(EAGAIN)) {
                continue; // 需要更多数据
            } else {
                return false; // 解码错误
            }
        }
        av_packet_unref(m_packet);
    }

    return false; // 文件结束
}

void UniversePlaybackDataModel::extractAndUpdateDmxData() {
    /* 函数级注释：
     * 从当前帧提取 DMX 数据并写入 dmxDataList（每个 Universe 使用 512 字节的 QByteArray）。
     * 灰度格式：直接按列读取；RGBA 兼容路径：使用 R 通道。
     * 注意：按行（universe）遍历，未覆盖的列保持为 0。
     */
    if (!m_frame || !m_frame->data[0]) {
        return;
    }

    // 新方案：1行=1个Universe，宽度至少覆盖512像素
    int universeCount = m_videoHeight;
    int channelsPerUniverse = 512;
    int usableWidth = qMin(m_videoWidth, channelsPerUniverse);

    // 遍历每个Universe行
    for (int universe = 0; universe < universeCount; ++universe) {
        QByteArray dmxData;
        dmxData.resize(channelsPerUniverse);
        dmxData.fill(char(0x00));

        // 当前行起始地址
        const uint8_t* rowData = m_frame->data[0] + universe * m_frame->linesize[0];

        if (m_frame->format == AV_PIX_FMT_GRAY8) {
            // 单通道灰度：按列直接读取像素值
            for (int col = 0; col < usableWidth; ++col) {
                dmxData[col] = char(rowData[col]);
            }
        } else {
            // 兼容路径（旧RGBA）：使用R通道作为DMX值
            for (int col = 0; col < usableWidth; ++col) {
                const uint8_t* pixelData = rowData + col * 4; // RGBA步长4
                dmxData[col] = char(pixelData[0]);            // 取R通道
            }
        }

        // 写入当前 Universe 的数据
        dmxDataList[universe] = dmxData;
    }

    // 更新所有Universe输出
    updateAllUniverseData();
}

void UniversePlaybackDataModel::updateAllUniverseData() {
        for (int i = 0; i < currentUniverseCount; i++) {
            updateUniverseData(i);
        }
    }

void UniversePlaybackDataModel::updateUniverseData(int universeIndex) {
        /* 函数级注释：
         * 将指定 Universe 的 DMX 数据打包为 Artnet 数据结构并写入输出。
         * 由于 dmxDataList 为 QByteArray（元素类型为 char），读取时统一按 unsigned char 转为 int，
         * 避免符号扩展导致的负值（>127 的字节在有符号 char 上会变负）。
         */
        if (universeIndex < 0 || universeIndex >= universeOutputs.size()) {
            return;
        }
    
        universeOutputs[universeIndex] = std::make_shared<VariableData>();
    
        // 创建完整的Artnet Universe数据包
        QVariantMap artnetPacket;
    
        // Artnet协议头信息
        artnetPacket["protocol"] = "Art-Net";
        artnetPacket["version"] = 14;       // Artnet协议版本
        artnetPacket["opcode"] = 0x5000;    // ArtDMX操作码
    
        // Universe寻址信息（基础Universe + 索引）
        int baseUniverse = widget->universeSpinBox->value();
        int subnet = widget->subnetSpinBox->value();
        int net = widget->netSpinBox->value();
        int currentUniverse = (baseUniverse + universeIndex) % 16;
    
        artnetPacket["universe"] = currentUniverse;
        artnetPacket["subnet"] = subnet;
        artnetPacket["net"] = net;
    
        // 计算完整的Universe地址
        int fullUniverse = (net << 8) | (subnet << 4) | currentUniverse;
        artnetPacket["fullUniverse"] = fullUniverse;
    
        // DMX数据（无符号转换为 int）
        QVariantList dmxList;
        const QByteArray& dmx = dmxDataList[universeIndex];
        dmxList.reserve(512);
        for (int i = 0; i < 512; ++i) {
            dmxList.append(int(static_cast<unsigned char>(dmx[i])));
        }
        artnetPacket["dmxData"] = dmxList;
        artnetPacket["dataLength"] = 512;
    
        // 统计信息（无符号转换）
        int activeChannels = 0;
        artnetPacket["activeChannels"] = activeChannels;
        // 时间戳
        artnetPacket["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        // 设置输出数据
        universeOutputs[universeIndex]->insert("default", artnetPacket);
        // 发出数据更新信号
        Q_EMIT dataUpdated(universeIndex);
    }

void UniversePlaybackDataModel::onLoopStateChanged(bool enabled) {
    isLooping = enabled;
}