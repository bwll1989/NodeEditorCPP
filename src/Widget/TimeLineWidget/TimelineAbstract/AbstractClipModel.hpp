#ifndef ABSTRACTCLIPMODEL_HPP
#define ABSTRACTCLIPMODEL_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include "Widget/TimeLineWidget/timelinetypes.h"

class AbstractClipModel : public QObject {
    Q_OBJECT
public:
    explicit AbstractClipModel(int start, int end, const QString& type, TimecodeType timecodeType=TimecodeType::PAL, QObject* parent = nullptr)
        : QObject(parent),m_type(type), m_start(start), m_end(end), m_timecodeType(timecodeType)  
        {
            // 是否可调整大小
            RESIZEABLE = true;
            // 是否显示小部件
            EMBEDWIDGET = true;
            // 是否显示边框
            SHOWBORDER = true;
        }
    /**
     * 析构函数
     */
    virtual ~AbstractClipModel();

    // Getters
    /**
     * 开始
     * @return int 开始
     */
    int start() const ;
    /**
     * 结束
     * @return int 结束
     */
    int end() const ;
    /**
     * 开始时间码
     * @return Timecode 开始时间码
     */
    Timecode getStartTimeCode() const ;
    /**
     * 结束时间码
     * @return Timecode 结束时间码
     */
    Timecode getEndTimeCode() const ;
    /**
     * 设置时间码类型
     * @param TimecodeType timecodeType 时间码类型
     */
    void setTimecodeType(TimecodeType timecodeType);
    /**
     * 类型
     * @return QString 类型
     */
    QString type() const;
    /**
     * 长度
     * @return int 长度
     */
    int length() const ;
    /**
     * 轨道索引
     * @return int 轨道索引
     */
    int trackIndex() const ;
    /**
     * 是否可调整大小
     * @return bool 是否可调整大小
     */
    bool isResizable() const ;
    /**
     * 是否显示小部件
     * @return bool 是否显示小部件
     */
    bool isEmbedWidget() const ;
    /**
     * 是否显示边框
     * @return bool 是否显示边框
     */
    bool isShowBorder() const ;
    // Setters
    /**
     * 设置开始
     * @param int start 开始
     */
    void setStart(int start) ;
    /**
     * 设置结束
     * @param int end 结束
     */
    void setEnd(int end);
    /**
     * 设置轨道索引
     * @param int index 轨道索引
     */
    void setTrackIndex(int index);
    /**
     * 设置是否可调整大小
     * @param bool resizable 是否可调整大小
     */
    void setResizable(bool resizable) ;
    /**
     * 设置是否显示小部件
     * @param bool embedWidget 是否显示小部件
     */
    void setEmbedWidget(bool embedWidget) ;
    /** 
     * 设置显示边框
     * @param bool showBorder 显示边框
    */
    void setShowBorder(bool showBorder);
    /**
     * 保存
     * @return QJsonObject 数据
     */
    virtual QJsonObject save() const ;
    /**
     * 加载
     * @param const QJsonObject& json 数据
     */
    virtual void load(const QJsonObject& json);
    /**
     * 获取数据
     * @param int role 角色
     * @return QVariant 数据
     */
    virtual QVariant data(int role) const {
        switch (role) {
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue<AbstractClipModel*>(const_cast<AbstractClipModel*>(this));
            default:
                return QVariant();
        }
    }
    /**
     * 获取当前音频数据
     * @param int currentFrame 当前帧
     * @return QVariantMap 当前音频数据
     */
    virtual QVariantMap currentAudioData(int currentFrame) const;
    /**
     * 获取当前视频数据
     * @param int currentFrame 当前帧
     * @return QVariantMap 当前视频数据
     */
    virtual QVariantMap currentVideoData(int currentFrame) const;
    /**
     * 获取当前控制数据
     * @param int currentFrame 当前帧
     * @return QVariantMap 当前控制数据
     */
    virtual QVariantMap currentControlData(int currentFrame) const;
Q_SIGNALS:
    /**
     * 数据变化信号
     */
    void lengthChanged();  // 添加长度变化信号
//    void startChanged();  // 添加开始变化信号
//    void endChanged();  // 添加结束变化信号
    void filePathChanged(const QString& filePath);  // 添加文件路径变化信号

    void sizeChanged(QSize size);   //添加尺寸变化信号
    void posChanged(QPoint position); //位置变化信号

    void rotateChanged(int rotete); //旋转变化信号
    void timelinePositionChanged(int frame); //时间轴上位置变化信号
    void videoDataUpdate() const; //视频数据更新
    void audioDataUpdate() const; //音频数据更新
    void controlDataUpdate() const; //控制数据更新

protected:
    // 开始
    int m_start;
    // 开始时间码
    Timecode m_startTimeCode;
    // 结束
    int m_end;
    // 结束时间码
    Timecode m_endTimeCode;
    // 类型
    QString m_type;
    // 轨道索引
    int m_trackIndex;
    // 是否可调整大小
    bool RESIZEABLE;
    // 是否显示小部件
    bool EMBEDWIDGET;
    // 是否显示边框
    bool SHOWBORDER;
    // 时间码类型
    TimecodeType m_timecodeType;
};

Q_DECLARE_METATYPE(AbstractClipModel*)

#endif // ABSTRACTCLIPMODEL_HPP 