// ============================================================================
//  ControlClipPlugin —— Control 指令片段插件（时间轴片段类型）
//
//  定位（2026-11-13 用户定案）：合并前 Trigger 片段的语义后继——只在时间轴
//  中存在，无媒体文件、无画布呈现；播放头进入片段区间（上升沿）时，结合
//  StatusContainer 把片段携带的指令逐条发送（经 parseOSC 入状态广播链，
//  WS 8992 出站即达外部/控件联动）。
//
//  与旧 TriggerClipModel（src/Nodes/TimeLineNode/，节点体系内嵌静态注册）
//  的差异：本插件走磁盘插件体系（bin/plugins/TimeLine/control.plugin），
//  类型表纯磁盘驱动；消息列表存 ms::Clip.control_messages（随场景 pb
//  分发，field 10，OSCMessageListWidget save/load 兼容格式），不再是
//  节点委托模型的 save()/load() 载荷。
//
//  触发语义（上升沿，2026-11-13 定案）：旧 Trigger 为「区间内每帧触发」，
//  收敛为「进入区间触发一次」——检测挂在宿主适配器播放头推进链（#42），
//  本插件只承载数据与编辑区。
//
//  专属编辑区 = 本目录 ControlClipSection（OSCMessageListWidget + 测试
//  发送按钮），经 createPropertySection 工厂加载；Control 不绑定素材
// （asset_id 恒空——工厂不查 Asset，与 Text/Live 的素材校验不同）。
// ============================================================================

#ifndef CONTROLCLIPPLUGIN_HPP
#define CONTROLCLIPPLUGIN_HPP

#include <QObject>
#include <QWidget>
#include <QPainter>

#include "SceneClipModel.hpp"
#include "AbstractClipInterface.h"

#include "TimeLineStyle.h" // ClipBorderColour（左侧竖线色，旧 Trigger 同源）

// 专属编辑区（随本插件分发）：ControlClipSection 的实现（含 moc 槽符号）
// 只在 control.plugin 内，主程序不含任何片段实现。
#include "ControlClipSection.hpp"       // 同目录：指令编辑区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

// ---------------------------------------------------------------------------
// Control 片段模型：橙色块（合并前 Trigger 类型色 #f08a5d）
// 无画布呈现（SceneCanvasWidget 按 kind==Control 跳过标注/收集），定长
//（RESIZEABLE=false——触发窗口语义，长度仅用于时间轴占位与上升沿判定）。
//
// 【2026-11-13 可视性重构（用户定案「做成原 trigger clip 的样子」）】：
// paint 逐行复刻旧 TriggerClipModel::paint——窄块成「一条线」贴在触发帧上：
//   * 唯一绘制矩形 = textRect = rect.adjusted(4,10,40,-10)（右缘右扩
//     40px），bg 填充即文字色底；**不画块体矩形**（1:1 复刻旧实现——
//     先画块体会多出一条窄橙线，文字溢出块体部分会裸露无色底）；
//   * SHOWBORDER=false → pen=填充色无边线（同旧 Trigger）；
//   * 左侧 4px 金色竖线（ClipBorderColour）；
//   * 窄块（触发时刻贴帧）时 textRect 仍约 46px 宽 → 一条橙线 + 带色底
//     文字落在触发帧「旁边」，宽块时文字自然居中；
//   * 文字 = displayText()（备注名优先，回退类型名——面板「备注名」即
//     改这里的显示；旧 Trigger 同语义）。
// ---------------------------------------------------------------------------
class ControlClipModel : public SceneClipModel {
    Q_OBJECT
public:
    ControlClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Control", {}, parent) {
        RESIZEABLE  = false; // 定长：合并前 Trigger 语义
        EMBEDWIDGET = false;
        SHOWBORDER  = false;
        setColor(QColor(0xF0, 0x8A, 0x5D)); // 橙（原 Trigger 类型色）
    }

    /// 片段绘制：旧 TriggerClipModel::paint 逐行复刻（triggerclipmodel.hpp 100-134）
    void paint(QPainter* painter, const QRect& rect, bool selected) const override {
        painter->save();  // 保存状态
        painter->setRenderHint(QPainter::Antialiasing);
        const QColor fill = color();
        if (selected) {
            painter->setBrush(fill.darker(150));
        } else {
            painter->setBrush(fill);
        }

        // 设置细边框（SHOWBORDER=false → pen=填充色无边线，同旧 Trigger）
        if (SHOWBORDER) {
            painter->setPen(QPen(ClipBorderColour, 1));
        } else {
            painter->setPen(QPen(fill, 0));
        }
        // 唯一绘制矩形 = textRect（右缘右扩 40px）：bg 填充即文字色底——
        // 窄块时呈「一条橙线 + 带色底文字」落在触发帧旁（旧 Trigger 标志样式）
        auto textRect = rect.adjusted(4, 10, 40, -10);
        painter->drawRect(textRect);
        painter->setPen(fill.lightness() > 160 ? QColor(30, 30, 30) : Qt::white);
        QFont font = painter->font();
        font.setPointSize(8);  // 设置较小的字体大小
        painter->setFont(font);

        const QString text = painter->fontMetrics().elidedText(
            displayText(), Qt::ElideRight, qMax(0, textRect.width() - 4));
        painter->drawText(textRect, Qt::AlignCenter, text);
        // 绘制左侧竖线
        painter->setPen(QPen(ClipBorderColour, 4));  // 稍微粗一点的线
        painter->drawLine(rect.left() + 2, rect.top(),
                          rect.left() + 2, rect.bottom());

        // 设置回细边框
        painter->setPen(QPen(ClipBorderColour, 1));
        painter->restore();  // 恢复状态
    }
};

// ---------------------------------------------------------------------------
// Control 插件：注册进类型表（type = "Control"）
// ---------------------------------------------------------------------------
class ControlClipPlugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid) // QPluginLoader 按 IID 识别
    Q_INTERFACES(ClipPlugInterface)
public:
    explicit ControlClipPlugin(QObject* parent = nullptr) : QObject(parent) {}

    QString clipType() const override { return "Control"; }

    AbstractClipModel* createModel(qint64 startUs) override {
        return new ControlClipModel(startUs);
    }

    /// 片段属性面板专属编辑区工厂：Control 无素材（asset_id 恒空），
    /// 只校验片段上下文存在即建区（与 Text/Live 的素材校验不同）。
    QWidget* createPropertySection(void* ctx, QWidget* parent) override {
        auto* c = static_cast<ms::ClipSectionContext*>(ctx);
        if (!c || !c->clip) return nullptr;
        return new ms::ControlClipSection(*c, parent);
    }
};

#endif // CONTROLCLIPPLUGIN_HPP
