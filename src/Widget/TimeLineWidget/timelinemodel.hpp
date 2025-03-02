#ifndef TIMELINEMODEL_HPP
#define TIMELINEMODEL_HPP

#include <QAbstractItemModel>
#include <unordered_set>
#include <unordered_map>
#include "trackmodel.hpp"
#include "abstractclipmodel.hpp"
#include <string>
#include <QMimeData>
#include <QIODevice>
#include <algorithm>
#include <vector>
#include <QJsonArray>
#include "timelinetypes.h"
#include "timelinestyle.hpp"
#include <QFile>
#include <QJsonDocument>
#include "pluginloader.hpp"
#include "timelinestage.hpp"
#include "timelinescreen.hpp"
#include "timecodegenerator.hpp"
// TimelineModel类继承自QAbstractItemModel
class TimelineModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    // 构造函数
    TimelineModel(QObject* parent = nullptr) 
        : QAbstractItemModel(parent), m_pluginLoader(nullptr) ,m_stage(new TimelineStage()),m_timecodeGenerator(new TimecodeGenerator(this)) 
        {
        m_pluginLoader = new PluginLoader();
        m_pluginLoader->loadPlugins();
        
        m_timecodeGenerator = new TimecodeGenerator();
        // 连接时间码生成器信号
        connect(m_timecodeGenerator, &TimecodeGenerator::currentFrameChanged,
            this, &TimelineModel::setPlayheadPos);
        qRegisterMetaType<AbstractClipModel*>();

        // 连接播放头移动信号处理当前帧数据
        connect(m_timecodeGenerator, &TimecodeGenerator::currentFrameChanged,
            this, [this](qint64 frame) {
                QList<QVariantMap> clipDataList = getCurrentClipsData(frame);
                emit frameImageUpdated(clipDataList);
            });

        // setStage(new TimelineStage());
    }

    ~TimelineModel();
    /**
     * 创建轨道
     * @param const QString& type 类型
     */
    void addTrack(const QString& type);

    /**
     * 获取播放头位置
     * @return int 播放头位置
     */
    int getPlayheadPos() const;
    /**
     * 设置播放头位置
     * @param int newPlayheadPos 新的播放头位置
     */
    void setPlayheadPos(int newPlayheadPos);
    /**
     * 获取索引
     * @param int row 行
     * @param int column 列
     * @param QModelIndex parent 父索引
     * @return QModelIndex 索引
     */
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    /**
     * 获取父索引
     * @param const QModelIndex &child 子索引
     * @return QModelIndex 父索引
     */
    QModelIndex parent(const QModelIndex &child) const override;
    /**
     * 获取行数
     * @param const QModelIndex &parent 父索引
     * @return int 行数 
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    /**
     * 获取列数
     * @param const QModelIndex &parent 父索引
     * @return int 列数
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    /**
     * 获取数据
     * @param const QModelIndex &index 索引
     * @param int role 角色
     * @return QVariant 数据
     */
    QVariant data(const QModelIndex &index, int role) const override;
    /**
     * 设置数据
     * @param const QModelIndex &index 索引
     * @param const QVariant &value 值
     * @param int role 角色
     * @return bool 是否设置成功
     */
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    /**
     * 获取时间码格式
     * @return TimecodeType 时间码格式
     */
    TimecodeType getTimecodeType() const { return m_timecodeGenerator->getTimecodeType(); }
    /**
     * 设置时间码格式
     * @param TimecodeType type 时间码格式
     */
    void setTimecodeType(TimecodeType type){
        m_timecodeGenerator->setTimecodeType(type);
    }
    /**
     * 获取支持的拖放操作
     * @return Qt::DropActions 支持的拖放操作
     */
    Qt::DropActions supportedDropActions() const override;
    /**
     * 获取项目标志
     * @param const QModelIndex &index 索引
     * @return Qt::ItemFlags 项目标志
     */
    // 获取项目标志
    Qt::ItemFlags flags(const QModelIndex &index) const ;
    /**
     * 删除片段
     * @param QModelIndex clipIndex 片段索引
     */
    void deleteClip(QModelIndex clipIndex);
    /**
     * 清除
     */
    void clear();
    /**
     * 保存模型
     * @return QJsonObject 保存的模型
     */
    QJsonObject save() const;
    /**
     * 加载模型
     * @param const QJsonObject &modelJson 加载的模型
     */
    void load(const QJsonObject &modelJson) ;
    /**
     * 获取轨道数量
     * @return int 轨道数量
     */
    int getTrackCount() const ;
    /**
     * 获取轨道
     * @return QVector<TrackModel*> 轨道
     */
    QVector<TrackModel*> getTracks() const { return m_tracks; }
    /**
     * 获取轨道
     * @param int index 轨道索引
     * @return TrackModel* 轨道
     */
    QVector<TrackModel*> m_tracks; // 轨道
    //插件加载器
    PluginLoader* m_pluginLoader; // 插件加载器
    /**
     * 获取插件加载器
     * @return PluginLoader* 插件加载器
     */
    PluginLoader* getPluginLoader() const ;
    /**
     * 获取舞台
     * @return TimelineStage* 舞台  
     */
    TimelineStage* getStage() const { return m_stage; }
    /**
     * 设置舞台
     * @param TimelineStage* stage 舞台
     */
    void setStage(TimelineStage* stage);
    /**
     * 获取时间码生成器
     * @return TimecodeGenerator* 时间码生成器
     */
    TimecodeGenerator* getTimecodeGenerator() const { return m_timecodeGenerator; }
    /**
     * 获取时间码显示格式
     */
    TimedisplayFormat getTimeDisplayFormat() const {
        return  m_timeDisplayFormat;
    }
    /**
     * 设置时间码显示格式
     */
    void setTimeDisplayFormat(TimedisplayFormat val){
        m_timeDisplayFormat=val;
    }
    /**
     * 获取当前播放头位置的所有片段数据
     * @param int currentFrame 当前帧
     * @return QList<QVariantMap> 片段图像数据列表
     */
    QList<QVariantMap> getCurrentClipsData(int currentFrame) const {
        QList<QVariantMap> clipDataList;
        
        // 遍历所有轨道
        for (TrackModel* track : m_tracks) {
            // 遍历轨道中的所有片段
            for (AbstractClipModel* clip : track->getClips()) {
                // 检查当前帧是否在片段范围内
                if (currentFrame >= clip->start() && currentFrame < clip->end()) {
                    // 获取片段在当前帧的数据
                    QVariantMap data = clip->currentData(currentFrame);
                    data["layer"]=track->trackIndex();
                    clipDataList.append(data);
                }
            }
        }
        
        return clipDataList;
    }
signals:
    /**
     * 时间线更新
     */
    void S_timelineUpdated();
    /**
     * 轨道移动
     * @param int oldIndex 旧索引
     * @param int newIndex 新索引
     */
    void S_trackMoved(int oldIndex, int newIndex);
    /**
     * 播放头移动
     * @param int frame 帧
     */
    void S_playheadMoved(int frame);
    /**
     * 轨道改变
     */
    void S_trackChanged();
    /**
     * 舞台改变
     */
    void S_stageChanged();
    /**
     * 屏幕改变
     */
    void S_screensChanged();
    /**
     * 当前帧图像更新信号
     * @param const QVariantMap& image 图像
     */
    void frameImageUpdated(const QList<QVariantMap>& image);

public slots:
    /**
     * 开始播放
     */
    void onStartPlay();
    /**
     * 暂停播放
     */
    void onPausePlay();
    /**
     * 停止播放
     */
    void onStopPlay();
    /**
     * 移动播放头
     * @param int dx 移动的帧数
     */
    /**
     * 创建轨道
     * @param const QString& type 类型
     */
    void onAddTrack(const QString& type);
    /**
     * 删除轨道
     * @param int trackIndex 轨道索引
     */
    void onDeleteTrack(int trackIndex);

    /**
     * 设置时间码格式
     * @param TimecodeType type 时间码格式
     */
    void onTimecodeTypeChanged(TimecodeType type);
    /**
     * 计算时间线长度
     */
    void onTimelineLengthChanged();

private:
    // 查找片段所在轨道
    TrackModel* findParentTrackOfClip(AbstractClipModel* clip) const ;
    // 查找轨道行
    int findTrackRow(TrackModel* track) const ;
    // 设置插件加载器
    void setPluginLoader(PluginLoader* loader) ;
    // 舞台对象
    TimelineStage* m_stage; 
    // 默认时间码格式
    // TimecodeType m_timecodeType = TimecodeType::PAL;

    TimecodeGenerator* m_timecodeGenerator;

    // 时间显示格式，默认显示时间码
    TimedisplayFormat m_timeDisplayFormat = TimedisplayFormat::TimeCodeFormat;

    // /**
    //  * 处理当前帧数据
    //  * @param const QList<QVariant>& dataList 数据列表
    //  */
    // void processCurrentFrameData(const QList<QVariantMap>& dataList) {
    //     for (const QVariantMap& data : dataList) {
    //         if (data.contains("image")) {
    //             QImage image = data["image"].value<QImage>();
          
                    
                
    //         }
    //     }
    // }
};

#endif // TIMELINEMODEL_H
