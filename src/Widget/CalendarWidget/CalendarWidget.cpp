#include "CalendarWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QShowEvent>
#include <QMimeData>
#include <QMenu>
#include <QTableView>
#include <QDataStream>
#include <QFontMetrics>
#include <QTimer>
#include <QAbstractItemView>
#include <QTextCharFormat>
#include <QModelIndex>
#include <algorithm>

namespace {

constexpr int kDayNumberHeight = 18;
constexpr int kChipHeight = 16;
constexpr int kChipGap = 2;
constexpr int kCellPad = 3;
constexpr int kMaxChips = 5;
const char kOscMime[] = "application/x-osc-address";
const QColor kAccent(0, 120, 212); // 与 QSS #0078d4 一致

/** 日历绘制用色，按亮/暗主题分支 */
struct ThemeColors {
    QColor cellBg;
    QColor outsideBg;
    QColor dayText;
    QColor weekendText;
    QColor selectionFill;
    QColor todayBorder;
    QColor chipOnceBg;
    QColor chipOnceFg;
    QColor chipLoopBg;
    QColor chipLoopFg;
    QColor chipActiveBg;
    QColor chipActiveFg;
    QColor moreText;
};

ThemeColors themeColors(const QPalette& pal)
{
    ThemeColors c;
    const bool dark = pal.color(QPalette::Base).lightness() < 128;

    if (dark) {
        c.cellBg = QColor(53, 53, 53);
        c.outsideBg = QColor(45, 45, 45);
        c.dayText = QColor(220, 220, 220);
        c.weekendText = QColor(180, 140, 140);
        c.selectionFill = QColor(0, 120, 212, 40);
        c.todayBorder = kAccent;
        c.chipOnceBg = QColor(55, 100, 140, 200);
        c.chipOnceFg = QColor(230, 240, 250);
        c.chipLoopBg = QColor(55, 120, 105, 200);
        c.chipLoopFg = QColor(225, 245, 238);
        c.chipActiveBg = kAccent;
        c.chipActiveFg = Qt::white;
        c.moreText = QColor(150, 150, 155);
    } else {
        c.cellBg = QColor(255, 255, 255);
        c.outsideBg = QColor(245, 245, 245);
        c.dayText = QColor(45, 45, 45);
        c.weekendText = QColor(160, 100, 100);
        c.selectionFill = QColor(0, 120, 212, 28);
        c.todayBorder = kAccent;
        c.chipOnceBg = QColor(227, 240, 250);
        c.chipOnceFg = QColor(30, 90, 140);
        c.chipLoopBg = QColor(228, 243, 236);
        c.chipLoopFg = QColor(40, 110, 90);
        c.chipActiveBg = kAccent;
        c.chipActiveFg = Qt::white;
        c.moreText = QColor(130, 130, 130);
    }
    return c;
}

} // namespace

OscCalendarWidget::OscCalendarWidget(ScheduledTaskModel* model, QWidget* parent)
    : QCalendarWidget(parent)
    , m_model(model)
{
    setAcceptDrops(true);
    setGridVisible(false);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    setNavigationBarVisible(true);
    setDateEditEnabled(false);
    setFirstDayOfWeek(Qt::Monday);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    applyWeekdayFormats();

    // 内部 QTableView 在构造后才可用，延迟挂接事件
    QTimer::singleShot(0, this, &OscCalendarWidget::ensureViewHooks);
}

OscCalendarWidget::~OscCalendarWidget() = default;

bool OscCalendarWidget::hasTasksOnDate(const QDate& date) const
{
    return m_model && !m_model->rowIndexesForDate(date).isEmpty();
}

void OscCalendarWidget::clearSelectedSourceRow()
{
    if (m_selectedSourceRow < 0) {
        return;
    }
    m_selectedSourceRow = -1;
    updateCells();
}

void OscCalendarWidget::refresh()
{
    updateCells();
}

void OscCalendarWidget::applyWeekdayFormats()
{
    // 覆盖 Qt 默认周末大红字
    const ThemeColors colors = themeColors(palette());
    QTextCharFormat weekendFmt;
    weekendFmt.setForeground(colors.weekendText);
    setWeekdayTextFormat(Qt::Saturday, weekendFmt);
    setWeekdayTextFormat(Qt::Sunday, weekendFmt);

    QTextCharFormat weekdayFmt;
    weekdayFmt.setForeground(colors.dayText);
    for (int d = Qt::Monday; d <= Qt::Friday; ++d) {
        setWeekdayTextFormat(static_cast<Qt::DayOfWeek>(d), weekdayFmt);
    }
}

void OscCalendarWidget::showEvent(QShowEvent* event)
{
    QCalendarWidget::showEvent(event);
    ensureViewHooks();
}

void OscCalendarWidget::ensureViewHooks()
{
    if (m_viewHooksInstalled) {
        return;
    }

    QTableView* view = calendarView();
    if (!view) {
        QTimer::singleShot(50, this, &OscCalendarWidget::ensureViewHooks);
        return;
    }

    // 默认 NoDragDrop，必须显式打开，否则拖放无效
    view->setDragDropMode(QAbstractItemView::DropOnly);
    view->setDefaultDropAction(Qt::CopyAction);
    view->setAcceptDrops(true);
    view->viewport()->setAcceptDrops(true);
    view->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
    view->viewport()->installEventFilter(this);

    connect(view->viewport(), &QWidget::customContextMenuRequested, this,
            [this, view](const QPoint& pos) {
                const QPoint globalPos = view->viewport()->mapToGlobal(pos);
                QDate date = dateAtGlobalPos(globalPos);
                if (!date.isValid()) {
                    date = selectedDate();
                }
                if (date.isValid()) {
                    showDateContextMenu(date, globalPos);
                }
            });

    m_viewHooksInstalled = true;
}

QTableView* OscCalendarWidget::calendarView() const
{
    return findChild<QTableView*>();
}

bool OscCalendarWidget::isOscMime(const QMimeData* mime)
{
    return mime && mime->hasFormat(QLatin1String(kOscMime));
}

OSCMessage OscCalendarWidget::oscFromMime(const QMimeData* mime)
{
    OSCMessage msg;
    if (!isOscMime(mime)) {
        return msg;
    }
    QByteArray data = mime->data(QLatin1String(kOscMime));
    QDataStream stream(data);
    stream >> msg.host >> msg.port >> msg.address >> msg.type >> msg.value;
    return msg;
}

QDate OscCalendarWidget::dateFromModelIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }

    QDate date = index.data(Qt::UserRole).toDate();
    if (date.isValid()) {
        return date;
    }

    // 兼容：个别环境下 UserRole 无日期时，用显示日 + 当前月推算
    const int day = index.data(Qt::DisplayRole).toInt();
    if (day > 0) {
        const QDate candidate(yearShown(), monthShown(), day);
        if (candidate.isValid()) {
            return candidate;
        }
    }
    return {};
}

QDate OscCalendarWidget::dateAtGlobalPos(const QPoint& globalPos) const
{
    QTableView* view = calendarView();
    if (!view || !view->model()) {
        return {};
    }
    const QPoint local = view->viewport()->mapFromGlobal(globalPos);
    return dateFromModelIndex(view->indexAt(local));
}

QRect OscCalendarWidget::cellRectInViewport(const QDate& date) const
{
    QTableView* view = calendarView();
    if (!view || !view->model() || !date.isValid()) {
        return {};
    }

    const QAbstractItemModel* model = view->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 0; col < model->columnCount(); ++col) {
            const QModelIndex idx = model->index(row, col);
            if (dateFromModelIndex(idx) == date) {
                return view->visualRect(idx);
            }
        }
    }
    return {};
}

int OscCalendarWidget::maxChipsForHeight(int cellHeight)
{
    const int available = cellHeight - kDayNumberHeight - kCellPad;
    return std::clamp((available + kChipGap) / (kChipHeight + kChipGap), 0, kMaxChips);
}

QString OscCalendarWidget::shortAddress(const QString& address)
{
    if (address.isEmpty()) {
        return QStringLiteral("(空)");
    }
    const int slash = address.lastIndexOf(QLatin1Char('/'));
    return (slash >= 0 && slash + 1 < address.size()) ? address.mid(slash + 1) : address;
}

OscCalendarWidget::ChipLayout OscCalendarWidget::buildChipLayout(const QDate& date,
                                                                const QRect& cellRect) const
{
    ChipLayout layout;
    if (!m_model || !cellRect.isValid()) {
        return layout;
    }

    const QVector<int> allRows = m_model->rowIndexesForDate(date);
    const int maxChips = maxChipsForHeight(cellRect.height());
    // 任务过多时预留一行显示 "+N"
    const bool needMore = allRows.size() > maxChips && maxChips > 0;
    const int showCount = needMore ? std::max(0, maxChips - 1)
                                   : std::min(static_cast<int>(allRows.size()), maxChips);

    layout.rows = allRows.mid(0, showCount);
    layout.overflow = allRows.size() - showCount;

    const int lineCount = layout.rows.size()
                          + (layout.overflow > 0 && maxChips > layout.rows.size() ? 1 : 0);
    int y = cellRect.top() + kDayNumberHeight;
    for (int i = 0; i < lineCount; ++i) {
        layout.rects.push_back(QRect(cellRect.left() + kCellPad,
                                     y,
                                     cellRect.width() - 2 * kCellPad,
                                     kChipHeight));
        y += kChipHeight + kChipGap;
    }
    return layout;
}

OscCalendarWidget::ChipHit OscCalendarWidget::hitTest(const QPoint& globalPos) const
{
    ChipHit hit;
    hit.date = dateAtGlobalPos(globalPos);
    if (!hit.date.isValid()) {
        return hit;
    }

    QTableView* view = calendarView();
    if (!view) {
        return hit;
    }

    const QRect cell = cellRectInViewport(hit.date);
    if (!cell.isValid()) {
        return hit;
    }

    const QPoint local = view->viewport()->mapFromGlobal(globalPos);
    const ChipLayout layout = buildChipLayout(hit.date, cell);
    for (int i = 0; i < layout.rows.size() && i < layout.rects.size(); ++i) {
        if (layout.rects[i].contains(local)) {
            hit.sourceRow = layout.rows[i];
            break;
        }
    }
    return hit;
}

void OscCalendarWidget::paintCell(QPainter* painter, const QRect& rect, QDate date) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const ThemeColors colors = themeColors(palette());
    const bool inMonth = (date.month() == monthShown() && date.year() == yearShown());
    const bool selected = (date == selectedDate());
    const bool today = (date == QDate::currentDate());

    painter->fillRect(rect, inMonth ? colors.cellBg : colors.outsideBg);
    if (!inMonth) {
        painter->restore();
        return;
    }

    if (selected) {
        painter->fillRect(rect.adjusted(1, 1, -1, -1), colors.selectionFill);
    }
    if (today) {
        painter->setPen(QPen(colors.todayBorder, 1.5));
        painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 4, 4);
    }

    // 日期数字
    painter->setPen(date.dayOfWeek() >= 6 ? colors.weekendText : colors.dayText);
    QFont dayFont = painter->font();
    dayFont.setBold(today || selected);
    painter->setFont(dayFont);
    painter->drawText(QRect(rect.left() + kCellPad, rect.top() + 1,
                            rect.width() - 2 * kCellPad, kDayNumberHeight),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      QString::number(date.day()));

    // 任务条（paintCell 的 rect 已是 viewport 坐标，与 hitTest 共用布局算法）
    const ChipLayout layout = buildChipLayout(date, rect);

    QFont chipFont = painter->font();
    chipFont.setBold(false);
    chipFont.setPointSize(std::max(8, chipFont.pointSize() - 1));
    painter->setFont(chipFont);
    const QFontMetrics fm(chipFont);

    for (int i = 0; i < layout.rows.size() && i < layout.rects.size(); ++i) {
        const int sourceRow = layout.rows[i];
        const QModelIndex idx = m_model->index(sourceRow, 0);
        const QTime t = idx.data(ScheduledTaskModel::RoleTime).toTime();
        const QString remarks = idx.data(ScheduledTaskModel::RoleRemarks).toString().trimmed();
        const QString label = remarks.isEmpty()
            ? shortAddress(idx.data(ScheduledTaskModel::RoleAddress).toString())
            : remarks;
        const bool isLoop = idx.data(ScheduledTaskModel::RoleScheduleType).toString()
                                .compare(QStringLiteral("loop"), Qt::CaseInsensitive) == 0;
        const bool active = (sourceRow == m_selectedSourceRow);

        QColor bg = colors.chipOnceBg;
        QColor fg = colors.chipOnceFg;
        if (active) {
            bg = colors.chipActiveBg;
            fg = colors.chipActiveFg;
        } else if (isLoop) {
            bg = colors.chipLoopBg;
            fg = colors.chipLoopFg;
        }

        const QRect chip = layout.rects[i];
        QPainterPath path;
        path.addRoundedRect(chip, 3, 3);
        painter->fillPath(path, bg);
        painter->setPen(fg);

        QString text = QStringLiteral("%1 %2")
                           .arg(t.isValid() ? t.toString(QStringLiteral("HH:mm")) : QStringLiteral("--:--"),
                                label);
        if (isLoop) {
            text.prepend(QStringLiteral("↻ "));
        }
        painter->drawText(chip.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                          fm.elidedText(text, Qt::ElideRight, chip.width() - 6));
    }

    if (layout.overflow > 0 && layout.rects.size() > layout.rows.size()) {
        painter->setPen(colors.moreText);
        painter->drawText(layout.rects[layout.rows.size()].adjusted(4, 0, -4, 0),
                          Qt::AlignVCenter | Qt::AlignLeft,
                          tr("+%1 项").arg(layout.overflow));
    }

    painter->restore();
}

bool OscCalendarWidget::eventFilter(QObject* watched, QEvent* event)
{
    QTableView* view = calendarView();
    if (!view || watched != view->viewport()) {
        return QCalendarWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (onViewportMousePress(static_cast<QMouseEvent*>(event))) {
            return true;
        }
        break;
    case QEvent::MouseButtonDblClick:
        if (onViewportMouseDoubleClick(static_cast<QMouseEvent*>(event))) {
            return true;
        }
        break;
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::Drop:
        if (onViewportDrag(event)) {
            return true;
        }
        break;
    default:
        break;
    }
    return false; // 未处理则交给 viewport 默认逻辑
}

bool OscCalendarWidget::onViewportMousePress(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return false;
    }

    const ChipHit hit = hitTest(event->globalPosition().toPoint());
    if (hit.date.isValid()) {
        setSelectedDate(hit.date);
    }

    if (hit.sourceRow >= 0) {
        m_selectedSourceRow = hit.sourceRow;
        updateCells();
        emit taskChipClicked(hit.sourceRow, event->globalPosition().toPoint());
        return true; // 吞掉事件，避免与表格默认选中逻辑冲突
    }

    if (m_selectedSourceRow >= 0) {
        m_selectedSourceRow = -1;
        updateCells();
    }
    return false;
}

bool OscCalendarWidget::onViewportMouseDoubleClick(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return false;
    }
    const ChipHit hit = hitTest(event->globalPosition().toPoint());
    if (hit.date.isValid() && hit.sourceRow < 0) {
        emit emptyDateDoubleClicked(hit.date);
        return true;
    }
    return false;
}

void OscCalendarWidget::showDateContextMenu(const QDate& date, const QPoint& globalPos)
{
    if (!date.isValid()) {
        return;
    }
    setSelectedDate(date);

    QMenu menu(this);
    menu.setWindowFlags(menu.windowFlags() | Qt::NoDropShadowWindowHint);

    QAction* title = menu.addAction(date.toString(QStringLiteral("yyyy-MM-dd")));
    title->setEnabled(false);
    menu.addSeparator();

    connect(menu.addAction(tr("新建任务")), &QAction::triggered, this, [this, date]() {
        emit requestAddTask(date);
    });

    QAction* clearAction = menu.addAction(tr("清空当日任务"));
    clearAction->setEnabled(hasTasksOnDate(date));
    connect(clearAction, &QAction::triggered, this, [this, date]() {
        emit requestClearDate(date);
    });

    if (m_selectedSourceRow >= 0) {
        menu.addSeparator();
        connect(menu.addAction(tr("删除选中任务")), &QAction::triggered,
                this, &OscCalendarWidget::requestDeleteSelectedTask);
    }

    menu.exec(globalPos);
}

bool OscCalendarWidget::onViewportDrag(QEvent* event)
{
    if (event->type() == QEvent::DragEnter) {
        auto* e = static_cast<QDragEnterEvent*>(event);
        if (!isOscMime(e->mimeData())) {
            return false;
        }
        e->acceptProposedAction();
        return true;
    }

    if (event->type() == QEvent::DragMove) {
        auto* e = static_cast<QDragMoveEvent*>(event);
        if (!isOscMime(e->mimeData())) {
            return false;
        }
        // 进入 viewport 即接受，日期在 Drop 时再解析
        e->acceptProposedAction();
        return true;
    }

    if (event->type() == QEvent::Drop) {
        auto* e = static_cast<QDropEvent*>(event);
        if (!isOscMime(e->mimeData())) {
            return false;
        }
        QTableView* view = calendarView();
        const QPoint global = view ? view->viewport()->mapToGlobal(e->position().toPoint())
                                   : mapToGlobal(e->position().toPoint());
        acceptOscDrop(global, e->mimeData());
        e->acceptProposedAction();
        return true;
    }

    return false;
}

void OscCalendarWidget::acceptOscDrop(const QPoint& globalPos, const QMimeData* mime)
{
    QDate date = dateAtGlobalPos(globalPos);
    if (!date.isValid()) {
        date = selectedDate();
    }
    if (!date.isValid()) {
        return;
    }
    setSelectedDate(date);
    emit oscMessageDropped(date, oscFromMime(mime));
}

void OscCalendarWidget::contextMenuEvent(QContextMenuEvent* event)
{
    const QDate date = dateAtGlobalPos(event->globalPos());
    if (date.isValid()) {
        showDateContextMenu(date, event->globalPos());
        return;
    }
    QCalendarWidget::contextMenuEvent(event);
}

// 以下三个重写作为导航栏等非 viewport 区域的拖放兜底
void OscCalendarWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (isOscMime(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    QCalendarWidget::dragEnterEvent(event);
}

void OscCalendarWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (isOscMime(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void OscCalendarWidget::dropEvent(QDropEvent* event)
{
    if (!isOscMime(event->mimeData())) {
        event->ignore();
        return;
    }
    acceptOscDrop(mapToGlobal(event->position().toPoint()), event->mimeData());
    event->acceptProposedAction();
}
