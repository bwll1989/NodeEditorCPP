//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineView.h"
#include "TimeLineToolbar.h"

#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

AbstractClipModel* takeNewlyAddedClip(BaseTimeLineModel* model, const QList<int>& countsBefore)
{
    if (!model)
        return nullptr;
    const QList<TrackData*> tracks = model->getTracks();
    for (int i = 0; i < tracks.size(); ++i) {
        TrackData* track = tracks.at(i);
        const int before = i < countsBefore.size() ? countsBefore.at(i) : 0;
        if (track && track->clips.size() > before)
            return track->clips.back();
    }
    return nullptr;
}

void applyDroppedMedia(AbstractClipModel* clip, const QMimeData* mime)
{
    if (!clip || !mime)
        return;
    if (mime->hasFormat("application/x-osc-address")) {
        QDataStream stream(mime->data("application/x-osc-address"));
        QString address;
        QString host;
        QString type;
        QVariant value;
        int port = 0;
        stream >> host >> port >> address >> type >> value;
        QJsonObject msg;
        msg["address"] = address;
        msg["host"] = host.isEmpty() ? QStringLiteral("127.0.0.1") : host;
        msg["port"] = port == 0 ? 8991 : port;
        msg["type"] = type.isEmpty() ? QStringLiteral("Int") : type;
        msg["value"] = value.isValid() ? value.toString() : QStringLiteral("1");
        clip->setMedia(msg);
        return;
    }
    if (!mime->hasFormat("application/media-item"))
        return;
    QJsonParseError parseError;
    const QJsonDocument mimeDoc = QJsonDocument::fromJson(mime->data("application/media-item"), &parseError);
    if (parseError.error != QJsonParseError::NoError || !mimeDoc.isObject()) {
        qWarning() << "Invalid media-item mime JSON:" << parseError.errorString();
        return;
    }
    clip->setMedia(mimeDoc.object().value("name").toString());
}

} // namespace

QString TimeLineView::isMimeAcceptable(const QMimeData *Mime) const
{
    if (!Mime)
        return {};
    if (Mime->hasFormat("application/x-osc-address"))
        return QStringLiteral("Trigger");
    if (Mime->hasFormat("application/media-item")) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(Mime->data("application/media-item"), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "Invalid media-item mime JSON:" << parseError.errorString();
            return {};
        }
        const QJsonObject obj = doc.object();
        const QString tag = obj.value("tag").toString();
        const QString name = obj.value("name").toString();
        if (tag.isEmpty() || name.isEmpty()) {
            qWarning() << "media-item missing required fields: tag/name";
            return {};
        }
        if (tag == QLatin1String("DMX"))
            return QStringLiteral("Artnet");
        if (tag == QLatin1String("Video") || tag == QLatin1String("Image") || tag == QLatin1String("Audio"))
            return tag;
        return {};
    }
    return BaseTimelineView::isMimeAcceptable(Mime);
}

void TimeLineView::dropEvent(QDropEvent *event)
{
    QList<int> countsBefore;
    if (BaseTimeLineModel* timeline = getModel()) {
        const QList<TrackData*> tracks = timeline->getTracks();
        countsBefore.reserve(tracks.size());
        for (TrackData* track : tracks)
            countsBefore.append(track ? track->clips.size() : 0);
    }
    BaseTimelineView::dropEvent(event);
    applyDroppedMedia(takeNewlyAddedClip(getModel(), countsBefore), event ? event->mimeData() : nullptr);
}

void TimeLineView::initToolBar(BaseTimelineToolbar *toolbar)
{
    BaseTimelineView::initToolBar(toolbar);
    auto* bar = dynamic_cast<TimeLineToolBar*>(m_toolbar);
    if (!bar)
        return;

    connect(bar, &TimeLineToolBar::playClicked, this, [this] {
        getModel()->onStartPlay();
    });
    connect(bar, &TimeLineToolBar::stopClicked, this, [this] {
        getModel()->onStopPlay();
    });
    connect(bar, &TimeLineToolBar::pauseClicked, this, [this] {
        getModel()->onPausePlay();
    });
    connect(bar, &TimeLineToolBar::prevFrameClicked, this, [this] {
        getModel()->onSetPlayheadPos(qMax(0, static_cast<int>(getModel()->getPlayheadPos()) - 1));
    });
    connect(bar, &TimeLineToolBar::nextFrameClicked, this, [this] {
        getModel()->onSetPlayheadPos(static_cast<int>(getModel()->getPlayheadPos()) + 1);
    });
    connect(bar, &TimeLineToolBar::moveClipClicked, this, [this](int dx) {
        moveSelectedClip(dx, 0, false);
    });
    connect(bar, &TimeLineToolBar::deleteClipClicked, this, [this] {
        if (!selectionModel() || selectionModel()->selectedIndexes().isEmpty())
            return;
        getModel()->onDeleteClip(selectionModel()->currentIndex());
        selectionModel()->clearSelection();
        emit currentClipChanged(nullptr);
    });
    connect(bar, &TimeLineToolBar::zoomInClicked, this, [this] {
        currentScale = qBound(0.0, currentScale * 1.1, 1.0);
        setScale(currentScale);
    });
    connect(bar, &TimeLineToolBar::zoomOutClicked, this, [this] {
        currentScale = qBound(0.0, currentScale * 0.9, 1.0);
        setScale(currentScale);
    });

    auto* timeline = dynamic_cast<TimeLineModel*>(getModel());
    if (!timeline || !timeline->getClock())
        return;
    connect(timeline->getClock(), &TimeLineClock::timecodePlayingChanged, bar, &TimeLineToolBar::setPlaybackState);
    connect(timeline->getClock(), &TimeLineClock::loopingChanged, bar, &TimeLineToolBar::setLoopState);
    connect(bar, &TimeLineToolBar::loopToggled, this, [timeline](bool loop) {
        timeline->onSetLoop(loop);
    });
}
