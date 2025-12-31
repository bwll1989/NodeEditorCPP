#pragma once
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include <QJsonArray>
#include "TimeLineDefines.h"
#include <QPushButton>

#include "TimeCodeDefines.h"
#include <QSlider>
// #include "BaseTimeLineModel.h"
#include "../AbstractClipDelegateModel.h"
#include "ConstantDefines.h"
#include "../../Common/Devices/ClientController/SocketTransmitter.h"
#include "ArtnetSender/ArtnetTransmitter.h"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}
namespace Clips
{
    class ArtnetClipModel : public AbstractClipDelegateModel {
        Q_OBJECT
    public:
        explicit ArtnetClipModel(int start,const QString& filePath = QString(), QObject* parent = nullptr);

        ~ArtnetClipModel() override;

        // 设置文件路径并加载视频信息
        void setMedia(const QVariant& path) override;

        void setStart(int start) override ;
        void setEnd(int end) override  ;

        // 重写保存和加载函数
        QJsonObject save() const override ;

        void load(const QJsonObject& json) ;
        QVariant data(int role) const ;
        QWidget* clipPropertyWidget() override;

        /**
         * 函数级注释：在 timeline 上按帧回放 DMX 数据
         * - 当 currentFrame 处于剪辑区间内时：
         *   1) 打开并定位到对应视频帧（基于 timeline 帧率与 FFmpeg time_base）
         *   2) 提取每一行（universe）的 512 字节 DMX 数据到 m_dmxDataList
         *   3) 构建与 UniversePlaybackDataModel 对齐的 artnetPacket 列表返回
         *   4) 同步通过 m_artnetTransmitter 将每个 Universe 的帧入队广播
         * - 在剪辑开始帧返回 "/file"，结束前一帧返回 "/stop" 以保持与其他 Clip 一致的习惯
         */
        QVariantMap currentData(int currentFrame) const override ;


    public Q_SLOTS:
        void onPropertyChanged();


        /**
         * 函数级注释：读取媒体文件基本信息并据此设置剪辑长度
         * 仅用于设定剪辑时长，不进行数据解码；真正的解码在 currentData 内完成
         */
        void loadArtnetInfo(const QString& path) ;

        /**
         * 函数级注释：确保视频已打开并初始化 FFmpeg 解码器（FFmpeg 7.1）
         * - 校验尺寸：宽度=512，height>=1
         * - 设定 m_universeCount = m_videoHeight
         * - 仅当尚未打开或路径改变时执行
         */
        void ensureVideoOpened(const QString& path) ;

        /**
         * 函数级注释：按 timeline 的 currentFrame 定位并解码一帧
         * - timeline 帧号 -> 秒 = (frame - start) / timelineFps
         * - 秒 -> 流时间戳单位 = 秒 / time_base
         * - av_seek_frame + avcodec_flush_buffers 再读取解码
         */
        bool readFrameAt(int currentFrame) ;

        /**
         * 函数级注释：从当前解码帧提取每个 Universe 的 512 字节 DMX 数据
         * - 灰度：直接读取每列像素值
         * - 非灰度（兼容旧 RGBA）：取 R 通道
         */
        void extractDMXFromCurrentFrame() ;
        /**
         * 函数级注释：将当前帧的每个 Universe 的 DMX 数据封装为 ArtnetFrame 并入队发送
         * - 参考 ArtnetOutDataModel 的发送逻辑
         * - 使用广播地址 `m_targetHost`；可根据需要扩展属性面板增加目标主机设置
         * - 通过 m_lastSentFrameIndex 防止 timeline 对同帧的重复调用造成重复发送
         */
        void sendArtnetFrames(int baseUniverse, int subnet, int net, int currentFrame) ;

        /**
         * 更新文件信息
         * @param fileName
         * @param duration
         * @param fps
         * @param videoHeight
         */
        void updateFileInfo(const QString& fileName, double duration, double fps,int videoHeight) {
            QFileInfo fileInfo(fileName);
            videoInfoLabel->setText(QString("时长: %1s, 帧率: %2fps, 域数量: %3")
                                   .arg(duration, 0, 'f', 1).arg(fps, 0, 'f', 1).arg(videoHeight));
        }
    private:
        AVFormatContext* m_formatContext {nullptr};
        AVCodecContext* m_codecContext {nullptr};
        const AVCodec* m_codec {nullptr};
        AVFrame* m_frame {nullptr};
        AVPacket* m_packet {nullptr};
        int m_videoStreamIndex {-1};
        int m_videoWidth {0};
        int m_videoHeight {0};
        double m_timeBase {0.0};
        double m_fps {25.0};
        double m_duration {0.0};

        QString m_filePath;
        QString m_openedPath;              // 已打开的真实路径（避免重复打开）
        QWidget* m_editor;
        SelectorComboBox* mediaSelector;
        QSpinBox* m_startUniverse {nullptr};
        QLineEdit* targetHostEdit = new QLineEdit();   
        int m_lastFrameIndex {-1};
        int m_universeCount {0};
        QLabel* videoInfoLabel;
        QList<QByteArray> m_dmxDataList;

        bool m_canNotify = false;
        ArtnetTransmitter* m_artnetTransmitter;  // ArtnetTransmitter单例
        QString m_targetHost = "192.168.1.255";  // 目标主机地址（默认广播）
        int m_lastSentFrameIndex {-1};           // 已发送的最后帧号，避免重复发送
        
    };
}
#pragma once
