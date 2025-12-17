
#include <QImage>
#include <QDebug>
#include "TimeLineDefines.h"
#include "AbstractClipModel.hpp"
#include "TimeCodeDefines.h"
#include "artnetclipmodel.hpp"
#include "ConstantDefines.h"

Clips::ArtnetClipModel::ArtnetClipModel(int start,const QString& filePath, QObject* parent)
            : AbstractClipModel(start, "Artnet", parent),m_filePath(filePath),m_editor(nullptr)
    {
        EMBEDWIDGET = false;
        SHOWBORDER = true;
        ClipColor=QColor("#66cccc");
        AbstractClipModel::initPropertyWidget();
        if (!filePath.isEmpty()) {
            loadArtnetInfo(filePath);
        }

        m_canNotify = true;
        // 初始化 ArtnetTransmitter 单例
        m_artnetTransmitter = ArtnetTransmitter::getInstance();

        // 异步触发一次初始通知，确保事件循环就绪
        QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
    }

Clips::ArtnetClipModel::~ArtnetClipModel() {}

// 设置文件路径并加载视频信息
void Clips::ArtnetClipModel::setMedia(const QVariant& path) {
   mediaSelector->setText(path.toString());
   emit mediaSelector->textChanged(path.toString());
}

void Clips::ArtnetClipModel::setStart(int start)  {
    AbstractClipModel::setStart(start);
    QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
}
void Clips::ArtnetClipModel::setEnd(int end)   {
    AbstractClipModel::setEnd(end);
    QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
}

// 重写保存和加载函数
QJsonObject Clips::ArtnetClipModel::save() const  {
    QJsonObject json = AbstractClipModel::save();
    json["file"] = m_filePath;
    json["type"] = "Artnet";
    json["targetHost"]=m_targetHost;
    json["startUniverse"]=m_startUniverse->value();
    json["startTime"]=timecode_frame_to_time(frames_to_timecode_frame(start(),getTimeCodeType()),getTimeCodeType());
    json["endTime"]=timecode_frame_to_time(frames_to_timecode_frame(end(),getTimeCodeType()),getTimeCodeType());
    return json;
}

void Clips::ArtnetClipModel::load(const QJsonObject& json)  {

    m_filePath = json["file"].toString();
    if (!m_filePath.isEmpty()) {
        loadArtnetInfo(AppConstants::MEDIA_LIBRARY_STORAGE_DIR+"/"+m_filePath);
    }
    mediaSelector->setText(m_filePath);
    AbstractClipModel::load(json);
    m_startUniverse->setValue(json["startUniverse"].toInt());
    targetHostEdit->setText( json["targetHost"].toString());
}

QVariant Clips::ArtnetClipModel::data(int role) const {
    switch (role) {
    case Qt::DisplayRole:
        return m_filePath;
    case TimelineRoles::ClipModelRole:
        return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast< ArtnetClipModel*>(this)));
    default:
        return AbstractClipModel::data(role);
    }
}


QWidget* Clips::ArtnetClipModel::clipPropertyWidget() {
    m_editor = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(4);
    // 基本设置组
    auto* basicGroup = new QGroupBox("文件属性", m_editor);
    auto* basicLayout = new QGridLayout(basicGroup);
    // 文件名显示
    // 媒体文件选择
    basicLayout->addWidget(new QLabel("回放起始域:"), 1, 0,1,1);
    // 函数级注释：使用成员指针持有起始域控件，便于 currentData 读取
    m_startUniverse = new QSpinBox(basicGroup);
    m_startUniverse->setMinimum(0);
    m_startUniverse->setMaximum(65536);
    registerOSCControl("/universe", m_startUniverse);
    basicLayout->addWidget(m_startUniverse, 1, 1,1,1);

    basicLayout->addWidget(new QLabel("目标主机:"), 3, 0,1,1);
    targetHostEdit = new QLineEdit(basicGroup);
    targetHostEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")));
    targetHostEdit->setToolTip("输入目标主机IP地址，如：192.168.1.100 或 192.168.1.255（广播）");
    registerOSCControl("/host", targetHostEdit);
    // 设置默认值
    targetHostEdit->setText(m_targetHost);
    basicLayout->addWidget(targetHostEdit, 3, 1,1,1);
    connect(targetHostEdit,&QLineEdit::textChanged,[this]() {
        m_targetHost = targetHostEdit->text().trimmed();
    });
    basicLayout->addWidget(new QLabel("Artnet数据:"), 2, 0,1,1);
    mediaSelector = new SelectorComboBox(MediaLibrary::Category::DMX,basicGroup);
    registerOSCControl("/file", mediaSelector);
    basicLayout->addWidget(mediaSelector, 2, 1,1,1);
    videoInfoLabel=new QLabel(basicGroup);
    // 连接信号槽
    basicLayout->addWidget(videoInfoLabel, 4, 0,2,2);
    connect(mediaSelector,&SelectorComboBox::textChanged,[=](const QString& text){
        if (m_filePath != text) {
           m_filePath = text;
           loadArtnetInfo(AppConstants::MEDIA_LIBRARY_STORAGE_DIR+"/"+m_filePath);
           emit filePathChanged(text);
           emit onPropertyChanged();
        }
    });

    mainLayout->addWidget(basicGroup);
    return m_editor;
}

        /**
         * 函数级注释：在 timeline 上按帧回放 DMX 数据
         * - 当 currentFrame 处于剪辑区间内时：
         *   1) 打开并定位到对应视频帧（基于 timeline 帧率与 FFmpeg time_base）
         *   2) 提取每一行（universe）的 512 字节 DMX 数据到 m_dmxDataList
         *   3) 构建与 UniversePlaybackDataModel 对齐的 artnetPacket 列表返回
         *   4) 同步通过 m_artnetTransmitter 将每个 Universe 的帧入队广播
         * - 在剪辑开始帧返回 "/file"，结束前一帧返回 "/stop" 以保持与其他 Clip 一致的习惯
         */
QVariantMap Clips::ArtnetClipModel::currentData(int currentFrame) const {
    QVariantMap data;

    // 非剪辑区间，直接返回空
    if (currentFrame < start() || currentFrame >= end()) {
        return data;
    }

    // 在开始帧返回文件标记（对齐其他 Clip）
    if (currentFrame == start()) {
        data["/file"] = m_filePath;
    }
    // 在结束前一帧返回停止标记（对齐其他 Clip）
    if (currentFrame + 1 == end()) {
        data["/stop"] = 1;
    }

    // 文件路径校验
    if (m_filePath.isEmpty()) {
        return data;
    }

    // 懒加载打开视频
    const_cast<ArtnetClipModel*>(this)->ensureVideoOpened(AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + m_filePath);
    if (!m_formatContext || !m_codecContext || m_videoStreamIndex < 0) {
        return data;
    }

    // 解码并提取当前帧 DMX
    if (const_cast<ArtnetClipModel*>(this)->readFrameAt(currentFrame)) {
        const_cast<ArtnetClipModel*>(this)->extractDMXFromCurrentFrame();

        // 构建 artnetPacket 列表
        QVariantList packets;
        int baseUniverse = (m_startUniverse ? m_startUniverse->value() : 0);
        int subnet = 0; // Timeline 版本简化：默认 subnet=0
        int net = 0;    // Timeline 版本简化：默认 net=0

        for (int universeIndex = 0; universeIndex < m_universeCount; ++universeIndex) {
            QVariantMap artnetPacket;
            artnetPacket["protocol"] = "Art-Net";
            artnetPacket["version"] = 14;
            artnetPacket["opcode"] = 0x5000;

            int currentUniverse = (baseUniverse + universeIndex) % 16;
            artnetPacket["universe"] = currentUniverse;
            artnetPacket["subnet"] = subnet;
            artnetPacket["net"] = net;
            int fullUniverse = (net << 8) | (subnet << 4) | currentUniverse;
            artnetPacket["fullUniverse"] = fullUniverse;

            const QByteArray& dmx = m_dmxDataList[universeIndex];
            QVariantList dmxList;
            dmxList.reserve(512);
            for (int i = 0; i < 512; ++i) {
                dmxList.append(int(static_cast<unsigned char>(dmx[i])));
            }
            artnetPacket["dmxData"] = dmxList;
            artnetPacket["dataLength"] = 512;

            packets.push_back(artnetPacket);
        }
        data["artnetPackets"] = packets;

        // 同步发送：避免重复发送同一帧
        const_cast<ArtnetClipModel*>(this)->sendArtnetFrames(baseUniverse, subnet, net, currentFrame);
    }

    return data;
}



void Clips::ArtnetClipModel::onPropertyChanged(){

}


void Clips::ArtnetClipModel::loadArtnetInfo(const QString& path) {
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, path.toUtf8().constData(), nullptr, nullptr) < 0) {
        return;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return;
    }

    // 查找视频流
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVStream* stream = formatContext->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 获取视频帧率
            // 计算视频总帧数和时长
            double duration = stream->duration * av_q2d(stream->time_base);
            if (duration <= 0) {
                duration = formatContext->duration / (double)AV_TIME_BASE;
            }
            // 计算总时长在timeline中的映射
            int totalFrames = static_cast<int>(duration * timecode_frames_per_sec(getTimeCodeType())); // Round to nearest frame
            if (totalFrames > 0) {
                setEnd(start() + totalFrames);
            }
            updateFileInfo(m_filePath,duration,av_q2d(stream->avg_frame_rate),stream->codecpar->height);
            break;
        }
    }

    avformat_close_input(&formatContext);

}

/**
 * 函数级注释：确保视频已打开并初始化 FFmpeg 解码器（FFmpeg 7.1）
 * - 校验尺寸：宽度=512，height>=1
 * - 设定 m_universeCount = m_videoHeight
 * - 仅当尚未打开或路径改变时执行
 */
void Clips::ArtnetClipModel::ensureVideoOpened(const QString& path) {
    if (m_openedPath == path && m_formatContext) {
        return;
    }

    // 清理旧资源
    if (m_frame) { av_frame_free(&m_frame); m_frame = nullptr; }
    if (m_packet) { av_packet_free(&m_packet); m_packet = nullptr; }
    if (m_codecContext) { avcodec_free_context(&m_codecContext); m_codecContext = nullptr; }
    if (m_formatContext) { avformat_close_input(&m_formatContext); m_formatContext = nullptr; }

    // 打开输入
    if (avformat_open_input(&m_formatContext, path.toUtf8().constData(), nullptr, nullptr) < 0) {
        return;
    }
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
        return;
    }

    // 查找视频流
    m_videoStreamIndex = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = static_cast<int>(i);
            break;
        }
    }
    if (m_videoStreamIndex == -1) {
        return;
    }

    // 编解码器参数
    AVCodecParameters* codecpar = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    m_videoWidth = codecpar->width;
    m_videoHeight = codecpar->height;

    if (m_videoWidth != 512 || m_videoHeight < 1) {
        // 尺寸不合法
        return;
    }

    // 查找并打开解码器（FFV1）
    m_codec = avcodec_find_decoder(codecpar->codec_id);
    if (!m_codec || m_codec->id != AV_CODEC_ID_FFV1) {
        return;
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        return;
    }
    if (avcodec_parameters_to_context(m_codecContext, codecpar) < 0) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
        return;
    }
    if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
        return;
    }

    // 分配帧与包
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    if (!m_frame || !m_packet) {
        if (m_frame) av_frame_free(&m_frame);
        if (m_packet) av_packet_free(&m_packet);
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
        return;
    }

    // 基本信息
    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    m_timeBase = av_q2d(videoStream->time_base);
    m_fps = av_q2d(videoStream->r_frame_rate);
    m_duration = m_formatContext->duration / (double)AV_TIME_BASE;

    // Universe 数量与缓冲初始化
    m_universeCount = m_videoHeight;
    m_dmxDataList.clear();
    for (int i = 0; i < m_universeCount; ++i) {
        QByteArray universeData;
        universeData.resize(512);
        universeData.fill(char(0x00));
        m_dmxDataList.append(universeData);
    }

    m_lastFrameIndex = -1;
    m_openedPath = path;
}

/**
 * 函数级注释：按 timeline 的 currentFrame 定位并解码一帧
 * - timeline 帧号 -> 秒 = (frame - start) / timelineFps
 * - 秒 -> 流时间戳单位 = 秒 / time_base
 * - av_seek_frame + avcodec_flush_buffers 再读取解码
 */
bool Clips::ArtnetClipModel::readFrameAt(int currentFrame) {
    if (!m_formatContext || !m_codecContext) {
        return false;
    }

    double timelineFps = timecode_frames_per_sec(getTimeCodeType());
    int relFrame = currentFrame - start();
    if (relFrame < 0) relFrame = 0;
    double seconds = relFrame / timelineFps;
    int64_t targetTs = static_cast<int64_t>(seconds / m_timeBase);

    // 定位到目标时间戳
    if (av_seek_frame(m_formatContext, m_videoStreamIndex, targetTs, AVSEEK_FLAG_BACKWARD) < 0) {
        return false;
    }
    avcodec_flush_buffers(m_codecContext);

    // 解码第一帧
    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {
            if (avcodec_send_packet(m_codecContext, m_packet) < 0) {
                av_packet_unref(m_packet);
                continue;
            }
            int ret = avcodec_receive_frame(m_codecContext, m_frame);
            av_packet_unref(m_packet);
            if (ret == 0) {
                m_lastFrameIndex = currentFrame;
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

/**
 * 函数级注释：从当前解码帧提取每个 Universe 的 512 字节 DMX 数据
 * - 灰度：直接读取每列像素值
 * - 非灰度（兼容旧 RGBA）：取 R 通道
 */
void Clips::ArtnetClipModel::extractDMXFromCurrentFrame() {
    if (!m_frame || !m_frame->data[0]) {
        return;
    }
    int channelsPerUniverse = 512;
    int usableWidth = qMin(m_videoWidth, channelsPerUniverse);

    for (int universe = 0; universe < m_universeCount; ++universe) {
        QByteArray dmxData;
        dmxData.resize(channelsPerUniverse);
        dmxData.fill(char(0x00));

        const uint8_t* rowData = m_frame->data[0] + universe * m_frame->linesize[0];

        if (m_frame->format == AV_PIX_FMT_GRAY8) {
            for (int col = 0; col < usableWidth; ++col) {
                dmxData[col] = char(rowData[col]);
            }
        } else {
            for (int col = 0; col < usableWidth; ++col) {
                const uint8_t* pixelData = rowData + col * 4; // RGBA 步长 4
                dmxData[col] = char(pixelData[0]);            // 取 R 通道
            }
        }
        m_dmxDataList[universe] = dmxData;
    }
}


/**
 * 函数级注释：将当前帧的每个 Universe 的 DMX 数据封装为 ArtnetFrame 并入队发送
 * - 参考 ArtnetOutDataModel 的发送逻辑
 * - 使用广播地址 `m_targetHost`；可根据需要扩展属性面板增加目标主机设置
 * - 通过 m_lastSentFrameIndex 防止 timeline 对同帧的重复调用造成重复发送
 */
void Clips::ArtnetClipModel::sendArtnetFrames(int baseUniverse, int subnet, int net, int currentFrame) {
    if (!m_artnetTransmitter) {
        return;
    }
    if (m_lastSentFrameIndex == currentFrame) {
        return;
    }

    for (int universeIndex = 0; universeIndex < m_universeCount; ++universeIndex) {
        int currentUniverse = (baseUniverse + universeIndex) % 16;
        int fullUniverse = (net << 8) | (subnet << 4) | currentUniverse;

        ArtnetFrame frame;
        frame.host = m_targetHost;
        frame.sequence = 0;
        frame.timestamp = QDateTime::currentMSecsSinceEpoch();
        frame.universe = static_cast<quint16>(fullUniverse);
        frame.setDmxData(m_dmxDataList[universeIndex]);

        m_artnetTransmitter->enqueueFrame(frame);
    }

    m_lastSentFrameIndex = currentFrame;
}

