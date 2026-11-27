// ============================================================================
//  MappingClipModel —— Mapping 映射片段模型（src/Clips/Mapping/）
//
//  【2026-11-15 片段移植】自节点体系 mappingclipmodel.hpp（NodeClipDelegate
//  Model 帧域，header-only）移植为 SceneClipModel（μs 域）子类（同样
//  header-only，对齐 ControlClipModel.hpp 先例）。移植对照表（节点体系 →
//  本插件）：
//    * currentData(currentFrame) 拉取驱动（帧域，[m_start, m_end] 双闭、
//      m_currentFrame 判等去重）→ onPlayheadAdvanced(prev, now) 推送驱动
//     （μs 域，同 [start, end] 双闭口径；去重键 = 绝对帧号 usToFrames(now)）；
//    * 求值依赖编辑控件的 QJSEngine（每卡片一份，PI/E 全局 + $percent/
//      $frame 每拍注入）→ 模型自带【一份】QJSEngine（PI/E 全局同源，
//      $percent/$frame 每拍注入）——持久化即表达式文本
//     （OSCMessageListWidget::save() 把 value 覆写为 getExpression() 原文，
//      求值只在发送时发生），控件生命周期与回放解耦，无控件也能完整求值；
//    * 表达式求值逐行复刻 OSCMessageItemWidget::getMessage（移植保真唯一
//      权威出处）：
//        - 值以 '[' 开头 → type="List"，with(Math) { (expr) } 求值取数组
//         （引擎报错/非数组回退 QJsonDocument 直解；再不成 = 空列表）；
//        - type=="Int"/"Float" → with(Math) { expr } 求值（报错回退 0）；
//        - 其余 → type="String"，表达式原文直发（不修剪——与控件同口径）；
//    * trigger()（getOSCMessages → 逐条 StatusContainer::parseOSC）→
//      求值后直接 StatusContainer::parseOSC（同一入口：入状态容器 →
//      statusUpdated → WS 8992 广播 + GlobalEventBus 控件联动）；
//    * save()/load() QJsonObject 载荷 → ms::Clip.clip_data（pb field 13，
//      {"messages":[...]}）经 setClipPayload 灌入模型（适配器装载/面板
//      写回两路）；
//    * GlobalEventBus /start_frame /end_frame /position_frame 外部控制
//      订阅：本体系属性绑定体系未覆盖片段业务字段——暂缓（见 Doc.md）。
//
//  几何/绘制走基类（玫红块 #B83B5E，可拖拽调长）；默认时长 = 100 帧
// （旧版构造 m_end = m_start + 100 的帧域口径，μs 化 = 100×kFrameUs）。
// ============================================================================

#pragma once

#include <QColor>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QtMath> // M_PI/M_E（QJSEngine 全局常量，OSCMessageItemWidget 同源）
#include <algorithm>
#include <utility>
#include <vector>

#include "SceneClipModel.hpp"
#include "TimeLineDefines.h"            // kFrameUs/usToFrames（旧帧域口径 μs 化）
#include "Common/AppConfig/OSCMessage.h" // OSCMessage（AppConfig；include 根经
                                         //  BaseClass 传播可达，ControlMessages
                                         //  同款解析路径）
#include "StatusContainer.h"             // parseOSC 发送入口（与 trigger 同源）

class MappingClipModel : public SceneClipModel {
    Q_OBJECT
public:
    explicit MappingClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, QStringLiteral("Mapping"), {}, parent) {
        RESIZEABLE  = true;   // 可拖拽调长（时长 = 映射播放区间）
        EMBEDWIDGET = false;
        SHOWBORDER  = false;  // 【2026-11-27 用户定案】所有片段无边框
        setColor(QColor(0xB8, 0x3B, 0x5E)); // 玫红（节点体系 Mapping 类型色 #b83b5e）
        setEnd(startUs + 100 * QtTimeline::kFrameUs); // 旧 ctor：m_end = m_start + 100 帧
        engine_.globalObject().setProperty(QStringLiteral("PI"), M_PI);
        engine_.globalObject().setProperty(QStringLiteral("E"),  M_E);
    }

    /// 【AbstractClipModel】工作副本业务数据镜像：{"messages":[...]} 原文
    /// 存表（表达式文本不求值）+ 复位去重（编辑后下一拍重新触发——旧体系
    /// load 后同理从头触发）。幂等：原文未变直接返回（on_edited 高频回推）。
    void setClipPayload(const QString& json) override {
        if (json == last_payload_) return;
        last_payload_ = json;
        applyPayload(json);
    }

    /// 【AbstractClipModel】播放头推进（适配器每拍调用）：区间内逐帧求值
    /// 表达式表 → 逐条 parseOSC。
    ///
    /// 【2026-11-22 退役标注】适配器已停止调用本钩子——逐帧求值发送由
    /// PlaybackSendScheduler 独立线程承担（时钟 deadline 直读主时钟，语义
    /// 逐位移植于本实现；见 src/Widget/TimeLineWidget/SendScheduler.cpp）。
    /// 本实现保留一版作回滚路径，验证稳定后清理。
    ///
    /// 【2026-11-20 补帧修复（GUI 饥饿丢发，用户实测 100 帧少收）】播放
    /// 权威时钟是 wall-clock（session_time_us / 预览 t0 锚点），33ms GUI
    /// tick 只是采样——界面操作（拖拽/重绘风暴）推迟 tick 派发时，一次
    /// onPlayheadAdvanced 的 prev→now 会跨多帧。旧行为只发当前帧（绝对帧
    /// 号判等去重）→ 被跨过帧的指令全部静默丢失。修复：prev→now 跨过的
    /// 每一帧都补发（$percent/$frame 按各帧代表时刻求值），"每帧一次"语义
    /// 不再依赖 tick 节奏：
    ///   * 中间帧代表时刻 = framesToUs(f)（usToFrames 回射恒等，去重键与
    ///     旧行为同空间；clamp 到 st 防 percent 负值）；末帧沿用 nowUs 本
    ///     尊——无跳变时旧行为逐位一致；
    ///   * 跨帧数 > kMaxCatchupFrames（人为 seek 大跳变）放弃补帧只发当
    ///     前帧——scrub 不该产出指令风暴；
    ///   * 出区间复位去重：循环回绕/seek 离开再进入 = 重新从首帧触发。
    void onPlayheadAdvanced(qint64 prevUs, qint64 nowUs) override {
        const qint64 st = start();
        const qint64 en = end();
        if (nowUs < st || nowUs > en) {       // 旧口径 [m_start, m_end] 双闭
            lastFrame_ = -1;                  // 出区间复位（再进=重新触发）
            return;
        }
        const qint64 fNow = QtTimeline::usToFrames(nowUs);
        if (prevUs >= 0 && prevUs < nowUs) {
            // 补帧迭代：首帧 = prev 之后第一个不早于区间起点的帧
            const qint64 fFrom = std::max(QtTimeline::usToFrames(prevUs) + 1,
                                          QtTimeline::usToFrames(st));
            if (fNow - fFrom < kMaxCatchupFrames)
                for (qint64 f = fFrom; f < fNow; ++f)
                    emitForFrame(std::max(QtTimeline::framesToUs(f), st));
        }
        emitForFrame(nowUs);                  // 末帧（旧行为）
    }

private:
    /// 补帧上限（≈4.8s@25fps）：播放期 tick 饥饿的补发窗口远小于此；
    /// 超过视为人为 seek 大跳变，放弃补帧（防 scrub 指令风暴）。
    static constexpr qint64 kMaxCatchupFrames = 120;

    /// 消息表条目（clip_data 镜像；value 持久化即表达式原文，不求值）
    struct Entry {
        QString address;     ///< OSC 地址
        QString type;        ///< 控件类型（"Int"/"Float"/"String"）
        QString expression;  ///< 表达式原文（save() 覆写口径）
    };

    /// 解析 {"messages":[...]} → entries_（坏 JSON/空 = 清空表，静默容错
    /// ——播放路径不容异常逃逸）。value 兼容旧非字符串值（数字/布尔字面
    /// 量化后同样按表达式求值——语义等价）。
    void applyPayload(const QString& json) {
        entries_.clear();
        lastFrame_ = -1; // 编辑落定后下一拍重新触发（与重灌一致）
        if (json.isEmpty()) return;
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (!doc.isObject()) return;
        const QJsonArray arr =
            doc.object()[QStringLiteral("messages")].toArray();
        entries_.reserve(static_cast<size_t>(arr.size()));
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            Entry e;
            e.address = o[QStringLiteral("address")].toString();
            e.type    = o[QStringLiteral("type")].toString();
            const QJsonValue jv = o[QStringLiteral("value")];
            if (jv.isString())      e.expression = jv.toString();
            else if (jv.isDouble()) e.expression = QString::number(jv.toDouble());
            else if (jv.isBool())   e.expression = jv.toBool()
                                        ? QStringLiteral("true")
                                        : QStringLiteral("false");
            entries_.push_back(std::move(e));
        }
    }

    /// 单帧求值 + 发送（去重 → $percent/$frame 注入 → 全表求值）。
    /// tUs = 该帧代表时刻（末帧 = nowUs 本尊；补帧 = clamp 后的
    /// framesToUs(f)）。$percent = 区间内归一化进度（旧
    /// (currentFrame-m_start)/(m_end-m_start)）；$frame = 区间内相对帧号
    ///（表达式经 with(Math) 求值，$percent/$frame/PI/E 全部可见）。
    void emitForFrame(qint64 tUs) {
        const qint64 frame = QtTimeline::usToFrames(tUs);
        if (frame == lastFrame_) return;      // 同帧去重（帧域语义）
        lastFrame_ = frame;

        const qint64 st = start();
        const qint64 len = length();
        const double percent = len > 0
            ? double(tUs - st) / double(len) : 0.0;
        const int relFrame = static_cast<int>(QtTimeline::usToFrames(tUs - st));
        engine_.globalObject().setProperty(QStringLiteral("$percent"), percent);
        engine_.globalObject().setProperty(QStringLiteral("$frame"),   relFrame);

        for (const Entry& e : entries_) evaluateAndSend(e);
    }

    /// 单条求值 + 发送（OSCMessageItemWidget::getMessage 逐行复刻——移植
    /// 保真唯一权威出处；发送 = StatusContainer::parseOSC，与旧 trigger()
    /// 同一入口）。
    void evaluateAndSend(const Entry& e) {
        OSCMessage m;
        m.address = e.address;
        const QString trimmed = e.expression.trimmed();

        if (trimmed.startsWith(QLatin1Char('['))) {
            // 列表：with(Math) 求值（引擎报错/非数组回退 JSON 直解；再不
            // 成 = 空列表——控件 parseList 同口径）
            m.type  = QStringLiteral("List");
            m.value = evalList(trimmed);
        } else if (e.type == QLatin1String("Int")) {
            const QJSValue r = engine_.evaluate(
                QStringLiteral("with(Math) { %1 }").arg(e.expression));
            m.type  = QStringLiteral("Int");
            m.value = r.isError() ? 0 : r.toInt();
        } else if (e.type == QLatin1String("Float")) {
            const QJSValue r = engine_.evaluate(
                QStringLiteral("with(Math) { %1 }").arg(e.expression));
            m.type  = QStringLiteral("Float");
            m.value = r.isError() ? 0.0 : r.toNumber();
        } else {
            // String：表达式原文直发（不修剪——控件同口径）
            m.type  = QStringLiteral("String");
            m.value = e.expression;
        }
        StatusContainer::instance()->parseOSC(m);
    }

    /// 列表达式求值（控件 getMessage::parseList 同源）：数组元素按
    /// number/bool/其余字符串收窄；回退 QJsonDocument 直解；兜底空列表。
    QVariantList evalList(const QString& text) {
        const QJSValue result = engine_.evaluate(
            QStringLiteral("with(Math) { (%1) }").arg(text));
        if (!result.isError() && result.isArray()) {
            QVariantList list;
            const int n = result.property(QStringLiteral("length")).toInt();
            list.reserve(n);
            for (int i = 0; i < n; ++i) {
                const QJSValue el = result.property(i);
                if (el.isNumber())    list.append(el.toNumber());
                else if (el.isBool()) list.append(el.toBool());
                else                  list.append(el.toString());
            }
            return list;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
        if (doc.isArray()) return doc.array().toVariantList();
        return QVariantList{};
    }

    // ---- 状态 ----------------------------------------------------------------
    QJSEngine engine_;          ///< 求值引擎（单份；PI/E 全局 ctor 注入，
                                ///  $percent/$frame 每拍注入——控件引擎同源）
    std::vector<Entry> entries_;///< 消息表（clip_data 镜像；权威在
                                ///  ms::Clip.clip_data）
    QString   last_payload_;    ///< 上次灌入原文（幂等防抖）
    qint64    lastFrame_ = -1;  ///< 去重：上次已触发的绝对帧号（旧
                                ///  m_currentFrame 判等同口径；payload 变更复位）
};
