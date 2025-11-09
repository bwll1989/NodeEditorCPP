#ifndef ARTNETCLIPMODEL_HPP
#define ARTNETCLIPMODEL_HPP
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include <QJsonArray>
#include "TimeLineDefines.h"
#include <QPushButton>
#include "AbstractClipModel.hpp"
#include "TimeCodeDefines.h"
#include <QSlider>
// #include "BaseTimeLineModel.h"
#include "../../Common/Devices/ClientController/SocketTransmitter.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
namespace Clips
{
    class ArtnetClipModel : public AbstractClipModel {
        Q_OBJECT
    public:
        explicit ArtnetClipModel(int start,const QString& filePath = QString(), QObject* parent = nullptr)
            : AbstractClipModel(start, "Artnet", parent)
        ,m_filePath(filePath)
        ,m_editor(nullptr)
        ,fileNameLabel(new QLineEdit())

        {
            EMBEDWIDGET = false;
            SHOWBORDER = true;
            ClipColor=QColor("#66cccc");
            initPropertyWidget();
            if (!filePath.isEmpty()) {
                loadArtnetInfo(filePath);
            }

            m_canNotify = true;

            // 异步触发一次初始通知，确保事件循环就绪
            QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
        }

        ~ArtnetClipModel() override
        {
        }

        // 设置文件路径并加载视频信息
        void setFilePath(const QString& path) {
            if (m_filePath != path) {
                m_filePath = path;
                loadArtnetInfo(path);
                fileNameLabel->setText(m_filePath);
                emit filePathChanged(path);
            }
        }

        void setStart(int start) override  {
            AbstractClipModel::setStart(start);
            QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
        }
        void setEnd(int end) override  {
            AbstractClipModel::setEnd(end);
            QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
        }
        // 其他 getter/setter 保持不变
        QString filePath() const { return m_filePath; }


        // 重写保存和加载函数
        QJsonObject save() const override {
            QJsonObject json = AbstractClipModel::save();
            json["file"] = m_filePath;
            json["type"] = "Artnet";
            json["startTime"]=timecode_frame_to_time(frames_to_timecode_frame(start(),getTimeCodeType()),getTimeCodeType());
            json["endTime"]=timecode_frame_to_time(frames_to_timecode_frame(end(),getTimeCodeType()),getTimeCodeType());
            return json;
        }

        void load(const QJsonObject& json) override {
            AbstractClipModel::load(json);
            m_filePath = json["file"].toString();

            if(!m_filePath.isEmpty()) {
                loadArtnetInfo(m_filePath);
                fileNameLabel->setText(m_filePath);
            }

        }

        QVariant data(int role) const override {
            switch (role) {
            case Qt::DisplayRole:
                return m_filePath;
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast< ArtnetClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
            }
        }


        QWidget* clipPropertyWidget() override{
            m_editor = new QWidget();
            QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            mainLayout->setSpacing(4);
            // 基本设置组
            auto* basicGroup = new QGroupBox("文件属性", m_editor);
            auto* basicLayout = new QGridLayout(basicGroup);
            // 文件名显示
            fileNameLabel->setReadOnly(true);
            basicLayout->addWidget(fileNameLabel, 0, 0, 1, 2);
            // 媒体文件选择
            basicLayout->addWidget(new QLabel("回放起始域:"), 1, 0,1,1);
            auto* startUniverse = new QSpinBox(basicGroup);
            startUniverse->setMinimum(0);
            startUniverse->setMaximum(65536);
            basicLayout->addWidget(startUniverse, 1, 1,1,1);


            basicLayout->addWidget(new QLabel("Artnet数据:"), 4, 0);
            auto* mediaButton = new QPushButton("选择媒体文件", basicGroup);
            basicLayout->addWidget(mediaButton, 4, 1);
            // 连接信号槽
            connect(mediaButton, &QPushButton::clicked, [=]() {
                QString filePath = QFileDialog::getOpenFileName(m_editor,
                    "选择Artnet数据",
                    "",
                    "视频文件 (*.mov);");

                if (!filePath.isEmpty()) {
                    setFilePath(filePath);  // 这会触发视频信息加载和长度更新
                }
            });
            mainLayout->addWidget(basicGroup);
            return m_editor;
        }

        QVariantMap currentData(int currentFrame) const override {
            QVariantMap data;


            return data;
        }


    public Q_SLOTS:
        void onPropertyChanged(){

        }

    private:
        void loadArtnetInfo(const QString& path) {
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
                    break;
                }
            }

            avformat_close_input(&formatContext);

        }


        QString m_filePath;
        QWidget* m_editor;
        QLineEdit* fileNameLabel;
        bool m_canNotify = false;
    };
}
#endif // ArtnetClipModel_HPP