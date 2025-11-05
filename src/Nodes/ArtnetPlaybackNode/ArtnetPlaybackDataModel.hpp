#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "ArtnetPlaybackInterface.hpp"
#include <QVariantMap>
#include <QByteArray>
#include <QJsonArray>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
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
class ArtnetPlaybackDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数，初始化Artnet Universe回放节点
     */
    ArtnetPlaybackDataModel();

    /**
     * @brief 析构函数
     */
    ~ArtnetPlaybackDataModel() override {
        stopPlayback();
        cleanupFFmpeg();
        delete widget;
    }

    /**
     * @brief 获取端口标题
     * @param portType 端口类型
     * @param portIndex 端口索引
     * @return 端口标题
     */
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
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

    /**
     * @brief 获取端口数据类型
     * @param portType 端口类型
     * @param portIndex 端口索引
     * @return 数据类型
     */
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex)
        Q_UNUSED(portType)
        return VariableData().type();
    }

    /**
     * @brief 获取输出数据
     * @param port 端口索引
     * @return 输出数据
     */
    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        if (port >= 0 && port < universeOutputs.size()) {
            return universeOutputs[port];
        }
        return nullptr;
    }

    /**
     * @brief 处理输入数据（此节点无输入）
     */
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (!data) return;

        auto variableData = std::dynamic_pointer_cast<VariableData>(data);
        if (!variableData) return;

        QVariant value = variableData->value();
        bool trigger = value.toBool();


        switch (portIndex) {
        case 0: // 播放
            if (trigger)
                onPlayClicked();
            break;
        case 1: // 循环播放
            widget->loopCheckBox->setChecked(trigger);
            break;
        case 2: // 停止
            if (trigger)
                onStopClicked();
            break;
        case 3: // 清除数据
            if (trigger)
                onClearDataClicked();
            break;
        default:
            break;
        }

    }

    /**
     * @brief 保存节点状态
     * @return JSON对象
     */
    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        // 保存Universe配置
        modelJson1["universe"] = widget->universeSpinBox->value();
        modelJson1["subnet"] = widget->subnetSpinBox->value();
        modelJson1["net"] = widget->netSpinBox->value();
        
        // 保存循环播放状态
        modelJson1["isLooping"] = widget->loopCheckBox->isChecked();
        
        // 保存视频文件信息
        modelJson1["videoFilePath"] = m_fileName;
        modelJson1["videoWidth"] = m_videoWidth;
        modelJson1["videoHeight"] = m_videoHeight;
        modelJson1["videoFPS"] = m_fps;
        modelJson1["videoDuration"] = m_duration;
        
        // 保存当前Universe数量
        modelJson1["currentUniverseCount"] = currentUniverseCount;
        
        // 保存播放状态（但不保存isPlaying，因为重新加载时不应该自动播放）
        modelJson1["playbackPosition"] = 0.0; // 可以后续扩展为保存当前播放位置
    
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["UniverseSettings"] = modelJson1;
        return modelJson;
    }

    /**
     * @brief 加载节点状态
     * @param p JSON对象
     */
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["UniverseSettings"];
        if (!v.isUndefined() && v.isObject()) {
            auto settings = v.toObject();
    
            // 加载Universe配置
            widget->universeSpinBox->setValue(settings["universe"].toInt(0));
            widget->subnetSpinBox->setValue(settings["subnet"].toInt(0));
            widget->netSpinBox->setValue(settings["net"].toInt(0));
            
            // 加载循环播放状态
            bool loopState = settings["isLooping"].toBool(false);
            widget->loopCheckBox->setChecked(loopState);
            isLooping = loopState;
            
            // 加载视频文件信息
            QString videoFilePath = settings["videoFilePath"].toString();
            if (!videoFilePath.isEmpty() && videoFilePath != "未选择文件") {
                // 尝试重新打开视频文件
                if (QFile::exists(videoFilePath)) {
                    if (openVideo(videoFilePath)) {
                        // 恢复视频信息显示
                        double savedDuration = settings["videoDuration"].toDouble(m_duration);
                        double savedFPS = settings["videoFPS"].toDouble(m_fps);
                        widget->updateFileInfo(videoFilePath, savedDuration, savedFPS);
                        
                        qDebug() << "已重新加载视频文件:" << videoFilePath;
                    } else {
                        widget->showError("无法重新加载视频文件: " + videoFilePath);
                        widget->fileNameLabel->setText("文件不存在: " + QFileInfo(videoFilePath).fileName());
                    }
                } else {
                    widget->showError("视频文件不存在: " + videoFilePath);
                    widget->fileNameLabel->setText("文件不存在: " + QFileInfo(videoFilePath).fileName());
                }
            }
            
            // 加载其他保存的信息（用于显示或验证）
            int savedUniverseCount = settings["currentUniverseCount"].toInt(1);
            if (savedUniverseCount > 0 && savedUniverseCount != currentUniverseCount) {
                qDebug() << "Universe数量从保存的" << savedUniverseCount << "调整为当前" << currentUniverseCount;
            }
    
            // 更新输出数据
            updateAllUniverseData();
    
            NodeDelegateModel::load(p);
        }
    }

        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override {
            return widget;
        }

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
signals:
    /**
     * @brief 错误发生信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief 循环播放状态改变处理
     * @param enabled 是否启用循环播放
     */
    void onLoopStateChanged(bool enabled);  // 新增：循环播放状态改变槽函数
    /**
     * @brief Universe改变时的处理
     * @param universe 新的Universe值
     */
    void onUniverseChanged(int universe) ;

    /**
     * @brief Subnet改变时的处理
     * @param subnet 新的Subnet值
     */
    void onSubnetChanged(int subnet) ;

    /**
     * @brief Net改变时的处理
     * @param net 新的Net值
     */
    void onNetChanged(int net) ;

    /**
     * @brief 清空数据按钮点击处理
     * 将所有Universe的512个DMX通道清零
     */
    void onClearDataClicked() ;

    /**
     * @brief 选择视频文件
     */
    void onSelectFileClicked() ;

    /**
     * @brief 播放按钮点击
     */
    void onPlayClicked() ;

    /**
     * @brief 停止按钮点击
     */
    void onStopClicked() ;

    /**
     * @brief 播放定时器回调
     */
    void onPlaybackTimer() ;

private:
    /**
     * @brief 初始化FFmpeg
     */
    void initializeFFmpeg() ;

    /**
     * @brief 清理FFmpeg资源（内部使用，不加锁）
     */
    void cleanupFFmpegInternal() ;

    /**
     * @brief 清理FFmpeg资源（公共接口，加锁）
     */
    void cleanupFFmpeg() ;

    void adjustUniverseCount(int videoHeight);
    /**
     * @brief 打开视频文件
     * @param filename 视频文件路径
     * @return bool 是否成功打开
     */
    bool openVideo(const QString& filename);

    /**
     * @brief 开始播放
     */
    void startPlayback() ;

    /**
     * @brief 停止播放
     */
    void stopPlayback() ;

    /**
     * @brief 读取下一帧
     * @return bool 是否成功读取
     */
    bool readNextFrame() ;
    /**
     * @brief 从帧数据提取DMX数据并更新输出
     */
    void extractAndUpdateDmxData() ;

    /**
     * @brief 更新所有Universe数据输出
     */
    void updateAllUniverseData() ;

    /**
     * @brief 更新指定Universe数据输出
     * @param universeIndex Universe索引
     */
    void updateUniverseData(int universeIndex) ;

private:
    // FFmpeg相关成员变量
    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    const AVCodec* m_codec;
    AVFrame* m_frame;
    AVPacket* m_packet;
    int m_videoStreamIndex;
    int m_videoWidth;
    int m_videoHeight;
    double m_timeBase;
    double m_fps;
    double m_duration;
    QMutex m_mutex;
    QString m_fileName;
    // 播放控制
    QTimer* playbackTimer;
    bool isPlaying;
    bool isLooping;
    // 动态Universe支持
    int currentUniverseCount;  // 当前Universe数量

    // 数据存储（动态大小）
    QList<QVector<int>> dmxDataList;                    // 动态数量的Universe的512通道DMX数据
    QList<std::shared_ptr<VariableData>> universeOutputs; // 动态数量的Universe输出数据

    Nodes::ArtnetPlaybackInterface * widget = new Nodes::ArtnetPlaybackInterface();
    };
}

