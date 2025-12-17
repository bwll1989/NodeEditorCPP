//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineView.h"
#include "DefaultTimeLineToolBar.h"
TimeLineView::TimeLineView(TimeLineModel* model, QWidget *parent) : BaseTimelineView(model, parent)
{
    //将时间游标的显示与模型中的时钟相绑定
    auto* derivedModel = dynamic_cast<TimeLineModel*>(getModel());
    // 连接时间码生成器信号
    // 使用lambda表达式来处理时间码变化信号
    connect(derivedModel->getClock(), &TimeLineClock::currentFrameChanged,
        [this](int frame) {
            viewport()->update();  // 触发viewport的更新
        });

    setAcceptDrops(true);
}
TimeLineView::~TimeLineView() = default;

void TimeLineView::movePlayheadToFrame(int frame)
{
    if (auto* derivedModel = dynamic_cast<TimeLineModel*>(getModel()))
    {

        getModel()->onSetPlayheadPos(frame);
        viewport()->update();
    }
}


void TimeLineView::mouseMoveEvent(QMouseEvent *event)
{
    // 处理右键事件
    if (event->button() == Qt::RightButton) {
        QAbstractItemView::mouseMoveEvent(event);
        return;
    }

    // 处理鼠标按住拖动的情况
    if (mouseHeld) {
        handleMouseDrag(event);
        return;
    }

    // 处理鼠标悬停状态
    updateMouseHoverState(event);
    updateCursorShape();

    QAbstractItemView::mouseMoveEvent(event);
}


QString TimeLineView::isMimeAcceptable(const QMimeData *Mime) const
{
    // 检查是否为 OSC 地址
    if (Mime->hasFormat("application/x-osc-address"))
    {
        return "Trigger";
    }
    // 检查是否为自定义媒体库条目
    if (Mime->hasFormat("application/media-item")) {
        const QByteArray raw = Mime->data("application/media-item");
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "Invalid media-item mime JSON:" << parseError.errorString();
            return "";
        }
        const QJsonObject obj = doc.object();
        const QString tag  = obj.value("tag").toString();
        const QString name = obj.value("name").toString();

        if (tag.isEmpty() || name.isEmpty()) {
            qWarning() << "media-item missing required fields: tag/name";
            return "";
        }

       if(tag=="DMX") return "Artnet";
       if (tag=="Video" || tag=="Image" || tag=="Audio") return tag;
    }
    return "";
}

/**
 * 函数：TimeLineView::dropEvent
 * 作用：处理媒体拖拽到时间线后的新剪辑创建，并构造嵌套的 OSC 消息 JSON：
 *       "messages": { "messages": [ {..value:"1"}, {..value:""} ] }
 * 关键点：
 *  - 正确读取/更新 QJsonObject 与 QJsonArray（取出 -> 修改 -> 写回）
 *  - 生成两条消息（开始播放与清空值），符合期望格式
 */
void TimeLineView::dropEvent(QDropEvent *event)
{
    if(m_isSupportMedia){
        m_lastDragPos = event->position().toPoint();
        QModelIndex trackIndex;
        QRect rullerRect(-m_scrollOffset.x(),0,viewport()->width() + m_scrollOffset.x(),rulerHeight);
        /* If above or on the ruler drop on the first track*/
        if(m_lastDragPos.y()<0 || rullerRect.contains(m_lastDragPos)){
            if(getModel()->rowCount()>0)
                trackIndex = model()->index(0, 0);
        }else{
            /* Find track at drop point */
            for(int i = 0; i < getModel()->rowCount(); i++){
                if (visualRect(getModel()->index(i, 0)).contains(m_lastDragPos)){
                    trackIndex = getModel()->index(i,0);
                }
            }
        }
        /* If dropped out side of tracks */
        if(!trackIndex.isValid()){
            return;
        }
        int pos = pointToFrame(m_lastDragPos.x());

        getModel()->onAddClip(trackIndex.row(),pos);
        viewport()->update();
        if(event->mimeData()->hasFormat("application/x-osc-address")){
            QDataStream stream(event->mimeData()->data("application/x-osc-address"));
            QString address, host,type;
            QVariant value;
            int port;
            stream >>  host >> port >> address >>type>>value;
            QJsonObject msg;
                msg["address"] = address;
                msg["host"]    = host.isEmpty() ? "127.0.0.1" : host;
                msg["port"]    = port == 0 ? 8991 : port;
                msg["type"]    = type.isEmpty() ? "Int" : type;
                msg["value"]   = value.isValid() ? value.toString() : "1";
            getModel()->getTracks()[trackIndex.row()]->clips.back()->setMedia(msg);
            return;
        }
        if(event->mimeData()->hasFormat("application/media-item")){
            const QByteArray raw = event->mimeData()->data("application/media-item");
            QJsonParseError parseError;
            const QJsonDocument mime = QJsonDocument::fromJson(raw, &parseError);
            if (parseError.error != QJsonParseError::NoError || !mime.isObject()) {
                qWarning() << "Invalid media-item mime JSON:" << parseError.errorString();
                return;
            }
            const QJsonObject obj = mime.object();
            const QString name = obj.value("name").toString();
            getModel()->getTracks()[trackIndex.row()]->clips.back()->setMedia(name);

        }
            
    }else{
        QAbstractItemView::dropEvent(event);
    }
}

void TimeLineView::initToolBar(BaseTimelineToolbar *toolbar)
{
    this->m_toolbar = toolbar;
    this->m_toolbar->setFixedHeight(toolbarHeight-4);
    // 设置工具栏位置
     this->m_toolbar->move(0, 2);
    // 连接工具栏播放按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(this->m_toolbar), &DefaultTimeLineToolBar::playClicked, [this]() {
        getModel()->onStartPlay();
    });
    // 连接工具栏停止按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::stopClicked, [this]() {
        getModel()->onStopPlay();
    });
    // 连接工具栏暂停按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::pauseClicked, [this]() {
        getModel()->onPausePlay();
    });
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar),&DefaultTimeLineToolBar::prevFrameClicked,[this](){
       getModel()->onSetPlayheadPos(qMax(0,getModel()->getPlayheadPos()-1));
    });
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar),&DefaultTimeLineToolBar::nextFrameClicked,[this](){
        getModel()->onSetPlayheadPos(getModel()->getPlayheadPos()+1);
    });
    // 连接移动剪辑按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::moveClipClicked, [this](int dx) {
        moveSelectedClip(dx,0,false);
    });
    // 连接删除剪辑按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::deleteClipClicked, [this]() {
        if(selectionModel()->selectedIndexes().isEmpty())
            return;

        QModelIndex currentIndex = selectionModel()->currentIndex();
        getModel()->onDeleteClip(currentIndex);

        // 清除选择并发送 nullptr
        selectionModel()->clearSelection();
        emit currentClipChanged(nullptr);

        viewport()->update();
    });

    // 连接放大按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::zoomInClicked, [this]() {
        currentScale = currentScale * 1.1;

                // 允许达到 0.0，从而触达 setScale 的 minTimescale
        currentScale = qBound(0.0, currentScale, 1.0);
       setScale(currentScale);
    });
    // 连接缩小按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::zoomOutClicked, [this]() {
        currentScale = currentScale * 0.9;

        // 允许达到 0.0，从而触达 setScale 的 minTimescale
        currentScale = qBound(0.0, currentScale, 1.0);
        setScale(currentScale);
    });
    connect( dynamic_cast<TimeLineModel*>(getModel())->getClock(), &TimeLineClock::timecodePlayingChanged,
       dynamic_cast<DefaultTimeLineToolBar*>(this->m_toolbar),&DefaultTimeLineToolBar::setPlaybackState);

    // 连接循环按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::loopToggled, [this](bool loop) {
        dynamic_cast<TimeLineModel*>(getModel())->onSetLoop(loop);
    });
    connect( dynamic_cast<TimeLineModel*>(getModel())->getClock(), &TimeLineClock::loopingChanged,
       dynamic_cast<DefaultTimeLineToolBar*>(this->m_toolbar),&DefaultTimeLineToolBar::setLoopState);
}
