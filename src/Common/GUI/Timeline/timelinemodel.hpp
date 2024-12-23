#ifndef TIMELINEMODEL_H
#define TIMELINEMODEL_H

#include <QAbstractItemModel>
#include <unordered_set>
#include <unordered_map>
#include "trackmodel.hpp"
#include "clipmodel.hpp"
#include "trackmodel.hpp"
#include <string>
#include <QMimeData>
#include <QIODevice>
#include <algorithm>
#include <vector>
#include <QJsonArray>
#include "timelinetypes.h"
#include "timelinestyle.hpp"
class TimelineModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    TimelineModel(){
        createTrack(MediaType::AUDIO);
        createTrack(MediaType::VIDEO);
        createTrack(MediaType::CONTROL);
    };

    void addClip(int trackIndex, int in, int out,QString clipname=""){
        TrackModel* track;
        /* If fake index create new track. */
        if(!hasIndex(trackIndex,0)){
            createTrack();
        }

        track = m_tracks.at(trackIndex);
        if (!track) {
            return;
        }

        QModelIndex parentIndex = index(trackIndex, 0, QModelIndex());

        ClipModel* clip = new ClipModel(in,out,clipname,track,track->type());

        insertClip(clip,parentIndex);

        reCalculateLength();

        emit timelineUpdated();


        return;
    }

    void deleteClip(QModelIndex clipIndex){
        if(m_clipIDs.find(clipIndex.internalId())==m_clipIDs.cend())
            return;

        if(!clipIndex.isValid())
            return;

        beginRemoveRows(clipIndex.parent(),clipIndex.row(),clipIndex.row());
        quint64 id = clipIndex.internalId();
        {
            std::unordered_map<quint64, void*>::iterator it = m_idToObjectMap.begin();
            ClipModel* clip = (ClipModel*)FromID(clipIndex.internalId());

            m_tracks.at(clipIndex.parent().row())->removeClip(clip);

            auto clipsIt = std::find(m_clips.begin(),m_clips.end(),clip);
            if(clipsIt!=m_clips.end())
                m_clips.erase(clipsIt);

            delete clip;

            for(;it!=m_idToObjectMap.end();){
                if(it->first==id){
                    it = m_idToObjectMap.erase(it);
                    break;
                }
                else {
                    ++it;
                }
            }
        }


        for (auto it = m_clipIDs.begin(); it != m_clipIDs.end();) {
            if (*it == id) {
                it = m_clipIDs.erase(it);
                break;
            } else {
                ++it;
            }
        }


        endRemoveRows();
    }

    int moveClipToTrack(QModelIndex clipIndex,QModelIndex newTrackIndex){
        if(!clipIndex.isValid()){
            qDebug()<<"invalid clipindex when moving clip to track";
            return-1;
        }
        const QModelIndex parentTrackIndex = clipIndex.parent();

        if(!parentTrackIndex.isValid()){
            qDebug()<<"invalid parentTrackIndex when moving clip to track";
            return -1;
        }

        if(!newTrackIndex.isValid() ){
            return -1;

        }

        //if index is of a clip
        if(newTrackIndex.parent().isValid()){
            return -1;
        }
        TrackModel* parentTrack = (TrackModel*)FromID(parentTrackIndex.internalId());
        ClipModel* clip = (ClipModel*)FromID(clipIndex.internalId());


        //index is for a new track
        if(!hasIndex(newTrackIndex.row(),newTrackIndex.column(),newTrackIndex.parent())){
            if(newTrackIndex!=QModelIndex()){
                createTrack(clip->type());
                newTrackIndex = index(rowCount(QModelIndex())-1,columnCount(QModelIndex())-1,QModelIndex());
            }
        }else {
            TrackModel* newTrack = (TrackModel*)FromID(newTrackIndex.internalId());

            if(clip->type()!=newTrack->type()){
                return -1;
            }
        }

        int trackNumber =  newTrackIndex.row();


        TrackModel* newTrack = (TrackModel*)FromID(newTrackIndex.internalId());

        beginMoveRows(parentTrackIndex,clipIndex.row(),clipIndex.row(),newTrackIndex,rowCount(newTrackIndex));

        parentTrack->removeClip(clip);
        newTrack->addClip(clip);

        endMoveRows();

        emit timelineUpdated();
        emit underPlayhead(getUnderPlayhead());

        return trackNumber;

    }
    //re calcualtes the length of timeline in frames

    void reCalculateLength(){
        int max = 0;
        for(const ClipModel* clip : m_clips){
            max =  std::max(max,clip->out() - clip->in() + clip->pos());
        }
        m_length = max;

    }

    void cutClip(QModelIndex clipIndex, int cutFrame){
        //IF SELECTED IS NOT A CLIP JUST IN CASE
        if(!clipIndex.isValid() || !clipIndex.parent().isValid() )
            return;
        // calculate new in out and positions
        ClipModel* copy = new ClipModel(*(ClipModel*)FromID(clipIndex.internalId()));
        int offset = copy->pos();
        int originalIn = copy->in();
        int originalOut = copy->out();
        int length = copy->out() - copy->in();
        QString title=clipIndex.data(TimelineRoles::ClipTitle).value<QString>();
        int originalNewOut = cutFrame-offset-1  + originalIn;

        ((ClipModel*)FromID(clipIndex.internalId()))->setOut(originalNewOut);
        copy->setIn(originalNewOut+1);
        copy->setPos(cutFrame);
        copy->setName(title);
        QModelIndex parentIndex = clipIndex.parent();

        insertClip(copy,parentIndex);

        emit timelineUpdated();



    }

    void createTrack(MediaType type = MediaType::CONTROL){

        int rows = rowCount(QModelIndex());

        TrackModel* track = new TrackModel(rowCount(QModelIndex()),type);

        beginInsertRows(QModelIndex(),rows,rows); //zero indexed
        m_tracks.push_back(track);
        quint64 id = assignIdToTrack(track);
        endInsertRows();

        emit tracksChanged();
        emit timelineUpdated();
    }

    void* FromID(quint64 id) const{
        return m_idToObjectMap.at(id);
    }

    //function for creating fake index, used when creating a new track. one that isnt QModelIndex() but is still invalid
    QModelIndex createFakeIndex()
    {
        return createIndex(rowCount(QModelIndex()),columnCount(QModelIndex()),nullptr);
    }

    void movePlayhead(int dx){
        if(m_playheadPos>=0)
            m_playheadPos+=dx;
        if(m_playheadPos>m_length)
            m_playheadPos-=dx;
        if(m_playheadPos<0)
            m_playheadPos=0;

        qDebug()<<"frame: " <<m_playheadPos;
        emit playheadMoved(m_playheadPos);
        emit timelineUpdated();
    }

    int getPlayheadPos() const{
        return m_playheadPos;
    }

    void setPlayheadPos(int newPlayheadPos)
    {
        m_playheadPos = newPlayheadPos;

        emit playheadMoved(newPlayheadPos);
        emit timelineUpdated();
        emit underPlayhead(getUnderPlayhead());
    }

    /* gets a list*/
    std::vector<std::pair<const ClipModel*, int>> getUnderPlayhead()
    {
        std::vector<std::pair<const ClipModel*,int>> frameList;
        for(TrackModel* track : m_tracks){
            for(const ClipModel* clip : track->getClips()){
                if(m_playheadPos<clip->pos())
                    continue;
                if(m_playheadPos>clip->pos()+(clip->out()-clip->in()))
                    continue;
                int frame = m_playheadPos - clip->pos() + clip->in();

                frameList.emplace_back(std::make_pair(clip,frame));
            }
        }

        return frameList;
    }

    void moveTrack(QModelIndex track,QModelIndex dest){}

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {

        //not in model
        if (hasIndex(row, column, parent)==false) {
            return QModelIndex();
        }

        TrackModel* track;


        //track item
        if(parent.isValid()==false){
            TrackModel* track = m_tracks.at(row);
            if(track){
                quint64 trackID = findID(track);

                return createIndex(row, column,  quintptr(trackID));
            }

            qDebug("TimelineModel::index - could not make track at row: %i",row);
            return QModelIndex();
        }

        track = m_tracks.at(parent.row());
        if(!track){
            qDebug()<<"NOT VALID";
            return QModelIndex();
        }

        ClipModel* clip = track->getClips().at(row);

        if(clip){
            quint64 clipID = findID(clip);
            return createIndex(row, column, quintptr(clipID));
        }
        qDebug("CANT MAKE CLIP INDEX");

        return QModelIndex();
    }

    QString convertFramesToTimeString(int frames, double fps)
    {
        int totalMilliseconds = static_cast<int>((frames / fps) * 1000);
        int hours = totalMilliseconds / (1000 * 60 * 60);
        int minutes = (totalMilliseconds / (1000 * 60)) % 60;
        int seconds = (totalMilliseconds / 1000) % 60;
        int milliseconds = totalMilliseconds % 1000;

        return QString("%1:%2:%3.%4")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0'));
    }


    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid()) return {QModelIndex()};

        quint64 id = child.internalId();

        if(isTrack(id))
            return QModelIndex();

        if(isClip(id)){
            ClipModel* clip = (ClipModel*)(FromID(id));

            if (!clip) {
                qDebug("TimelineModel::parent - could not make child at row: %i",child.row());
                return QModelIndex();
            }
            TrackModel* track = findParentTrackOfClip(clip);

            if (!track) {
                // If the clip doesn't have an associated track, return an invalid QModelIndex.
                qDebug("TimelineModel::parent - could not make track at row: %i",0);
                return QModelIndex();
            }


            //Q_ASSERT(parentItem);
            return createIndex(findTrackRow(track), 0,quintptr(findID(track)));
        }


        return QModelIndex();


    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {

        TrackModel* track;
        // Case 1: Top-level items (tracks)
        if (!parent.isValid()) {
            return m_tracks.size();
        }


        //if parent of clip
        quint64 id = parent.internalId();

        if(isTrack(id)){
            track = (TrackModel*)FromID(id);
            if(!track){
                qDebug("NOT A TRAZK");
                return 1;
            }

            return track->getClips().size();
        }

        if(isClip(id)){
            return 0;
        }


        return 1;

    }

    int columnCount(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()){
            if(role==TimelineLengthRole)
                return QVariant::fromValue(m_length);
            return QVariant();
        }


        //if (role != Qt::DisplayRole)
        //    return QVariant();

        //needed due to fake index created by TimelineModel::indexAt, tiggered by tooltips etc
        if(!hasIndex(index.row(),index.column(),index.parent())){
            return QVariant();
        }

        if(!index.parent().isValid()){
            TrackModel* track;
            switch (role){
                case Qt::ToolTipRole:
                    track = (TrackModel*)FromID(index.internalId());
                    return QVariant::fromValue("track " + QString::number(index.row()));
                    break;
                case TrackNumberRole:
                    track = (TrackModel*)FromID(index.internalId());
                    return QVariant::fromValue(index.row());
                    return QVariant::fromValue("track " + QString::number(index.row()));
                    break;
                case TrackTypeRole:
                    track = (TrackModel*)FromID(index.internalId());
                    return QVariant::fromValue(track->type());
                    break;
                default:
                    return QVariant::fromValue(NULL);
            }

            /*if(role==Qt::ToolTipRole){
                track = (TrackModel*)FromID(index.internalId());
                return QVariant::fromValue("track " + QString::number(index.row()));
            }

            if(role ==TrackNumberRole){
                track = (TrackModel*)FromID(index.internalId());
                return QVariant::fromValue(index.row());
            }
            return QVariant();*/
        }
        ClipModel* clip;
        switch (role){
            case ClipInRole:
                clip = (ClipModel*)FromID(index.internalId());
                return QVariant::fromValue(clip->in());
                break;
            case ClipOutRole:
                clip = (ClipModel*)FromID(index.internalId());
                return QVariant::fromValue(clip->out());
                break;
            case ClipPosRole:
                clip = (ClipModel*)FromID(index.internalId());
                return QVariant::fromValue(clip->pos());
                break;
            case ClipLengthRole:
                clip = (ClipModel*)FromID(index.internalId());
                return QVariant::fromValue(clip->length());
                break;
            case ClipTypeRole:
                clip = (ClipModel*)FromID(index.internalId());
                return QVariant::fromValue(clip->type());
                break;
            case ClipTitle:
                clip = (ClipModel*)FromID(index.internalId());
                return QVariant::fromValue(clip->name());
                break;

            default:
                return QVariant::fromValue(NULL);
        }
        return QVariant::fromValue(NULL);
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {

        if (!index.isValid()){
            return false;
        }



        if(!index.parent().isValid()){
            TrackModel* track;
            if(role==Qt::ToolTipRole){
                track = (TrackModel*)FromID(index.internalId());
            }
            return true;
        }
        ClipModel* clip;
        switch (role){
            case ClipInRole:
                clip = (ClipModel*)FromID(index.internalId());
                clip->setIn(value.toInt());
                emit timelineUpdated();
                emit underPlayhead(getUnderPlayhead());
                return true;
                break;
            case ClipOutRole:
                clip = (ClipModel*)FromID(index.internalId());
                clip->setOut(value.toInt());
                emit timelineUpdated();
                emit underPlayhead(getUnderPlayhead());

                return true;
                break;
            case ClipPosRole:
                clip = (ClipModel*)FromID(index.internalId());
                clip->setPos(value.toInt());
                emit timelineUpdated();
                emit underPlayhead(getUnderPlayhead());
                return true;
                break;
            defualt:
                return false;;
        }

        return QAbstractItemModel::setData(index,value,role);
    }

    //bool insertRows(int row, int count, const QModelIndex &parent) override;

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[ClipInRole] = "clipIn";
        roles[ClipOutRole] = "clipOut";
        roles[ClipPosRole] = "clipPos";
        roles[ClipTypeRole] = "clipType";
        roles[ClipTitle] = "clipTitle";
        roles[TrackNumberRole] = "trackNumber";
        roles[TrackTypeRole] = "trackType";
        roles[SelectedRole] = "selected";
        roles[InRole] = "in";
        roles[OutRole] = "out";
        roles[TimelineLengthRole] = "timelineLength";


        return roles;
    }

    QStringList mimeTypes() const override
    {
        QStringList types;
        types << "application/vnd.track";
        return types;
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        QMimeData *mimeData = new QMimeData();
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::WriteOnly);

                foreach (const QModelIndex &index, indexes) {
                if (index.isValid()) {
                    //int track = data(index,TrackNumberRole ).toInt(); // Assuming tracks are stored as QString
                    int track = index.row();
                    stream << track;
                }
            }

        mimeData->setData("application/vnd.track", encodedData);
        return mimeData;
    }

    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override
    {
        Q_UNUSED(action);
        Q_UNUSED(row);
        Q_UNUSED(parent);

        if (!data->hasFormat("application/vnd.track"))
            return false;

        if (column > 0)
            return false;

        return true;

    }

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override
    {
        if (!canDropMimeData(data, action, row, column, parent))
            return false;

        if (action == Qt::IgnoreAction)
            return true;

        int newRow = parent.row();


        if(!parent.isValid()){
            newRow=0;
        }
        if(!hasIndex(parent.row(),parent.column(),parent.parent())){
            newRow = parent.row()-1;
        }

        QByteArray encodedData = data->data("application/vnd.track");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<int> newItems;
        int rows = 0;

        while (!stream.atEnd()) {
            int track;
            stream >> track;
            newItems << track;
            ++rows;
        }

        for (int &track : newItems) {

            if (track < 0 || track >= m_tracks.size() || newRow < 0 || newRow > m_tracks.size()) {
                return false;
            }

            if(track + 1 == newRow){
                std::swap(track,newRow);
            }

            if(track==newRow)
                return false;
            beginMoveRows(QModelIndex(),track,track,QModelIndex(),newRow);
            std::vector<TrackModel*> itemsToMove;
            itemsToMove.push_back(m_tracks.at(track));
            m_tracks.erase(m_tracks.begin()+ track);
            m_tracks.insert(m_tracks.begin() + newRow,itemsToMove.begin(),itemsToMove.end());
            endMoveRows();
            emit trackMoved(track,newRow);


        }
        return true;


    }

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        return QAbstractItemModel::flags(index) |= Qt::ItemIsDropEnabled;
        //return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }

    QJsonObject save() const
    {
        QJsonObject modelJson;
        QJsonArray jsonArray;

        for(auto const &index:m_tracks){
            jsonArray.append(index->save());
        }

//        sceneJson["nodes"] = nodesJsonArray;
        modelJson["track"] = jsonArray;
        return modelJson;
    }
public:

    std::vector<TrackModel*> m_tracks;

    std::vector<ClipModel*> m_clips;


signals:
    void timelineUpdated();

    void newClip(int row,int track);

    void trackMoved(int oldIndex,int newIndex);

    void playheadMoved(int frame);

    /* Signal contains list of clips along with the frame for each clip the playhead is on. */
    void underPlayhead(std::vector<std::pair<const ClipModel*, int>>);

    void tracksChanged();

private:
    //length of the timeline, grows automaticly with clips
    int m_length = 0;

    int m_playheadPos = 0;

    void insertClip(ClipModel* clip,QModelIndex parentTrackIndex){

        int rows = rowCount(parentTrackIndex);

        beginInsertRows(parentTrackIndex,rows,rows);
        m_clips.push_back(clip);
        m_tracks.at(parentTrackIndex.row())->addClip(clip);
        quint64 id = assignIdToClip(clip);

        endInsertRows();

        emit newClip(rows,parentTrackIndex.row());
    }

    TrackModel* findParentTrackOfClip(ClipModel* clip) const{
        if(!clip){
            qDebug("Clip not in Model");
        }
        if(!clip->Parent())
            qDebug("Clip HAS NO PARENT?!");
        for (TrackModel* track : m_tracks) {
            if(std::find(track->getClips().cbegin(),track->getClips().cend(),clip)!=track->getClips().cend()){

                return track;
            }
        }
        qDebug("returning null");
        return nullptr;
    }

    int findTrackRow(TrackModel* track) const{
        auto it = std::find(m_tracks.begin(),m_tracks.end(),track);

        return it-m_tracks.begin();
    }

    int findClipRow(TrackModel* track,ClipModel* clip) const
    {
        std::vector<ClipModel*> clips = track->getClips();
        auto it = std::find(clips.begin(),clips.end(),clip);
        return it-clips.begin();
    }

    quint64 assignIdToTrack(TrackModel* track){
        quint64 id = nextId++;
        m_idToObjectMap[id] = track;
        m_trackIDs.insert(id);
        return id;
    };

    quint64 assignIdToClip(ClipModel* clip){
        quint64 id = nextId++;
        m_idToObjectMap[id] = clip;
        m_clipIDs.insert(id);
        return id;
    };

    quint64 findID(void* object) const{
        auto it = m_idToObjectMap.cbegin();
        while(it!= m_idToObjectMap.cend()){
            if(it->second == object){
                return it->first;
            }
            it++;
        }
        return quint64(0);
    };

    bool isClip(quint64 id) const{
        return getType(id)==ItemType::Clip;
    }

    bool isTrack(quint64 id) const{
        return getType(id)==ItemType::Track;
    }

    ItemType getType(quint64 id) const{
        if(m_trackIDs.find(id) != m_trackIDs.end()){
            return ItemType::Track;
        }

        if(m_clipIDs.find(id) != m_clipIDs.end()){
            return ItemType::Clip;
        }

        return ItemType::Invalid;
    }

    std::unordered_map<quint64, void*> m_idToObjectMap;
    std::unordered_set<quint64> m_trackIDs;
    std::unordered_set<quint64> m_clipIDs;
    quint64 nextId = 1;


};

#endif // TIMELINEMODEL_H
