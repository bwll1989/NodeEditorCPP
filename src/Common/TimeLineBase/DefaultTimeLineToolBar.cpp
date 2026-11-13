#include "DefaultTimeLineToolBar.h"

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QMouseEvent>
#include <QMimeData>
#include <QVariant>
#include "Common/AppConfig/ConfigManager.h"
#include "Common/AppConfig/OscAddressMimeUtil.hpp"

DefaultTimeLineToolBar::DefaultTimeLineToolBar(QWidget* parent)
    : QToolBar(parent)
{
    createActions();
    setupUI();
    // 【OSC 拖拽】登记播放/停止按钮控件（Ctrl+拖动导出地址；未 setOscBase
    // 时 eventFilter 不触发拖拽——纯登记无副作用）
    if (m_playAction) {
        if (auto* w = widgetForAction(m_playAction))
            installOscDrag(w, QStringLiteral("/play"));
    }
    if (m_stopAction) {
        if (auto* w = widgetForAction(m_stopAction))
            installOscDrag(w, QStringLiteral("/stop"));
    }
}

DefaultTimeLineToolBar::~DefaultTimeLineToolBar() = default;

void DefaultTimeLineToolBar::createActions()
{
    // 播放/暂停切换动作（空格快捷键；图标随状态切换）
    m_playAction = new QAction(this);
    m_playAction->setIcon(QIcon(":/icons/icons/play.png"));
    m_playAction->setToolTip(tr("Play"));
    m_playAction->setShortcut(QKeySequence(Qt::Key_Space));
    connect(m_playAction, &QAction::triggered, [this]() {
        m_isPlaying = !m_isPlaying;
        m_playAction->setIcon(QIcon(m_isPlaying ? ":/icons/icons/pause.png" : ":/icons/icons/play.png"));
        m_playAction->setToolTip(m_isPlaying ? tr("Pause") : tr("Play"));
        if (m_isPlaying) {
            emit playClicked();
        } else {
            emit pauseClicked();
        }
    });

    // 停止动作
    m_stopAction = new QAction(this);
    m_stopAction->setIcon(QIcon(":/icons/icons/stop.png"));
    m_stopAction->setToolTip(tr("Stop"));
    connect(m_stopAction, &QAction::triggered, [this]() {
        if (m_isPlaying) {
            m_isPlaying = false;
            m_playAction->setIcon(QIcon(":/icons/icons/play.png"));
            m_playAction->setToolTip(tr("Play"));
        }
        emit stopClicked();
    });
}

void DefaultTimeLineToolBar::setupUI()
{
    addAction(m_playAction);
    addAction(m_stopAction);
    setMovable(false);
    setIconSize(QSize(toolbarButtonWidth, toolbarButtonWidth));
}

void DefaultTimeLineToolBar::setPlaybackState(bool isPlaying)
{
    m_isPlaying = isPlaying;
    m_playAction->setIcon(QIcon(m_isPlaying ? ":/icons/icons/pause.png" : ":/icons/icons/play.png"));
    m_playAction->setToolTip(m_isPlaying ? tr("Pause") : tr("Play"));
}

// ============================================================================
// OSC 地址拖拽（2026-11-12 保留旧 TimeLineToolBar 能力）
// 触发：Ctrl+左键在登记控件上按下并拖动超过系统拖拽阈值。
// 产出：地址入剪贴板 + "application/x-osc-address" mime
//      （字段序 = 消费端读取序：host >> port >> address >> type >> value）。
// 消费：OSCMessageListWidget（消息卡片）/ CalendarWidget / 时间轴 Trigger
//      剪辑等既有拖放目标，格式与其一致，无需改动。
// ============================================================================

void DefaultTimeLineToolBar::installOscDrag(QWidget* w, const QString& rel)
{
    if (!w || rel.isEmpty()) return;
    m_oscWidgets[w] = rel;
    w->installEventFilter(this);
}

bool DefaultTimeLineToolBar::eventFilter(QObject* watched, QEvent* event)
{
    // 只关心登记过的控件（旧 BaseTimelineToolbar::eventFilter 同逻辑）
    const auto it = m_oscWidgets.constFind(
        qobject_cast<QWidget*>(watched));
    if (it != m_oscWidgets.constEnd() && !m_oscBase.isEmpty()) {
        QWidget* widget = it.key();
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton
                    && (mouseEvent->modifiers() & Qt::ControlModifier)) {
                    m_dragStartPos = mouseEvent->pos();
                    m_dragging = true;
                }
                break;
            }
            case QEvent::MouseMove: {
                if (!m_dragging) break;
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if ((mouseEvent->pos() - m_dragStartPos).manhattanLength()
                    >= QApplication::startDragDistance()) {
                    m_dragging = false;
                    startDrag(widget);
                    return true; // 拖拽启动，吞掉后续移动事件
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                m_dragging = false;
                break;
            }
            default:
                break;
        }
    }
    return QToolBar::eventFilter(watched, event);
}

void DefaultTimeLineToolBar::startDrag(QWidget* widget)
{
    const QString full = oscAddressFor(widget);
    if (full.isEmpty()) return;

    // 拖拽同时复制地址（旧 TimeLineToolBar::startDrag 行为）
    QApplication::clipboard()->setText(full);

    // mime 编解码走 OscAddressMimeUtil（全工程统一：消费端字段序
    // host>>port>>address>>type>>value）。port 取外部控制端口配置（默认 8991）。
    auto* mimeData = new QMimeData;
    OscAddressMime::encode(mimeData, QStringLiteral("127.0.0.1"),
                           ConfigManager::instance().getExtraControlPort(),
                           full, QStringLiteral("Int"), QVariant(0));

    auto* drag = new QDrag(widget);
    drag->setMimeData(mimeData);
    drag->setPixmap(OscAddressMime::addressPixmap(full));
    drag->setHotSpot(QPoint(100, 15));

    drag->exec(Qt::CopyAction);
}
