// ============================================================================
// TimelineEngine.h —— TimeLineNode 内置时间线引擎（单头版）
//
// 本文件由原 QtTimeLine 副本的 18 个头文件合并而成（TimeLineDefines/TimeLineStyle/
// AbstractClipModel[现名 TimeLineClipModel]/AbstractClipInterface/NodeClipDelegateModel/
// BasePluginLoader/BaseTimeLineModel/BaseTimeLineView/BaseTrackDelegate/
// BaseTrackListView/BaseTimeLineToolBar/ClipTimePanel/ClipAppearancePanel），
// 实现在 TimelineEngine.cpp（原 9 个 .cpp 合并）。
// 其中 Compiler/OperatingSystem/TimelineExports 已精简为下方 STATIC-only 导出宏，
// OSCMessage/TimeCodeDefines 改为引用 src/Common/AppConfig/ 的全软件唯一版本。
//
// 设计要点：
//  - 与主 timeline（原 QtTimeLine 体系 + plugins/TimeLine/*.plugin）完全
//    隔离，主 timeline 升级不影响本节点。
//  - 以 NODE_TIMELINE_STATIC 编译，NODE_TIMELINE_PUBLIC 展开为空；
//    即使 BaseClass 经 INTERFACE 传入 NODE_TIMELINE_SHARED 也以 STATIC 优先。
//  - 资源前缀 :/tlnode/icons（tlnode_resources.qrc，prefix "tlnode"），
//    由 TimeLineDataModel 构造函数 Q_INIT_RESOURCE(tlnode_resources) 初始化。
//  - clip 不走 QPluginLoader：BasePluginLoader::loadPlugins() 静态注册内置
//    的 6 个剪辑工厂；剪辑代理基类 NodeClipDelegateModel 的基类链完全在
//    本节点内，绝不与 BaseClass.dll 的 AbstractClipDelegateModel 混用。
// ============================================================================
#pragma once

// ============================================================================
// 导出宏（精简版）—— 本节点的 QtTimeLine 副本一律静态编译/静态链接。
// 主工程的 BaseClass.dll 等依赖会经 INTERFACE 传递 NODE_TIMELINE_SHARED，
// 与静态模式冲突；此处强制 NODE_TIMELINE_STATIC 优先，
// 保证副本头不受宿主传递的共享宏影响（与 install/include 原版行为隔离）。
// ============================================================================
#if defined(NODE_TIMELINE_STATIC)
#define NODE_TIMELINE_PUBLIC
#else
#error "TimeLineNode only builds as NODE_TIMELINE_STATIC (see TimeLineNode/CMakeLists.txt)"
#endif

// ---------- 公共定义统一引用主工程 AppConfig ----------
// OSCMessage / TimeCodeDefines 不再在本文件内重复定义，
// 直接引用 src/Common/AppConfig/ 下的唯一版本（与全软件其余部分一致）。
// TimeLineNode 的所有 target 均带 -I src，此相对路径可解析。
#include "Common/AppConfig/OSCMessage.h"
#include "Common/AppConfig/TimeCodeDefines.h"

// ---------- TimeLineDefines.h ----------
#ifndef TIMELINETYPES_H
#define TIMELINETYPES_H
#include "QString"
#include <QMetaType>
#include <QtGlobal>
#include <QTime>
#include <QtCore/QMetaObject>
namespace QtTimeline
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
NODE_TIMELINE_PUBLIC Q_NAMESPACE
#else
Q_NAMESPACE_EXPORT(NODE_TIMELINE_PUBLIC)
#endif

    enum  hoverState {LEFT,RIGHT,CENTER,NONE};
    enum class ItemType {
        Track, // 轨道
        Clip, // 剪辑
        Invalid // 无效
    };
    Q_ENUM_NS(ItemType)

    enum TimelineRoles {
        ClipIdRole, // 剪辑ID
        ClipInRole, // 剪辑开始时间
        ClipOutRole, // 剪辑结束时间
        ClipOscWidgetsRole, //剪辑OSC控件地址
        ClipLengthRole, // 剪辑长度
        ClipPosRole,//剪辑位置
        ClipTypeRole, // 剪辑类型
        ClipShowWidgetRole, // 剪辑是否显示小部件
        ClipResizableRole, // 剪辑是否可调整大小
        ClipShowBorderRole, // 剪辑是否显示边框
        ClipModelRole,  // 用于获取完整的模型对象
        TrackTypeRole, // 轨道类型
        TrackNameRole, // 轨道名称
        TrackModelRole, // 轨道模型
        TrackClipsCountRole, // 轨道剪辑数量
        TrackClipsRole, // 轨道剪辑列表
        TimelineLengthRole, // 时间轴长度
        TimeCodeTypeRole, //时间码格式
        TimelineModelAliasRole, // 时间线模型别名（用于 OSC 地址前缀）
        ClipColorRole, // 片段颜色
        ClipNoteRole, // 片段备注名
    };
    Q_ENUM_NS(TimelineRoles)

    using ClipId = unsigned int;
    // namespace TimecodeFormat {
    //     // 标准电影帧率
    //     constexpr int FILM = 24;           // 24 fps - 标准电影
    //     constexpr double FILM_DF = 23.976; // 24 fps - 标准电影 Drop Frame
    //     // 标准电视帧率 constexpr int FILM
    //     constexpr int NTSC = 30;          // 29.97 fps - NTSC
    //     constexpr double NTSC_DF = 29.97;  // 29.97 fps - NTSC Drop Frame
    //     constexpr int PAL = 25;           // 25 fps - PAL/SECAM
    //     constexpr double PAL_DF = 24.975; // 25 fps - PAL/SECAM Drop Frame
    // }



    // 时间显示格式
    enum class TimedisplayFormat {
        TimeCodeFormat, // 时间码
        AbsoluteTimeFormat, // 绝对时间
    };
    Q_ENUM_NS(TimedisplayFormat)

    enum class ClockSource {
        LTC,
        MTC,
        Internal,
    };
    Q_ENUM_NS(ClockSource)
    // 定义支持的文件类型列表
    inline QStringList VideoTypes = {"mp4", "avi", "mov", "wmv", "mkv", "flv"};
    inline QStringList AudioTypes = {"mp3", "wav", "aac", "ogg", "flac"};
    inline QStringList ImageTypes = {"jpg", "jpeg", "png", "bmp", "gif", "tiff"};
    inline QStringList ControlTypes = {"ilk"};
    // 将track类型转换为字符串
    static QString TrackTypeString(const QString& type) {
        return type;
    }
}
#endif // TIMELINETYPES_H

// ---------- TimeLineStyle.h ----------
#ifndef TIMELINESTYLE_H
#define TIMELINESTYLE_H
//
// Created by bwll1 on 2024/5/30.
//
// TimeLineStyle —— QtTimeLine 样式默认值（QSS 回退）
//
// ⚠️ 样式来源约定（重要）：
// 1. 运行时的外观颜色一律通过 QSS 配置：
//      BaseTimelineView  { qproperty-timelineBgColor: ...; qproperty-... }
//      BaseTracklistView { qproperty-trackColor: ...; qproperty-... }
//      ClipAppearancePanel { qproperty-swatchBorderColor: ...; qproperty-... }
// 2. 本文件中的颜色仅作为「QSS 未生效时的回退默认值」：
//      - 视图成员 m_xxxColor 在构造时从这里取默认值；
//      - QSS 通过 qproperty- 覆盖成员值（见 BaseTimeLineView.h / BaseTrackListView.h）。
// 3. 标注 [clip-sync] 的变量是绘制桥接缓存：
//      BaseTimelineView::applyStyleFromProperties() 会在 QSS 生效/变化时，
//      把视图成员颜色同步到这些全局量，供片段(TimeLineClipModel 子类)的
//      paint() 代码读取——片段模型不是 QWidget，无法直接被 QSS qproperty- 定向。
//      除该同步函数外，任何代码都不应再写这些全局量。
//

#include "QColor"

// ===================== 布局度量（结构参数，非颜色） =====================
// 说明：这些参数参与滚动/命中/绘制几何计算，随 QSS 热切换会导致缓存错位，
// 因此保持为编译期常量，不通过 QSS 配置。
inline int toolbarButtonWidth = 16;  // 工具栏按钮宽度
inline int toolbarHeight = 30;       // 工具栏高度
inline int trackHeight = 40;         // 轨道高度
inline int rulerHeight = 30;         // 刻度高度
inline int clipoffset = 2;           // clip 偏移
inline int clipround = 0;            // clip 圆角
inline int textoffset = 13;          // 文本偏移
inline int fontSize = 25;            // 显示时间字体大小
inline int playheadwidth = 5;        // 播放头宽度
inline int playheadheight = 12;      // 播放头高度
inline int playheadCornerHeight = playheadheight / 4; // 播放头圆角高度
inline int ClipBorderWidth = 2;      // clip 选中边框宽度

// ===================== 时间刻度（缩放参数） =====================
/**
 * 参数说明：timescale初始值（像素/帧）
 * - 语义：当前时间轴上“每帧占据的像素宽度”，单位 px/frame。
 * - 作用：被用于 frameToPoint/pointToFrame、刻度绘制、剪辑位置与宽度计算等所有横向度量相关的逻辑。
 * - 缩放行为：timescale 越大，时间轴越“放大”（每帧更宽）；timescale 越小，时间轴越“缩小”（每帧越窄）。
 * - 取值范围：通常应在 [minTimescale, baseTimeScale] 之间；UI 层通过 currentScale（0~1）线性映射到此范围。
 *   映射示例：timescale = minTimescale + currentScale * (baseTimeScale - minTimescale)
 * - 初始化建议：若修改 timescale 的默认值，建议同步更新 BaseTimeLineView.h 中 currentScale 的初始值为
 *   (timescale - minTimescale) / (baseTimeScale - minTimescale)，避免程序第一次滚轮缩放出现“跳变”。
 */
inline double timescale = 0.5; // 时间刻度(像素/帧)

/**
 * 参数说明：minTimescale初始值（最小像素/帧）
 * - 语义：允许的最小缩放下限，即每帧占用的最小像素宽度，限制过度缩小导致文本/控件不可见或精度问题。
 * - 调参建议：根据帧率与字体大小设定。一般在 25fps 与默认字体下，可取 0.02~0.10；
 *   若出现刻度文本过密、重叠，可适当增大 minTimescale 或配合刻度步长自适应策略。
 * - 与 currentScale 的关系：当 currentScale=0 时应映射到 minTimescale，以保证最小缩放可达。
 */
inline double minTimescale = 0.005; // 最小缩放(像素/帧)，可按需调小，如 0.02

inline int baseTimeScale = 10; // 基础时间刻度，用于计算缩放上限

// ===================== 颜色默认值（均可被 QSS qproperty- 覆盖） =====================
// 颜色取值遵循 docs/brand/Flow-Brand-Visual-Identity-Manual.html（流动绿 #79AE31）：
// 选中/强调 = #79AE31；悬停 = 深一档绿 #39571E(800)；片段悬停 = #9CC455(400)。

// ---- 时间轴视图 BaseTimelineView 的可配置颜色（QSS 属性名见 BaseTimeLineView.h）----
inline QColor bgColour            = QColor(53, 53, 53);        // 背景（qproperty-timelineBgColor）
inline QColor seperatorColour     = QColor("#414141");         // 轨道分隔线（qproperty-timelineSeparatorColor）
inline QColor rulerColour         = QColor("#a7a7a7");         // 刻度/标尺（qproperty-timelineRulerColor）
inline QColor playheadColour      = QColor(206, 91, 87);       // 播放头（qproperty-timelinePlayheadColor）
inline QColor ClipColor           = QColor(126, 188, 105);     // 片段默认填充（qproperty-clipFillColor）
inline QColor ClipBorderColour    = QColor(235, 154, 5);       // 片段边框 [clip-sync]（qproperty-clipBorderColor）
inline QColor trackSelectedColour = QColor("#79AE31");         // 轨道选中·品牌流动绿（qproperty-trackSelectedColor）

// ---- 轨道列表视图 BaseTracklistView 的可配置颜色（QSS 属性名见 BaseTrackListView.h）----
inline QColor trackColour     = QColor(53, 53, 53);            // 轨道行背景（qproperty-trackColor）
inline QColor trackTitleColor = QColor(255, 255, 255);         // 轨道名文字（qproperty-trackTitleColor）
inline QColor trackHoverColour = QColor("#39571E");            // 轨道悬停·绿 800（qproperty-trackHoverColor）

// ---- 片段绘制桥接缓存 [clip-sync]：由 BaseTimelineView::applyStyleFromProperties() 维护 ----
// 片段 paint() 中只读；QSS 通过 clipFillColor/clipBorderColor 等属性间接更新。
inline QColor ClipHoverColor    = QColor("#9CC455");           // 片段悬停色·绿 400 [clip-sync]（保留供子类参考）
inline QColor ClipSelectedColor = QColor("#39571E");           // 片段选中色·绿 800 [clip-sync]（保留供子类参考）

// ---- 标尺时间文字颜色（qproperty-timelinePlayheadColor on BaseTracklistView）----
// 与播放头同色系，标尺中央显示的当前时间文字使用。
#endif // TIMELINESTYLE_H


// ---------- 片段模型基类段（源自旧 QtTimeLine AbstractClipModel.hpp；类已改名 TimeLineClipModel）----------
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QAbstractItemModel>
#include <QWidget>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QEvent>
#include <QPushButton>
#include <QSlider>
#include <QMimeData>
#include <QGroupBox>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QRect>
#include <QColor>
#include <QFont>
#include <qDrag>
#include <QCheckBox>
// 显式依赖：本头使用 std::unordered_map（OSC 映射表）与 std::find_if（eventFilter）。
// 不得依赖其他头（如已移除的面板头）的传递包含。
#include <unordered_map>
#include <algorithm>
// 注意：不要在本头文件 include ClipTimePanel.hpp / ClipAppearancePanel.hpp。
// 本类持有它们的指针成员（timeGroupBox），用前置声明即可；若在此 include，
// 所有 include 本头的消费方（宿主/插件）TU 都会连带展开面板类的
// tr() / staticMetaObject 符号——后者是静态数据符号，无法通过导入库解析，
// 将导致消费方链接失败（LNK2019）。完整定义只在库内 .cpp 中使用。
class ClipTimePanel;
class ClipAppearancePanel;
using namespace QtTimeline;
class NODE_TIMELINE_PUBLIC TimeLineClipModel : public QObject {
    Q_OBJECT
public:
    explicit TimeLineClipModel(int start,const QString& type, QObject* parent=nullptr): QObject(parent),
    m_type(type),
    m_start(start),
    m_end(start+100),
    m_clipPropertyWidget(nullptr),
    m_standardPropertyWidget(nullptr){
        // 是否可调整大小
        RESIZEABLE = true;
        // 是否在片段中嵌入编辑器
        EMBEDWIDGET = false;
        // 是否显示边框
        SHOWBORDER = true;

    }
    /**
     * 析构函数
     */
~TimeLineClipModel() override 
{
    // 关闭并删除属性窗口
    if (m_standardPropertyWidget && m_standardPropertyWidget->isVisible()) {
        m_standardPropertyWidget->close();
        m_standardPropertyWidget->deleteLater();
    }
    
}

ClipId id() const { return m_id; }

virtual void setId(ClipId id) { m_id = id; }

void setTimelineModel(QAbstractItemModel* model) { m_timelineModel = model; }
QAbstractItemModel* timelineModel() const { return m_timelineModel; }

    // Getters
    /**
     * 开始
     * @return int 开始
     */
int start() const { return m_start; }
    /**
     * 结束
     * @return int 结束
     */
int end() const { return m_end; }
    /**
     * 类型
     * @return QString 类型
     */
QString type() const { return m_type; }
QString noteName() const { return m_noteName; }
QString displayText() const { return m_noteName.isEmpty() ? m_type : m_noteName; }
QColor color() const { return m_color.isValid() ? m_color : ClipColor; }
bool hasCustomColor() const { return m_color.isValid(); }
    /**
     * 长度
     * @return int 长度
     */
int length() const { return m_end - m_start; }
    /**
     * 是否可调整大小
     * @return bool 是否可调整大小
     */
bool isResizable() const { return RESIZEABLE; }
    /**
     * 是否显示小部件
     * @return bool 是否显示小部件
     */
bool isEmbedWidget() const { return EMBEDWIDGET; }
        // 标准属性窗口
QWidget* m_standardPropertyWidget;
    /**
     * 是否显示边框
     * @return bool 是否显示边框
     */
bool isShowBorder() const { return SHOWBORDER; }
    // Setters
    /**
     * 设置开始
     * @param int start 开始
     */
virtual void setStart(int start) { 
    if (m_start != start) {
        if(RESIZEABLE){
           
            m_start = start; 

            emit timelinePositionChanged(m_start);
        }else{
            auto length = this->length();
            m_start = start;
            m_end = m_start + length;
            emit timelinePositionChanged(m_start);
            emit lengthChanged(m_end);
        }
        stateFeedBack("/start_frame", start);
        stateFeedBack("/position_frame",start);

    }
}
virtual void setMedia(const QVariant& filePath){};
/**
 * 设置结束
 * @param int end 结束
 */
virtual void setEnd(int end) { 
    if (m_end != end) {
        if(RESIZEABLE){
            m_end = end;

            emit lengthChanged(m_end);
        }else{
            auto length = this->length();
            m_end = end;
            m_start = m_end - length;
            emit lengthChanged(m_end);
            emit timelinePositionChanged(m_start);
        }
        stateFeedBack("/end_frame", end);
    }
}
    /**
     * 设置是否可调整大小
     * @param bool resizable 是否可调整大小
     */
virtual void setResizable(bool resizable) { RESIZEABLE = resizable; }
    /**
     * 设置是否显示小部件
     * @param bool embedWidget 是否显示小部件
     */
void setEmbedWidget(bool embedWidget) { 
    if (EMBEDWIDGET != embedWidget) {
        EMBEDWIDGET = embedWidget;
    }
}
    /** 
     * 设置显示边框
     * @param bool showBorder 显示边框
    */
void setShowBorder(bool showBorder) { SHOWBORDER = showBorder; }

void setNoteName(const QString& name) {
    if (m_noteName == name)
        return;
    m_noteName = name;
    emit appearanceChanged();
}

void setColor(const QColor& color) {
    if (!color.isValid() || m_color == color)
        return;
    m_color = color;
    emit appearanceChanged();
}
    /**
     * 保存
     * @return QJsonObject 数据
     */

virtual void setTimeCodeType(TimeCodeType timeCodeType){m_timeCodeType=timeCodeType;}

virtual TimeCodeType getTimeCodeType() const {return m_timeCodeType;}

virtual QJsonObject save() const {
    QJsonObject clipJson;
    clipJson["start"] = m_start;
    clipJson["end"] = m_end;
    clipJson["type"] = m_type;
    clipJson["Id"]=static_cast<qint64>(m_id);
    clipJson["note"] = m_noteName;
    if (m_color.isValid())
        clipJson["color"] = m_color.name(QColor::HexRgb);
    return clipJson;
}
    /**
     * 加载
     * @param const QJsonObject& json 数据
     */
    /**
     * @brief 从 JSON 加载片段模型并通知界面刷新
     * @details 读取 start/end/type/Id 后，主动发出 timelinePositionChanged 与 lengthChanged，
     *          以便 ClipTimePanel 等已连接的控件能显示真实数据（避免默认 start+100）。
     */
    virtual void load(const QJsonObject& json) {
        m_start = json["start"].toInt();
        m_end   = json["end"].toInt();
        m_type  = json["type"].toString();
        m_id    = json["Id"].toInt();
        m_noteName = json["note"].toString();
        if (json.contains("color")) {
            const QColor loaded(json["color"].toString());
            if (loaded.isValid())
                m_color = loaded;
        }

        // 发出信号以刷新 UI 显示
        emit timelinePositionChanged(m_start);
        emit lengthChanged(m_end);
        emit appearanceChanged();
    }
    /**
     * 获取数据
     * @param int role 角色
     * @return QVariant 数据
     */
virtual QVariant data(int role) const {
    switch (role) {
        case TimelineRoles::ClipModelRole:
            return QVariant::fromValue<TimeLineClipModel*>(const_cast<TimeLineClipModel*>(this));
        case TimelineRoles::TrackNameRole:
            return QVariant::fromValue<QString>(m_type);
        case TimelineRoles::ClipNoteRole:
            return m_noteName;
        case TimelineRoles::ClipColorRole:
            return color();
        default:
            return QVariant();
    }
}

    /**
     * 获取当前视频数据
     * @param int currentFrame 当前帧
     * @return QVariantMap 当前视频数据
     */
virtual QVariantMap currentData(int currentFrame) const {

    return QVariantMap();
}
    /**
     * 获取当前控制数据
     * @param int currentFrame 当前帧
     * @return QVariantMap 当前控制数据
     */

virtual QWidget* clipPropertyWidget(){return nullptr;};
    /**
     * 生成标准属性窗口，其中包含自定义的属性
     * @return QWidget* 属性窗口
     */
// 实现位于 TimelineEngine.cpp（保持与 initPropertyWidget 一致的非内联导出方式）
void showPropertyWidget();
    /**
     * 绘制片段
     * @param QPainter* painter 画笔
     * @param const QRect& rect 绘制区域
     * @param bool selected 是否被选中
     */
// TimeLineClipModel::initPropertyWidget（原 AbstractClipModel::initPropertyWidget）
// 函数说明：初始化属性面板，此处将“时间属性”界面独立为 ClipTimePanel，并保持原有功能与信号连接。
// 实现说明：本函数创建 ClipTimePanel / ClipAppearancePanel 等 Q_OBJECT 窗口，
// 为避免宿主插件在内联展开时引用面板的 staticMetaObject 数据符号（数据符号无法
// 从导入库解析），实现已移至 TimelineEngine.cpp，此处仅保留声明。
virtual void initPropertyWidget();

virtual void paint(QPainter* painter, const QRect& rect, bool selected) const {

    // 绘制背景

    drawBackground(painter, rect, selected);

    // 绘制标题

    drawTitle(painter, rect);
    

}
/**
 * 注册控件OSC地址和Widget指针
 */
virtual void registerExternalControl(const QString& oscAddress, QWidget* control)
{
    // 如果oscAddress不以"/"开头，则不注册
    if (!oscAddress.startsWith("/")) return;
    // 构建完整的OSC地址，自动给OSC地址添加前缀，包括节点ID
    if (!control) return;
    // 如果已存在相同地址的映射，先移除旧的
    auto it = _OscMapping.find(oscAddress);
    if (it != _OscMapping.end()) {
        _OscMapping.erase(it);
    }

    // 添加新的映射
    control->installEventFilter(this);
    control->setMouseTracking(true);
    _OscMapping[oscAddress] = control;
}
/**
 * 注销控件OSC地址和Widget指针
 */
virtual void unregisterExternalControl(const QString& oscAddress)
{
    if (!oscAddress.startsWith("/")) return;
    auto it = _OscMapping.find(oscAddress);
    if (it != _OscMapping.end()) {
        _OscMapping.erase(it);
    }
}
/**
 * 获取控件OSC地址和Widget指针
 */
virtual QWidget* getWidgetFromAddress(const QString& oscAddress) const
{
    auto it = _OscMapping.find(oscAddress);
    return it != _OscMapping.end() ? it->second : nullptr;
}
/**
 * 获取OSC地址和控件的映射
 */
virtual std::unordered_map<QString, QWidget*> getExternalControlAddressMapping() const
{
    return _OscMapping;
}

virtual void stateFeedBack(const QString& oscAddress, QVariant value){
    Q_UNUSED(oscAddress);
    Q_UNUSED(value);
}
Q_SIGNALS:
    /**
     * 数据变化信号
     */
    void lengthChanged(qint64 length);  // 添加长度变化信号
    void filePathChanged(const QString& filePath);  // 添加文件路径变化信号
    void sizeChanged(QSize size);   //添加尺寸变化信号
    void posChanged(QPoint position); //位置变化信号
    void rotateChanged(int rotete); //旋转变化信号
    void timelinePositionChanged(int frame); //时间轴上位置变化信号，即开始时间
    void videoDataUpdate() const; //视频数据更新
    void audioDataUpdate() const; //音频数据更新
    void controlDataUpdate() const; //控制数据更新
    void appearanceChanged(); //颜色或备注名变化

protected:
    // 开始
    int m_start;
    // 结束
    int m_end;
    // 类型
    QString m_type;
    // 轨道索引
    // int m_trackIndex;
    // 是否可调整大小
    bool RESIZEABLE;
    // 是否显示小部件
    bool EMBEDWIDGET;
    // 是否显示边框
    bool SHOWBORDER;
    //控件OSC地址映射
    std::unordered_map<QString, QWidget*> _OscMapping;
    // 片段ID
    ClipId m_id;
    QString m_noteName;
    QColor m_color;

    QAbstractItemModel* m_timelineModel {nullptr};

    // 拖拽起始位置
    QPoint dragStartPosition;
    // 是否正在拖拽
    bool isDragging = false;
    // 布局
    QVBoxLayout* m_layout;
    // //开始帧

    // 代理窗口
    QWidget* m_clipPropertyWidget;
    //时间码类型
    TimeCodeType m_timeCodeType;
    ClipTimePanel *timeGroupBox;
    // 函数说明：把 OSC 控件拖拽数据打包为 mime 并启动拖拽（Ctrl+左键拖动控件时触发）。
    // 实现说明：原为头文件内联实现；因 qobject_cast<IntDragValueWidget*> 需要控件完整
    // 类型（IntDragValueWidget 属宿主 GUIElements 库），公共头不得引入，故移入库内实现，
    // 避免 <IntDragValueWidget.hpp> 经本头进入消费方（同 ClipTimePanel/ClipAppearancePanel 的处理）。
    void startDrag(QWidget* widget);
    
    bool eventFilter(QObject* watched, QEvent* event) override
{
    // 检查watched是否是_OscMapping中的控件
    auto it = std::find_if(_OscMapping.begin(), _OscMapping.end(),
        [watched](const auto& pair) { return pair.second == watched; });
    
    if (it != _OscMapping.end()) {

        QWidget* widget = it->second;
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton && (mouseEvent->modifiers() & Qt::ControlModifier)) {
                    dragStartPosition = mouseEvent->pos();
                    isDragging = true;
                   
                }
                break;
            }
            case QEvent::MouseMove: {
                 
                if (!isDragging) break;
               
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if ((mouseEvent->pos() - dragStartPosition).manhattanLength() 
                    >= QApplication::startDragDistance()) {
                    
                    startDrag(widget);
                    isDragging = false;
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                isDragging = false;
                break;
            }
            default:
                break;
        }
    }
    return false;
}
    /**

     * 绘制背景

     */

virtual void drawBackground(QPainter* painter, const QRect& rect, bool selected) const {

    painter->save();
    // 设置背景颜色
    const QColor fill = color();
    QColor bgColor = selected ? fill.darker(150) : fill;

    painter->setBrush(bgColor);
    if(SHOWBORDER){
        painter->setPen(QPen(ClipBorderColour, ClipBorderWidth));
    }else{
        painter->setPen(QPen(bgColor, 0));
    }
    if (clipround <= 0)
        painter->drawRect(rect);
    else
        painter->drawRoundedRect(rect, clipround, clipround);

    painter->restore();

}

    /**

     * 绘制标题

     */

virtual void drawTitle(QPainter* painter, const QRect& rect) const {

    painter->save();
    // 设置文字颜色和字体
    const QColor fill = color();
    painter->setPen(fill.lightness() > 160 ? QColor(30, 30, 30) : Qt::white);

    QFont font = painter->font();

    font.setPointSize(8);

    painter->setFont(font);
    const QString text = painter->fontMetrics().elidedText(displayText(), Qt::ElideRight, qMax(0, rect.width() - 8));
    painter->drawText(rect, Qt::AlignCenter, text);
    painter->restore();

}

};

Q_DECLARE_METATYPE(TimeLineClipModel*)

// ---------- AbstractClipInterface.h ----------
#include <QtPlugin>

class ClipPlugInterface
{
public:
    /**
     * 析构函数
     */
    virtual ~ClipPlugInterface() {}
    /**
     * 获取类型
     * @return QString 类型
     */
    virtual QString clipType() const = 0;
    /**
     * 创建模型
     * @param int start 开始
     * @return TimeLineClipModel* 模型
     */
    virtual TimeLineClipModel* createModel(int start) = 0;
    /**
     * 创建代理
     * @return AbstractClipDelegate* 代理
     */
    // virtual AbstractClipDelegate* createDelegate() = 0;
};

#define ClipPlugInterface_iid "com.timeline.ClipPlugInterface"
Q_DECLARE_INTERFACE(ClipPlugInterface, ClipPlugInterface_iid)


// ---------- NodeClipDelegateModel.h ----------
//
// Created by WuBin on 2026/11/3.
//
// NodeClipDelegateModel —— TimeLineNode 私有版剪辑代理基类。
//
// 说明：【2026-11-13 两套片段基类已各自归位、名字不再撞车】
//   * 编辑器时间轴体系：AbstractClipModel（src/Common/BaseClass.dll 导出，
//     随 SceneClipModel/ClipPlugInterface 供 Flow 主 timeline 用）；
//   * 本节点引擎体系：TimeLineClipModel（本头文件内置静态副本，原同名
//     AbstractClipModel 改名以消除混淆），剪辑必须继承本目录的
//     NodeClipDelegateModel → TimeLineClipModel 链，绝不可混入
//     BaseClass.dll 的 AbstractClipModel——两条继承链 vtable 不同，
//     同一 clip 对象跨链使用必崩。
// 本类完整复刻主工程 AbstractDelegateModel 的对外行为：
//   registerExternalControl / makeFullOscAddress / stateFeedBack /
//   onModelReady / afterModelReady，但基类换成静态副本。
//


#include <QComboBox>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVariant>
#include <unordered_map>

#include "IntDragValueWidget/IntDragValueWidget.hpp"
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

class NodeClipDelegateModel : public TimeLineClipModel {
    Q_OBJECT
public:
    explicit NodeClipDelegateModel(int start, const QString& filePath = QString(), QObject* parent = nullptr)
        : TimeLineClipModel(start, filePath, parent) {}

    ~NodeClipDelegateModel() override = default;

    void setId(ClipId id) override {
        // 函数级注释：设置剪辑 ID 后触发一次统一的控件注册与状态推送
        TimeLineClipModel::setId(id);
        onModelReady();
    }

    /**
     * 函数级注释：注册剪辑属性控件到本地映射（相对 OSC 地址）
     * - 不直接与 StatusContainer/事件总线交互，等待 ID 就绪后由 onModelReady 统一处理
     */
    void registerExternalControl(const QString& oscAddress, QWidget* control) override
    {
        if (!oscAddress.startsWith("/")) return;
        if (!control) return;
        auto it = _OscMapping.find(oscAddress);
        if (it != _OscMapping.end()) {
            _OscMapping.erase(it);
        }
        control->installEventFilter(this);
        control->setMouseTracking(true);
        _OscMapping[oscAddress] = control;
    }

    /**
     * 函数级注释：根据剪辑 ID 构造完整的 OSC 地址（/<modelAlias>/<id><relative>）
     */
    QString makeFullOscAddress(const QString& oscAddress) const {
        const QString norm = oscAddress.startsWith("/") ? oscAddress : ("/" + oscAddress);
        const QString modelAlias = m_timelineModel
           ? m_timelineModel->data(QModelIndex(), TimelineRoles::TimelineModelAliasRole).toString()
           : QStringLiteral("default");
        return  modelAlias + "/" + QString::number(m_id)  + norm;
    }

    /**
     * 函数级注释：通过事件总线发送状态反馈事件，避免直接操作 StatusContainer
     * @param oscAddress 相对地址（形如 "/gain"），内部自动补全为完整地址
     */
    void stateFeedBack(const QString& oscAddress, QVariant value) override {
        GlobalEventBus::instance()->publishState(makeFullOscAddress(oscAddress), value);
    }

    /**
     * 函数级注释：剪辑 ID 与别名就绪后统一注册控件并推送初始状态
     * - 为每个控件在 StatusContainer 中注册完整地址
     * - 根据控件类型将当前值以 StateFeedback 事件形式推送到事件总线
     * - 调用 afterModelReady 供派生类执行额外初始化（例如事件总线订阅命令）
     */
    void onModelReady()
    {
        for (auto it = _OscMapping.begin(); it != _OscMapping.end(); ++it) {
            const QString& rel = it->first;
            QWidget* control = it->second;
            if (!control) continue;
            const QString full = makeFullOscAddress(rel);
            StatusContainer::instance()->registerWidget(control, full);

            if (auto* button = qobject_cast<QAbstractButton*>(control)) {
                if (button->isCheckable()){
                    this->stateFeedBack(rel, QVariant(button->isChecked()));
                }
            } else if (auto* slider = qobject_cast<QAbstractSlider*>(control)) {
                this->stateFeedBack(rel, QVariant(slider->value()));
            } else if (auto* spinBox = qobject_cast<QSpinBox*>(control)) {
                this->stateFeedBack(rel, QVariant(spinBox->value()));
            } else if (auto* dragInt = qobject_cast<IntDragValueWidget*>(control)) {
                this->stateFeedBack(rel, QVariant(dragInt->value()));
            } else if (auto* lineEdit = qobject_cast<QLineEdit*>(control)) {
                this->stateFeedBack(rel, QVariant(lineEdit->text()));
            } else if (auto* comboBox = qobject_cast<QComboBox*>(control)) {
                this->stateFeedBack(rel, QVariant(comboBox->currentIndex()));
            } else if (auto* checkBox = qobject_cast<QCheckBox*>(control)) {
                this->stateFeedBack(rel, QVariant(checkBox->isChecked()));
            }
        }
        afterModelReady();
    }

protected:
    /**
     * 函数级注释：剪辑模型初始化完成后的扩展点（ClipId 与别名已就绪）
     * - 派生类可重写，在此阶段基于完整地址执行事件总线订阅等操作
     */
    virtual void afterModelReady() {}

private:
    QObject* _oscConnectionProxy = nullptr;
};

// ---------- BasePluginLoader.h ----------
#include <QObject>
#include <QDir>
#include <QPluginLoader>
#include <QDebug>
#include <QApplication>
class NODE_TIMELINE_PUBLIC BasePluginLoader : public QObject
{
    Q_OBJECT
public:
    explicit BasePluginLoader(QObject *parent = nullptr) ;

    /**
     * 函数级注释：静态注册一个内置剪辑工厂（TimeLineNode 私有副本专用）
     * - 本节点不再从 bin/plugins/TimeLine 动态加载插件（那些插件继承
     *   旧 QtTimeLine 的类，与静态副本不兼容），clip 一律静态注册。
     * @param ClipPlugInterface* plugin 剪辑工厂（所有权由加载器接管）
     */
    virtual void registerPlugin(ClipPlugInterface* plugin);
    /**
     * 加载插件（静态副本：注册内置剪辑，不扫描磁盘目录）
     */
    virtual void loadPlugins() ;
    /**
     * 获取加载的插件列表
     * @return QList<QObject*> 加载的插件列表
     */
    virtual QList<QObject*> getLoadedPlugins() const;
    /**
     * 获取所有插件类型
     * @return QStringList 插件类型列表
     */
    virtual QStringList getAvailableTypes() const;
    /**
     * 获取指定类型的插件
     * @param const QString& type 类型
    */
    virtual ClipPlugInterface* getPluginByType(const QString& type) const;
    /**
     * 创建指定类型的模型
     * @param const QString& type 类型
     * @param int start 开始
    */      
    virtual TimeLineClipModel* createModelForType(const QString& type, int start) const ;
    /**
     * 创建指定类型的代理
     * @param const QString& type 类型
    */
    // AbstractClipDelegate* createDelegateForType(const QString& type) const;

signals:
    /**
     * 插件加载完成信号
     * @param QObject* plugin 插件
    */
    void pluginLoaded(QObject* plugin);
    /**
     * 所有插件加载完成信号
     */
    void pluginsLoaded();

private:
    // 加载的插件列表
    QList<QObject*> m_loadedPlugins;
    // 插件类型列表
    QMap<QString, ClipPlugInterface*> m_pluginsByType;
};


// ---------- BaseTimeLineModel.h ----------
#include <QAbstractItemModel>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <QMimeData>
#include <QIODevice>
#include <algorithm>
#include <QList>
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
using namespace QtTimeline;
struct TrackData {
    QString type;
    QString name;
    // 修改前: QVector<std::unique_ptr<TimeLineClipModel>> clips;
    // 修改后: 使用 QList 替代 QVector
    QList<TimeLineClipModel*> clips;
    // 按 start 排序的片段下标。只用于视口裁剪，不改变 clips 的绘制顺序。
    QList<int> clipsByStart;
    int maxClipSpan = 0;
    QJsonObject save() const
{
    QJsonObject trackJson;
    trackJson["type"] = type;
    trackJson["trackName"] = name;
    return trackJson;
}
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
    virtual QVariant clipData(ClipId clipID, TimelineRoles role) const ;

    virtual bool setClipData(ClipId clipID, TimelineRoles role, QVariant value);

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
     * 保存模型
     * @return QJsonObject 保存的模型
     */
    virtual QJsonObject save() const;
    /**
     * 加载模型
     * @param const QJsonObject &modelJson 加载的模型
     */
    virtual void load(const QJsonObject &modelJson) ;
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

    QString getModelAlias() const { return m_modelAlias; }
    // 设置模型别名
    void setModelAlias(const QString& alias) {
        m_modelAlias = alias.startsWith("/") ? alias : ("/" + alias);
    }

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
    void S_playheadMoved(int frame);

    /**
     * @brief 时间线总长度变化信号
     * @details
     * 总长度是聚合值（由所有 clip 的 end 取 max 计算），不对应某一行的直接字段。
     */
    void S_LengthChanged(qint64 length);
public slots:
    //开始播放槽函数
    virtual void onStartPlay();
    //暂停播放槽函数
    virtual void onPausePlay();
    //停止播放槽函数
    virtual void onStopPlay();
    //时间轴长度变化槽函数
    virtual qint64 onUpdateTimeLineLength();
    //设置播放头位置槽函数
    virtual void onSetPlayheadPos(int newPlayheadPos);
    //通过轨道索引和开始帧添加片段
    virtual void onAddClip(int trackIndex,int startFrame);
    //删除片段槽函数
    virtual void onDeleteClip(QModelIndex clipIndex);
    //通过类型添加轨道槽函数
    virtual void onAddTrack(const QString& type);
    //删除轨道槽函数
    virtual void onDeleteTrack(int trackIndex);
    //移动轨道槽函数
    virtual void onMoveTrack(int sourceRow, int targetRow);

    // 拖拽期间只改片段几何，松手后再排序、重算长度并发出 dataChanged
    void beginInteractiveGeometryChange(TimeLineClipModel* clip);
    void endInteractiveGeometryChange();
    void syncInteractiveClipOrder();

protected:
    /**
     * @brief 通过 clip 指针反查其在模型中的索引
     * @details
     * ClipTimePanel 会直接调用 TimeLineClipModel::setStart/setEnd 等修改片段。
     * 为了让 View 能走 Qt 标准路径刷新，这里提供从 clip 指针构建 QModelIndex 的能力，
     * 以便在监听到 clip 的信号后发出 dataChanged。
     */
    QModelIndex indexForClip(TimeLineClipModel* clip) const;

    struct ClipLocation {
        int trackRow = -1;
        int clipRow = -1;
        int end = 0;
    };

    // 增删轨道或片段后重建，供 parent/data/indexForClip 做 O(1) 查询
    void rebuildIndex();
    void rebuildTrackClipOrder(TrackData* track);
    void noteClipGeometryChanged(TimeLineClipModel* clip);
    void watchClip(TimeLineClipModel* clip);
    void queueClipGeometryNotify(TimeLineClipModel* clip);
    void flushClipGeometryNotify();
    // 新终点超过当前总长时直接更新。只有原来的最大终点变小才重扫。
    void applyLengthChange(int oldEnd, int newEnd);
    bool isTrackPointer(void* ptr) const;

    // 查找片段所在轨道
    TrackData* findParentTrackOfClip(TimeLineClipModel* clip) const ;
    // 查找轨道行
    int findTrackRow(TrackData* track) const ;

    // 时间显示格式，默认显示时间码
    TimedisplayFormat m_timeDisplayFormat = TimedisplayFormat::AbsoluteTimeFormat;
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
    std::unordered_map<TimeLineClipModel*, ClipLocation> m_clipLocation;
    std::unordered_set<TrackData*> m_trackSet;
    bool m_deferGeometryNotify = false;
    TimeLineClipModel* m_interactiveClip = nullptr;
    int m_interactiveOriginalEnd = 0;
    bool m_geometryNotifyQueued = false;
    QList<TimeLineClipModel*> m_pendingGeometryClips;

    qint64 m_currentFrame = 0; // 当前帧

    qint64 m_lengthFrame = 0; // 总帧数

    ClipId m_clipNextId=0;

    QString m_modelAlias = "default123";

    TimeCodeType m_timeCodeType=TimeCodeType::PAL;
};


// ---------- BaseTimeLineView.h ----------
#include <QAbstractItemView>
#include <unordered_map>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QFileInfo>
#include <QMimeData>
#include "QMenu"
// TimelineView类继承自QAbstractItemView
class BaseTimelineToolbar;
class NODE_TIMELINE_PUBLIC BaseTimelineView : public QAbstractItemView
{
Q_OBJECT
    // 允许通过 QSS 设置颜色：qproperty-<name>: <color>
    Q_PROPERTY(QColor timelineBgColor READ timelineBgColor WRITE setTimelineBgColor)
    Q_PROPERTY(QColor timelineSeparatorColor READ timelineSeparatorColor WRITE setTimelineSeparatorColor)
    Q_PROPERTY(QColor timelineRulerColor READ timelineRulerColor WRITE setTimelineRulerColor)
    Q_PROPERTY(QColor timelinePlayheadColor READ timelinePlayheadColor WRITE setTimelinePlayheadColor)
    Q_PROPERTY(QColor clipFillColor READ clipFillColor WRITE setClipFillColor)
    Q_PROPERTY(QColor clipBorderColor READ clipBorderColor WRITE setClipBorderColor)
    Q_PROPERTY(QColor trackSelectedColor READ trackSelectedColor WRITE setTrackSelectedColor)
public:
    // 构造函数，初始化视图模型
    explicit BaseTimelineView(BaseTimeLineModel *viewModel, QWidget *parent = nullptr);
    ~BaseTimelineView() override ;
    /**
     * 返回给定索引的可视矩形
     * @param QModelIndex index 索引
     * @return QRect 可视矩形
     */
    virtual QRect visualRect(const QModelIndex &index) const override;
    /**
     * 滚动到指定索引
     * @param QModelIndex index 索引
     * @param ScrollHint hint 滚动提示
     */
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint) override{};

    /**
     * 返回给定点的索引
     * @param QPoint point 点
     * @return QModelIndex 索引
     */
    virtual QModelIndex indexAt(const QPoint &point) const override;
    /**
     * 显示剪辑属性
     * @param QModelIndex index 片段索引
     */
    virtual void showClipProperty(const QModelIndex& index);

    virtual void initToolBar(BaseTimelineToolbar* toolbar);
    // 时间线工具栏
    BaseTimelineToolbar* m_toolbar;
    // 时间线定时器
signals:
    /**
     * 滚动信号
     * @param int dx 水平滚动
     * @param int dy 垂直滚动
     */
    void scrolled(int dx,int dy);
    /**
     * 当前剪辑改变信号
     * @param TimeLineClipModel* clip 当前剪辑
     */
    void currentClipChanged(TimeLineClipModel* clip);
public slots:
    /**
     * 更新可视区域（原方法）
     * 优化：改为调用 scheduleRedraw 合并重绘，避免每次信号都立即执行完整重绘。
     */
    virtual void onUpdateViewport();
    // 播放头移动：缓存不失效，只刷新旧位置和新位置
    virtual void onPlayheadMoved(int frame);
    // 片段几何或外观变化：只重画该片段的旧位置和新位置
    void updateClipGeometry(const QModelIndex& index);
    /**
     * 滚动视图
     * @param int dx 水平滚动
     * @param int dy 垂直滚动
     */
    virtual void onScroll(int dx, int dy);
    /**
     * 水平滚动
     * @param double dx 水平滚动
     */
    virtual void horizontalScroll(double dx);
    /**
     * 更新滚动条
     */
    virtual void updateScrollBars();
    /**
     * 滚动内容
     * @param int dx 水平滚动
     * @param int dy 垂直滚动
     */
    virtual void scrollContentsBy(int dx, int dy) override;
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) override;
    /**
     * 设置缩放
     * @param double value 缩放值
     */
    virtual void setScale(double value);

    /**
     * 在指定位置添加剪辑
     * @param QModelIndex index 索引
     * @param QPoint pos 位置
     */
    virtual void addClipAtPosition(const QModelIndex& index, const QPoint& pos);


protected:
    /**
     * 移动光标
     * @param CursorAction cursorAction 光标动作
     * @param Qt::KeyboardModifiers modifiers 键盘修饰符
     * @return QModelIndex 索引
     */
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override {return QModelIndex();}

    /**
     * 水平偏移
     * @return int 水平偏移
     */
    virtual int horizontalOffset() const override{return 0;}

    /**
     * 垂直偏移
     * @return int 垂直偏移
     */
    virtual int verticalOffset() const override{return 0;}

    /**
     * 是否隐藏索引
     * @param QModelIndex index 索引
     * @return bool 是否隐藏
     */
    virtual bool isIndexHidden(const QModelIndex &index) const override{ return false; }

    /**
     * 设置选择
     * @param QRect rect 矩形
     * @param QItemSelectionModel::SelectionFlags command 命令
     */
    virtual void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override{}

    /**
     * 设置选择区域
     * @param QItemSelection selection 选择区域
     * @return QRegion 区域
     */
    virtual QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}

    /**
     * 绘制事件
     * @param QPaintEvent *event 绘制事件
     */
    virtual void paintEvent(QPaintEvent *event) override;

    /**
     * 鼠标按下事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mousePressEvent(QMouseEvent *event) override;

    /**
     * 鼠标移动事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * 鼠标释放事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * 鼠标双击事件
     * @param QMouseEvent *event 鼠标事件
     */
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    /**
     * 鼠标离开事件
     * @param QEvent *event 事件
     */
    virtual void leaveEvent(QEvent *event) override;

    // bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * 上下文菜单事件
     * @param QContextMenuEvent* event 上下文菜单事件
     */
    virtual void contextMenuEvent(QContextMenuEvent* event) override;
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    /**
     * 调整大小事件
     * @param QResizeEvent *event 调整大小事件
     */
    virtual void resizeEvent(QResizeEvent *event) override;

    /**
     * 显示事件
     * @param QShowEvent *event 显示事件
     */
    virtual void showEvent(QShowEvent *event) override;

    /**
     * 拖入事件
     * @param QDragEnterEvent *event 拖入事件
     */
    virtual void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * 拖动移动事件
     * @param QDragMoveEvent *event 拖动移动事件
     */
    virtual void dragMoveEvent(QDragMoveEvent *event) override;

    /**
     * 拖动释放事件
     * @param QDropEvent *event 拖动释放事件
     */
    virtual void dropEvent(QDropEvent *event) override;

    /**
     * 滚轮事件
     * @param QWheelEvent *event 滚轮事件
     */
    virtual void wheelEvent(QWheelEvent *event) override;
    /**
     * 获取时间线背景色
     * @return QColor 背景色
     */
    QColor timelineBgColor() const { return m_timelineBgColor; }
    /**
     * 设置时间线背景色
     * @param QColor color 背景色
     */
    void setTimelineBgColor(const QColor& color);
    /**
     * 获取时间线分隔线颜色
     * @return QColor 分隔线颜色
     */
    QColor timelineSeparatorColor() const { return m_timelineSeparatorColor; }
    /**
     * 设置时间线分隔线颜色
     * @param QColor color 分隔线颜色
     */
    void setTimelineSeparatorColor(const QColor& color);
    /**
     * 获取标尺颜色
     * @return QColor 标尺颜色
     */
    QColor timelineRulerColor() const { return m_timelineRulerColor; }
    /**
     * 设置标尺颜色
     * @param QColor color 标尺颜色
     */
    void setTimelineRulerColor(const QColor& color);
    /**
     * 获取播放头颜色
     * @return QColor 播放头颜色
     */
    QColor timelinePlayheadColor() const { return m_timelinePlayheadColor; }
    /**
     * 设置播放头颜色
     * @param QColor color 播放头颜色
     */
    void setTimelinePlayheadColor(const QColor& color);
    /**
     * 获取片段填充颜色
     * @return QColor 片段填充颜色
     */
    QColor clipFillColor() const { return m_clipFillColor; }
    /**
     * 设置片段填充颜色
     * @param QColor color 片段填充颜色
     */
    void setClipFillColor(const QColor& color);
    /**
     * 获取片段边框颜色
     * @return QColor 片段边框颜色
     */
    QColor clipBorderColor() const { return m_clipBorderColor; }
    /**
     * 设置片段边框颜色
     * @param QColor color 片段边框颜色
     */
    void setClipBorderColor(const QColor& color);
    /**
     * 获取轨道选中颜色
     * @return QColor 轨道选中颜色
     */
    QColor trackSelectedColor() const { return m_trackSelectedColor; }
    /**
     * 设置轨道选中颜色
     * @param QColor color 轨道选中颜色
     */
    void setTrackSelectedColor(const QColor& color);
    /**
     * 绘制垂直时间线
     * @param QPainter* painter 绘图设备
     * @param QRect& rect 矩形
     */
    virtual void drawVerticalTimeLines(QPainter* painter, const QRect& rect);
    /**
     * 绘制背景
     * @param QPainter* painter 绘图设备
     * @param QRect& rect 矩形
     */
    virtual void drawBackground(QPainter* painter, const QRect& rect);
    /**
     * 绘制播放头
     * @param QPainter* painter 绘图设备
     */
    virtual void drawPlayhead(QPainter* painter);
    /**
     * 绘制时间刻度
     * @param QPainter* painter 绘图设备
     * @param QRect& rect 矩形
     */
    virtual void drawTimeRuler(QPainter* painter, const QRect& rect);
    /**
     * 绘制轨道
     * @param QPainter* painter 绘图设备
     */
    virtual void drawTracks(QPainter* painter);
    /**
     * 绘制时间标记
     * @param QPainter* painter 绘图设备
     * @param int startMarker 开始标记
     * @param int endMarker 结束标记
     * @param int frameStep 帧步长
     */
    virtual void drawTimeMarkers(QPainter* painter, int startMarker, int endMarker, int frameStep);
    /**
     * 计算帧步长
     * @param double frameRate 帧率
     * @return int 帧步长
     */
    virtual int calculateFrameStep(double frameRate) const;

    virtual BaseTimeLineModel* getModel() const;

    /**
     * 获取轨道宽度
     * @return int 轨道宽度
     */
    virtual int getTrackWdith() const;
    /**
     * 移动选定的剪辑
     * @param int dx 水平移动
     * @param int dy 垂直移动
     * @param bool isMouse 是否鼠标移动
     */
    virtual void moveSelectedClip(int dx, int dy,bool isMouse = true);
    /**
     * 移动播放头到指定帧
     * @param int frame 帧
     */
    virtual void movePlayheadToFrame(int frame);
    // 更新帧位置
    virtual void onFrameChanged(qint64 frame);
    // 播放状态改变
    virtual void onPlaybackStateChanged(bool isPlaying);
    // 判断文件对应的track类型

    virtual QString isMimeAcceptable(const QMimeData *Mime) const;



    /**
     * 处理鼠标拖动操作
     * @param event 鼠标事件对象
     */
    virtual void handleMouseDrag(QMouseEvent *event);
    void beginClipDrag(TimeLineClipModel* clip, const QModelIndex& index);
    void finishClipDrag();

    /**
     * 更新鼠标悬停状态
     * @param event 鼠标事件对象
     */
    virtual void updateMouseHoverState(QMouseEvent *event);

    /**
     * 根据鼠标悬停状态更新鼠标指针形状
     */
    virtual void updateCursorShape();



protected slots:
    /**
     * 选择更改
     * @param QItemSelection selected 选择
     * @param QItemSelection deselected 取消选择
     */
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

    /**
     * 获取索引的代理
     * @param QModelIndex index 索引
     * @return QAbstractItemDelegate* 代理
     */
    // QAbstractItemDelegate* itemDelegateForIndex(const QModelIndex &index) const override ;

protected:
    // 当前缩放
    /**
     * 每个视图实例自己的时间刻度参数（避免跨窗口相互影响）
     * - m_timescale：当前像素/帧，仅用于本视图实例
     * - m_minTimescale：本视图允许的最小像素/帧，用于缩放下限
     * - m_baseTimeScale：本视图允许的最大像素/帧，用于缩放上限
     * 说明：这些成员在构造时从 TimeLineStyle.h 的默认值初始化，但后续不再写回全局变量
     */
    double m_timescale     = timescale;
    double m_minTimescale  = minTimescale;
    int    m_baseTimeScale = baseTimeScale;

    /**
     * 当前缩放归一化比例（0~1）：与本实例的 m_timescale 保持一致
     * 公式：ratio = (m_timescale - m_minTimescale) / (m_baseTimeScale - m_minTimescale)
     * 这样首次滚轮缩放不会跳变
     */
    double currentScale = (m_timescale - m_minTimescale) / (static_cast<double>(m_baseTimeScale) - m_minTimescale);
    // 时间线模型
    BaseTimeLineModel *Model;
    /**
     * 获取索引的矩形
     * @param QModelIndex index 索引
     * @return QRect 矩形
     */
    QRect itemRect(const QModelIndex &index) const;

    /**
     * @brief 内容区顶部偏移（统一几何口径）
     * @details
     * TrackListView 与 TimelineView 的坐标体系必须一致，否则会出现“同一行不对齐/滚动不同步”。
     * 这里将内容区顶部定义为：标尺高度 + 工具栏高度。
     */
    int contentTop() const { return rulerHeight + toolbarHeight; }

    // 当前视口里能看见的轨道行，闭区间。没有可见行时 lastRow < firstRow。
    void visibleTrackRange(int& firstRow, int& lastRow) const;

    /**
     * @brief 当前滚动偏移（与滚动条保持一致）
     * @details
     * 历史代码在多个位置直接读写 m_scrollOffset，并与 QAbstractItemView 内部滚动条机制并存，
     * 容易产生“滚动条值”和“m_scrollOffset 值”不一致，进而导致绘制/命中测试/拖拽计算错乱。
     * 重构策略：滚动条为唯一真值来源，m_scrollOffset 仅作为缓存，必须通过同步函数更新。
     */
    QPoint m_scrollOffset;

    /**
     * @brief 从滚动条同步 m_scrollOffset（滚动条为真值来源）
     * @details
     * - 当 scrollBar valueChanged 触发 QAbstractItemView::scrollContentsBy 后，应调用此函数更新缓存。
     * - 所有使用 m_scrollOffset 参与绘制/计算的代码，都依赖此同步保证正确性。
     */
    void syncScrollOffsetFromScrollBars();

    /**
     * @brief 设置滚动偏移并同步滚动条（自动夹紧到合法范围）
     * @param offset 目标偏移（x=水平，y=垂直）
     * @details
     * - 禁止直接写 m_scrollOffset（会导致滚动条不动、视图内部状态不一致）。
     * - 统一通过此函数写入：先夹紧，再写滚动条，再同步缓存。
     */
    void setScrollOffsetClamped(const QPoint& offset);

    /**
     * 点转换为帧
     * @param int point 点
     * @return int 帧
     */
    int pointToFrame(int point) const;
    /**
     * 帧转换为点
     * @param int frame 帧
     * @return int 点
     */
    int frameToPoint(int frame) const;    
    // 鼠标开始
    QPoint m_mouseStart;
    // 鼠标结束
    QPoint m_mouseEnd;
    // 鼠标偏移
    QPoint m_mouseOffset;
    // 鼠标是否按下
    bool mouseHeld = false;
    bool m_interactiveDrag = false;
    TimeLineClipModel* m_dragClip = nullptr;
    QPersistentModelIndex m_dragIndex;
    // 播放头是否选中
    bool m_playheadSelected = false;
    // 是否正在拖放媒体
    bool m_isSupportMedia = false;
    // 鼠标最后拖动位置
    QPoint m_lastDragPos;
    // 鼠标悬停状态
    hoverState m_mouseUnderClipEdge = NONE;
    // 鼠标悬停索引
    QModelIndex m_hoverIndex = QModelIndex();

    // ========== 新增：重绘合并机制 ==========
    /**
     * 合并重绘请求（去抖）
     * - 将多次重绘请求在 delayMs 毫秒内合并为一次，默认 16ms（约一帧）
     * - 降低 updateEditorGeometries/updateScrollBars/viewport()->update 的调用频率
     */
    void scheduleRedraw(int delayMs = 16);

    /**
     * 统一执行一次重绘（实际操作）
     * - 防重入：使用 m_isRedrawing，避免重入引发递归或重复开销
     * - 内容变化时更新滚动条并整页刷新。播放头移动不走这里。
     */
    void performRedraw();
    void ensureStaticCache();
    bool staticCacheReady() const;
    // 滚动时平移已有缓存，只重画新露出来的条带。平移失败时返回 false，调用方整页重建。
    bool shiftStaticCache(int dx, int dy);
    void repaintStaticCache(const QRegion& region);
    QRect clipContentBounds(TimeLineClipModel* clip, int trackRow) const;
    QRect playheadDirtyRect(int frame) const;

    // 合并重绘用的定时器（单次触发）
    QTimer* m_redrawCoalesceTimer {nullptr};

    // 防重入标志：防止 performRedraw 期间再次触发 performRedraw
    bool m_isRedrawing {false};

    // 不含播放头的视口缓存。播放头移动时贴上这张图，再画播放头。
    QPixmap m_staticCache;
    bool m_staticCacheDirty = true;
    int m_paintedPlayheadFrame = -1;
    // 片段在内容坐标中的上次绘制范围，供局部刷新擦掉旧位置。
    std::unordered_map<TimeLineClipModel*, QRect> m_clipContentRects;
    // ========== 新增结束 ==========
    /**
     * 画布平移状态
     * - m_isPanning: 是否处于拖动画布模式
     * - m_panStart: 鼠标按下的起始点（viewport 坐标）
     * - m_panInitialOffset: 拖拽开始时的滚动偏移
     * - m_panMoved: 是否在拖拽中产生了足够位移（用于区分点击与拖拽）
     */
    bool m_isPanning = false;
    QPoint m_panStart;
    QPoint m_panInitialOffset;
    /**
     * 处理样式变化事件（QSS/调色板变更）
     * @param QEvent* e 事件
     */
    virtual void changeEvent(QEvent* e) override;
    /**
     * 从属性应用颜色到全局样式（兼容旧绘制逻辑）
     */
    void applyStyleFromProperties();
    // 基于 TimeLineStyle.h 的默认值初始化，可被 QSS 属性覆盖
    QColor m_timelineBgColor       = bgColour;
    QColor m_timelineSeparatorColor= seperatorColour;
    QColor m_timelineRulerColor    = rulerColour;
    QColor m_timelinePlayheadColor = playheadColour;
    QColor m_clipFillColor         = ClipColor;
    QColor m_clipBorderColor       = ClipBorderColour;
    QColor m_trackSelectedColor    = trackSelectedColour;
};







// ---------- BaseTrackDelegate.h ----------
#include <QAbstractItemDelegate>
#include <QObject>

#include<QPainter>
#include <QPushButton>
#include <QLayout>
#include <QSpacerItem>
#include "QLabel"
class NODE_TIMELINE_PUBLIC BaseTrackDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    /**
     * 构造函数
     * @param QObject *parent 父窗口
     */
    explicit BaseTrackDelegate(QObject *parent = nullptr);
    /**
     * 绘制
     * @param QPainter *painter 绘图器
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     */
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    /**
     * 大小提示
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     * @return QSize 大小
     */
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    /**
     * 创建编辑器
     * @param QWidget *parent 父窗口
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     * @return QWidget * 编辑器
     */
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    // QAbstractItemDelegate interface
public:
    /**
     * 更新编辑器几何形状
     * @param QWidget *editor 编辑器
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     */
    virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /**
     * 设置编辑器数据
     * @param QWidget *editor 编辑器
     * @param const QModelIndex &index 索引
     */
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override ;

};



// ---------- BaseTrackListView.h ----------
#include <QAbstractItemView>
#include <QMenu>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QDrag>
#include <QApplication>
#include <QPersistentModelIndex>
#include <QVector>
#include <QTimer>
#include <QMessageBox>
class NODE_TIMELINE_PUBLIC BaseTracklistView : public QAbstractItemView
{
    Q_OBJECT
    Q_PROPERTY(QColor trackColor READ getTrackColor WRITE setTrackColor)
    Q_PROPERTY(QColor trackSelectedColor READ getTrackSelectedColor WRITE setTrackSelectedColor)
    Q_PROPERTY(QColor trackHoverColor READ getTrackHoverColor WRITE setTrackHoverColor)
    Q_PROPERTY(QColor trackBgColor READ trackBgColor WRITE setTrackBgColor)
    Q_PROPERTY(QColor trackTitleColor READ getTrackTitleColor WRITE setTrackTitleColor)
    // 标尺区域当前时间文字颜色（与播放头同色系），可由 QSS 配置
    Q_PROPERTY(QColor timelinePlayheadColor READ timelinePlayheadColor WRITE setTimelinePlayheadColor)
public:
    explicit BaseTracklistView(BaseTimeLineModel *viemModel, QWidget *parent = nullptr);

    QColor getTrackColor() const { return m_trackColor; }
    void setTrackColor(const QColor &color);

    QColor getTrackSelectedColor() const { return m_trackSelectedColor; }
    void setTrackSelectedColor(const QColor &color);

    QColor getTrackHoverColor() const { return m_trackHoverColor; }
    void setTrackHoverColor(const QColor &color);
    
    QColor trackBgColor() const { return m_trackBgColor; }
    void setTrackBgColor(const QColor& color){ m_trackBgColor=color;}

    QColor getTrackTitleColor() const { return m_trackTitleColor; }
    void setTrackTitleColor(const QColor &color);

    QColor timelinePlayheadColor() const { return m_timelinePlayheadColor; }
    void setTimelinePlayheadColor(const QColor& color);

    ~BaseTracklistView() override=default;
    /**
     * 更新滚动条
     */
    virtual void updateScrollBars();
    // 只刷新顶部时间文字，不重画轨道行
    void updatePlayheadHeader();
    /**
     * 获取可视区域
     * @param QModelIndex index 索引
     * @return QRect 可视区域
     */
    virtual QRect visualRect(const QModelIndex &index) const override;
    /**
     * 滚动到指定索引
     * @param QModelIndex index 索引
     * @param ScrollHint hint 提示
     */
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint) override{}
    /**
     * 获取指定点的索引
     */
    virtual QModelIndex indexAt(const QPoint &point) const override;

    virtual BaseTimeLineModel * getModel();


public slots:
    /**
     * @brief 更新视图
     */
    virtual void onUpdateViewport();
    
protected:
    /**
     * @brief 确保可视范围内的轨道行存在 persistent editor
     */
    void ensureVisiblePersistentEditors();

    /**
     * 移动光标
     * @param CursorAction cursorAction 光标动作
     * @param Qt::KeyboardModifiers modifiers 键盘修饰符
     * @return QModelIndex 索引
     */
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override { return QModelIndex();}
    /**
     * 水平偏移
     * @return int 水平偏移
     */
    virtual int horizontalOffset() const override {return 0;}
    /**
     * 垂直偏移
     * @return int 垂直偏移
     */
    virtual int verticalOffset() const override {return 0;}
    /**
     * 是否隐藏索引
     * @param QModelIndex index 索引
     * @return bool 是否隐藏
     */ 
    virtual bool isIndexHidden(const QModelIndex &index) const override { return false; }
    /**
     * 设置选择
     * @param QRect rect 矩形
     * @param QItemSelectionModel::SelectionFlags command 命令
     */
    virtual void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override ;
    /**
     * 获取选择区域
     * @param QItemSelection selection 选择
     * @return QRegion 区域
     */
    virtual QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}
    /**
     * 绘制事件
     * @param QPaintEvent *event 绘制事件
     */
    virtual void paintEvent(QPaintEvent *event) override;

    virtual void drawTitle(QPainter *painter);
    /**
     * 鼠标按下事件
     * @param QMouseEvent *event 鼠标事件
     */
    void mousePressEvent(QMouseEvent *event) override;


    /**
     * 鼠标移动事件
     * @param QMouseEvent *event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    /**
     * 离开事件
     * @param QEvent *event 事件
     */
    void leaveEvent(QEvent *event) override;
    /**
     * 调整事件
     * @param QResizeEvent *event 调整事件
     */
    void resizeEvent(QResizeEvent *event) override;
    /**
     * 显示事件
     * @param QShowEvent *event 显示事件
     */
    void showEvent(QShowEvent *event) override;
    /**
     * 样式变化事件（QSS qproperty- 生效后重绘视口）
     * @param QEvent *event 事件
     */
    void changeEvent(QEvent *event) override;
    /**
     * 滚动内容
     * @param int dx 水平滚动
     * @param int dy 垂直滚动
     */
    void scrollContentsBy(int dx, int dy) override;
    /**
     * 获取项目矩形
     * @param QModelIndex index 索引
     * @return QRect 矩形
     */
    virtual QRect itemRect(const QModelIndex &index) const;
    /**
     * 事件过滤
     * @param QObject *watched 观察对象
     * @param QEvent *event 事件
     * @return bool 是否过滤
     */
    bool eventFilter(QObject *watched, QEvent *event) override;
    /**
     * 上下文菜单事件
     * @param QContextMenuEvent* event 上下文菜单事件
     */
    void contextMenuEvent(QContextMenuEvent* event) override ;
protected slots:
    /**
     * 删除轨道
     */
    virtual void onDeleteTrack();
    /**
     * 选择改变
     * @param QItemSelection selected 选择
     * @param QItemSelection deselected 取消选择
     */
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    /**
     * 更新编辑几何
     */
    virtual void updateEditorGeometries() override;
    /**
     * 拖入事件
     * @param QDragEnterEvent *event 拖入事件
     */
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    /**
     * 拖动移动事件
     * @param QDragMoveEvent *event 拖动移动事件
     */
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    /**
     * 拖动释放事件
     * @param QDropEvent *event 拖动释放事件
     */
    virtual void dropEvent(QDropEvent *event) override;

public:
    /**
     * 删除轨道动作
     */
    QAction* m_deleteTrackAction;
protected:
    //拖动开始位置
    QPoint m_dragStartPosition;
    //悬停索引
    QModelIndex m_hoverIndex = QModelIndex();
    //模型
    BaseTimeLineModel *Model;

    // 当前已打开的 persistent editor（用于可视范围内按需开关，避免滚动时只增不减）
    QVector<QPersistentModelIndex> m_openPersistentEditors;

private:
   // 颜色默认值来自 TimeLineStyle.h，运行时由 QSS qproperty- 覆盖
    QColor m_trackColor = trackColour;
    QColor m_trackTitleColor = trackTitleColor;
    QColor m_trackSelectedColor = trackSelectedColour;
    QColor m_trackHoverColor = trackHoverColour;
    QColor m_trackBgColor       = bgColour;
    QColor m_timelinePlayheadColor = playheadColour; // 标尺时间文字颜色
};


// ---------- BaseTimeLineToolBar.h ----------
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QSpinBox>
#include <QLabel>
#include <QMimeData>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QToolButton>
class NODE_TIMELINE_PUBLIC BaseTimelineToolbar : public QToolBar {
    Q_OBJECT
public:
    explicit BaseTimelineToolbar(QWidget* parent = nullptr);
    ~BaseTimelineToolbar() override;

    //控件OSC地址映射
    std::shared_ptr<std::unordered_map<QString, QAction*>> _OscMapping;

    /**
     * 获取OSC地址和控件的映射
     */
     virtual std::shared_ptr<std::unordered_map<QString, QAction*>> getOscMapping() const
    {
        return _OscMapping;
    }

    /**
     * 播放状态变化回调（视图 → 工具栏）。默认空实现，
     * 由需要响应播放状态的工具栏子类按需覆写。
     */
    virtual void setPlaybackState(bool isPlaying) { Q_UNUSED(isPlaying); }

    /**
     * 创建动作
     */
    virtual void createActions()=0;
    /**
     * 设置UI
     */
    virtual void setupUI()=0;
    /**
 * 注册控件OSC地址和Widget指针
 */
    virtual void registerOSCControl(const QString& oscAddress, QAction* control)
    {
        // 如果oscAddress不以"/"开头，则不注册
        if (!oscAddress.startsWith("/")) return;
        // 构建完整的OSC地址，自动给OSC地址添加前缀，包括节点ID
        if (!control) return;
        // 如果已存在相同地址的映射，先移除旧的
        auto it = _OscMapping.get()->find(oscAddress);
        if (it != _OscMapping.get()->end()) {
            _OscMapping.get()->erase(it);
        }

        // 添加新的映射
         QWidget* widget = widgetForAction(control);
         if (widget) {
             widget->installEventFilter(this);
         }
         (*_OscMapping)[oscAddress] = control;
    }
    /**
     * 注销控件OSC地址和Widget指针
     */
    virtual void unregisterOSCControl(const QString& oscAddress)
    {
        if (!oscAddress.startsWith("/")) return;
        auto it = _OscMapping->find(oscAddress);
        if (it != _OscMapping->end()) {
            _OscMapping->erase(it);
        }
    }
    /**
     * 获取控件OSC地址和Widget指针
     */
    virtual QAction* getWidgetFromOSCAddress(const QString& oscAddress) const
    {
        auto it = _OscMapping->find(oscAddress);
        return it !=_OscMapping->end() ? it->second : nullptr;
    }

protected:
    virtual void startDrag(QAction* widget);

    virtual bool eventFilter(QObject* watched, QEvent* event) override;
private:
    // 拖拽起始位置
    QPoint dragStartPosition;
    // 是否正在拖拽
    bool isDragging = false;

};


// ---------- ClipTimePanel.hpp ----------
// ClipTimeGroupBox
// 类说明：封装“时间属性”界面，将原先 TimeLineClipModel::initPropertyWidget 中的 timeGroupBox 独立为 QGroupBox 子类。
// 职责：构建开始/结束/总长的控件，统一显示为绝对时间（毫秒）00:00:00:000；
//       管理信号连接：文本框修改触发 setStart/setEnd，与 spinbox 行为一致；响应模型的时间/长度变化信号同步更新 UI。

#include "IntDragValueWidget/IntDragValueWidget.hpp"
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTimeEdit>
class TimeLineClipModel;
class NODE_TIMELINE_PUBLIC ClipTimePanel : public QGroupBox {
    Q_OBJECT
public:
    // 构造函数：创建并布置 UI，连接信号到 TimeLineClipModel，保持原有功能
    explicit ClipTimePanel(TimeLineClipModel* owner, QWidget* parent = nullptr);
    void initSignalConnections();
public:
    IntDragValueWidget* m_startFrameSpinBox {nullptr};
    QTimeEdit*  m_startTimeCodeLineEdit {nullptr};

    IntDragValueWidget* m_positionFrameSpinBox {nullptr};
    QTimeEdit*  m_positionTimeLineEdit {nullptr};

    IntDragValueWidget* m_endFrameSpinBox {nullptr};
    QTimeEdit*  m_endTimeCodeLineEdit {nullptr};

Q_SIGNALS:
    void startFrameChanged(int value);
    void positionChanged(int value);
    void endFrameChanged(int value);
public Q_SLOTS:
    void updateStartFrame(int frames);
    void updateStartTimeCode(const QTime& tc);
    void updateEndFrame(int frames);
    void updateEndTimeCode(const QTime& tc);
    void updatePositionFrame(int frames);
    void updatePositionTimeCode(const QTime& tc);
private:
    int clampStartFrame(int frames) const;
    int clampEndFrame(int frames) const;
    void refreshFrameRanges();
private:
    TimeLineClipModel* m_owner {nullptr};
    // 本地控件（不强制写回到 TimeLineClipModel 的成员，避免循环依赖）
    QGridLayout* m_timeLayout {nullptr};
    QLabel*      m_startFrameLabel {nullptr};
    QLabel*      m_lengthFrameLabel {nullptr};
    QLabel*      m_endFrameLabel {nullptr};

};

// ---------- ClipAppearancePanel.hpp ----------
#include <QGroupBox>
#include <QColor>
#include <QList>

class TimeLineClipModel;
class QButtonGroup;
class QLineEdit;
class QLabel;

inline QList<QColor> titleColorPresets()
{
    return {
        QColor(255, 140, 0),   // orange
        QColor(0, 170, 255),   // bright blue
        QColor(255, 105, 180), // pink
        QColor(160, 120, 220), // purple
        QColor(0, 180, 170),   // teal
        QColor(220, 50, 60),   // red
        QColor(240, 190, 40),  // yellow
        QColor(100, 180, 70),  // green
    };
}

/**
 * @brief 片段外观面板（颜色/备注名）
 * @details
 * 样式约定：
 * - 色块的动态背景色来自片段颜色数据（titleColorPresets / owner->color()），属于数据而非样式；
 * - 边框等装饰样式通过 Q_PROPERTY 暴露，可被 QSS 覆盖：
 *     ClipAppearancePanel {
 *         qproperty-swatchBorderColor: #666666;
 *         qproperty-swatchCheckedBorderColor: #ffffff;
 *         qproperty-currentColorBorderColor: #888888;
 *     }
 * - 主题切换（QEvent::StyleChange）时自动重建色块样式。
 */
class NODE_TIMELINE_PUBLIC ClipAppearancePanel : public QGroupBox {
    Q_OBJECT
    // 颜色预设按钮边框（未选中）
    Q_PROPERTY(QColor swatchBorderColor READ swatchBorderColor WRITE setSwatchBorderColor)
    // 颜色预设按钮边框（选中态）
    Q_PROPERTY(QColor swatchCheckedBorderColor READ swatchCheckedBorderColor WRITE setSwatchCheckedBorderColor)
    // 当前颜色指示块边框
    Q_PROPERTY(QColor currentColorBorderColor READ currentColorBorderColor WRITE setCurrentColorBorderColor)
public:
    explicit ClipAppearancePanel(TimeLineClipModel* owner, QWidget* parent = nullptr);

    QColor swatchBorderColor() const { return m_swatchBorderColor; }
    void setSwatchBorderColor(const QColor& color) { m_swatchBorderColor = color; }

    QColor swatchCheckedBorderColor() const { return m_swatchCheckedBorderColor; }
    void setSwatchCheckedBorderColor(const QColor& color) { m_swatchCheckedBorderColor = color; }

    QColor currentColorBorderColor() const { return m_currentColorBorderColor; }
    void setCurrentColorBorderColor(const QColor& color) { m_currentColorBorderColor = color; }

protected:
    /**
     * 样式变化事件：QSS 重新生效后重建色块样式，保证 qproperty- 新值即时生效
     */
    void changeEvent(QEvent* event) override;

private:
    void syncFromOwner();

    TimeLineClipModel* m_owner {nullptr};
    QButtonGroup* m_colors {nullptr};
    QLineEdit* m_noteEdit {nullptr};
    QLabel* m_currentColor {nullptr};
    bool m_syncing {false};

    // 边框样式默认值，可被 QSS qproperty- 覆盖
    QColor m_swatchBorderColor {QColor(102, 102, 102)};        // #666666
    QColor m_swatchCheckedBorderColor {Qt::white};             // 选中描边
    QColor m_currentColorBorderColor {QColor(136, 136, 136)};  // #888888
};
