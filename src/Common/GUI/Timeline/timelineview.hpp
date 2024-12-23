#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QAbstractItemView>
#include "clipdelegate.hpp"
#include <unordered_map>
#include "timelinetypes.h"
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QFileInfo>
#include <QMimeData>
#include "timelinemodel.hpp"
#include "timelinestyle.hpp"
#include "trackdelegate.hpp"
#include "clipmodel.hpp"
#include "QTimer"

//cache rect if needed;
struct Clip{
    int start;          //    clip开始
    int originalStart;  //clip原始开始
    int end;            //clip 结束
    int originalEnd;    //clip 原始结束
    int track;
    // 轨道索引
    QString name;
    Clip() : start(0), originalStart(0), end(0), originalEnd(0), track(0), name("") {}

    Clip(int pos,int in, int out,int track,QString n) : start(pos),originalStart(in),end(out),track(track), originalEnd(out),name(n)
    {}
};

class TimelineView : public QAbstractItemView
{
    Q_OBJECT
public:
    TimelineView(QWidget *parent = nullptr)
    {
        horizontalScrollBar()->setSingleStep(10);
        horizontalScrollBar()->setPageStep(100);
        verticalScrollBar()->setSingleStep(trackHeight);
        verticalScrollBar()->setPageStep(trackHeight * 5);
        viewport()->setMinimumHeight(trackHeight + rulerHeight);
        setMinimumHeight(trackHeight + rulerHeight);
        setSelectionMode(SingleSelection);
        setSelectionBehavior(SelectItems);
        setEditTriggers(EditTrigger::AllEditTriggers);
        setItemDelegateForColumn(0,new TrackDelegate);
        setItemDelegateForRow(0,new TrackDelegate);
        setItemDelegate(new TrackDelegate);
        setAutoScroll(true);
        setAutoScrollMargin(5);
        setMouseTracking(true);
        setAcceptDrops(true);
        connect(timer, &QTimer::timeout, this, &TimelineView::onTimeout);
        installEventFilter(this);
    }

    void setModel(QAbstractItemModel *model) override
    {

        QAbstractItemView::setModel(model);

        clipMap.clear();
        for(int i=0;i<this->model()->rowCount();i++){
            QModelIndex trackIndex = model->index(i, 0);
            for(int j = 0; j<this->model()->rowCount(trackIndex);j++){
                QModelIndex clipIndex = this->model()->index(j,0,trackIndex);
                int clipIn = this->model()->data(clipIndex,TimelineRoles::ClipInRole).toInt();
                int clipOut = this->model()->data(clipIndex,TimelineRoles::ClipOutRole).toInt();
                int clipPos = this->model()->data(clipIndex,TimelineRoles::ClipPosRole).toInt();
                QString title=this->model()->data(clipIndex,TimelineRoles::ClipTitle).toString();
                Clip* clip = new Clip(clipPos,clipIn,clipOut,i,title);
                clipMap[clipIndex.internalId()] = clip;

            }
        }

    }

    QRect visualRect(const QModelIndex &index) const override
    {

        return itemRect(index).translated(-m_scrollOffset);
    }

    void scrollTo(const QModelIndex &index, ScrollHint hint) override{};

    QModelIndex indexAt(const QPoint &point) const override
    {

        QModelIndex index;
        QModelIndex parent;

        QRect rullerRect(-m_scrollOffset.x(),0,viewport()->width() + m_scrollOffset.x(),rulerHeight);
        if(point.y()<0)//if above the ruler
            return index;
        if(rullerRect.contains(point)){
            return index;
        }


        int columnIndex = model()->columnCount()-1;
        for(int i = 0; i < model()->rowCount(); i++){

            if (visualRect(model()->index(i, columnIndex,parent)).contains(point))
            {
                index = model()->index(i,columnIndex,parent);
                parent = index;
                //selection priority should happen in reverse of paint order
                for(int j=model()->rowCount(index)-1; j>=0; j--){

                    if(visualRect(model()->index(j, columnIndex,index)).contains(point)){
                        return model()->index(j,columnIndex,index);
                        //return index;
                    }

                }

//            qDebug()<< "IndexAt Track: " << i;
                return index;
            }
        }
//    qDebug()<< "IndexAt Track: " << -1;
        index = ((TimelineModel*)model())->createFakeIndex();
        if(!index.isValid()){
//        qDebug()<<"dont worry still invalid";
        }
        return index;
    }

private:

    std::unordered_map<quint64 ,Clip*> clipMap;

    Clip* getClipFromMap(quint64 id) const{
        const auto clipIter = clipMap.find(id);
        return clipMap.at(id);
    }/**/

    QRect itemRect(const QModelIndex &index) const
    {
        int trackwidth = getTrackWdith() + 500;//500 is for extra width outside of current timeline length;
        //if track
        if(index.parent()==QModelIndex()){
            return QRect(0, (index.row() * trackHeight) + rulerHeight, trackwidth, trackHeight);
        }
            //if Clip
        else{

            Clip* clip = clipMap.at(index.internalId()) ;

            int in = clip->originalStart;
            int out = clip->end +1;
            int pos = frameToPoint(clip->start);
            int track = clip->track;


            int length = frameToPoint(out-in);


            QPoint topLeft(pos,track*trackHeight + rulerHeight);

            return QRect(topLeft.x(),topLeft.y(),length,trackHeight);

        }
        return QRect();
    }

    QPoint m_scrollOffset;

    QTimer *timer = new QTimer(this);

    int pointToFrame(int point) const
    {
        return point/timescale ;
    }

    int frameToPoint(int frame) const
    {
        return frame*timescale;
    }

    void cutClip()
    {
        QModelIndexList list = selectionModel()->selectedIndexes();
        if(list.isEmpty())
            return;

        //IF SELECTED IS NOT A CLIP JUST IN CASE
        if(!list[0].isValid() || !list[0].parent().isValid())
            return;

        int playheadPos = pointToFrame(getPlayheadPos());
        int clipIn = model()->data(list[0],TimelineRoles::ClipInRole).toInt();
        int clipOut = model()->data(list[0],TimelineRoles::ClipOutRole).toInt();
        int clipPos = model()->data(list[0],TimelineRoles::ClipPosRole).toInt();

        int clipLength = clipOut - clipIn ;

        if(playheadPos> clipPos  && playheadPos<=clipLength + clipPos){
            ((TimelineModel*)model())->cutClip(list[0],playheadPos);
        }

        Clip* clip = getClipFromMap(list[0].internalId());

        clip->end = model()->data(list[0],TimelineRoles::ClipOutRole).toInt();

        viewport()->update();;

    }

    ClipDelegate clipDelegate;

    QPoint m_mouseStart;
    QPoint m_mouseEnd;
    QPoint m_mouseOffset;
    bool mouseHeld = false;
    bool m_playheadSelected = false;
    bool m_isDroppingMedia = false;
    QPoint m_lastDragPos;
    hoverState m_mouseUnderClipEdge = NONE;

    QModelIndex m_hoverIndex = QModelIndex();

    int getTrackWdith() const
    {
        return frameToPoint(model()->data(QModelIndex(),TimelineRoles::TimelineLengthRole).toInt() + 1);

    }

    int getPlayheadPos()
    {
        return frameToPoint(((TimelineModel*)model())->getPlayheadPos());
    }

    //this function moves clips by frame, not viewport pixels
    void moveSelectedClip(int dx, int dy,bool isMouse = true)
    {

        QModelIndexList list = selectionModel()->selectedIndexes();
        if(list.isEmpty())
            return;
        int newPos;
        if(isMouse){
            newPos = dx;
        }else{
            newPos = model()->data(list[0],TimelineRoles::ClipPosRole).toInt() + dx;
        }


        model()->setData(list[0],newPos,TimelineRoles::ClipPosRole);
        Clip* c = getClipFromMap(list[0].internalId());
        c->start = model()->data(list[0],TimelineRoles::ClipPosRole).toInt();



        if(isMouse && indexAt(m_mouseEnd)!=list[0].parent()){
            int newTrack = ((TimelineModel*)model())->moveClipToTrack(list[0],indexAt(m_mouseEnd));
            if(newTrack!=-1)
                c->track = newTrack;

        }else if(!isMouse && dy!=0){
            QModelIndex nextTrack;
            if( dy>0 && list[0].parent().row()==model()->rowCount()-1){
                nextTrack = ((TimelineModel*)model())->createFakeIndex();
            }else{
                nextTrack = list[0].parent().siblingAtRow(list[0].parent().row()+dy);
            }
            int newTrack =((TimelineModel*)model())->moveClipToTrack(list[0],nextTrack);
            if(newTrack!=-1)
                c->track = newTrack;
        }


        ((TimelineModel*)model())->reCalculateLength();
        updateScrollBars();


    }

    void movePlayheadToFrame(int frame)
    {
        ((TimelineModel*)model())->setPlayheadPos(frame);
    }

    bool isVideoFile(const QString& filePath)
    {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();
        QStringList supportedVideoFormats = {"mp4", "mkv", "mov"}; // Add more video formats as needed

        return supportedVideoFormats.contains(suffix);
    }

signals:

    void scrolled(int dx,int dy);

public slots:
    void timelinestart(){
        if(timer->isActive()){
            timer->stop();
        }else{
            timer->start(40);
        }


};

    void onTimeout()
    {

        ((TimelineModel*)model())->setPlayheadPos( ((TimelineModel*)model())->getPlayheadPos()+1);
        this->viewport()->update();
    }

    void scroll(int dx, int dy){    m_scrollOffset -= QPoint(dx, dy);
        QAbstractItemView::scrollContentsBy(dx, dy);}

    void updateScrollBars()
    {
        if (!model())
            return;
        int max = 0;
        max = getTrackWdith() -  viewport()->width();
        horizontalScrollBar()->setRange(0, max);
        verticalScrollBar()->setRange(0, model()->rowCount() * trackHeight + rulerHeight - viewport()->height());
    }

    void scrollContentsBy(int dx, int dy) override
    {
        m_scrollOffset -= QPoint(dx, dy);
        QAbstractItemView::scrollContentsBy(dx, dy);
        emit scrolled(dx,dy);
    }

    void setScale(double value)
    {


        // int focusFrame=((TimelineModel*)model())->getPlayheadPos();
        // int oldPointFocus = frameToPoint(focusFrame);

        // 设置新的缩放比例
        timescale = value;

        // // 计算缩放后的焦点在点坐标系中的位置
        // int newPointFocus = frameToPoint(focusFrame);

        // // 计算旧焦点和新焦点的差异
        // int diff = oldPointFocus - newPointFocus;

        // qDebug() << "old point focus: " << oldPointFocus << " new point focus: " << newPointFocus;
        // qDebug() << "scroll offset before: " << m_scrollOffset.x() << " diff: " << diff;

        // // 调整滚动偏移以保持缩放焦点一致
        // if (m_scrollOffset.x() + diff < 0) {
        //     diff = -m_scrollOffset.x();
        // }
        // scrollContentsBy(-diff, 0);

        // qDebug() << "scroll offset after: " << m_scrollOffset.x();

        // 更新滚动条
        updateScrollBars();

        // 更新视口
        viewport()->update();
    }

    void addClipToMap(int row,int track)
    {
        QModelIndex clipIndex = model()->index(row,0,model()->index(track, 0));
        int clipIn = this->model()->data(clipIndex,TimelineRoles::ClipInRole).toInt();
        int clipOut = this->model()->data(clipIndex,TimelineRoles::ClipOutRole).toInt();
        int clipPos = this->model()->data(clipIndex,TimelineRoles::ClipPosRole).toInt();
        Clip* clip = new Clip(clipPos,clipIn,clipOut,track,"adc");

        clipMap[clipIndex.internalId()] = clip;
    }

    void TrackMoved(int src,int dest)
    {


        auto it = clipMap.begin();
        if (src > dest) {
            while (it != clipMap.end()) {
                if (it->second->track >= dest && it->second->track < src)
                    it->second->track++;
                else if (it->second->track == src)
                    it->second->track = dest;
                it++;
            }
        } else {
            while (it != clipMap.end()) {
                if (it->second->track > src && it->second->track <= dest)
                    it->second->track--;
                else if (it->second->track == src)
                    it->second->track = dest;
                it++;
            }
        }

    }

protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override {return QModelIndex();}

    int horizontalOffset() const override
    {return 0;}

    int verticalOffset() const override
    {return 0;}

    bool isIndexHidden(const QModelIndex &index) const override
    { return false; }

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override{}

    QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}

    void paintEvent(QPaintEvent *event) override
    {
        int trackwidth = getTrackWdith();
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(QBrush(bgColour));
        painter.drawRect(event->rect());
        painter.setPen(seperatorColour);

        //draw tracks
        for (int i = 0; i < model()->rowCount(); ++i){

            int trackSplitter = (i+1) * trackHeight + rulerHeight -  m_scrollOffset.y();


            painter.fillRect(QRect(0,(i * trackHeight)+ rulerHeight -m_scrollOffset.y(),
                                   trackwidth,trackHeight),
                             fillColour);

            painter.drawLine(0, trackSplitter, event->rect().width(), trackSplitter);

        }


        // draws the vertical lines
        painter.setPen(seperatorColour);
        int lineheight = model()->rowCount() * trackHeight + rulerHeight;
        int frameStep;

        if (timescale > 70) {
            frameStep = 1;// Draw text at every frame
        } else if (timescale > 30) {
            frameStep = 10;  // Draw text every 10 frames
        } else if (timescale > 5) {
            frameStep = 25; // Draw text every 25 frames
        } else {
            frameStep = 50; // Draw text every 50 frames
        }

        int startMarker = static_cast<int>(pointToFrame(m_scrollOffset.x()))*timescale+1 ;
        int endMarker = event->rect().width() + m_scrollOffset.x();
        startMarker = pointToFrame(startMarker);
        startMarker-= (startMarker%frameStep);
        startMarker = frameToPoint(startMarker);

        for (int i = startMarker; i<endMarker; i += timescale*frameStep)
        {
            painter.drawLine(i - m_scrollOffset.x(), std::max(rulerHeight, event->rect().top()),
                             i - m_scrollOffset.x(), lineheight);
        }


        //draw clips

        QStyleOptionViewItem option;
        QAbstractItemView::initViewItemOption(&option);
        for(int i=0;i<model()->rowCount();i++){
            // Start timer before painting

            QModelIndex trackIndex = model()->index(i, 0);

            for(int j = 0; j<model()->rowCount(trackIndex);j++){

                QModelIndex clipIndex = model()->index(j,0,trackIndex);
                if(selectionModel()->isSelected(clipIndex)){
                    option.state |= QStyle::State_Selected;
                }else{
                    option.state &= ~QStyle::State_Selected;
                }
                if(m_hoverIndex==clipIndex){
                    option.state |= QStyle::State_MouseOver;
                }else{
                    option.state &= ~QStyle::State_MouseOver;
                }
                //option.text = QString(QString::number(model()->data(clipIndex,TimelineModel::ClipInRole).toInt()) + "-" + QString::number(model()->data(clipIndex,TimelineModel::ClipOutRole).toInt()));

                option.rect = visualRect(clipIndex);


                clipDelegate.paint(&painter,option,clipIndex);

            }

        }


        //draw ruler

        painter.setBrush(QBrush(bgColour));
        painter.drawRect(-m_scrollOffset.x(),0,event->rect().width() + m_scrollOffset.x(),rulerHeight);

        painter.setPen(rulerColour);


        static int jump = 1;
        if(  baseTimeScale%timescale == 0){
            jump =  baseTimeScale/timescale;
        }

        for(int i = startMarker;i < endMarker; i+=timescale*frameStep){

            int number = pointToFrame(i);
            QString text     = tr("%1").arg(number);
            QRect   textRect = painter.fontMetrics().boundingRect(text);

            textRect.translate(-m_scrollOffset.x(), 0);
            textRect.translate(i - textRect.width() / 2, rulerHeight - textoffset);

            painter.drawLine(i  - m_scrollOffset.x(),rulerHeight - textoffset +5,
                             i  - m_scrollOffset.x(),rulerHeight);

            painter.drawText(textRect, text);
        }

        //draw inbetween
        for(int i = startMarker;i < endMarker; i+=timescale){
            int number = pointToFrame(i);
            int boost = 0 ;

            if (timescale > 20) {
                break;  // Draw text at every frame
            } else if (timescale > 10) {
                if(number%5==0)
                    continue;// skip every 5th marker

            } else if (timescale > 5) {
                if(number%10==0)
                    continue; //skip every 10th marker

                if(number%5==0)
                    boost =5; //hilight every 5th
            } else {
                if(number%25==0)
                    continue; //skip every 25th marker

                if(number%5==0)
                    boost =5; //hilight every 5th

            }
//
            painter.drawLine(i  - m_scrollOffset.x(),rulerHeight - textoffset +10 -boost ,
                             i  - m_scrollOffset.x(),rulerHeight);
        }



        //draw playhead

        QPoint kite[5]{
                QPoint(0,0),QPoint(-playheadwidth,-playheadCornerHeight),QPoint(-playheadwidth,-playheadheight),QPoint(playheadwidth,-playheadheight),QPoint(playheadwidth,-playheadCornerHeight)
        };
        int playheadPos = frameToPoint(((TimelineModel*)model())->getPlayheadPos()) -m_scrollOffset.x();
        for(QPoint &p:kite){
            p.setX(p.x()+playheadPos);
            p.setY(p.y()+rulerHeight);
        }
        painter.setBrush(Qt::white);
        painter.drawConvexPolygon(kite,5);
        painter.setPen(Qt::white);
        painter.drawLine(QPoint(playheadPos,rulerHeight),QPoint(playheadPos,viewport()->height()));

    }

    void mousePressEvent(QMouseEvent *event) override
    {
        m_mouseStart = event->pos();
        m_mouseEnd = m_mouseStart;
        mouseHeld = true;
        m_playheadSelected = false;
        m_hoverIndex = QModelIndex();
        selectionModel()->clearSelection();

        int playheadPos = frameToPoint(((TimelineModel*)model())->getPlayheadPos());
        QRect playheadHitBox(QPoint(playheadPos-3,rulerHeight),QPoint(playheadPos+2,viewport()->height()));
        QRect playheadHitBox2(QPoint(playheadPos-playheadwidth,-playheadheight + rulerHeight),QPoint(playheadPos+playheadwidth,rulerHeight));

        if(playheadHitBox.contains(m_mouseStart)||playheadHitBox2.contains(m_mouseStart)){
            m_playheadSelected = true;
            return QAbstractItemView::mousePressEvent(event);
        }

        m_playheadSelected = false;


        QModelIndex item = indexAt(event->pos());
        selectionModel()->clearSelection();

        //item pressed was a clip
        if(item.parent().isValid()){
            selectionModel()->select(item,QItemSelectionModel::Select);
            m_mouseOffset.setX(frameToPoint(getClipFromMap(item.internalId())->start) - m_mouseStart.x());

        }
        if(selectionModel()->selectedIndexes().isEmpty()){
            movePlayheadToFrame(pointToFrame(std::max(0,m_mouseEnd.x()+m_scrollOffset.x())));
            viewport()->update();
        }

        QAbstractItemView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if(mouseHeld){
            m_mouseEnd = event->pos();
            if(m_mouseUnderClipEdge!=hoverState::NONE && !selectionModel()->selectedIndexes().isEmpty()){
                QModelIndex clip = selectedIndexes().at(0);

                Clip* c = clipMap.at(clip.internalId());
                int in = c->originalStart;
                int out = c->end;
                int pos = c->start;
                int length = c->originalEnd;
                if(m_mouseUnderClipEdge==hoverState::LEFT){
                    int newIn = std::clamp(in+ pointToFrame(m_mouseEnd.x() + m_scrollOffset.x()) - pos,0,out);
                    model()->setData(clip,newIn,TimelineRoles::ClipInRole);
                    //clamp to prevent clip moveing when resizing
                    moveSelectedClip(std::clamp(pointToFrame(m_mouseEnd.x() + m_scrollOffset.x()) - pos,-in,out-in),0+0,false);
                    Clip* c = getClipFromMap(clip.internalId());
                    c->originalStart = model()->data(clip,TimelineRoles::ClipInRole).toInt();
                    viewport()->update();
                }else if(m_mouseUnderClipEdge==hoverState::RIGHT){
                    //clamped to not go over clipin or src media length
//                限制拉伸
//               int newOut = std::clamp(out + pointToFrame(m_mouseEnd.x() + m_scrollOffset.x()) - pos+in-out,in,length);

                    int newOut = out + pointToFrame(m_mouseEnd.x() + m_scrollOffset.x()) - pos+in-out;
                    model()->setData(clip,newOut,TimelineRoles::ClipOutRole);
                    Clip* c = getClipFromMap(clip.internalId());
                    c->end = model()->data(clip,TimelineRoles::ClipOutRole).toInt();
                    viewport()->update();
                }

            }else if(!selectionModel()->selectedIndexes().isEmpty()&&m_mouseEnd.x()>=0){
                moveSelectedClip(pointToFrame(m_mouseEnd.x()+m_mouseOffset.x()),m_mouseEnd.y()+m_mouseOffset.y());
                viewport()->update();
            }else{

                movePlayheadToFrame(pointToFrame(std::max(0,m_mouseEnd.x() + m_scrollOffset.x())));
                viewport()->update();
            }

            if(m_playheadSelected){

                movePlayheadToFrame(pointToFrame(std::max(0,m_mouseEnd.x() + m_scrollOffset.x())));
                viewport()->update();
            }
            return QAbstractItemView::mouseMoveEvent(event);
        }

        QPoint pos = event->pos();
        m_hoverIndex = indexAt(event->pos());
        QRect rect = visualRect(m_hoverIndex);
        m_mouseUnderClipEdge = hoverState::NONE;
        //5 is hitbox size + -5px
        //see if item is a clip
        if((m_hoverIndex.isValid() && m_hoverIndex.parent().isValid())){
            if(abs(pos.x() - rect.left())<=5){
                m_mouseUnderClipEdge=hoverState::LEFT;
            }else if(abs(pos.x() - rect.right())<=5){
                m_mouseUnderClipEdge=hoverState::RIGHT;
            }

        }
        if (m_mouseUnderClipEdge != hoverState::NONE) {
            setCursor(Qt::SizeHorCursor);
        }else {
            unsetCursor();
        }

        QAbstractItemView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        mouseHeld = false;
        m_playheadSelected = false;
        m_mouseEnd = event->pos();


        //pressed outside of selection
        QAbstractItemView::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        QAbstractItemView::mouseDoubleClickEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        mouseHeld = false;
        selectionModel()->clear();
        m_hoverIndex = QModelIndex();
        QAbstractItemView::leaveEvent(event);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {

        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            QModelIndexList list = selectionModel()->selectedIndexes();

            TimelineModel* timelinemodel = (TimelineModel*)model();
            if (watched == this) {
                switch (keyEvent->key()){
                    case Qt::Key_Right:
                        if(list.isEmpty())
                            movePlayheadToFrame(timelinemodel->getPlayheadPos()+1);
                        moveSelectedClip(1,0,false);
                        viewport()->update();
                        return true;  // 表示事件已处理，不再向其他控件传递
                    case Qt::Key_Left:
                        if(list.isEmpty())
                            movePlayheadToFrame(timelinemodel->getPlayheadPos()-1);
                        moveSelectedClip(-1,0,false);
                        viewport()->update();
                        return true;  // 表示事件已处理，不再向其他控件传递
                    case Qt::Key_Down :
                        moveSelectedClip(0,1,false);
                        break;
                    case Qt::Key_Up:
                        moveSelectedClip(0,-1,false);
                        return true;  // 表示事件已处理，不再向其他控件传递
                    case Qt::Key_S:
                        cutClip();
                        return true;  // 表示事件已处理，不再向其他控件传递
                    case Qt::Key_I:
                        timelinemodel->addClip(1,timelinemodel->getPlayheadPos(),timelinemodel->getPlayheadPos()+100,"dsad");
                        viewport()->update();
                        return true;  // 表示事件已处理，不再向其他控件传递
                    case Qt::Key_Delete:
                        if(list.isEmpty())
                            break;
                        timelinemodel->deleteClip(list[0]);
                        clipMap.erase(list[0].internalId());
                        clearSelection();
                        return true;  // 表示事件已处理，不再向其他控件传递
                    defualt:
                        break;
                }

            }
        }
        return QObject::eventFilter(watched, event);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        updateScrollBars();
        QAbstractItemView::resizeEvent(event);
    }

    void showEvent(QShowEvent *event) override
    {
        updateScrollBars();
        QAbstractItemView::showEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        /* TODO */
        /* Open media and get info of streams */
        /* On Drop Create clips form Streams */
        if (event->mimeData()->hasUrls())
        {
            bool hasVideoFiles = false;
            QList<QUrl> urls = event->mimeData()->urls();
            for (const QUrl& url : urls)
            {
                if (isVideoFile(url.toLocalFile()))
                {
                    hasVideoFiles = true;
                    break;
                }
                /* TODO */
                /* add support for dropping mutliple items at once*/
                /* first item goes at drop position */
                /* all subsequent items postions are after the end of then previouse item */
                break;
            }

            if (hasVideoFiles)
            {
                m_isDroppingMedia = true;
                m_lastDragPos = event->position().toPoint();
                event->acceptProposedAction();
            }
        }else{
            QAbstractItemView::dragEnterEvent(event);
        }


    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        if(m_isDroppingMedia){
            m_lastDragPos = event->position().toPoint();
            event->acceptProposedAction();
        }else{
            QAbstractItemView::dragMoveEvent(event);
        }
    }

    void dropEvent(QDropEvent *event) override
    {
        if(m_isDroppingMedia){
            m_lastDragPos = event->position().toPoint();
            QModelIndex trackIndex;
            TimelineModel* timelineModel = ((TimelineModel*)model());
            QRect rullerRect(-m_scrollOffset.x(),0,viewport()->width() + m_scrollOffset.x(),rulerHeight);
            /* If above or on the ruler drop on the first track*/
            if(m_lastDragPos.y()<0 || rullerRect.contains(m_lastDragPos)){
                if(timelineModel->rowCount()>0)
                    trackIndex = model()->index(0, 0);
            }else{
                /* Find track at drop point */
                for(int i = 0; i < timelineModel->rowCount(); i++){
                    if (visualRect(timelineModel->index(i, 0)).contains(m_lastDragPos)){
                        trackIndex = timelineModel->index(i,0);
                    }
                }
            }
            /* If dropped out side of tracks */
            if(!trackIndex.isValid()){
                trackIndex = timelineModel->createFakeIndex();
            }
            int pos = pointToFrame(m_lastDragPos.x());


            viewport()->update();

        }else{
            QAbstractItemView::dropEvent(event);
        }
    }

protected slots:

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override
    {

        QAbstractItemView::selectionChanged(selected,deselected);
        viewport()->update();
    }
};

#endif // TIMELINEVIEW_H
