// ============================================================================
//  PanelTimingProbe.hpp —— 片段属性面板时延诊断探针（临时埋点 2026-11-25）
//
//  背景：用户报片段属性弹窗「标题先现、内容很多时候 2-3s 后才出」。拆帧
//  显示（2026-11-20）已把标题与装配拆到两帧，延迟落在帧 2（工厂装配）的
//  调度或执行里，需实测数据定位。诊断结束后整体删除：本头文件 +
//  AbstractClipModel.cpp / QtTimelineModelAdapter.cpp 两处调用点。
//
//  双通道输出：GUI 双击启动时 stderr 是哑的（UCRT 重定向陷阱，memory
//  2026-11-24 有案，qDebug/fflush 用户看不到），故 qWarning（控制台/
//  DebugView 可见）之外再落一份文件 Documents/Flow/log/panel-timing.log，
//  任何启动方式均可事后查看。
// ============================================================================
#pragma once

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QStandardPaths>
#include <QTime>
#include <QString>
#include <mutex>

namespace panel_timing {

inline void probe(const QString& msg)
{
    qWarning().noquote() << "panel-timing:" << msg;
    static std::mutex mu; // 探针可能从任意线程触发，文件追加串行化
    std::lock_guard<std::mutex> lk(mu);
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QStringLiteral("/Flow/log");
    if (!QDir().mkpath(dir))
        return;
    QFile f(dir + QStringLiteral("/panel-timing.log"));
    if (!f.open(QIODevice::Append | QIODevice::Text))
        return;
    f.write(QStringLiteral("[%1] %2\n")
                .arg(QTime::currentTime().toString(
                         QStringLiteral("HH:mm:ss.zzz")),
                     msg)
                .toUtf8());
}

} // namespace panel_timing
