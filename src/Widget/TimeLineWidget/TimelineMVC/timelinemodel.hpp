#ifndef TIMELINEMODEL_HPP
#define TIMELINEMODEL_HPP

#include <QAbstractItemModel>
#include <unordered_set>
#include <unordered_map>
#include "trackmodel.hpp"
#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include <string>
#include <QMimeData>
#include <QIODevice>
#include <algorithm>
#include <vector>
#include <QJsonArray>
#include "Widget/TimeLineWidget/TimelineAbstract/timelinetypes.h"
#include "timelinestyle.hpp"
#include "ltc.h"
#include <QFile>
#include <QJsonDocument>
#include "pluginloader.hpp"
#include "Widget/TimeLineWidget/TimelineStageWidget/timelinestage.hpp"
#include "Widget/TimeLineWidget/TimelineScreenWidget/timelinescreen.hpp"
#include "Widget/TimeLineWidget/TimeCodeGenerator/timecodegenerator.hpp"
#include "TimeCodeMessage.h"
// TimelineModel类继承自QAbstractItemModel
class TimelineModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    // 构造函数
    TimelineModel(QObject* parent = nullptr) 
        : QAbstractItemModel(parent), m_pluginLoader(nullptr) ,m_stage(new TimelineStage())
        {
        m_pluginLoader = new PluginLoader();
        m_pluginLoader->loadPlugins();
        m_timecodeGenerator = new TimeCodeGenerator();
        // 连接时间码生成器信号
        connect(m_timecodeGenerator, &TimeCodeGenerator::currentFrameChanged,
            this, &TimelineModel::onSetPlayheadPos);
        qRegisterMetaType<AbstractClipModel*>();

        // 连接播放头移动信号处理当前帧数据
        connect(m_timecodeGenerator, &TimeCodeGenerator::currentFrameChanged,
            this, [this](qint64 frame) {
                    onCreateCurrentVideoData(frame);
               
            });
        //轨道移动后更新当前帧数据
        connect(this, &TimelineModel::S_trackMoved, this, [this](int oldindex,int newindex){
            onCreateCurrentVideoData(this->getPlayheadPos());
        });
        //删除轨道后更新当前帧数据
        connect(this, &TimelineModel::S_trackDelete, this, [this](){
            onCreateCurrentVideoData(this->getPlayheadPos());
        });
        //删除片段后更新当前帧数据
        connect(this, &TimelineModel::S_deleteClip, this, [this](){
            onCreateCurrentVideoData(this->getPlayheadPos());
        });
    }

    ~TimelineModel();
    /**
     * 获取播放头位置
     * @return int 播放头位置
     */
    int getPlayheadPos() const;
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
    TimeCodeType getTimecodeType() const { return m_timecodeGenerator->getTimecodeType(); }
    /**
     * 设置时间码格式
     * @param TimecodeType type 时间码格式
     */
    void setTimecodeType(TimeCodeType type){
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
     * 获取时间码生成器
     * @return TimecodeGenerator* 时间码生成器
     */
    TimeCodeGenerator* getTimecodeGenerator() const { return m_timecodeGenerator; }
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
    void S_trackAdd();
    void S_trackDelete();
    /**
     * 舞台初始化信号
     */
    void S_stageInited();

    /**
     * 屏幕改变
     */
    void S_addScreen();
    void S_deleteScreen();
    /**
     * 当前帧图像更新信号
     * @param const QVariantMap& image 图像
     */
    void S_frameImageUpdated(const QList<QVariantMap>& image) const ;

    void S_timelineLengthChanged();

    void S_addClip();

    void S_deleteClip();

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
     * 设置时间码格式
     * @param TimecodeType type 时间码格式
     */
    void onTimecodeTypeChanged(TimeCodeType type);
    /**
     * 计算时间线长度
     */
    void onTimelineLengthChanged();
     /**
     * 获取当前播放头位置的所有片段数据
     * @param int currentFrame 当前帧
     * @return QList<QVariantMap> 片段图像数据列表
     */
    QList<QVariantMap> onCreateCurrentVideoData(int currentFrame) const {
        QList<QVariantMap> clipDataList;
        // 遍历所有轨道
        for (TrackModel* track : m_tracks) {
            // 遍历轨道中的所有片段
            for (AbstractClipModel* clip : track->getClips()) {
                // 检查当前帧是否在片段范围内
                if (currentFrame >= clip->start() && currentFrame <= clip->end()) {
                    // 获取片段在当前帧的数据
                    QVariantMap data = clip->currentData(currentFrame);
                    if(!data.isEmpty()) {
                        clipDataList.append(data);
                    }
                }
            }
        }
        
        
        emit S_frameImageUpdated(clipDataList);
        
        return clipDataList;
    }
    /**
     * 设置播放头位置
     * @param int newPlayheadPos 新的播放头位置
     */
    void onSetPlayheadPos(int newPlayheadPos);
    //通过轨道索引和开始帧添加片段
    void onAddClip(int trackIndex,int startFrame);
    //通过片段模型添加片段
    void onAddClip(AbstractClipModel* clip);
    /**
     * 删除片段
     * @param QModelIndex clipIndex 片段索引
     */
    void onDeleteClip(QModelIndex clipIndex);
    /**
     * 设置舞台
     * @param TimelineStage* stage 舞台
     */
    void onSetStage(TimelineStage* stage);
    /**
     * 创建轨道
     * @param const QString& type 类型
     */
    void onAddTrack(const QString& type);
    //通过片段模型添加轨道
    void onAddTrack(TrackModel* track);
    /**
    * 删除轨道
    * @param int trackIndex 轨道索引
    */
    void onDeleteTrack(int trackIndex);

    void onMoveTrack(int sourceRow, int targetRow);

    void onClockSourceChanged(ClockSource source);
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

    TimeCodeGenerator* m_timecodeGenerator;

    // 时间显示格式，默认显示时间码
    TimedisplayFormat m_timeDisplayFormat = TimedisplayFormat::AbsoluteTimeFormat;
    //插件加载器
    PluginLoader* m_pluginLoader; // 插件加载器
    /**
     * 获取轨道
     * @param int index 轨道索引
     * @return TrackModel* 轨道
     */
    QVector<TrackModel*> m_tracks; // 轨道
 
};

#endif // TIMELINEMODEL_H
