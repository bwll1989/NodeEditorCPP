//
// Created by WuBin on 2025/8/21.
//
#include "ArtnetPlaybackDataModel.hpp"
using namespace Nodes;

ArtnetPlaybackDataModel::ArtnetPlaybackDataModel()
{
    InPortCount = 4;   // 无输入端口，从视频文件读取
    OutPortCount = 1;   // 初始1个输出端口，会根据视频动态调整
    PortEditable = true;
    CaptionVisible = true;
    Caption = "Artnet Playback";
    WidgetEmbeddable = false;
    Resizable = false;

    // 初始化FFmpeg
    initializeFFmpeg();

    // 初始化播放定时器（25fps = 40ms间隔）
    playbackTimer = new QTimer(this);
    playbackTimer->setInterval(40); // 25fps
    connect(playbackTimer, &QTimer::timeout, this, &ArtnetPlaybackDataModel::onPlaybackTimer);

    // 连接界面信号
    connect(widget->universeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &ArtnetPlaybackDataModel::onUniverseChanged);
    connect(widget->subnetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &ArtnetPlaybackDataModel::onSubnetChanged);
    connect(widget->netSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &ArtnetPlaybackDataModel::onNetChanged);

    // 连接控制按钮信号
    connect(widget, &Nodes::ArtnetPlaybackInterface::clearDataClicked,
        this, &ArtnetPlaybackDataModel::onClearDataClicked);
    connect(widget, &Nodes::ArtnetPlaybackInterface::selectFileClicked,
        this, &ArtnetPlaybackDataModel::onSelectFileClicked);
    connect(widget, &Nodes::ArtnetPlaybackInterface::playClicked,
        this, &ArtnetPlaybackDataModel::onPlayClicked);
    connect(widget, &Nodes::ArtnetPlaybackInterface::stopClicked,
        this, &ArtnetPlaybackDataModel::onStopClicked);
    
    // 连接循环播放信号
    connect(widget, &Nodes::ArtnetPlaybackInterface::loopStateChanged,
        this, &ArtnetPlaybackDataModel::onLoopStateChanged);  // 新增：连接循环播放信号

    // 注册OSC控制
    NodeDelegateModel::registerOSCControl("/universe", widget->universeSpinBox);
    NodeDelegateModel::registerOSCControl("/subnet", widget->subnetSpinBox);
    NodeDelegateModel::registerOSCControl("/net", widget->netSpinBox);
    NodeDelegateModel::registerOSCControl("/clear", widget->clearButton);
    NodeDelegateModel::registerOSCControl("/play", widget->playButton);
    NodeDelegateModel::registerOSCControl("/stop", widget->stopButton);
}


void ArtnetPlaybackDataModel::onUniverseChanged(int universe) {
    Q_UNUSED(universe)
    updateAllUniverseData();
}

void ArtnetPlaybackDataModel::onSubnetChanged(int subnet) {
    Q_UNUSED(subnet)
    updateAllUniverseData();
}

void ArtnetPlaybackDataModel::onNetChanged(int net) {
    Q_UNUSED(net)
    updateAllUniverseData();
}

void ArtnetPlaybackDataModel::onClearDataClicked() {
    // 清空所有DMX数据
    for (int i = 0; i < dmxDataList.size(); i++) {
        dmxDataList[i].fill(0);
    }

    // 更新输出数据
    updateAllUniverseData();
}

void ArtnetPlaybackDataModel::onSelectFileClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "选择Artnet映射文件",
        "",
        "HAP视频文件 (*.mov *.mp4);;所有文件 (*)"
    );

    if (!fileName.isEmpty()) {
        if (openVideo(fileName)) {
            widget->updateFileInfo(fileName, m_duration, m_fps);
        } else {
            widget->showError("无法打开视频文件");
        }
    }
}

void ArtnetPlaybackDataModel::onPlayClicked() {
    if (m_formatContext && !isPlaying) {
        startPlayback();
    }
}


void ArtnetPlaybackDataModel::onStopClicked() {
    stopPlayback();
}

/**
 * @brief 播放定时器回调，处理视频帧读取和循环播放逻辑
 */
void ArtnetPlaybackDataModel::onPlaybackTimer() {
    if (!isPlaying || !m_formatContext) {
        return;
    }

    // 读取下一帧
    if (readNextFrame()) {
        // 提取DMX数据并更新输出
        extractAndUpdateDmxData();
    } else {
        // 播放结束，检查是否需要循环播放
        if (isLooping) {
            // 循环播放：重新定位到文件开头
            if (av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD) >= 0) {
                // 清空解码器缓冲区
                avcodec_flush_buffers(m_codecContext);
                qDebug() << "循环播放：重新开始播放";
            } else {
                qDebug() << "循环播放失败：无法定位到文件开头";
                stopPlayback();
            }
        } else {
            // 非循环播放：停止播放
            stopPlayback();
        }
    }
}

void ArtnetPlaybackDataModel::initializeFFmpeg() {
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

void ArtnetPlaybackDataModel::cleanupFFmpegInternal() {
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

void ArtnetPlaybackDataModel::cleanupFFmpeg() {
    QMutexLocker locker(&m_mutex);
    cleanupFFmpegInternal();
}

void ArtnetPlaybackDataModel::adjustUniverseCount(int videoHeight)
{
    // 计算Universe数量（每4行对应一个Universe）
    int newUniverseCount = videoHeight / 4;

    if (newUniverseCount != currentUniverseCount) {
        // 清空现有数据
        dmxDataList.clear();
        universeOutputs.clear();

        // 重新初始化数据结构
        for (int i = 0; i < newUniverseCount; i++) {
            QVector<int> universeData(512, 0);
            dmxDataList.append(universeData);

            auto outputData = std::make_shared<VariableData>();
            universeOutputs.append(outputData);
        }

        // // 更新输出端口数量
        // OutPortCount = newUniverseCount;
        currentUniverseCount = newUniverseCount;}
}

bool ArtnetPlaybackDataModel::openVideo(const QString& filename)
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

    if (m_videoWidth != 128) {
        emit errorOccurred(QString("视频宽度必须为128像素，当前宽度: %1").arg(m_videoWidth));
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

    // 检查是否为HAP编码
    if (m_codec->id != AV_CODEC_ID_HAP) {
        qDebug() << "警告：视频不是HAP编码，但将尝试解码";
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

    qDebug() << QString("视频已打开: %1x%2, FPS: %3, 时长: %4秒, Universe数量: %5")
                    .arg(m_videoWidth).arg(m_videoHeight).arg(m_fps).arg(m_duration).arg(currentUniverseCount);
    m_fileName=filename;
    return true;
}

void ArtnetPlaybackDataModel::startPlayback() {
    if (!m_formatContext) {
        return;
    }

    isPlaying = true;
    widget->updatePlaybackState(true);
    playbackTimer->start();
}

void ArtnetPlaybackDataModel::stopPlayback() {
    isPlaying = false;
    playbackTimer->stop();
    widget->updatePlaybackState(false);

    // 重置到文件开头
    if (m_formatContext) {
        av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    }
}

bool ArtnetPlaybackDataModel::readNextFrame() {
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

void ArtnetPlaybackDataModel::extractAndUpdateDmxData() {
    if (!m_frame || !m_frame->data[0]) {
        return;
    }

    // 使用当前实际的Universe数量
    for (int universe = 0; universe < currentUniverseCount; universe++) {
        // 清空当前Universe数据
        dmxDataList[universe].fill(0);

        // 处理当前Universe的4行
        int startRow = universe * 4;
        int channelIndex = 0;

        for (int row = startRow; row < startRow + 4 && row < m_videoHeight; row++) {
            uint8_t* rowData = m_frame->data[0] + row * m_frame->linesize[0];

            // 每行128像素，每个像素的RGBA四个通道作为DMX通道值
            for (int col = 0; col < 128 && col < m_videoWidth && channelIndex < 512; col++) {
                // RGBA格式，每个像素占4字节，分别对应4个DMX通道
                uint8_t* pixelData = rowData + col * 4;

                // R通道
                if (channelIndex < 512) {
                    dmxDataList[universe][channelIndex] = pixelData[0];
                    channelIndex++;
                }
                // G通道
                if (channelIndex < 512) {
                    dmxDataList[universe][channelIndex] = pixelData[1];
                    channelIndex++;
                }
                // B通道
                if (channelIndex < 512) {
                    dmxDataList[universe][channelIndex] = pixelData[2];
                    channelIndex++;
                }
                // A通道
                if (channelIndex < 512) {
                    dmxDataList[universe][channelIndex] = pixelData[3];
                    channelIndex++;
                }

                if (channelIndex >= 512) {
                    break;
                }
            }

            if (channelIndex >= 512) {
                break;
            }
        }
    }

    // 更新所有Universe输出
    updateAllUniverseData();
}

void ArtnetPlaybackDataModel::updateAllUniverseData() {
        for (int i = 0; i < currentUniverseCount; i++) {
            updateUniverseData(i);
        }
    }

void ArtnetPlaybackDataModel::updateUniverseData(int universeIndex) {
        if (universeIndex < 0 || universeIndex >= universeOutputs.size()) {
            return;
        }

        universeOutputs[universeIndex] = std::make_shared<VariableData>();

        // 创建完整的Artnet Universe数据包
        QVariantMap artnetPacket;

        // Artnet协议头信息
        artnetPacket["protocol"] = "Art-Net";
        artnetPacket["version"] = 14;  // Artnet协议版本
        artnetPacket["opcode"] = 0x5000;  // ArtDMX操作码

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

        // DMX数据
        QVariantList dmxList;
        for (int i = 0; i < 512; ++i) {
            dmxList.append(dmxDataList[universeIndex][i]);
        }
        artnetPacket["dmxData"] = dmxList;
        artnetPacket["dataLength"] = 512;

        // 统计信息
        int activeChannels = 0;
        int maxValue = 0;
        for (int i = 0; i < 512; ++i) {
            if (dmxDataList[universeIndex][i] > 0) {
                activeChannels++;
            }
            maxValue = qMax(maxValue, dmxDataList[universeIndex][i]);
        }
        artnetPacket["activeChannels"] = activeChannels;
        artnetPacket["maxValue"] = maxValue;

        // 时间戳
        artnetPacket["timestamp"] = QDateTime::currentMSecsSinceEpoch();

        // 设置输出数据
        universeOutputs[universeIndex]->insert("default", artnetPacket);

        // 发出数据更新信号
        Q_EMIT dataUpdated(universeIndex);
    }

void ArtnetPlaybackDataModel::onLoopStateChanged(bool enabled) {
    isLooping = enabled;
}