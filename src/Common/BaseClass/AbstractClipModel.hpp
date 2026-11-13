#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include <QAbstractItemModel>
#include "TimeLineDefines.h"
#include "TimeLineStyle.h"
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
#include "Common/AppConfig/TimeCodeDefines.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "BaseClassExport.h"
#include <QCheckBox>
#include <QPointer>
#include <QVariant>
#include <unordered_map>
// 注意：不要在本头文件 include ClipTimePanel.hpp / ClipAppearancePanel.hpp。
// 本类持有它们的指针成员（timeGroupBox），用前置声明即可；若在此 include，
// 所有 include 本头的消费方（宿主/插件）TU 都会连带展开面板类的
// tr() / staticMetaObject 符号——后者是静态数据符号，无法通过导入库解析，
// 将导致消费方链接失败（LNK2019）。完整定义只在库内 .cpp 中使用。
class ClipTimePanel;
class ClipAppearancePanel;
class QMenu; // appendKeyframeContextMenu 形参（引用传递，前置声明足够）
using namespace QtTimeline;
class BASECLASS_EXPORT AbstractClipModel : public QObject {
    Q_OBJECT
public:
    explicit AbstractClipModel(qint64 start,const QString& type, QObject* parent=nullptr): QObject(parent),
    m_type(type),
    m_start(start),
    m_end(start + kFrameUs),  // 默认 1 帧（40ms@PAL；旧值 100μs 在任何缩放下
                              // 都不足 1px——拖入后恒为"一条线"，2026-11-11）
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
~AbstractClipModel() override 
{
    // 关闭并删除属性窗口
    if (m_standardPropertyWidget && m_standardPropertyWidget->isVisible()) {
        m_standardPropertyWidget->close();
        m_standardPropertyWidget->deleteLater();
    }
    
}

ClipId id() const { return m_id; }

virtual void setId(ClipId id) {
    m_id = id;
    // 【2026-11-12 恢复 OSC 外部控制】ID 就绪即统一登记控件并推送初始状态
    //（控件常在 setId 之后才经 initPropertyWidget 创建——那里会再推一次，幂等）
    pushOscInitialState();
}

// ---- OSC 外部控制体系（2026-11-12 恢复，自旧 SDK AbstractClipModel /
//      NodeClipDelegateModel 收编；实现位于 AbstractClipModel.cpp）----
// 控件以【相对地址】（如 "/start_frame"）登记；完整地址 = m_oscAlias + "/" + id + rel。
// registerExternalControl 同时 installEventFilter(this)——Ctrl+左键拖动控件
// 经 eventFilter → startDrag 导出地址（剪贴板 + application/x-osc-address mime）。
virtual void registerExternalControl(const QString& oscAddress, QWidget* control);
QString makeFullOscAddress(const QString& oscAddress) const;
virtual void stateFeedBack(const QString& oscAddress, QVariant value);

void setTimelineModel(QAbstractItemModel* model) { m_timelineModel = model; }
QAbstractItemModel* timelineModel() const { return m_timelineModel; }

    // Getters
    /**
     * 开始（μs；2026-11-10 路线 C 微秒原生——库内时间单位=微秒，与 Scene 权威域同基）
     */
qint64 start() const { return m_start; }
    /**
     * 结束（μs，开区间末端 = start + duration）
     */
qint64 end() const { return m_end; }
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
     * 长度（μs）
     */
qint64 length() const { return m_end - m_start; }
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
     * 设置开始（μs）
     */
virtual void setStart(qint64 start) {
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
        // 【2026-11-12 恢复】OSC 外部反馈（旧 SDK setStart 同款三地址；库内 μs
        // → PAL 帧语义换算，payload 保持 Int 与旧帧基 SDK 一致）
        const int f = static_cast<int>(usToFrames(m_start));
        stateFeedBack(QStringLiteral("/start_frame"), f);
        stateFeedBack(QStringLiteral("/position_frame"), f);
    }
}
/**
 * 设置结束（μs，开区间末端）
 */
virtual void setEnd(qint64 end) {
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
        // 【2026-11-12 恢复】OSC 外部反馈（μs → PAL 帧语义）
        stateFeedBack(QStringLiteral("/end_frame"), static_cast<int>(usToFrames(m_end)));
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

virtual void setTimeCodeType(TimeCodeType timeCodeType){m_timeCodeType=timeCodeType;}

virtual TimeCodeType getTimeCodeType() const {return m_timeCodeType;}

    /**
     * 获取数据
     * @param int role 角色
     * @return QVariant 数据
     */
virtual QVariant data(int role) const {
    switch (role) {
        case TimelineRoles::ClipModelRole:
            return QVariant::fromValue<AbstractClipModel*>(const_cast<AbstractClipModel*>(this));
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
     * 获取当前控制数据
     * @param int currentFrame 当前帧
     * @return QVariantMap 当前控制数据
     */

virtual QWidget* clipPropertyWidget(){return nullptr;};
    /**
     * 生成标准属性窗口，其中包含自定义的属性
     * @return QWidget* 属性窗口
     */
// 实现位于 AbstractClipModel.cpp（保持与 initPropertyWidget 一致的非内联导出方式）
void showPropertyWidget();
    /**
     * 绘制片段
     * @param QPainter* painter 画笔
     * @param const QRect& rect 绘制区域
     * @param bool selected 是否被选中
     */
// AbstractClipModel::initPropertyWidget
// 函数说明：初始化属性面板，此处将“时间属性”界面独立为 ClipTimePanel，并保持原有功能与信号连接。
// 实现说明：本函数创建 ClipTimePanel / ClipAppearancePanel 等 Q_OBJECT 窗口，
// 为避免宿主插件在内联展开时引用面板的 staticMetaObject 数据符号（数据符号无法
// 从导入库解析），实现已移至 AbstractClipModel.cpp，此处仅保留声明。
virtual void initPropertyWidget();

// ---- 关键帧子元素通用扩展点（2026-11-05 Tween 时间轴集成）----------------
// 定位：视图在"进入片段拖拽"与"悬停分区判定"两处先向模型询问命中；
// 命中关键帧标记则进入关键帧拖拽状态机（视图驱动，帧量化后回调
// moveKeyframe）。模型负责自己的标记绘制（paint）与坐标公式——"绘制
// 与命中共用同一映射"是本钩子的核心纪律，否则菱形画在哪、点在哪各说
// 各话。默认实现 = 无关键帧（不命中 / no-op）：不含关键帧的片段与全部
// 既有子类行为零变化。
// 关键帧命中结果：channel/key 为模型自定义下标语义（如 tween 通道序）
struct KeyframeHit {
    bool hit = false;
    int  channel = -1;
    int  key = -1;
};
/**
 * @brief 询问模型：片段矩形内的点是否命中本模型的关键帧标记
 * @param local 相对片段矩形左上角的坐标（与 paint 的 rect 同一坐标系）
 * @param rect  片段当前绘制矩形（viewport 坐标，含绘制内缩）
 * @return KeyframeHit 命中时 hit=true 并带通道/关键帧下标
 */
virtual KeyframeHit hitKeyframe(const QPoint& local, const QRect& rect) const {
    Q_UNUSED(local);
    Q_UNUSED(rect);
    return {};
}
/**
 * @brief 询问模型：在片段矩形内的空白曲线处添加一个关键帧（WATCHOUT 语义：
 *        点击 tween track 曲线上无点处 = 以该处曲线值新增 tween point）
 * @param local 相对片段矩形左上角的坐标
 * @param rect  片段当前绘制矩形
 * @return KeyframeHit 添加成功时 hit=true 并带新键的通道/下标；未添加
 *         （点不在曲线区/无通道/与邻键过近）返回 {}——视图据此继续
 *         走片段拖拽或忽略
 */
virtual KeyframeHit addKeyframeAt(const QPoint& local, const QRect& rect) {
    Q_UNUSED(local);
    Q_UNUSED(rect);
    return {};
}
/**
 * @brief 请求模型删除一个关键帧（WATCHOUT 语义：右键菜单删除 tween point）
 * @return 是否确实删除（视图据此决定是否刷新）
 */
virtual bool removeKeyframe(int channel, int key) {
    Q_UNUSED(channel);
    Q_UNUSED(key);
    return false;
}
/**
 * @brief 拖动关键帧（视图已完成帧量化）。带垂直分量：localY 是鼠标在
 *        【片段矩形内】的像素 y（rect 同一坐标系，允许为负——鼠标越出
 *        片段顶时模型应钳到值域顶），由模型按自己的曲线带几何/值域换算
 *        为业务值——视图不做任何归一化假设（值域是模型知识）。
 *        rect 传无效矩形（默认）表示本次拖动不改变值（纯水平移动）。
 * @param channel    通道下标（hitKeyframe 返回的同一下标）
 * @param key        通道内关键帧下标
 * @param localFrame 目标位置（片段本地时刻，μs 0..length()）；模型自行做
 *                   邻居升序钳制，并经自身机制通知视图重绘
 * @param localY     片段内鼠标像素 y（参照 rect 顶；可越界，模型钳制）
 * @param rect       片段绘制矩形（localY 的参照系；曲线带几何由模型定义。
 *                   无效 = 纯水平拖动，不改值）
 */
virtual void moveKeyframe(int channel, int key, double localFrame,
                          double localY = -1.0, const QRect& rect = QRect()) {
    Q_UNUSED(channel);
    Q_UNUSED(key);
    Q_UNUSED(localFrame);
    Q_UNUSED(localY);
    Q_UNUSED(rect);
}

/**
 * @brief 关键帧右键菜单自定义（2026-11-09 合并关键帧模式）：命中关键帧
 *        后由模型向菜单追加自己的语义项（如"删除该帧全部/按参数删除"）。
 * @param menu 宿主右键菜单（模型直接 addAction/addMenu）
 * @param kf   hitKeyframe 的命中结果（channel/key = 模型自定义下标语义）
 * @return true = 模型已提供菜单（库不再追加默认"Delete Keyframe"单项）；
 *         false = 库回退到默认 Delete Keyframe 单项（保留通用兜底）
 */
virtual bool appendKeyframeContextMenu(QMenu& menu, const KeyframeHit& kf) {
    Q_UNUSED(menu);
    Q_UNUSED(kf);
    return false;
}

virtual void paint(QPainter* painter, const QRect& rect, bool selected) const {

    // 绘制背景

    drawBackground(painter, rect, selected);

    // 绘制标题

    drawTitle(painter, rect);


}
Q_SIGNALS:
    /**
     * 数据变化信号
     */
    void lengthChanged(qint64 length);  // 添加长度变化信号
    void timelinePositionChanged(qint64 frame); //时间轴上位置信号（μs，2026-11-10 微秒原生）
    void appearanceChanged(); //颜色或备注名变化

public Q_SLOTS:
    /**
     * 【2026-11-12 恢复】OSC 命令入站：GlobalEventBus 订阅回调（见
     * pushOscInitialState 的 subscribe）。按 ev.address 反查已登记控件并
     * 反向驱动（setValue/setChecked/setText）——控件信号照常传播，经
     * ClipTimePanel 换算链落模型 setter，再由 setter 内 stateFeedBack 出站
     * 反馈形成闭环。进程内 Feedback 不触发 Command 订阅者，无回环。
     * QTimeEdit（时间码控件）无对应分支不驱动（与旧 SDK 行为一致）。
     */
    void onOscCommand(const GlobalEvent& ev);

protected:
    // ---- OSC 外部控制内部机制（2026-11-12 恢复）----
    // 遍历 _OscMapping：向 StatusContainer 登记完整地址，并按控件类型推送初始值
    //（由 setId 与 initPropertyWidget 双触发，幂等）。
    void pushOscInitialState();
    // Ctrl+左键拖动 OSC 控件时导出地址（剪贴板 + application/x-osc-address mime）。
    // qobject_cast<IntDragValueWidget*> 需控件完整类型——实现留在库内 .cpp，
    // 防止该 GUIElements 头经本公共头进入消费方（同 ClipTimePanel 的处理纪律）。
    virtual void startDrag(QWidget* widget);
    bool eventFilter(QObject* watched, QEvent* event) override;

    // 开始（μs）
    qint64 m_start;
    // 结束（μs，开区间末端）
    qint64 m_end;
    // 类型
    QString m_type;
    // 是否可调整大小
    bool RESIZEABLE;
    // 是否显示小部件
    bool EMBEDWIDGET;
    // 是否显示边框
    bool SHOWBORDER;
    // 控件 OSC 相对地址映射（rel → 控件；QPointer 防控件销毁悬空，
    // 遍历/查询时以 data() 判空跳过失效项）
    std::unordered_map<QString, QPointer<QWidget>> _OscMapping;
    // 片段 OSC 地址前缀（别名）。新时间轴沿用旧面板地址空间 /timeline/master，
    // 外部脚本零迁移；宿主可经 setOscAlias 改写。
    QString m_oscAlias {QStringLiteral("/timeline/master")};
    // Ctrl+左键拖拽状态（eventFilter → startDrag）
    QPoint m_dragStartPos;
    bool m_oscDragging {false};
    // 片段ID（补初始化：旧版为未初始化垃圾值，会流进 OSC 地址）
    ClipId m_id {0};
    QString m_noteName;
    QColor m_color;

    QAbstractItemModel* m_timelineModel {nullptr};

    // 布局
    QVBoxLayout* m_layout;
    // //开始帧

    // 代理窗口
    QWidget* m_clipPropertyWidget;
    //时间码类型
    TimeCodeType m_timeCodeType;
    ClipTimePanel *timeGroupBox;
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

Q_DECLARE_METATYPE(AbstractClipModel*)
