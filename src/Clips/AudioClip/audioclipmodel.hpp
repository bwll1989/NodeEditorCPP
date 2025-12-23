#ifndef AUDIOCLIPMODEL_HPP
#define AUDIOCLIPMODEL_HPP
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
#include "ConstantDefines.h"
#include "../../Common/Devices/ClientController/SocketTransmitter.h"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
namespace Clips
{
    class AudioClipModel : public AbstractClipModel {
        Q_OBJECT
    public:
        explicit AudioClipModel(int start,const QString& filePath = QString(), QObject* parent = nullptr)
            : AbstractClipModel(start, "Audio", parent),
              m_filePath(filePath),
              m_editor(nullptr)
        {
            EMBEDWIDGET = false;
            SHOWBORDER = true;
            ClipColor=QColor("#6a2c70");
            initPropertyWidget();
            if (!filePath.isEmpty()) {
                loadAudioInfo(filePath);
            }
            m_server = getClientControlInstance();
            m_canNotify = true;

            // 异步触发一次初始通知，确保事件循环就绪
            QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
        }

        ~AudioClipModel() override
        {
            QJsonDocument doc;
            QJsonArray array;  // 创建一个JSON数组
            QJsonObject command=save();
            command["remove"] = true;
            array.append(command);
            doc.setObject(QJsonObject{{"fileList", array}}); // 正确设置JSON文档的根对象
            m_server->enqueueJson(doc);
            // AbstractClipModel::~AbstractClipModel();
        }

        // 设置文件路径并加载视频信息
        void setMedia(const QVariant& path) override{
            mediaSelector->setText(path.toString());
            emit mediaSelector->textChanged(path.toString());
            onPropertyChanged();
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
            json["type"] = "Audio";
            // json["startTime"] = timecode_frame_to_time(frames_to_timecode_frame(start(), m_timecodeType), m_timecodeType);
            // json["endTime"] = timecode_frame_to_time(frames_to_timecode_frame(end(), m_timecodeType), m_timecodeType);

            json["gain"] = gain->value();

            json["startTime"]=timecode_frame_to_time(frames_to_timecode_frame(start(),getTimeCodeType()),getTimeCodeType());
            json["endTime"]=timecode_frame_to_time(frames_to_timecode_frame(end(),getTimeCodeType()),getTimeCodeType());
            return json;
        }

        void load(const QJsonObject& json) override {

            m_filePath = json["file"].toString();
            if(!m_filePath.isEmpty()) {
                loadAudioInfo(AppConstants::MEDIA_LIBRARY_STORAGE_DIR+"/"+m_filePath);
            }
            AbstractClipModel::load(json);
            m_id = json["Id"].toInt();
            gain->setValue(json["gain"].toInt());
        }

        QVariant data(int role) const override {
            switch (role) {
            case Qt::DisplayRole:
                return m_filePath;
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast< AudioClipModel*>(this)));
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
            //auto* basicGroup = new QGroupBox("文件属性", m_editor);
            //auto* basicLayout = new QGridLayout(basicGroup);
            // 时间相关控件
            // basicLayout->addWidget(new QLabel("文件时长:"), 1, 0);
            // auto* durationBox = new QLineEdit(basicGroup);
            // durationBox->setReadOnly(true);
            // // 将帧数转换为时间码格式显示
            // // durationBox->setText(""(length(), timecode_frames_per_sec(getTimecodeType())));
            // basicLayout->addWidget(durationBox, 1, 1);
            // 文件名显示

            // 媒体文件选择

            // 连接信号槽
            // connect(mediaButton, &QPushButton::clicked, [=]() {
            //     QString filePath = QFileDialog::getOpenFileName(m_editor,
            //         "选择媒体文件",
            //         "",
            //         "音频文件 (*.mp3 *.wav *.aac *.flac);;"
            //         "所有文件 (*)");
            //
            //     if (!filePath.isEmpty()) {
            //         setMeida(filePath);  // 这会触发视频信息加载和长度更新
            //         fileNameLabel->setText(filePath);
            //         // 更新时长显示，使用时间码格式
            //         // durationBox->setText(FramesToTimeString(length(), getFrameRate(getTimecodeType())));
            //
            //     }
            // });
            //mainLayout->addWidget(basicGroup);
            // 添加尺寸位置参数设置
            auto* positionGroup = new QGroupBox("片段属性", m_editor);
            auto* positionLayout = new QGridLayout(positionGroup);
            // 位置
            mediaSelector = new SelectorComboBox(MediaLibrary::Category::Audio,positionGroup);
            positionLayout->addWidget(new QLabel("Media:"), 0, 0);
            positionLayout->addWidget(mediaSelector, 0, 1);
            connect(mediaSelector,&SelectorComboBox::textChanged,[=](const QString& text){
                if (m_filePath != text) {
                    m_filePath = text;
                    emit filePathChanged(m_filePath);
                    emit onPropertyChanged();
                }
            });
            gain=new QSpinBox(positionGroup);
            gain->setMinimum(-180);
            gain->setMaximum(180);
            gain->setValue(0);
            registerOSCControl("/gain",gain);
            positionLayout->addWidget(new QLabel("Gain:"), 1, 0);
            positionLayout->addWidget(gain, 1, 1);
            mainLayout->addWidget(positionGroup);
            // 连接信号槽
            connect(gain, QOverload<int>::of(&QSpinBox::valueChanged), [=]() {
                onPropertyChanged();
            });
            return m_editor;
        }

        QVariantMap currentData(int currentFrame) const override {
            QVariantMap data;
            // if (!m_oscSender) return data;

            if(currentFrame == start()) {
                data["/file"] = m_filePath;

                return data;
            }
            if(currentFrame+1 == end()) {
                data["/stop"]=1;
                return data;
            }

            return data;
        }


    public Q_SLOTS:
        void onPropertyChanged(){
            if (!m_server) {
                qWarning() << "[VideoClipModel] SocketTransmitter not ready, skip onPropertyChanged";
                return;
            }
            QJsonDocument doc;
            QJsonArray array;  // 创建一个JSON数组
            array.append(save()); // 将对象添加到数组中
            doc.setObject(QJsonObject{{"fileList", array}}); // 正确设置JSON文档的根对象
            m_server->enqueueJson(doc);
        }

    private:
        void loadAudioInfo(const QString& path) {
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
                if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
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

            onPropertyChanged();
        }

        SelectorComboBox* mediaSelector;
        QString m_filePath;
        QWidget* m_editor;
        QSpinBox* gain;
        SocketTransmitter* m_server;
        bool m_canNotify = false;
    };
}
#endif // AudioClipModel_HPP