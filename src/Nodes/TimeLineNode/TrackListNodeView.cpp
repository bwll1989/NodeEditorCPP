//
// Created by WuBin on 2025/6/3.
//

#include "TrackListNodeView.h"
#include "TimeCodeDefines.h"

void TrackListNodeView::drawTitle(QPainter *painter)
{
    if (!painter)
        return;
    painter->save();
    painter->setBrush(QBrush(trackBgColor()));
    const QRect ruler(0, 0, viewport()->width(), rulerHeight + toolbarHeight);
    painter->drawRect(ruler);
    painter->setPen(fontColor);
    QFont font;
    font.setPixelSize(fontSize);
    painter->setFont(font);
    if (auto* derivedModel = dynamic_cast<TimeLineNodeModel*>(getModel())) {
        if (getModel()->getTimeDisplayFormat() == TimedisplayFormat::TimeCodeFormat) {
            const auto tc = derivedModel->getClock()->getCurrentTimecode();
            painter->drawText(ruler, Qt::AlignCenter,
                              QString("%1:%2:%3.%4").arg(tc.hours).arg(tc.minutes).arg(tc.seconds).arg(tc.frames));
        } else {
            painter->drawText(ruler, Qt::AlignCenter, derivedModel->getClock()->getCurrentAbsoluteTime());
        }
    }
    painter->restore();
}
