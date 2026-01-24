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
    
    // 初始化FFmpeg
    initializeFFmpeg();
    playbackTimer = new QTimer(this);
    playbackTimer->setInterval(1000 / AppConstants::ARTNET_OUTPUT_FPS); // 50fps
    connect(playbackTimer, &QTimer::timeout, this, &UniversePlaybackDataModel::onPlaybackTimer);

    // 连接界面信号
    connect(widget->universeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, [this](int val) { setUniverse(val); });
    connect(widget->subnetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, [this](int val) { setSubnet(val); });
    connect(widget->netSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, [this](int val) { setNet(val); });

    // 连接控制按钮信号
    connect(widget, &Nodes::UniversePlaybackInterface::clearDataClicked,
        this, &UniversePlaybackDataModel::onClearDataClicked);
        
    connect(widget, &Nodes::UniversePlaybackInterface::selectFileClicked,
        this, [this](QString fileName) { setFileName(fileName); });
        
    connect(widget, &Nodes::UniversePlaybackInterface::playClicked,
        this, [this]() { setIsPlaying(true); });
        
    connect(widget, &Nodes::UniversePlaybackInterface::stopClicked,
        this, [this]() { setIsPlaying(false); });
    
    // 连接循环播放信号
    connect(widget, &Nodes::UniversePlaybackInterface::loopStateChanged,
        this, [this](bool checked) { setIsLooping(checked); });
    NodeDelegateModel::registerExternalControl("/universe", widget->universeSpinBox);
    NodeDelegateModel::registerExternalControl("/subnet", widget->subnetSpinBox);
    NodeDelegateModel::registerExternalControl("/net", widget->netSpinBox);
    NodeDelegateModel::registerExternalControl("/clear", widget->clearButton);
    NodeDelegateModel::registerExternalControl("/play", widget->playButton);
    NodeDelegateModel::registerExternalControl("/stop", widget->stopButton);
    NodeDelegateModel::registerExternalControl("/loop", widget->loopCheckBox);
    NodeDelegateModel::registerExternalControl("/filename", widget->_fileSelectComboBox);
}

    
void UniversePlaybackDataModel::afterModelReady() {
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/universe"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/subnet"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/net"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/clear"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/filename"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

void UniversePlaybackDataModel::setUniverse(int universe) {
    QMutexLocker locker(&m_mutex);
    if (m_universe == universe) return;
    m_universe = universe;
    {
        QSignalBlocker blocker(widget->universeSpinBox);
        widget->universeSpinBox->setValue(m_universe);
    }
    updateAllUniverseDataInternal();
    emit universeChanged(m_universe);
    AbstractDelegateModel::stateFeedBack("/universe", m_universe);
}

void UniversePlaybackDataModel::setSubnet(int subnet) {
    QMutexLocker locker(&m_mutex);
    if (m_subnet == subnet) return;
    m_subnet = subnet;
    {
        QSignalBlocker blocker(widget->subnetSpinBox);
        widget->subnetSpinBox->setValue(m_subnet);
    }
    updateAllUniverseDataInternal();
    emit subnetChanged(m_subnet);
    AbstractDelegateModel::stateFeedBack("/subnet", m_subnet);
}

void UniversePlaybackDataModel::setNet(int net) {
    QMutexLocker locker(&m_mutex);
    if (m_net == net) return;
    m_net = net;
    {
        QSignalBlocker blocker(widget->netSpinBox);
        widget->netSpinBox->setValue(m_net);
    }
    updateAllUniverseDataInternal();
    emit netChanged(m_net);
    AbstractDelegateModel::stateFeedBack("/net", m_net);
}

void UniversePlaybackDataModel::setIsLooping(bool looping) {
    QMutexLocker locker(&m_mutex);
    if (m_isLooping == looping) return;
    m_isLooping = looping;
    {
        QSignalBlocker blocker(widget->loopCheckBox);
        widget->loopCheckBox->setChecked(m_isLooping);
    }
    emit isLoopingChanged(m_isLooping);
    AbstractDelegateModel::stateFeedBack("/loop", m_isLooping);
}

void UniversePlaybackDataModel::setIsPlaying(bool playing) {
    QMutexLocker locker(&m_mutex);
    if (m_isPlaying == playing) return;
    
    if (playing) {
        if (m_formatContext) {
            startPlayback();
        } else {
            // Can't play if no file loaded
             // Maybe try to reload if filename exists?
             if (!m_fileName.isEmpty()) {
                 if(openVideo(AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + m_fileName)) {
                     startPlayback();
                 }
             }
        }
    } else {
        stopPlayback();
    }
    
    emit isPlayingChanged(m_isPlaying);
    AbstractDelegateModel::stateFeedBack("/play", m_isPlaying);
}

void UniversePlaybackDataModel::setFileName(const QString& fileName) {
    QMutexLocker locker(&m_mutex);
    if (m_fileName == fileName) return;
    m_fileName = fileName;
    {
        QSignalBlocker blocker(widget->_fileSelectComboBox);
        widget->_fileSelectComboBox->setText(m_fileName);
    }
    
    if (!m_fileName.isEmpty()) {
        auto filePath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + m_fileName;
        if (openVideo(filePath)) {
            widget->updateFileInfo(filePath, m_duration, m_fps, m_videoHeight);
            updateNodeState(QtNodes::NodeValidationState::State::Valid);
        } else {
             updateNodeState(QtNodes::NodeValidationState::State::Error, "cannot open file");
             widget->showError("无法打开视频文件");
        }
    }
    
    emit fileNameChanged(m_fileName);
    AbstractDelegateModel::stateFeedBack("/filename", m_fileName);
}

void UniversePlaybackDataModel::onGlobalEvent(const GlobalEvent& ev) {
    if (ev.kind == GlobalEventKind::Command) {
        QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
        if (localPath == "universe") setUniverse(ev.payload.toInt());
        else if (localPath == "subnet") setSubnet(ev.payload.toInt());
        else if (localPath == "net") setNet(ev.payload.toInt());
        else if (localPath == "loop") setIsLooping(ev.payload.toBool());
        else if (localPath == "play") setIsPlaying(ev.payload.toBool());
        else if (localPath == "stop") setIsPlaying(false);
        else if (localPath == "clear") onClearDataClicked();
        else if (localPath == "filename") setFileName(ev.payload.toString());
    }
}

QString UniversePlaybackDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == PortType::Out) {
        return QString("UNIVERSE%1").arg(portIndex + 1);  // UNIVERSE1, UNIVERSE2, ...
    }
    else if (portType == PortType::In) {
        switch (portIndex) {
            case 0: return "PLAY";
            case 1: return "LOOP";
            case 2: return "STOP";
            case 3: return "CLEAR";
            default: return "";
        }
    }
    return "";
}

NodeDataType UniversePlaybackDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portIndex)
    Q_UNUSED(portType)
    return VariableData().type();
}

std::shared_ptr<NodeData> UniversePlaybackDataModel::outData(PortIndex const port)
{
    QMutexLocker locker(&m_mutex);
    if (port >= 0 && port < universeOutputs.size()) {
        return universeOutputs[port];
    }
    return nullptr;
}

void UniversePlaybackDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
{
    if (!data) return;

    auto variableData = std::dynamic_pointer_cast<VariableData>(data);
    if (!variableData) return;

    QVariant value = variableData->value();
    bool trigger = value.toBool();

    switch (portIndex) {
    case 0: // 播放
        if (trigger) setIsPlaying(true);
        else setIsPlaying(false);
        break;
    case 1: // 循环播放
        setIsLooping(trigger);
        break;
    case 2: // 停止
        if (trigger) setIsPlaying(false);
        break;
    case 3: // 清除数据
        if (trigger) onClearDataClicked();
        break;
    default:
        break;
    }
}

QJsonObject UniversePlaybackDataModel::save() const
{
    // Save uses internal variables, safe to call (mutex usually not needed if GUI thread only, 
    // but if autosave runs in thread, might need lock. Q_PROPERTY usually implies main thread).
    QMutexLocker locker(&m_mutex); 
    QJsonObject modelJson1;
    modelJson1["universe"] = m_universe;
    modelJson1["subnet"] = m_subnet;
    modelJson1["net"] = m_net;
    modelJson1["isLooping"] = m_isLooping;
    modelJson1["videoFilePath"] = m_fileName;

    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson["UniverseSettings"] = modelJson1;
    return modelJson;
}

void UniversePlaybackDataModel::load(const QJsonObject &p)
{
    // Load runs on creation, usually safe.
    QJsonValue v = p["UniverseSettings"];
    if (!v.isUndefined() && v.isObject()) {
        auto settings = v.toObject();

        setUniverse(settings["universe"].toInt(0));
        setSubnet(settings["subnet"].toInt(0));
        setNet(settings["net"].toInt(0));
        setIsLooping(settings["isLooping"].toBool(false));
        
        QString videoFilePath = settings["videoFilePath"].toString();
        if (!videoFilePath.isEmpty()) {
            setFileName(videoFilePath);
        }

        // updateAllUniverseData is called by setters
        NodeDelegateModel::load(p);
    }
}

void UniversePlaybackDataModel::onClearDataClicked() {
    QMutexLocker locker(&m_mutex);
    // 清空所有DMX数据
    for (int i = 0; i < dmxDataList.size(); i++) {
        dmxDataList[i].fill(0);
    }
    // 更新输出数据
    updateAllUniverseDataInternal();
    AbstractDelegateModel::stateFeedBack("/clear", true);
}

void UniversePlaybackDataModel::onPlaybackTimer() {
    QMutexLocker locker(&m_mutex);

    if (!m_isPlaying || !m_formatContext) {
        return;
    }

    if (readNextFrame()) {
        extractAndUpdateDmxData();
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
        if (m_isLooping) {
            if (av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD) >= 0) {
                avcodec_flush_buffers(m_codecContext);
                m_currentTimestampMs = 0;
                widget->currentTimeEdit->setTime(QTime(0,0,0,0));
            } else {
                setIsPlaying(false);
            }
        } else {
            setIsPlaying(false);
        }
    }
}

void UniversePlaybackDataModel::initializeFFmpeg() {
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
    m_isPlaying = false;
    currentUniverseCount = 0;
}

void UniversePlaybackDataModel::cleanupFFmpegInternal() {
    // Expects mutex to be locked by caller
    if (m_frame) av_frame_free(&m_frame);
    if (m_packet) av_packet_free(&m_packet);
    if (m_codecContext) avcodec_free_context(&m_codecContext);
    if (m_formatContext) avformat_close_input(&m_formatContext);
    
    m_frame = nullptr;
    m_packet = nullptr;
    m_codecContext = nullptr;
    m_formatContext = nullptr;
}

void UniversePlaybackDataModel::cleanupFFmpeg() {
    QMutexLocker locker(&m_mutex);
    cleanupFFmpegInternal();
}

void UniversePlaybackDataModel::adjustUniverseCount(int videoHeight)
{
    // Expects mutex to be locked by caller (openVideo)
    int newUniverseCount = videoHeight ;

    if (newUniverseCount != currentUniverseCount) {
        dmxDataList.clear();
        universeOutputs.clear();

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
    // Called by setFileName, which locks mutex.
    // Recursive mutex allows re-locking if needed, but here caller holds it.
    // But since this function is private and called by setFileName which locks, 
    // we don't need another locker if we trust callers. 
    // However, for safety if called from elsewhere:
    // QMutexLocker locker(&m_mutex); 
    // Since setFileName holds lock, and we use Recursive Mutex, this is fine.
    // But let's assume openVideo is internal helper.
    // Wait, it is private.
    // Let's assume caller holds lock.
    
    // cleanupFFmpegInternal expects lock.
    cleanupFFmpegInternal();
    initializeFFmpeg();

    if (avformat_open_input(&m_formatContext, filename.toUtf8().constData(), nullptr, nullptr) < 0) {
        emit errorOccurred(QString("无法打开视频文件: %1").arg(filename));
        return false;
    }

    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        emit errorOccurred("无法获取流信息");
        return false;
    }

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

    AVCodecParameters* codecpar = m_formatContext->streams[m_videoStreamIndex]->codecpar;

    m_videoWidth = codecpar->width;
    m_videoHeight = codecpar->height;

    if (m_videoWidth != 512 || m_videoHeight < 1) {
        emit errorOccurred(QString("尺寸必须为512x1，当前尺寸: %1x%2").arg(m_videoWidth).arg(m_videoHeight));
        return false;
    }

    adjustUniverseCount(m_videoHeight);

    m_codec = avcodec_find_decoder(codecpar->codec_id);
    if (!m_codec) {
        emit errorOccurred("未找到合适的解码器");
        return false;
    }

    if (m_codec->id != AV_CODEC_ID_FFV1) {
        emit errorOccurred("视频必须使用FFV1编码");
        return false;
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        emit errorOccurred("无法分配编解码器上下文");
        return false;
    }

    if (avcodec_parameters_to_context(m_codecContext, codecpar) < 0) {
        emit errorOccurred("无法复制编解码器参数");
        return false;
    }

    if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0) {
        emit errorOccurred("无法打开编解码器");
        return false;
    }

    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();

    if (!m_frame || !m_packet) {
        emit errorOccurred("内存分配失败");
        return false;
    }

    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    m_timeBase = av_q2d(videoStream->time_base);
    m_fps = av_q2d(videoStream->r_frame_rate);
    m_duration = m_formatContext->duration / (double)AV_TIME_BASE;

    m_currentTimestampMs = 0;
    widget->currentTimeEdit->setTime(QTime(0,0,0,0));
    // m_fileName is set by caller
    return true;
}

void UniversePlaybackDataModel::startPlayback() {
    // Expects mutex to be locked
    if (!m_formatContext) {
        return;
    }

    m_isPlaying = true;
    widget->updatePlaybackState(true);
    m_currentTimestampMs = 0;
    widget->currentTimeEdit->setTime(QTime(0,0,0,0));
    playbackTimer->start();
}

void UniversePlaybackDataModel::stopPlayback() {
    // Expects mutex to be locked
    m_isPlaying = false;
    playbackTimer->stop();
    widget->updatePlaybackState(false);

    if (m_formatContext) {
        av_seek_frame(m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    }
}

bool UniversePlaybackDataModel::readNextFrame() {
    // Expects mutex to be locked
    if (!m_formatContext || !m_codecContext) {
        return false;
    }

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {
            if (avcodec_send_packet(m_codecContext, m_packet) < 0) {
                av_packet_unref(m_packet);
                continue;
            }

            int ret = avcodec_receive_frame(m_codecContext, m_frame);
            av_packet_unref(m_packet);

            if (ret == 0) {
                return true;
            } else if (ret == AVERROR(EAGAIN)) {
                continue;
            } else {
                return false;
            }
        }
        av_packet_unref(m_packet);
    }

    return false;
}

void UniversePlaybackDataModel::extractAndUpdateDmxData() {
    // Expects mutex to be locked
    if (!m_frame || !m_frame->data[0]) {
        return;
    }

    int universeCount = m_videoHeight;
    int channelsPerUniverse = 512;
    int usableWidth = qMin(m_videoWidth, channelsPerUniverse);

    for (int universe = 0; universe < universeCount; ++universe) {
        // dmxDataList accessed here
        QByteArray& dmxData = dmxDataList[universe];
        
        // Ensure size is correct (should be from adjustUniverseCount)
        if (dmxData.size() != channelsPerUniverse) {
             dmxData.resize(channelsPerUniverse);
             dmxData.fill(char(0x00));
        }

        const uint8_t* rowData = m_frame->data[0] + universe * m_frame->linesize[0];

        if (m_frame->format == AV_PIX_FMT_GRAY8) {
            for (int col = 0; col < usableWidth; ++col) {
                dmxData[col] = char(rowData[col]);
            }
        } else {
            // Assume RGBA/RGB, but check logic. 
            // Original code: const uint8_t* pixelData = rowData + col * 4; dmxData[col] = char(pixelData[0]);
            // This takes R channel of each pixel.
            for (int col = 0; col < usableWidth; ++col) {
                const uint8_t* pixelData = rowData + col * 4; 
                dmxData[col] = char(pixelData[0]);            
            }
        }
    }

    updateAllUniverseDataInternal();
}

void UniversePlaybackDataModel::updateAllUniverseDataInternal() {
    // Expects mutex to be locked
    for (int i = 0; i < currentUniverseCount; i++) {
        updateUniverseData(i);
    }
}

void UniversePlaybackDataModel::updateUniverseData(int universeIndex) {
    // Expects mutex to be locked
    if (universeIndex < 0 || universeIndex >= universeOutputs.size()) {
        return;
    }

    universeOutputs[universeIndex] = std::make_shared<VariableData>();

    QVariantMap artnetPacket;

    artnetPacket["protocol"] = "Art-Net";
    artnetPacket["version"] = 14;
    artnetPacket["opcode"] = 0x5000;

    int baseUniverse = m_universe;
    int subnet = m_subnet;
    int net = m_net;
    int currentUniverse = (baseUniverse + universeIndex) % 16;

    artnetPacket["universe"] = currentUniverse;
    artnetPacket["subnet"] = subnet;
    artnetPacket["net"] = net;

    int fullUniverse = (net << 8) | (subnet << 4) | currentUniverse;
    artnetPacket["fullUniverse"] = fullUniverse;

    QVariantList dmxList;
    const QByteArray& dmx = dmxDataList[universeIndex];
    dmxList.reserve(512);
    for (int i = 0; i < 512; ++i) {
        dmxList.append(int(static_cast<unsigned char>(dmx[i])));
    }
    artnetPacket["dmxData"] = dmxList;
    artnetPacket["dataLength"] = 512;

    int activeChannels = 0;
    artnetPacket["activeChannels"] = activeChannels;
    artnetPacket["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    universeOutputs[universeIndex]->insert("default", artnetPacket);
    Q_EMIT dataUpdated(universeIndex);
}
