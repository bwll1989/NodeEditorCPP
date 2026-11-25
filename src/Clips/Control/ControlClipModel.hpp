// ============================================================================
//  ControlClipModel —— Control 指令片段模型（src/Clips/Control/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】自
//  ControlClipPlugin.hpp 单头拆出——文件结构对齐节点插件基准。
//
//  定位（2026-11-13 用户定案）：合并前 Trigger 片段的语义后继——只在时间轴
//  中存在，无媒体文件、无画布呈现；播放头进入片段区间（上升沿）时，结合
//  StatusContainer 把片段携带的指令逐条发送（经 parseOSC 入状态广播链，
//  WS 8992 出站即达外部/控件联动）。消息列表存 ms::Clip.control_messages
// （随场景 pb 分发，field 10，OSCMessageListWidget save/load 兼容格式）。
//  触发语义（上升沿，2026-11-13 定案）：检测挂在宿主适配器播放头推进链
// （#42），本模型只承载数据与编辑区。不绑定素材（asset_id 恒空）。
//
//  默认色（2026-11-25 用户要求弃橙——旧 #f08a5d 与视频类型色撞且深字低对
//  比）改蓝 #4A90D9（指令语义；与音频紫/映射玫红/文本黄/Artnet·图片·Live
//  青系均不撞）。无画布呈现（SceneCanvasWidget 按 kind==Control 跳过标
//  注/收集），定长（RESIZEABLE=false——触发窗口语义，长度仅用于时间轴
//  占位与上升沿判定）。
//
//  【2026-11-25 绘制定稿·七轮迭代（终版=六轮 L 形角标）】
//  迭代史：①旧窄条（52px 行时代）行高降后文字条 6-16px 不可辨；②全高块
//  体+斜纹闪电——定长片段实际宽度仅数像素退化成金线；③补最小视觉宽度；
//  ④L 形角标（文字改下半）；⑤选中白描边——两矩形各自描边在相接处多出
//  分段线；⑥L 形外轮廓描边——用户「不要边线」，选中=仅加深；⑦五边形
//  标签（尖指标尺）——用户实测后要求撤回，恢复⑥。**终版定稿=L 形角标**：
//  顶部窄竖条（宽 5px 高 40%）贴触发帧 + 底部横条（高 60%）承载文字，
//  单一 QPolygonF 同色一体（无接缝无金线）。
//    * 最小视觉宽度 = 文本实测宽+16（上限 120）：定长片段窄至数像素时
//      延伸，宽块不受影响；延伸不改数据（右邻片段后绘覆盖）；
//    * 文字：加粗横条内居中，像素字号随条高钳制（9-12px），字色按底色
//      亮度自适应；displayText() 语义不变（备注名优先回退类型名）；
//    * 选中 = 底色加深（fill.darker(150)，与媒体片段同约定），无边线。
//  【颜色不持久化】装载链只恢复 note/clip_data/时长（appearanceChanged
//  仅回写 note），模型色恒为插件构造默认——改默认色即全体生效，无需迁移。
// ============================================================================

#pragma once

#include <QPainter>

#include "SceneClipModel.hpp"

class ControlClipModel : public SceneClipModel {
    Q_OBJECT
public:
    ControlClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Control", {}, parent) {
        RESIZEABLE  = false; // 定长：合并前 Trigger 语义
        EMBEDWIDGET = false;
        SHOWBORDER  = false;
        setColor(QColor(0x4A, 0x90, 0xD9)); // 蓝（2026-11-25 用户要求弃橙：
                                            // 指令语义，不与既有类型色撞车）
    }

    /// 片段绘制：2026-11-25 定稿——L 形角标（七轮五边形已按用户要求撤回）
    void paint(QPainter* painter, const QRect& rect, bool selected) const override {
        painter->save();  // 保存状态
        painter->setRenderHint(QPainter::Antialiasing);
        const QColor fill = color();

        // ---- 0. 几何：左上竖条（宽 5px 高 40%，贴触发帧）+ 底部文字条
        //         （高 60%）；字号随条高 ----
        const int stem_w = 5;
        const int stem_h = rect.height() * 2 / 5;
        const int bar_h  = rect.height() - stem_h;
        QFont font = painter->font();
        font.setPixelSize(qBound(9, bar_h - 4, 12));
        font.setBold(true);
        const QFontMetrics fm(font);
        const int text_full_w = fm.horizontalAdvance(displayText());

        // ---- 1. 最小视觉宽度（定长片段窄至数像素时延伸至容下文本）----
        const int min_w  = qMin(text_full_w + 16, 120);
        const int body_w = qMax(rect.width(), min_w);

        // ---- 2. L 形一体：竖条+横条单一多边形（同色无接缝；左缘锚定
        //         片段 start，不左溢——脏区登记矩形左边距仅 -4）----
        QPolygonF lshape;
        lshape << QPointF(rect.left(),                 rect.top())
               << QPointF(rect.left() + stem_w,        rect.top())
               << QPointF(rect.left() + stem_w,        rect.top() + stem_h)
               << QPointF(rect.left() + body_w,        rect.top() + stem_h)
               << QPointF(rect.left() + body_w,        rect.bottom() + 1.0)
               << QPointF(rect.left(),                 rect.bottom() + 1.0);

        painter->setPen(QPen(fill, 0)); // pen=填充色即无描边
        painter->setBrush(selected ? fill.darker(150) : fill);
        painter->drawPolygon(lshape);

        // ---- 3. 文字：底部横条内居中，字色按底色亮度自适应 ----
        const QRect bar(rect.left(), rect.top() + stem_h, body_w, bar_h);
        painter->setFont(font);
        painter->setPen(fill.lightness() > 160 ? QColor(30, 30, 30) : Qt::white);
        const QString text = fm.elidedText(
            displayText(), Qt::ElideRight, qMax(0, bar.width() - 10));
        if (!text.isEmpty())
            painter->drawText(bar.adjusted(5, 0, -5, 0),
                              Qt::AlignCenter, text);
        painter->restore();  // 恢复状态
    }
};
