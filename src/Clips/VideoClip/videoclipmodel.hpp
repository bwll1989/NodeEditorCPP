#ifndef VIDEOCLIPMODEL_HPP
#define VIDEOCLIPMODEL_HPP
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include <QJsonArray>
#include "TimeLineDefines.h"
#include <QPushButton>
#include "AbstractClipModel.hpp"
#include "TimeCodeMessage.h"
#include <QSlider>
// #include "BaseTimeLineModel.h"
#include "../../Common/Devices/SocketTransmitter/SocketTransmitter.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class VideoClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    explicit VideoClipModel(int start,const QString& filePath = QString(), QObject* parent = nullptr)
        : AbstractClipModel(start, "Video", parent),
          m_filePath(filePath),
          m_editor(nullptr)
    {
        EMBEDWIDGET = false;
        SHOWBORDER = true;
        ClipColor=QColor("#6666cc");
        initPropertyWidget();
        if (!filePath.isEmpty()) {
            loadVideoInfo(filePath);
        }
        m_server = getSharedInstance();

    }

    ~VideoClipModel() override
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
    void setFilePath(const QString& path) { 
        if (m_filePath != path) {
            m_filePath = path;
            loadVideoInfo(path);
            emit filePathChanged(path);
        }
    }

    void setStart(int start) override  {
        AbstractClipModel::setStart(start);
        onPropertyChanged();
    }
    void setEnd(int end) override  {
        AbstractClipModel::setEnd(end);
        onPropertyChanged();
    }
    // 其他 getter/setter 保持不变
    QString filePath() const { return m_filePath; }
   

    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();
        json["file"] = m_filePath;
        json["type"] = "Video";
        // json["startTime"] = timecode_frame_to_time(frames_to_timecode_frame(start(), m_timecodeType), m_timecodeType);
        // json["endTime"] = timecode_frame_to_time(frames_to_timecode_frame(end(), m_timecodeType), m_timecodeType);
        json["zIndex"] = layer->value();

        QJsonObject position;
        position["x"] = postion_x->value();
        position["y"] = postion_y->value();
        json["position"] = position;

        QJsonObject size;
        size["width"] = width->value();
        size["height"] = height->value();
        json["size"] = size;
        json["rotation"] = rotation->value();

        json["startTime"]=timecode_frame_to_time(frames_to_timecode_frame(start(),getTimeCodeType()),getTimeCodeType());
        json["endTime"]=timecode_frame_to_time(frames_to_timecode_frame(end(),getTimeCodeType()),getTimeCodeType());
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_filePath = json["filePath"].toString();

        if(!m_filePath.isEmpty()) {
            loadVideoInfo(m_filePath);
        }
        if(json.contains("position")) {
            QJsonObject position = json["position"].toObject();
            postion_x->setValue(position["x"].toInt());
            postion_y->setValue(position["y"].toInt());
        }

        if(json.contains("size")) {
            QJsonObject size = json["size"].toObject();
            width->setValue(size["width"].toInt());
            height->setValue(size["height"].toInt());
        }
        m_id = json["Id"].toInt();
        rotation->setValue(json["rotation"].toInt());
        layer->setValue(json["zIndex"].toInt());
    }

    QVariant data(int role) const override {
        switch (role) {
            case Qt::DisplayRole:
                return m_filePath;
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast< VideoClipModel*>(this)));
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
        // 时间相关控件
        basicLayout->addWidget(new QLabel("文件时长:"), 1, 0);
        // auto* durationBox = new QLineEdit(basicGroup);
        // durationBox->setReadOnly(true);
        // // 将帧数转换为时间码格式显示
        // // durationBox->setText(""(length(), timecode_frames_per_sec(getTimecodeType())));
        // basicLayout->addWidget(durationBox, 1, 1);
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
                "所有文件 (*)");

            if (!filePath.isEmpty()) {
                setFilePath(filePath);  // 这会触发视频信息加载和长度更新
                fileNameLabel->setText(filePath);
                // 更新时长显示，使用时间码格式
                // durationBox->setText(FramesToTimeString(length(), getFrameRate(getTimecodeType())));

            }
        });
        mainLayout->addWidget(basicGroup);
        // 添加尺寸位置参数设置
        auto* positionGroup = new QGroupBox("位置参数", m_editor);
        auto* positionLayout = new QGridLayout(positionGroup);
        // 位置
        positionLayout->addWidget(new QLabel("X:"), 1, 0);
        postion_x = new QSpinBox(positionGroup);
        postion_x->setMinimum(-10000);
        postion_x->setMaximum(10000);
        postion_x->setValue(0);
        positionLayout->addWidget(postion_x, 1, 1);
        positionLayout->addWidget(new QLabel("Y:"), 2, 0);
        postion_y = new QSpinBox(positionGroup);
        postion_y->setMinimum(-10000);
        postion_y->setMaximum(10000);
        postion_y->setValue(0);
        positionLayout->addWidget(postion_y, 2, 1);
        positionLayout->addWidget(new QLabel("Width:"), 3, 0);
        width = new QSpinBox(positionGroup);
        width->setMinimum(0);
        width->setMaximum(10000);
        width->setValue(100);
        positionLayout->addWidget(width, 3, 1);
        positionLayout->addWidget(new QLabel("Height:"), 4, 0);
        height = new QSpinBox(positionGroup);
        height->setMinimum(0);
        height->setMaximum(10000);
        height->setValue(100);
        positionLayout->addWidget(height, 4, 1);
        positionLayout->addWidget(new QLabel("Layer:"), 5, 0);
        layer = new QSlider(positionGroup);
        layer->setOrientation(Qt::Horizontal);
        layer->setTickInterval(1);
        layer->setSingleStep(1);

        layer->setMinimum(0);
        layer->setMaximum(5);
        layer->setValue(0);
        positionLayout->addWidget(layer, 5, 1);
        rotation=new QSpinBox(positionGroup);
        rotation->setMinimum(-180);
        rotation->setMaximum(180);
        rotation->setValue(0);
        positionLayout->addWidget(new QLabel("Rotate:"), 6, 0);
        positionLayout->addWidget(rotation, 6, 1);
        mainLayout->addWidget(positionGroup);
        // 连接信号槽
        connect(postion_x, QOverload<int>::of(&QSpinBox::valueChanged), [=]() {
            onPropertyChanged();
        });
        connect(postion_y, QOverload<int>::of(&QSpinBox::valueChanged), [=]() {
            onPropertyChanged();
        });
        connect(width, QOverload<int>::of(&QSpinBox::valueChanged), [=]() {
            onPropertyChanged();
        });
        connect(height, QOverload<int>::of(&QSpinBox::valueChanged), [=]() {
            onPropertyChanged();
        });
        connect(layer, QOverload<int>::of(&QSlider::valueChanged), [=]() {
            onPropertyChanged();
        });
        connect(rotation, QOverload<int>::of(&QSpinBox::valueChanged), [=]() {
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
        QJsonDocument doc;
        QJsonArray array;  // 创建一个JSON数组
        array.append(save()); // 将对象添加到数组中
        doc.setObject(QJsonObject{{"fileList", array}}); // 正确设置JSON文档的根对象
        m_server->enqueueJson(doc);
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

    QString m_filePath;
    QWidget* m_editor;
    QLineEdit* playerNameEdit;
    QLineEdit* playerIDEdit;
    QSpinBox* postion_x;
    QSpinBox* postion_y;
    QSpinBox* width;
    QSpinBox* height;
    QSlider* layer;
    QSpinBox* rotation;
    SocketTransmitter* m_server;
};

#endif // VideoClipModel_HPP