#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "UniversePlaybackInterface.hpp"
#include <QVariantMap>
#include <QByteArray>
#include <QJsonArray>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include <QSignalBlocker>
#include "Common/AppConfig/ConfigManager.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

// FFmpeg头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
/**
 * @brief Artnet Universe回放节点 - 从HAP视频文件回放DMX数据
 * 支持HAP 25fps格式视频，水平128像素RGBA代表512个DMX通道，竖直每4行代表一个Universe
 * 支持动态Universe数量，根据视频高度自动计算
 */
class UniversePlaybackDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(int universe READ getUniverse WRITE setUniverse NOTIFY universeChanged)
    Q_PROPERTY(int subnet READ getSubnet WRITE setSubnet NOTIFY subnetChanged)
    Q_PROPERTY(int net READ getNet WRITE setNet NOTIFY netChanged)
    Q_PROPERTY(bool isLooping READ getIsLooping WRITE setIsLooping NOTIFY isLoopingChanged)
    Q_PROPERTY(bool isPlaying READ getIsPlaying WRITE setIsPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(QString fileName READ getFileName WRITE setFileName NOTIFY fileNameChanged)

public:
    /**
     * @brief 构造函数，初始化Artnet Universe回放节点
     */
    UniversePlaybackDataModel();

    /**
     * @brief 析构函数
     */
    ~UniversePlaybackDataModel() override {
        if (playbackTimer) {
            playbackTimer->stop();
            // 断开定时器与本对象的连接，避免销毁后仍触发槽函数
            playbackTimer->disconnect(this);
        }
        if (widget) {
            // 断开界面与本对象的所有连接，避免本对象销毁后收到界面回调
            widget->disconnect(this);
        }

        // 无论播放状态如何，都设置为不播放并清理资源
        m_isPlaying = false;
        cleanupFFmpeg();
    }

    /**
     * @brief 获取端口标题
     */
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    /**
     * @brief 获取端口数据类型
     */
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    /**
     * @brief 获取输出数据
     */
    std::shared_ptr<NodeData> outData(PortIndex const port) override;

    /**
     * @brief 处理输入数据（此节点无输入）
     */
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override;

    /**
     * @brief 保存节点状态
     */
    QJsonObject save() const override;

    /**
     * @brief 加载节点状态
     */
    void load(const QJsonObject &p) override;

    /**
     * @brief 获取嵌入式控件
     */
    QWidget *embeddedWidget() override {
        return widget;
    }

    void afterModelReady() override;

    // Property Getters and Setters
    int getUniverse() const { return m_universe; }
    void setUniverse(int universe);

    int getSubnet() const { return m_subnet; }
    void setSubnet(int subnet);

    int getNet() const { return m_net; }
    void setNet(int net);

    bool getIsLooping() const { return m_isLooping; }
    void setIsLooping(bool looping);

    bool getIsPlaying() const { return m_isPlaying; }
    void setIsPlaying(bool playing);

    QString getFileName() const { return m_fileName; }
    void setFileName(const QString& fileName);

signals:
    void errorOccurred(const QString& error);
    void universeChanged(int universe);
    void subnetChanged(int subnet);
    void netChanged(int net);
    void isLoopingChanged(bool looping);
    void isPlayingChanged(bool playing);
    void fileNameChanged(QString fileName);

public slots:
    void onGlobalEvent(const GlobalEvent& ev);

private slots:
    void onClearDataClicked();
    void onPlaybackTimer();

private:
    void initializeFFmpeg();
    void cleanupFFmpegInternal();
    void cleanupFFmpeg();
    void adjustUniverseCount(int videoHeight);
    bool openVideo(const QString& filename);
    void startPlayback();
    void stopPlayback();
    bool readNextFrame();
    void extractAndUpdateDmxData();
    void updateAllUniverseData();
    void updateUniverseData(int universeIndex);
    void updateAllUniverseDataInternal();

private:
    // FFmpeg相关成员变量
    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    const AVCodec* m_codec = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_videoStreamIndex = -1;
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    double m_timeBase = 0.0;
    double m_fps = 25.0;
    double m_duration = 0.0;
    mutable QRecursiveMutex m_mutex;
    
    // Internal state
    int m_universe = 0;
    int m_subnet = 0;
    int m_net = 0;
    QString m_fileName;
    bool m_isLooping = false;
    bool m_isPlaying = false;

    // 播放控制
    QTimer* playbackTimer = nullptr;
    
    // 动态Universe支持
    int currentUniverseCount = 0;  // 当前Universe数量

    // 数据存储（动态大小）
    QList<QByteArray> dmxDataList;                    // 动态数量的Universe的512通道DMX数据
    QList<std::shared_ptr<VariableData>> universeOutputs; // 动态数量的Universe输出数据
    
    qint64 m_currentTimestampMs {0};

    Nodes::UniversePlaybackInterface * widget = new Nodes::UniversePlaybackInterface();
};
}
