// ============================================================================
//  TweenPalette —— Tween 标记的【公共视觉常量】（2026-11-05 建立；
//  2026-11-09 v2 打包属性适配；2026-11-10 遗留清理）
//
//  定位：SceneClipModel（片段合并菱形带）与 TweenSection（行指示色）共用
//  的最小公共集——颜色表、属性短名。
//
//  【v2 打包属性】（用户定案 DaVinci Resolve Inspector 模式）：二维值打包
//  成一条属性（位置/尺寸/角点各为一条），时间轴菱形统一色不分属性——
//  属性色仅存于面板行指示与右键"按参数删除"菜单图标。
//
//  【退役记录】帧域通道求值 evalTweenChannelAt（行头属性下拉读数）随
//  展开行/行头下拉退役已删（2026-11-10 遗留清理）；us 域权威求值恒在
//  src/core/engine/TweenEngine.cpp evalChannel，勿在此重建帧域副本。
// ============================================================================

#pragma once

#include <QColor>

#include "TweenDots.h" // TweenChannelDots/TweenKeyDot（帧域镜像数据结构）

// ---------------------------------------------------------------------------
// tween 打包属性 → 标记/行指示颜色（ms::TweenProp v3 枚举值 1..15）。
// 时间轴合并菱形【统一白】（用户定案"关键帧不分属性都一样"）不查此表；
// 本表服务于面板行指示色与删除菜单色块。
// ---------------------------------------------------------------------------
inline QColor tweenPropColor(int prop) {
    switch (prop) {
        case 1:  return QColor(0x4F, 0xC1, 0xFF); // 位置·蓝
        case 2:  return QColor(0xFF, 0x9F, 0x43); // 尺寸·橙
        case 3:  return QColor(0xFF, 0x6B, 0x9D); // 旋转·粉
        case 4:  return QColor(0xF2, 0xF2, 0xF2); // 不透明度·白
        case 5:  return QColor(0xB3, 0x88, 0xFF); // Z 层级·紫（v3 退役）
        case 6:  return QColor(0x26, 0xC6, 0xDA); // 角点 LT·青
        case 7:  return QColor(0x79, 0x86, 0xCB); // 角点 RT·靛
        case 8:  return QColor(0xFF, 0x8A, 0x65); // 角点 RB·珊瑚
        case 9:  return QColor(0xAE, 0xD5, 0x81); // 角点 LB·黄绿
        // ---- v3 扩展（俯仰/偏航与旋转同族暖色；裁切四边叶绿系四档）----
        // 10/11 v4 退役（独立姿态通道合并进 Rotation=3）——表项保留防御
        case 10: return QColor(0xE9, 0x1E, 0x63); // 俯仰·玫红（退役）
        case 11: return QColor(0xF4, 0x51, 0x1E); // 偏航·橙红（退役）
        case 12: return QColor(0x66, 0xBB, 0x6A); // 裁切左·绿
        case 13: return QColor(0x2E, 0x7D, 0x32); // 裁切右·深绿
        case 14: return QColor(0x9C, 0xCC, 0x65); // 裁切上·黄绿
        case 15: return QColor(0x00, 0x89, 0x5C); // 裁切下·蓝绿
        case 16: return QColor(0x26, 0xA6, 0x9A); // 音频增益·青绿（2026-11-10）
        default: return QColor(0xB0, 0xB0, 0xB0); // 未知·灰（防御）
    }
}

// ---------------------------------------------------------------------------
// tween 打包属性短名（右键"按参数删除"菜单显示）。与 v2 打包枚举同源
// （prop 枚举值同源 ms::TweenProp 1..16）。
// ---------------------------------------------------------------------------
inline const char* tweenPropShortName(int prop) {
    switch (prop) {
        case 1:  return "位置";
        case 2:  return "尺寸";
        case 3:  return "旋转";
        case 4:  return "不透明度";
        case 5:  return "Z";
        case 6:  return "角点 LT";
        case 7:  return "角点 RT";
        case 8:  return "角点 RB";
        case 9:  return "角点 LB";
        case 10: return "俯仰"; // v4 退役（合并进"旋转"）
        case 11: return "偏航"; // v4 退役（合并进"旋转"）
        case 12: return "裁切 左";
        case 13: return "裁切 右";
        case 14: return "裁切 上";
        case 15: return "裁切 下";
        case 16: return "音频增益"; // 2026-11-10（漏项修复：右键菜单曾显"?"）
        default: return "?";
    }
}
