// ============================================================================
//  MediaLibraryTree —— 媒体库树视图（拖拽增强：双 mime 通道）
//
//  2026-11-15：媒体库 → 时间轴拖拽落轨（用户需求）。原 m_tree 为裸
//  QTreeView，默认拖拽经 MediaLibrary::mimeData() 只产 item-model 自定义
//  mime（application/media-item，节点图/节点内时间轴识别）——时间轴侧
//  无法识别。本子类重写 startDrag：**以模型 mimeData() 为基础再附加
//  媒体导入 mime**——两条拖拽链并存：
//    1) application/media-item（标签+文件名+剪贴板模板）→ 节点图
//       CustomGraphicsView / TimeLineNodeView 按标签创建节点（原有链，
//       2026-11-15 下午实证被首版 startDrag 覆盖丢失，此处恢复）；
//    2) application/x-flow-media-import（绝对路径）→ 时间轴
//       isMimeAcceptable → dropEvent → onDropMediaFile 导入/绑定/时长
//       联动。【2026-11-20 通道收口】不再附加 urls——urls 与资源管理器
//       直拖同一通道无法区分来源，时间轴已只认自有 mime（媒体必须先
//       入库再拖出，资源管理器直拖被 dragEnter 拒收）。
//
//  组节点（顶层分类）filePathForIndex 返回空且模型 mimeData 为空 →
//  不启动拖拽（只可点选）。只产 CopyAction：拖拽不修改媒体库数据。
// ============================================================================

#pragma once

#include <QDrag>
#include <QMimeData>
#include <QTreeView>

#include "Common/AppConfig/MediaImportMimeUtil.hpp"
#include "MediaLibrary/MediaLibrary.h"

class MediaLibraryTree : public QTreeView {
public:
    // 无 Q_OBJECT：纯虚函数重写（startDrag）无需元对象系统；且本类为
    // header-only、不在主程序 SOURCES 清单——AUTOMOC 不扫此头，带
    // Q_OBJECT 会产生无 moc 产物的 LNK2019（2026-11-15 实证）。
    using QTreeView::QTreeView;

protected:
    /// 拖拽启动：模型 mimeData（media-item，节点图链）+ 媒体导入 mime（时间轴链）
    void startDrag(Qt::DropActions supportedActions) override {
        auto* lib = qobject_cast<MediaLibrary*>(model());
        if (!lib) {
            QTreeView::startDrag(supportedActions);
            return;
        }
        const QModelIndexList selected = selectionModel()
                                             ? selectionModel()->selectedIndexes()
                                             : QModelIndexList();
        if (selected.isEmpty())
            return;

        // 1) 原链：模型 mimeData（application/media-item + 剪贴板节点模板）。
        //    组节点等无效项模型内部已过滤（mimeData 返回无格式空对象）。
        //    MediaLibrary::mimeData 声明为 private 覆写——经 QAbstractItemModel
        //    public 虚接口调用（QTreeView 默认 startDrag 亦走此通道）。
        QMimeData* mime = static_cast<QAbstractItemModel*>(lib)->mimeData(selected);

        // 2) 时间轴链：附加自有媒体导入 mime（单选语义：只取第一个带
        //    路径的文件项）。【2026-11-20 收口】不再产 urls。
        for (const QModelIndex& idx : selected) {
            const QString path = lib->filePathForIndex(idx);
            if (!path.isEmpty()) {
                MediaImportMime::encode(mime, path);
                break; // 单选语义：只拖一个
            }
        }

        // 空 mime（纯组节点选中）：不启动拖拽（保持点击行为不变）
        if (mime->formats().isEmpty()) {
            delete mime;
            return;
        }

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mime);
        // 拖拽预览 = 条目自身图标（分类图标，与库内视觉一致）
        const QVariant deco = selected.first().data(Qt::DecorationRole);
        if (deco.canConvert<QPixmap>())
            drag->setPixmap(deco.value<QPixmap>());
        drag->exec(Qt::CopyAction, Qt::CopyAction);
    }
};
