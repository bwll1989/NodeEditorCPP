//
// Created by bwll1 on 2024/5/27.
//

#include "TimeLineWidget.hpp"

#include <QColor>
#include <QPalette>
#include <QScrollBar>

#include "DefaultTimeLineToolBar.h"

namespace {

/**
 * 依据当前调色板（亮/暗背景）生成 QSplitter 把手的样式，保证不同主题下的对比度一致。
 * 参数：palette 当前控件调色板
 * 返回：可直接传入 setStyleSheet 的样式字符串
 */
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
        if (!m_model) {
            return;
        }
        m_model->setModelAlias("timeline/master");
        if (m_view && m_view->m_toolbar) {
            if (auto* tb = dynamic_cast<TimeLineToolBar*>(m_view->m_toolbar)) {
                tb->bindBus(m_model->getModelAlias());
            }
        }
        auto refreshTimeline = [this]() {
            if (m_view) {
                m_view->onUpdateViewport();
            }
        };

        auto refreshTracklistFull = [this]() {
            if (m_tracklist) {
                m_tracklist->onUpdateViewport();
            }
        };

        auto refreshBothFull = [this, refreshTimeline, refreshTracklistFull]() {
            refreshTimeline();
            refreshTracklistFull();
        };

        connect(m_model, &QAbstractItemModel::modelReset, this, refreshBothFull);

        connect(m_model, &QAbstractItemModel::rowsInserted,
                this, [refreshTimeline, refreshTracklistFull](const QModelIndex& parent, int, int) {
                    if (parent.isValid()) {
                        refreshTimeline();
                        return;
                    }
                    refreshTimeline();
                    refreshTracklistFull();
                });

        connect(m_model, &QAbstractItemModel::rowsRemoved,
                this, [refreshTimeline, refreshTracklistFull](const QModelIndex& parent, int, int) {
                    if (parent.isValid()) {
                        refreshTimeline();
                        return;
                    }
                    refreshTimeline();
                    refreshTracklistFull();
                });

        connect(m_model, &QAbstractItemModel::rowsMoved,
                this, [refreshTimeline, refreshTracklistFull](const QModelIndex& sourceParent,
                                                             int,
                                                             int,
                                                             const QModelIndex& destinationParent,
                                                             int) {
                    if (sourceParent.isValid() || destinationParent.isValid()) {
                        refreshTimeline();
                        return;
                    }
                    refreshTimeline();
                    refreshTracklistFull();
                });

        connect(m_model, &QAbstractItemModel::dataChanged,
                this, [this, refreshTimeline](const QModelIndex& topLeft,
                                             const QModelIndex&,
                                             const QList<int>& roles) {
                    refreshTimeline();

                    if (!m_tracklist) {
                        return;
                    }

                    if (topLeft.parent().isValid()) {
                        return;
                    }

                    const bool rolesUnknown = roles.isEmpty();
                    const bool affectsTrackName = rolesUnknown || roles.contains(QtTimeline::TrackNameRole) || roles.contains(QtTimeline::TrackTypeRole);
                    if (affectsTrackName) {
                        m_tracklist->viewport()->update();
                    }
                });

        connect(m_model, &BaseTimeLineModel::S_LengthChanged, this, refreshTimeline);

        connect(m_model, &BaseTimeLineModel::S_playheadMoved,
                this, [this, refreshTimeline](int) {
                    refreshTimeline();
                    if (m_tracklist) {
                        m_tracklist->viewport()->update();
                    }
                });
    }

    void bindSelection()
    {
        if (!m_selection) {
            return;
        }

        connect(m_selection, &QItemSelectionModel::currentChanged,
                this, [this](const QModelIndex&, const QModelIndex&) {
                    if (m_tracklist) {
                        m_tracklist->viewport()->update();
                    }
                });
    }

    void bindScroll()
    {
        if (!m_view || !m_tracklist) {
            return;
        }

        connect(m_tracklist->verticalScrollBar(), &QScrollBar::valueChanged,
                this, [this](int value) {
                    if (!m_view || m_isSyncingScroll) return;
                    if (m_view->verticalScrollBar()->value() == value) return;

                    m_isSyncingScroll = true;
                    m_view->verticalScrollBar()->setValue(value);
                    m_isSyncingScroll = false;
                });

        connect(m_view->verticalScrollBar(), &QScrollBar::valueChanged,
                this, [this](int value) {
                    if (!m_tracklist || m_isSyncingScroll) return;
                    if (m_tracklist->verticalScrollBar()->value() == value) return;

                    m_isSyncingScroll = true;
                    m_tracklist->verticalScrollBar()->setValue(value);
                    m_isSyncingScroll = false;
                });
    }

    BaseTimeLineModel* m_model {nullptr};
    BaseTimelineView* m_view {nullptr};
    BaseTracklistView* m_tracklist {nullptr};
    QItemSelectionModel* m_selection {nullptr};
    bool m_isSyncingScroll {false};
};

} // namespace

TimelineWidget::TimelineWidget(TimeLineModel* model, QWidget *parent) : QWidget(parent), model(model) {
    createComponents();

}

TimelineWidget::~TimelineWidget() =default;

void TimelineWidget::showSettingsDialog()
{
    settingsDialog->show();
}

QJsonObject TimelineWidget::save() {
    const auto& saved = model->save();

    return model->save();
}

void TimelineWidget::load(const QJsonObject& json) {

    model->load(json);

    if (view && view->m_toolbar) {
        if (auto* tb = dynamic_cast<TimeLineToolBar*>(view->m_toolbar)) {
            //加载完成后发布状态
            tb->publishCurrentState();
        }
    }
}

void TimelineWidget::createComponents() {
    view = new TimeLineView(model, this);
    tracklist = new TrackListView(model, this);

    view->setProperty("qtimeline_managed", true);
    tracklist->setProperty("qtimeline_managed", true);

    m_sharedSelectionModel = new QItemSelectionModel(model, this);
    tracklist->setSelectionModel(m_sharedSelectionModel);
    view->setSelectionModel(m_sharedSelectionModel);

    if (m_bindings) {
        m_bindings->deleteLater();
        m_bindings = nullptr;
    }

    toolbar = new TimeLineToolBar(view);
    view->initToolBar(toolbar);

    mainlayout = new QVBoxLayout(this);
    mainlayout->setContentsMargins(0, 0, 0, 0);
    mainlayout->setSpacing(0);

    auto* mainwidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(mainwidget);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(tracklist);
    splitter->addWidget(view);
    splitter->setHandleWidth(0);
    splitter->setStyleSheet(buildSplitterHandleStyle(splitter->palette()));
    splitter->setMouseTracking(true);
    splitter->setSizes({100, 900});

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(splitter);

    mainlayout->addWidget(mainwidget);

    settingsDialog = new TimeLineSetting(model, this);

    m_bindings = new TimelineBindingsController(model, view, tracklist, m_sharedSelectionModel, this);

    connect(dynamic_cast<TimeLineToolBar*>(view->m_toolbar), &TimeLineToolBar::settingsClicked, this, &TimelineWidget::showSettingsDialog);
}

