#ifndef MEDIA_IMPORT_MIME_UTIL_H
#define MEDIA_IMPORT_MIME_UTIL_H

// 函数说明：媒体库 → 时间轴"媒体导入"拖拽 mime（application/x-flow-media-import）
//          的统一编解码——全工程唯一实现（2026-11-20 拖拽通道收口）。
// 实现说明：header-only inline（不产生跨 DLL 符号，任何链接方 include 即用，
//          无需新增 target 依赖）。此前媒体库拖拽与资源管理器直拖共用标准
//          urls 通道（QMimeData::urls），时间轴无法区分拖拽来源；收口后
//          媒体库拖拽产自有 mime 携带路径，时间轴只认此通道 + OSC 地址
//          通道，资源管理器直拖 urls 一并拒绝（媒体必须先入库再拖出）。

#include <QMimeData>
#include <QString>

namespace MediaImportMime {

// mime 类型标识（生产端 MediaLibraryTree::startDrag / 消费端
// BaseTimelineView::isMimeAcceptable 的识别契约）
inline constexpr const char* kMimeType = "application/x-flow-media-import";

// 编码（生产端）：携带媒体文件绝对路径（单选语义，路径即唯一载荷）
inline void encode(QMimeData* mimeData, const QString& filePath)
{
    mimeData->setData(kMimeType, filePath.toUtf8());
}

// 解码（消费端）：mime 不含该类型返回空 QString
inline QString decodePath(const QMimeData* mimeData)
{
    if (!mimeData || !mimeData->hasFormat(kMimeType))
        return {};
    return QString::fromUtf8(mimeData->data(kMimeType));
}

} // namespace MediaImportMime

#endif // MEDIA_IMPORT_MIME_UTIL_H
