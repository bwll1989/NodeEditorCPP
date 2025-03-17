#ifndef VIDEOCLIPMODEL_H
#define VIDEOCLIPMODEL_H

#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class VideoClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    explicit VideoClipModel(int start, int end, const QString& filePath = QString(), QObject* parent = nullptr)
        : AbstractClipModel(start, end, "Video", TimecodeType::PAL, parent), 
          m_filePath(filePath), 
          m_frameRate(25.0),
          m_width(1920), 
          m_height(1080) 
    {
        EMBEDWIDGET = false;
        if (!filePath.isEmpty()) {
            loadVideoInfo(filePath);
        }
    }

    ~VideoClipModel() override {
       m_sender.stop();
       m_sender.close();
       m_sender.disconnect();
       delete m_propertyWidget;
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
        json["width"] = m_width;
        json["height"] = m_height;
        json["posX"] = m_PosX;
        json["posY"] = m_PosY;
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_filePath = json["filePath"].toString();
        m_width = json["width"].toInt();
        m_height = json["height"].toInt();
        m_PosX = json["posX"].toInt();
        m_PosY = json["posY"].toInt();
    }

    QVariant data(int role) const override {
        switch (role) {
            case Qt::DisplayRole:
                return m_filePath;
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<VideoClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
        }
    }

    QVariantMap currentVideoData(int currentFrame) const override {
        QVariantMap data;
        // 创建一个固定的测试图像
        QImage image(m_width, m_height, QImage::Format_RGB888);
        image.fill(Qt::black);  // 背景填充黑色
        
        // 绘制一些测试图案
        QPainter painter(&image);
        painter.setPen(QPen(Qt::white, 2));
        
        // 绘制时间码
        QFont font;
        font.setPointSize(30);
        painter.setFont(font);
        painter.drawText(image.rect(), Qt::AlignCenter, 
                        QString("Frame: %1\nClip: %2")
                            .arg(currentFrame)
                            .arg(m_filePath.split("/").last()));
        
        // 绘制边框
        painter.drawRect(0, 0, m_width-1, m_height-1);
        
        // 绘制一个移动的图案，根据当前帧变化
        int x = (currentFrame * 5) % m_width;
        painter.fillRect(x, m_height/2 - 25, 50, 50, Qt::red);
        data["image"] = QVariant::fromValue(image);
        data["posX"] = m_PosX;
        data["posY"] = m_PosY;
        data["width"] = m_width;
        data["height"] = m_height;
        return data;
    }

    // 获取帧率
    double getFrameRate() const { return m_frameRate; }
    /**
     * 获取视频宽度
     */
    int getWidth() const { return m_width; }
    /**
     * 获取视频高度
     */
    int getHeight() const { return m_height; }
    /**
     * 获取视频水平位置
     */
    int getPosX() const { return m_PosX; }
    /**
     * 获取视频垂直位置
     */
    int getPosY() const { return m_PosY; }
    /**
     * 获取视频属性窗口
     */
    QWidget* propertyWidget() const override {
        if(!m_propertyWidget){
            m_propertyWidget = new QWidget();
            QVBoxLayout* mainLayout = new QVBoxLayout(m_propertyWidget);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            mainLayout->setSpacing(4);
             // 基本设置组
            auto* basicGroup = new QGroupBox("文件属性", editor);
            auto* basicLayout = new QGridLayout(basicGroup);
            // 时间相关控件
            basicLayout->addWidget(new QLabel("文件时长:"), 1, 0);
            auto* durationBox = new QLineEdit(basicGroup);
            durationBox->setReadOnly(true);
            // 将帧数转换为时间码格式显示
            durationBox->setText(FramesToTimeString(length(), getFrameRate()));
            basicLayout->addWidget(durationBox, 1, 1);
            // 文件名显示
            auto* fileNameLabel = new QLineEdit(filePath(), basicGroup);
            fileNameLabel->setReadOnly(true);
            basicLayout->addWidget(fileNameLabel, 2, 0, 1, 2);
            // 媒体文件选择
            basicLayout->addWidget(new QLabel("媒体文件:"), 4, 0);
            auto* mediaButton = new QPushButton("选择视频", basicGroup);
            basicLayout->addWidget(mediaButton, 4, 1);        
            // 连接信号槽
            connect(mediaButton, &QPushButton::clicked, [=]() {
                QString filePath = QFileDialog::getOpenFileName(editor,
                    "选择视频文件",
                    "",
                    "视频文件 (*.mp4 *.avi *.mkv *.mov);;所有文件 (*)");
                
                if (!filePath.isEmpty()) {
                    setFilePath(filePath);  // 这会触发视频信息加载和长度更新
                    fileNameLabel->setText(filePath);
                    // 更新时长显示，使用时间码格式
                    durationBox->setText(FramesToTimeString(length(), getFrameRate()));
                    m_sizeXSpinBox->setValue(getWidth());
                    m_sizeYSpinBox->setValue(getHeight());
                    m_xSpinBox->setValue(getPosX());
                    m_ySpinBox->setValue(getPosY());
                }
            });

            mainLayout->addWidget(basicGroup);

            basicLayout->addWidget(new QLabel("水平位置:"), 5, 0,1,1);
            m_xSpinBox->setRange(-9999, 9999);
            m_xSpinBox->setValue(getPosX());
            basicLayout->addWidget(m_xSpinBox, 5, 1,1,1);
            
            basicLayout->addWidget(new QLabel("垂直位置:"), 6, 0,1,1);
            m_ySpinBox->setRange(-9999, 9999);
            m_ySpinBox->setValue(getPosY());
            basicLayout->addWidget(m_ySpinBox, 6, 1,1,1);        
            mainLayout->addWidget(basicGroup);
            // 添加一个按钮，用于设置大小
            auto* setSizeXLabel = new QLabel("宽度", basicGroup);
            basicLayout->addWidget(setSizeXLabel, 7, 0, 1, 1);
            m_sizeXSpinBox->setRange(0, 9999);
            m_sizeXSpinBox->setValue(getWidth());
            basicLayout->addWidget(m_sizeXSpinBox, 7, 1, 1, 1);
            auto* setSizeYLabel = new QLabel("高度", basicGroup);
            basicLayout->addWidget(setSizeYLabel, 8, 0, 1, 1);
            m_sizeYSpinBox->setRange(0, 9999);
            m_sizeYSpinBox->setValue(getHeight());
            basicLayout->addWidget(m_sizeYSpinBox, 8, 1, 1, 1);
            connect(m_sizeXSpinBox, &QSpinBox::valueChanged, [=]() {
                setSize(m_sizeXSpinBox->value(),m_sizeYSpinBox->value());
            });
            connect(m_sizeYSpinBox, &QSpinBox::valueChanged, [=]() {
            setSize(m_sizeXSpinBox->value(),m_sizeYSpinBox->value());
            });
            connect(m_xSpinBox, &QSpinBox::valueChanged, [=]() {
            
                setPos(m_xSpinBox->value(),m_ySpinBox->value());
                
            });
            connect(m_ySpinBox, &QSpinBox::valueChanged, [=]() {
                if (m_model) {
                    setPos(m_xSpinBox->value(),m_ySpinBox->value());
                }
            });
            }
        return m_propertyWidget;
    }
public Q_SLOTS:

    /**
     * 设置视频显示位置
     */
    void setPos(int x,int y) {
        m_PosX=x;
        m_PosY = y; 
        emit posChanged(QPoint(m_PosX,m_PosY));
    }
    /**
     * 设置视频显示尺寸
     */
    void setSize(int width,int height){
        m_width = width;
        m_height=height;
        emit sizeChanged(QSize(m_width,m_height));
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
                m_frameRate = av_q2d(stream->avg_frame_rate);
                if (m_frameRate <= 0) {
                    m_frameRate = av_q2d(stream->r_frame_rate);
                }

                // 获取视频分辨率
                m_width = stream->codecpar->width;
                m_height = stream->codecpar->height;

                // 计算视频总帧数和时长
                double duration = stream->duration * av_q2d(stream->time_base);
                if (duration <= 0) {
                    duration = formatContext->duration / (double)AV_TIME_BASE;
                }

                // 计算总帧数并更新片段长度
                int totalFrames = static_cast<int>(duration * m_frameRate);
                if (totalFrames > 0) {
                    setEnd(start() + totalFrames);
                }
                break;
            }
        }

        avformat_close_input(&formatContext);
    }

    QString m_filePath;
    double m_frameRate;
    int m_width;
    int m_height;
    int m_PosX;
    int m_PosY;
    QSpinBox* m_xSpinBox;
    QSpinBox* m_ySpinBox;
    QSpinBox* m_sizeXSpinBox;
    QSpinBox* m_sizeYSpinBox;
    QWidget* m_propertyWidget;
};

#endif // VIDEOCLIPMODEL_H 