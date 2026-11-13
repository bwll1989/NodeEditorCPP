#ifndef BASETIMELINEMODEL_H
#define BASETIMELINEMODEL_H

#include <QAbstractItemModel>
#include <unordered_set>
#include <unordered_map>
#include "AbstractClipModel.hpp"
#include <string>
#include <QMimeData>
#include <QIODevice>
#include <algorithm>
#include <QList>
#include <QJsonArray>
#include "TimeLineDefines.h"
#include "TimeLineStyle.h"
#include <QFile>
#include <QJsonDocument>
#include "BasePluginLoader.h"
#include "TimelineExports.hpp"
#include "Common/AppConfig/TimeCodeDefines.h"
using namespace QtTimeline;
struct TrackData {
    QString type;
    QString name;
    // 修改前: QVector<std::unique_ptr<AbstractClipModel>> clips;
    // 修改后: 使用 QList 替代 QVector
    QList<AbstractClipModel*> clips;
    // 按 start 排序的片段下标。只用于视口裁剪，不改变 clips 的绘制顺序。
    QList<int> clipsByStart;
    int maxClipSpan = 0;
    // 轨道展开态（2026-11-06 轨道展开 + clip 内嵌 tween panel）：
    // true = 行高扩展（trackHeight + trackExpandedExtra），展开区按片段
    // x/宽摆放 clip 面板。UI 内存态，不随 save() 序列化。
    bool expanded = false;
};
// TimelineModel类继承自QAbstractItemModel
class NODE_TIMELINE_PUBLIC BaseTimeLineModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    // 构造函数
    BaseTimeLineModel(QObject* parent = nullptr);

    ~BaseTimeLineModel();
    //获取当前播放位置
    virtual qint64 getPlayheadPos() const;
    /**
     * 获取索引
     * @param int row 行
     * @param int column 列
     * @param QModelIndex parent 父索引
     * @return QModelIndex 索引
     */
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    /**
     * 获取父索引
     * @param const QModelIndex &child 子索引
     * @return QModelIndex 父索引
     */
    virtual QModelIndex parent(const QModelIndex &child) const override;
    /**
     * 获取行数
     * @param const QModelIndex &parent 父索引
     * @return int 行数 
     */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    /**
     * 获取列数
     * @param const QModelIndex &parent 父索引
     * @return int 列数
     */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    /**
     * 获取数据
     * @param const QModelIndex &index 索引
     * @param int role 角色
     * @return QVariant 数据
     */
    virtual QVariant data(const QModelIndex &index, int role) const override;
    /**
     * 设置数据
     * @param const QModelIndex &index 索引
     * @param const QVariant &value 值
     * @param int role 角色
     * @return bool 是否设置成功
     */
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
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
    virtual void clear();
    /**
     * 获取轨道数量
     * @return int 轨道数量
     */
    virtual int getTrackCount() const ;
    /**
     * 获取轨道
     * @return QVector<TrackData> 轨道
     */
    QList<TrackData*> getTracks() const { return m_tracks; }
    /**
     * 获取插件加载器
     * @return BasePluginLoader* 插件加载器
     */
    BasePluginLoader* getPluginLoader() const ;

    // ---- 轨道行几何（2026-11-06 轨道展开：per-row 行高，两视图同源同规则）----
    // 行高唯一权威：展开行 = trackHeight + trackExpandedExtra，否则 trackHeight。
    // Timeline/TrackList 两视图的绘制/命中/滚动全部经由本组函数，禁止再直接
    // 读写 trackHeight 乘除行号——左右对齐由"同一数据源"保证。
    /// 行高（展开 = trackHeight + trackExpandedExtra；越界行 = trackHeight）
    int trackRowHeight(int row) const;
    /// 行顶在【内容坐标】的 y（不含 contentTop；= 前面各行高之和）
    int trackRowY(int row) const;
    /// 内容 y（相对行区顶部，不含 contentTop）→ 行号；越界 = -1
    int trackRowAt(int contentY) const;
    /// 全部行高之和（updateScrollBars 用）
    int totalTracksHeight() const;
    /// 轨道展开态（越界 = false）
    bool isTrackExpanded(int row) const;
    /// 设置轨道展开态（发 dataChanged(TrackExpandedRole)，两视图自动重绘/重摆）
    void setTrackExpanded(int row, bool on);

    // 设置插件加载器
    void setPluginLoader(BasePluginLoader* loader) ;

    virtual TimeCodeType getTimeCodeType() const
    {
        return TimeCodeType::PAL;
    }

    virtual void setTimeCodeType(TimeCodeType type)
    {
        m_timeCodeType=type;
    }
signals:
    /**
     * @brief 播放头位置变化信号
     * @details
     * 播放头不是 Model/View 树中的某一行数据，因此不适合用 dataChanged 表达。
     * 保留此信号供 Timeline/TrackList 的标题区域轻量刷新使用。
     */
    void S_playheadMoved(qint64 frame); //播放头位置（μs）

    /**
     * @brief 时间线总长度变化信号
     * @details
     * 总长度是聚合值（由所有 clip 的 end 取 max 计算），不对应某一行的直接字段。
     */
    void S_LengthChanged(qint64 length);
public slots:
    //时间轴长度变化槽函数
    virtual qint64 onUpdateTimeLineLength();
    //设置播放头位置槽函数
    virtual void onSetPlayheadPos(qint64 newPlayheadPos);
    //通过轨道索引和开始帧添加片段
    virtual void onAddClip(int trackIndex,qint64 startUs);
    // 拖入媒体文件（资源管理器拖拽 dropEvent 唯一入口）。默认实现=纯占位
    // 片段（忽略文件——与旧 dropEvent 行为一致）；适配层覆写完成
    // "导入素材+绑定+按素材时长设长"（2026-11-11 音频片段时长联动）。
    virtual void onDropMediaFile(int trackIndex, qint64 startUs, const QString& file) {
        onAddClip(trackIndex, startUs); // 基类默认：无导入能力的模型走占位语义
    }
    //删除片段槽函数
    virtual void onDeleteClip(QModelIndex clipIndex);
    //通过类型添加轨道槽函数
    virtual void onAddTrack(const QString& type);
    //删除轨道槽函数
    virtual void onDeleteTrack(int trackIndex);
    //移动轨道槽函数
    virtual void onMoveTrack(int sourceRow, int targetRow);

    // 拖拽期间只改片段几何，松手后再排序、重算长度并发出 dataChanged
    void beginInteractiveGeometryChange(AbstractClipModel* clip);
    void endInteractiveGeometryChange();
    void syncInteractiveClipOrder();

protected:
    /**
     * @brief 通过 clip 指针反查其在模型中的索引
     * @details
     * ClipTimePanel 会直接调用 AbstractClipModel::setStart/setEnd 等修改片段。
     * 为了让 View 能走 Qt 标准路径刷新，这里提供从 clip 指针构建 QModelIndex 的能力，
     * 以便在监听到 clip 的信号后发出 dataChanged。
     */
    QModelIndex indexForClip(AbstractClipModel* clip) const;

    struct ClipLocation {
        int trackRow = -1;
        int clipRow = -1;
        qint64 end = 0;
    };

    // 增删轨道或片段后重建，供 parent/data/indexForClip 做 O(1) 查询
    void rebuildIndex();
    void rebuildTrackClipOrder(TrackData* track);
    void noteClipGeometryChanged(AbstractClipModel* clip);
    void watchClip(AbstractClipModel* clip);
    void queueClipGeometryNotify(AbstractClipModel* clip);
    void flushClipGeometryNotify();
    // 新终点超过当前总长时直接更新。只有原来的最大终点变小才重扫。
    void applyLengthChange(int oldEnd, int newEnd);
    bool isTrackPointer(void* ptr) const;

    //插件加载器
    BasePluginLoader* m_pluginLoader; // 插件加载器
    /**
     * 获取轨道
     * @param int index 轨道索引
     * @return TrackData 轨道
     */
    // 修改前: QVector<std::unique_ptr<TrackData>> m_tracks;
    // 修改后: 使用 QList 替代 QVector
    QList<TrackData*> m_tracks;

    /**
     * P4（timeline-draw-zoom-review）：行几何前缀和缓存（懒重建）。
     * m_rowPrefixY[i] = 前 i 行高之和（size = 行数+1）。行展开/增删/
     * 移动/清空时置脏，下次查询一次 O(n) 重建，此后查询 O(1)——
     * 百级轨道工程 drawTracks 单帧 O(n²) 的行几何求值降为 O(n)。
     */
    mutable std::vector<int> m_rowPrefixY;
    mutable bool m_rowPrefixDirty = true;
    void invalidateRowPrefix() const { m_rowPrefixDirty = true; }
    void ensureRowPrefix() const;

    std::unordered_map<AbstractClipModel*, ClipLocation> m_clipLocation;
    std::unordered_set<TrackData*> m_trackSet;
    bool m_deferGeometryNotify = false;
    AbstractClipModel* m_interactiveClip = nullptr;
    int m_interactiveOriginalEnd = 0;
    bool m_geometryNotifyQueued = false;
    QList<AbstractClipModel*> m_pendingGeometryClips;

    qint64 m_currentFrame = 0; // 播放头位置（μs）

    qint64 m_lengthFrame = 0; // 时间轴总时长（μs）

    ClipId m_clipNextId=0;

    TimeCodeType m_timeCodeType=TimeCodeType::PAL;
};
#endif // TIMELINEMODEL_H

