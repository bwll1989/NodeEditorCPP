// ============================================================================
//  MediaProgressDelegate —— 媒体库条目进度条 delegate
//
//  2026-11-27：导入复制与视频转码共用。条目 ProgressRole（0-100）有效时，
//  在默认绘制（图标/文本/选中态）完成后，按百分比宽度叠加一层半透明
//  Highlight 色——即"条目背景进度条"。叠绘而非预绘背景：
//    1) QStyledItemDelegate::paint 的选中/悬停高亮完全保留，主题自适应
//       （浅色/深色主题都取 palette.Highlight，无需硬编码颜色）；
//    2) 低 alpha（60）叠加下文本仍清晰可读。
//  percent<=0 或 >100 不画（-1=无进度，复制中仅文案阶段等）。
//  无 Q_OBJECT：纯虚重写 header-only，不进主程序 SOURCES（同 MediaLibraryTree
//  先例，避免 AUTOMOC 产生无 moc 产物的 LNK2019）。
// ============================================================================

#pragma once

#include <QPainter>
#include <QStyledItemDelegate>
#include <QVariant>

#include "MediaLibrary/MediaLibrary.h"

class MediaProgressDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);
        if (!painter || !index.isValid()) return;

        const QVariant v = index.data(MediaLibrary::ProgressRole);
        if (!v.isValid()) return;
        bool ok = false;
        const int percent = v.toInt(&ok);
        if (!ok || percent <= 0 || percent > 100) return;

        const int w = int(qreal(option.rect.width()) * percent / 100.0);
        if (w <= 0) return;
        painter->save();
        painter->setClipRect(option.rect.left(), option.rect.top(), w, option.rect.height());
        QColor c = option.palette.color(QPalette::Highlight);
        c.setAlpha(60);
        painter->fillRect(option.rect, c);
        painter->restore();
    }
};
