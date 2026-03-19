//
// Created by bwll1 on 2024/5/27.
//

#include "TimeLineNodeWidget.hpp"

#include <QColor>
#include <QPalette>
#include <QScrollBar>
#include <QItemSelectionModel>

namespace {

static QString buildSplitterHandleStyle(const QPalette& palette)
{
    const QColor base = palette.color(QPalette::Window);
    const bool isDark = base.lightness() < 128;

    const QColor handle = isDark ? base.lighter(135) : base.darker(110);
    const QColor border = isDark ? base.lighter(165) : base.darker(125);
    const QColor hover = isDark ? handle.lighter(112) : handle.darker(108);
    const QColor pressed = isDark ? handle.lighter(125) : handle.darker(118);

    return QStringLiteral(
               "QSplitter::handle { background-color: %1; border-left: 1px solid %2; border-right: 1px solid %2; } "
               "QSplitter::handle:hover { background-color: %3; } "
               "QSplitter::handle:pressed { background-color: %4; }")
        .arg(handle.name(QColor::HexRgb),
             border.name(QColor::HexRgb),
             hover.name(QColor::HexRgb),
             pressed.name(QColor::HexRgb));
}

class TimelineBindingsController final : public QObject {
public:
    TimelineBindingsController(BaseTimeLineModel* model,
                              BaseTimelineView* view,
                              BaseTracklistView* tracklist,
                              QItemSelectionModel* selection,
                              QObject* parent)
        : QObject(parent)
        , m_model(model)
        , m_view(view)
        , m_tracklist(tracklist)
        , m_selection(selection)
    {
        bindModel();
        bindSelection();
        bindScroll();
    }

private:
    void bindModel()
    {
        if (!m_model) return;

        auto refreshTimeline = [this]() {
            if (m_view) m_view->onUpdateViewport();
        };
        auto refreshTracklistFull = [this]() {
            if (m_tracklist) m_tracklist->onUpdateViewport();
        };

        connect(m_model, &QAbstractItemModel::modelReset, this, [=]{ refreshTimeline(); refreshTracklistFull(); });
        connect(m_model, &QAbstractItemModel::rowsInserted, this,
                [=](const QModelIndex& parent, int, int){ parent.isValid()? void(refreshTimeline()) : (void(refreshTimeline()), void(refreshTracklistFull())); });
        connect(m_model, &QAbstractItemModel::rowsRemoved, this,
                [=](const QModelIndex& parent, int, int){ parent.isValid()? void(refreshTimeline()) : (void(refreshTimeline()), void(refreshTracklistFull())); });
        connect(m_model, &QAbstractItemModel::rowsMoved, this,
                [=](const QModelIndex& sp, int, int, const QModelIndex& dp, int){ (sp.isValid()||dp.isValid())? void(refreshTimeline()) : (void(refreshTimeline()), void(refreshTracklistFull())); });

        connect(m_model, &QAbstractItemModel::dataChanged, this,
                [=](const QModelIndex& topLeft, const QModelIndex&, const QList<int>& roles){
                    refreshTimeline();
                    if (!m_tracklist) return;
                    if (topLeft.parent().isValid()) return;
                    const bool rolesUnknown = roles.isEmpty();
                    const bool affectsTrackName = rolesUnknown || roles.contains(QtTimeline::TrackNameRole) || roles.contains(QtTimeline::TrackTypeRole);
                    if (affectsTrackName) m_tracklist->viewport()->update();
                });

        connect(m_model, &BaseTimeLineModel::S_LengthChanged, this, [=](qint64){ refreshTimeline(); });
        connect(m_model, &BaseTimeLineModel::S_playheadMoved, this, [=](int){ refreshTimeline(); if (m_tracklist) m_tracklist->viewport()->update(); });
    }

    void bindSelection()
    {
        if (!m_selection) return;
        connect(m_selection, &QItemSelectionModel::currentChanged, this, [=](const QModelIndex&, const QModelIndex&){ if (m_tracklist) m_tracklist->viewport()->update(); });
    }

    void bindScroll()
    {
        if (!m_view || !m_tracklist) return;
        connect(m_tracklist->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int v){ if(m_isSyncing) return; m_isSyncing=true; m_view->verticalScrollBar()->setValue(v); m_isSyncing=false; });
        connect(m_view->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int v){ if(m_isSyncing) return; m_isSyncing=true; m_tracklist->verticalScrollBar()->setValue(v); m_isSyncing=false; });
    }

    BaseTimeLineModel* m_model {nullptr};
    BaseTimelineView* m_view {nullptr};
    BaseTracklistView* m_tracklist {nullptr};
    QItemSelectionModel* m_selection {nullptr};
    bool m_isSyncing {false};
};

} // namespace

TimelineNodeWidget::TimelineNodeWidget(TimeLineNodeModel* model, QWidget *parent) : QWidget(parent), model(model) {
    createComponents();

}

TimelineNodeWidget::~TimelineNodeWidget() =default;

QJsonObject TimelineNodeWidget::save() {
    const auto& saved = model->save();

    return model->save();
}

void TimelineNodeWidget::load(const QJsonObject& json) {

    model->load(json);
}

void TimelineNodeWidget::createComponents() {
    view = new TimeLineNodeView(model, this);
    tracklist = new TrackListNodeView(model, this);

    view->setProperty("qtimeline_managed", true);
    tracklist->setProperty("qtimeline_managed", true);

    m_sharedSelectionModel = new QItemSelectionModel(model, this);
    tracklist->setSelectionModel(m_sharedSelectionModel);
    view->setSelectionModel(m_sharedSelectionModel);

    if (m_bindings) { m_bindings->deleteLater(); m_bindings = nullptr; }

    toolbar = new TimeLineNodeToolBar(view);
    view->initToolBar(toolbar);

    mainlayout = new QVBoxLayout(this);
    mainlayout->setContentsMargins(0, 0, 0, 0);
    mainlayout->setSpacing(0);

    auto* mainwidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(mainwidget);

    splitter = new QSplitter(Qt::Horizontal,this);
    splitter->addWidget(tracklist);
    splitter->addWidget(view);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(buildSplitterHandleStyle(splitter->palette()));
    splitter->setMouseTracking(true);
    splitter->setSizes({200,900});
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(splitter);

    mainlayout->addWidget(mainwidget);

    m_bindings = new TimelineBindingsController(model, view, tracklist, m_sharedSelectionModel, this);
}

