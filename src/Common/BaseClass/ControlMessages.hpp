// ============================================================================
//  ControlMessages.hpp —— Control 片段指令解析/触发（header-only 工具）
//
//  定位（2026-11-13 Control 片段）：ms::Clip.control_messages（JSON，
//  OSCMessageListWidget save/load 兼容格式 {"messages":[...]}）的统一
//  解析与发送入口。两处消费者：
//    * ControlClipSection（control.plugin）——「测试发送」按钮；
//    * QtTimelineModelAdapter 触发引擎（Flow 主程序）——播放头上升沿。
//  放 BaseClass 的原因：插件与主程序都链 BaseClass（include 根可达），
//  而 MediaCore 不链 Qt（QJson 不可用）；本头 header-only inline，
//  符号在消费者 TU 实例化——消费者各自链 StatusContainer 即可，
//  BaseClass.dll 本身零改动零依赖。
//
//  发送语义（与合并前 TriggerClipModel::trigger 同一入口）：
//  StatusContainer::parseOSC → 入状态容器 → statusUpdated → WS 8992
//  广播出站 + GlobalEventBus 控件联动。host/port 不参与（本地状态容器）。
// ============================================================================

#pragma once

#include <string>

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVector>
#include <QVariant>

#include "Common/AppConfig/OSCMessage.h"
#include "StatusContainer.h"

namespace ms {

/// 解析 control_messages → OSCMessage 列表（空串/坏 JSON/无 messages 键
/// → 空列表，静默容错——触发路径不容异常逃逸）。
inline QVector<OSCMessage> parseControlMessages(const std::string& json) {
    QVector<OSCMessage> out;
    if (json.empty()) return out;
    const QJsonDocument doc =
        QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject()) return out;
    const QJsonArray arr =
        doc.object()[QStringLiteral("messages")].toArray();
    for (const auto& v : arr) {
        const QJsonObject o = v.toObject();
        OSCMessage m;
        m.address = o[QStringLiteral("address")].toString();
        m.type    = o[QStringLiteral("type")].toString();
        const QJsonValue val = o[QStringLiteral("value")];
        if (val.isArray())
            m.value = val.toArray().toVariantList();
        else if (val.isDouble())
            m.value = val.toDouble();
        else if (val.isBool())
            m.value = val.toBool();
        else
            m.value = val.toString();
        out.push_back(m);
    }
    return out;
}

/// 触发发送：逐条入状态容器（编辑器测试发送与播放触发同入口）。
inline void fireControlMessages(const std::string& json) {
    for (const auto& m : parseControlMessages(json))
        StatusContainer::instance()->parseOSC(m);
}

} // namespace ms
