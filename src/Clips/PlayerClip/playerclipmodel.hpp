#ifndef PLAYERCLIPMODEL_HPP
#define PLAYERCLIPMODEL_HPP

#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include "Widget/TimeLineWidget/TimelineAbstract/timelinetypes.h"
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include <QPushButton>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class PlayerClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    explicit PlayerClipModel(int start, int end, const QString& filePath = QString(), QObject* parent = nullptr)
        : AbstractClipModel(start, end, "Player", TimecodeType::PAL, parent), 
          m_filePath(filePath),
          m_oscHost("127.0.0.1:8992"),
          m_playerID("player1"),
          m_editor(nullptr)
    {
        EMBEDWIDGET = false;
        SHOWBORDER = false;
        setOscHost(m_oscHost);
        
        if (!filePath.isEmpty()) {
            loadVideoInfo(filePath);
        }
    }

    ~PlayerClipModel() override {
        delete m_editor;
    }

    // 设置文件路径并加载视频信息
    void setFilePath(const QString& path) { 
        if (m_filePath != path) {
            m_filePath = path;
            loadVideoInfo(path);
            emit filePathChanged(path);
        }
    }

    // 其他 getter/setter 保持不变
    QString filePath() const { return m_filePath; }
   

    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();
        json["filePath"] = m_filePath;
        json["oscHost"] = m_oscHost;
        json["playerID"] = m_playerID;
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_filePath = json["filePath"].toString();
        
        m_oscHost = json["oscHost"].toString(); 
        m_playerID = json["playerID"].toString();
        if(!m_filePath.isEmpty()) {
            loadVideoInfo(m_filePath);
        }
        setOscHost(m_oscHost);
    }

    QVariant data(int role) const override {
        switch (role) {
            case Qt::DisplayRole:
                return m_filePath;
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<PlayerClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
        }
    }

    virtual QWidget* clipPropertyWidget() override{
        m_editor = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(4);
        // 基本设置组
        auto* basicGroup = new QGroupBox("文件属性", m_editor);
        auto* basicLayout = new QGridLayout(basicGroup);
        // 时间相关控件
        basicLayout->addWidget(new QLabel("文件时长:"), 1, 0);
        auto* durationBox = new QLineEdit(basicGroup);
        durationBox->setReadOnly(true);
        // 将帧数转换为时间码格式显示
        durationBox->setText(FramesToTimeString(length(), Timecode::getFrameRate(getTimecodeType())));
        basicLayout->addWidget(durationBox, 1, 1);
         // 文件名显示
        auto* fileNameLabel = new QLineEdit(filePath(), basicGroup);
        fileNameLabel->setReadOnly(true);
        basicLayout->addWidget(fileNameLabel, 2, 0, 1, 2);
        // 媒体文件选择
        basicLayout->addWidget(new QLabel("媒体文件:"), 4, 0);
        auto* mediaButton = new QPushButton("选择媒体文件", basicGroup);
        basicLayout->addWidget(mediaButton, 4, 1);        
        // 连接信号槽
        connect(mediaButton, &QPushButton::clicked, [=]() {
            QString filePath = QFileDialog::getOpenFileName(m_editor,
                "选择媒体文件",
                "",
                "视频文件 (*.mp4 *.avi *.mkv *.mov);;"
                "音频文件 (*.mp3 *.wav *.aac *.flac);;"
                "图片文件 (*.jpg *.jpeg *.png *.bmp);;"
                "所有文件 (*)");
            
            if (!filePath.isEmpty()) {
                setFilePath(filePath);  // 这会触发视频信息加载和长度更新
                fileNameLabel->setText(filePath);
                // 更新时长显示，使用时间码格式
                durationBox->setText(FramesToTimeString(length(), Timecode::getFrameRate(getTimecodeType())));
     
            }
        });

        mainLayout->addWidget(basicGroup);
        auto* playGroup = new QGroupBox("播放器设置", m_editor);
        auto* playLayout = new QGridLayout(playGroup);
        QLineEdit* playerName = new QLineEdit(playGroup);
        playerName->setText(oscHost());
        connect(playerName, &QLineEdit::editingFinished, [=]() {
            setOscHost(playerName->text());
        });
        playLayout->addWidget(playerName, 0, 0,1,2);
        auto* playerID = new QLineEdit(playGroup);
        playerID->setText(this->playerID());
        connect(playerID, &QLineEdit::editingFinished, [=]() {
            setPlayerID(playerID->text());
        });
        playLayout->addWidget(playerID, 0, 2,1,2);
        auto* playButton = new QPushButton("播放", playGroup);
        connect(playButton, &QPushButton::clicked, [=]() {
            play();
        });
        auto* stopButton = new QPushButton("停止", playGroup);
        connect(stopButton, &QPushButton::clicked, [=]() {
            stop();
        });
        auto* pauseButton = new QPushButton("暂停", playGroup);
        connect(pauseButton, &QPushButton::clicked, [=]() {
            pause();
        });
        auto* fullScreenButton = new QPushButton("全屏", playGroup);
        fullScreenButton->setCheckable(true);
        connect(fullScreenButton, &QPushButton::clicked, [=]() {
            fullScreen(fullScreenButton->isChecked());
        });
        playLayout->addWidget(playButton, 1, 0);
        playLayout->addWidget(stopButton, 1, 1);
        playLayout->addWidget(pauseButton, 1, 2);
        playLayout->addWidget(fullScreenButton, 1, 3);
        mainLayout->addWidget(playGroup);
        return m_editor;
    }

    QVariantMap currentData(int currentFrame) const override {
        QVariantMap data;
        if (!m_oscSender) return data;

        if(currentFrame == start()) {
            data["/"+m_playerID+"/file"] = m_filePath;
            m_oscSender->sendMessage(data);
            return data;
        } 
         if(currentFrame+1 == end()) {
            data["/"+m_playerID+"/stop"]=1;
            m_oscSender->sendMessage(data);
            return data;
        } 
       
        return data;
    }

    QString oscHost() const { return m_oscHost; }
    QString playerID() const { return m_playerID; }
public Q_SLOTS:
    void setOscHost(const QString& host) {
        // 检查host字符串是否为空
        if (host.isEmpty()) {
            qDebug() << "Empty host string";
            return;
        }

        // 检查是否包含端口号
        QStringList parts = host.split(":");
        if (parts.size() != 2) {
            qDebug() << "Invalid host format. Expected format: host:port";
            return;
        }

        // 获取主机名和端口
        QString newHost = parts[0];
        bool ok;
        int port = parts[1].toInt(&ok);
        
        // 验证端口号
        if (!ok || port <= 0 || port > 65535) {
            qDebug() << "Invalid port number:" << parts[1];
            return;
        }

        // 更新成员变量
        m_oscHost = host;

        // 使用智能指针
        if (m_oscSender) {
            m_oscSender->setHost(newHost, port);
        } else {
            m_oscSender.reset(new OSCSender(newHost, port, this));
        }
    }
    void setPlayerID(const QString& id) {
        m_playerID = id;
    }
    void play() {
        QVariantMap data;
       
        data["/"+m_playerID+"/play"] = 1;
        
        m_oscSender->sendMessage(data);
    }

    void stop() {
        QVariantMap data;
        data["/"+m_playerID+"/stop"] = 1;
        m_oscSender->sendMessage(data);
    }
    void pause() {
        QVariantMap data;
        data["/"+m_playerID+"/pause"] = 1;
        m_oscSender->sendMessage(data);
    }
    void fullScreen(bool isFullScreen) {
        QVariantMap data;
        data["/"+m_playerID+"/fullscreen"] = isFullScreen;
        m_oscSender->sendMessage(data);
    }
    void setFile(const QString& filePath) {
        m_filePath = filePath;
        QVariantMap data;
        data["/"+m_playerID+"/file"] = filePath;
        m_oscSender->sendMessage(data);
    }
    
private:
    void loadVideoInfo(const QString& path) {
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
                int totalFrames = static_cast<int>(duration * Timecode::getFrameRate(m_timecodeType)); // Round to nearest frame
                if (totalFrames > 0) {
                    setEnd(start() + totalFrames);
                }
                break;
            }
        }

        avformat_close_input(&formatContext);
    }

    QString m_filePath;
    std::unique_ptr<OSCSender> m_oscSender; // 使用智能指针
    QString m_oscHost;
    QString m_playerID;
    QWidget* m_editor;
};

#endif // PLAYERCLIPMODEL_HPP 