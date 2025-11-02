//
// Created by WuBin on 2025/6/3.
//

#include "TrackListNodeView.h"
#include "TimeCodeDefines.h"
TrackListNodeView::TrackListNodeView(TimeLineNodeModel* model, QWidget *parent) : BaseTracklistView(model, parent)
{

    // 连接时间码生成器信号
    // 使用lambda表达式来处理时间码变化信号
    connect(model->getClock(), &TimeLineNodeClock::currentFrameChanged,
        [this](int frame) {
            viewport()->update();  // 触发viewport的更新
        });
    // this->layout()->setContentsMargins(2, 0, 2, 2);
}
TrackListNodeView::~TrackListNodeView() = default;

/**
 * @brief 绘制事件
 */
void TrackListNodeView::drawTitle(QPainter *painter) {

    painter->setBrush(QBrush(bgColour));
    painter->drawRect(0,0,viewport()->width(),rulerHeight+toolbarHeight);

    QRect ruler(0,  0, viewport()->width(), rulerHeight+toolbarHeight);
    painter->setPen(fontColor);
    QFont font;
    font.setPixelSize(fontSize);
    painter->setFont(font);
    if (auto* derivedModel = dynamic_cast<TimeLineNodeModel*>(getModel()))
    {
        if(getModel()->getTimeDisplayFormat()==TimedisplayFormat::TimeCodeFormat){
            // 需要将基类指针转换为派生类指针以访问getClock函数
            painter->drawText(ruler,Qt::AlignCenter,QString("%1:%2:%3.%4").arg(derivedModel->getClock()->getCurrentTimecode().hours)
                                                                        .arg(derivedModel->getClock()->getCurrentTimecode().minutes)
                                                                        .arg(derivedModel->getClock()->getCurrentTimecode().seconds)
                                                                        .arg(derivedModel->getClock()->getCurrentTimecode().frames));
        }else{
            painter->drawText(ruler,Qt::AlignCenter, derivedModel->getClock()->getCurrentAbsoluteTime());
        }
    }
    painter->restore();  // 恢复状态
}