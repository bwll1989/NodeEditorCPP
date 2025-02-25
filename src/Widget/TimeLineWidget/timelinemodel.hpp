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
// TimelineModel类继承自QAbstractItemModel
class TimelineModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    // 构造函数
    TimelineModel(QObject* parent = nullptr) 
        : QAbstractItemModel(parent), m_pluginLoader(nullptr) , m_stage(new TimelineStage()) {
        m_pluginLoader = new PluginLoader();
        m_pluginLoader->loadPlugins();

    }

    ~TimelineModel();
    void createTrack(const QString& type);

    // 移动播放头
    void movePlayhead(int dx);

    // 获取播放头位置
    int getPlayheadPos() const;
    // 设置播放头位置
    void setPlayheadPos(int newPlayheadPos);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    // 将帧转换为时间字符串
    QString convertFramesToTimeString(int frames, double fps);

    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    // 设置数据
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    // 获取支持的拖放操作
    Qt::DropActions supportedDropActions() const override;

    // 获取项目标志
    Qt::ItemFlags flags(const QModelIndex &index) const ;

    void deleteClip(QModelIndex clipIndex);

    void clear();
    // 保存模型
    QJsonObject save() const;

    void load(const QJsonObject &modelJson) ;
    // 计算时间线长度
    void calculateLength();
    // 获取时间线长度
    QVector<TrackModel*> m_tracks; // 轨道

    PluginLoader* m_pluginLoader; // 插件加载器
// 获取插件加载器
    PluginLoader* getPluginLoader() const ;

    // 获取和设置舞台
    TimelineStage* getStage() const { return m_stage; }
    void setStage(TimelineStage* stage);

signals:
    // 时间线更新
    void timelineUpdated();
    // 新片段
    void newClip(int row,int track);
    // 轨道移动
    void trackMoved(int oldIndex,int newIndex);
    // 播放头移动
    void playheadMoved(int frame);
    // 轨道改变
    void tracksChanged();
    void stageChanged();
    void screensChanged();

public slots:
    // 创建轨道
    void createTrackForType(const QString& type);
    // 删除轨道
    void deleteTrack(int trackIndex);

private:
    int m_length = 0; // 时间线长度

    int m_playheadPos = 0; // 播放头位置
    // 查找片段所在轨道
    TrackModel* findParentTrackOfClip(AbstractClipModel* clip) const ;
    // 查找轨道行
    int findTrackRow(TrackModel* track) const ;
    // 设置插件加载器
    void setPluginLoader(PluginLoader* loader) ;
    // 舞台对象
    TimelineStage* m_stage;                 
};

#endif // TIMELINEMODEL_H
