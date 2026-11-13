#ifndef OSC_ADDRESS_MIME_UTIL_H
#define OSC_ADDRESS_MIME_UTIL_H

// 函数说明：OSC 地址拖拽 mime（application/x-osc-address）的统一编解码与
//          拖拽预览绘制——全工程唯一实现（2026-11-12 合并三处重复生产端：
//          AbstractClipModel::startDrag / DefaultTimeLineToolBar::startDrag /
//          OSCMessageListWidget::startDrag；消费端 handleMimeDrop 同步收口）。
// 实现说明：header-only inline（不产生跨 DLL 符号，任何链接方 include 即用，
//          无需新增 target 依赖）。五元组字段序为消费端契约：
//          host >> port >> address >> type >> value——旧生产端 value/type
//          错位是历史 bug（QVariant≠QString 序列化错位），已修正，勿改回。

#include <QColor>
#include <QDataStream>
#include <QFont>
#include <QPainter>
#include <QPixmap>
#include <QMimeData>
#include <QSize>
#include <QVariant>
#include "OSCMessage.h"

namespace OscAddressMime {

// mime 类型标识（消费端 OSCMessageListWidget::handleMimeDrop 的识别契约）
inline constexpr const char* kMimeType = "application/x-osc-address";

// 五元组编码（生产端）。字段序 = 消费端读序，见文件头注释。
inline void encode(QMimeData* mimeData, const QString& host, int port,
                   const QString& address, const QString& type, const QVariant& value)
{
    QByteArray itemData;
    QDataStream stream(&itemData, QIODevice::WriteOnly);
    stream << host << port << address << type << value;
    mimeData->setData(kMimeType, itemData);
}

// 五元组解码（消费端）。mime 不含该类型返回 false 且不改写 out。
inline bool decode(const QMimeData* mimeData, OSCMessage& out)
{
    if (!mimeData || !mimeData->hasFormat(kMimeType))
        return false;
    QDataStream stream(mimeData->data(kMimeType));
    stream >> out.host >> out.port >> out.address >> out.type >> out.value;
    return true;
}

// 拖拽预览：半透明深灰圆角 + 白字地址文本（三处生产端统一样式）。
// leftPad 为文本左侧留白（列表卡片 8 / 控件拖拽 30，按视觉密度选择）。
inline QPixmap addressPixmap(const QString& address,
                             const QSize& hint = QSize(200, 30),
                             int leftPad = 30)
{
    QPixmap pixmap(hint);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(40, 40, 40, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect(), 5, 5);
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    painter.drawText(pixmap.rect().adjusted(leftPad, 0, -8, 0),
                     Qt::AlignVCenter | Qt::AlignLeft, address);
    return pixmap;
}

} // namespace OscAddressMime

#endif // OSC_ADDRESS_MIME_UTIL_H
