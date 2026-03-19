//
// Created by WuBin on 2025/11/1.
//

#include "TimeLineNodeToolBar.h"
#include "TimeLineStyle.h"
#include <QStyle>
#include <QApplication>
#include <QClipboard>
#include <QSignalBlocker>

TimeLineNodeToolBar::TimeLineNodeToolBar(QWidget* parent)
    : BaseTimelineToolbar(parent)
{
    createActions();
    setupUI();
}

TimeLineNodeToolBar::~TimeLineNodeToolBar()
{
}

QString TimeLineNodeToolBar::makeBusAddress(const QString& relative) const
{
    const QString norm = relative.startsWith('/') ? relative : ("/" + relative);
    return "/dataflow/" + m_parentAlias + "/" + QString::number(m_nodeId) + norm;
}

void TimeLineNodeToolBar::publishState(const QString& relative, const QVariant& value)
{
    if (!m_busBound) {
        return;
    }
    GlobalEventBus::instance()->publishState(makeBusAddress(relative), value);
}

void TimeLineNodeToolBar::bindBus(const QString& parentAlias, int nodeId)
{
    if (m_busBound) {
        return;
    }
    if (parentAlias.isEmpty() || nodeId < 0) {
        return;
    }
    //首先校验起始，保证以/开头
    m_parentAlias = parentAlias.startsWith('/') ? parentAlias : ("/" + parentAlias);
    m_nodeId = nodeId;
    m_busBound = true;

    GlobalEventBus::instance()->subscribe(makeBusAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeBusAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeBusAddress("/pause"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeBusAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
    connect(this, &TimeLineNodeToolBar::loopToggled, this, [this](bool enabled) {
        publishState("/loop", enabled);
    });
    //立即发送一次状态
    publishState("/play", m_isPlaying);
    publishState("/stop", !m_isPlaying);
    publishState("/loop", m_loopAction ? m_loopAction->isChecked() : false);
}

void TimeLineNodeToolBar::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    if (ev.address == makeBusAddress("/play")) {
        const bool wantPlay = !ev.payload.isValid() || ev.payload.toBool();
        if (wantPlay) {
            if (!m_isPlaying && m_playAction) {
                m_playAction->trigger();
            }
        } else {
            if (m_stopAction) {
                m_stopAction->trigger();
            }
        }
        return;
    }

    if (ev.address == makeBusAddress("/pause")) {
        const bool wantPause = !ev.payload.isValid() || ev.payload.toBool();
        if (wantPause && m_isPlaying && m_playAction) {
            m_playAction->trigger();
        }
        return;
    }

    if (ev.address == makeBusAddress("/stop")) {
        const bool wantStop = !ev.payload.isValid() || ev.payload.toBool();
        if (wantStop && m_stopAction) {
            m_stopAction->trigger();
        }
        return;
    }

    if (ev.address == makeBusAddress("/loop")) {
        const bool enabled = ev.payload.toBool();
        if (m_loopAction && m_loopAction->isChecked() != enabled) {
            QSignalBlocker blocker(m_loopAction);
            m_loopAction->setChecked(enabled);
            blocker.unblock();
            emit loopToggled(enabled);
        }
        return;
    }
}

void TimeLineNodeToolBar::createActions()
{
    // 创建播放动作
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

    // 创建停止动作
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

    // 创建循环动作
    m_loopAction = new QAction(this);
    m_loopAction->setIcon(QIcon(":/icons/icons/repeat.png"));
    m_loopAction->setToolTip(tr("Loop"));
    m_loopAction->setCheckable(true);
    connect(m_loopAction, &QAction::toggled, this, &TimeLineNodeToolBar::loopToggled);

    m_nextFrameAction = new QAction(this);
    m_nextFrameAction->setIcon(QIcon(":/icons/icons/rewind-forward.png"));
    m_nextFrameAction->setToolTip(tr("Next Frame"));
    m_nextFrameAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right));
    connect(m_nextFrameAction, &QAction::triggered, this, &TimeLineNodeToolBar::nextFrameClicked);

    m_previousFrameAction = new QAction(this);

    m_previousFrameAction->setIcon(QIcon(":/icons/icons/rewind-back.png"));
    m_previousFrameAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left));
    m_previousFrameAction->setToolTip(tr("Previous Frame"));
    connect(m_previousFrameAction, &QAction::triggered, this, &TimeLineNodeToolBar::prevFrameClicked);

    m_nextMediaAction = new QAction(this);
    m_nextMediaAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Right));
    m_nextMediaAction->setIcon(QIcon(":/icons/icons/play-next.png"));
    m_nextMediaAction->setToolTip(tr("Next Media"));
    // connect(m_nextMediaAction, &QAction::triggered, this, &TimeLineNodeToolBar::nextMediaClicked);

    m_previousMediaAction = new QAction(this);
    m_previousMediaAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Left));
    m_previousMediaAction->setIcon(QIcon(":/icons/icons/play-previous.png"));
    m_previousMediaAction->setToolTip(tr("Previous Media"));
    // connect(m_previousMediaAction, &QAction::triggered, this, &TimeLineNodeToolBar::previousMediaClicked);

    // m_fullscreenAction = new QAction(this);
    // m_fullscreenAction->setIcon(QIcon(":/icons/icons/fullscreen-enter.png"));
    // m_fullscreenAction->setToolTip(tr("Fullscreen"));
    // // connect(m_fullscreenAction, &QAction::triggered, this, &TimeLineNodeToolBar::fullscreenClicked);
    //
    // m_settingsAction = new QAction(this);
    // m_settingsAction->setIcon(QIcon(":/icons/icons/settings.png"));
    // m_settingsAction->setToolTip(tr("Settings"));
    // connect(m_settingsAction, &QAction::triggered, this, [this]() {
    //     emit settingsClicked();
    // });

    // m_outputAction = new QAction(this);
    // m_outputAction->setIcon(QIcon(":/icons/icons/views.png"));
    // m_outputAction->setToolTip(tr("Output Window"));
    // m_outputAction->setCheckable(true);
    // connect(m_outputAction, &QAction::toggled, this, &TimeLineNodeToolBar::outputWindowToggled);

    m_moveClipLeftAction = new QAction(this);
//    m_moveClipLeftAction->setShortcut(QKeySequence(Qt::Key_Left));
    m_moveClipLeftAction->setIcon(QIcon(":/icons/icons/move-left.png"));
    m_moveClipLeftAction->setToolTip(tr("Move Clip -1"));
    connect(m_moveClipLeftAction, &QAction::triggered, this,[this]() {
       emit moveClipClicked(-1);
    });

    m_moveClipRightAction = new QAction(this);
//    m_moveClipRightAction->setShortcut(QKeySequence(Qt::Key_Right));
    m_moveClipRightAction->setIcon(QIcon(":/icons/icons/move-right.png"));
    m_moveClipRightAction->setToolTip(tr("Move Clip +1"));
    connect(m_moveClipRightAction, &QAction::triggered, this,[this]() {
       emit moveClipClicked(1);
    });
    // 删除
    m_deleteClipAction = new QAction(this);
//    m_deleteClipAction->setShortcut(QKeySequence(Qt::Key_Delete));
    m_deleteClipAction->setIcon(QIcon(":/icons/icons/delete-clip.png"));
    m_deleteClipAction->setToolTip(tr("Delete Clip"));
    connect(m_deleteClipAction, &QAction::triggered, this, &TimeLineNodeToolBar::deleteClipClicked);
    // 创建缩放动作
    m_zoomInAction = new QAction(this);
    m_zoomInAction->setIcon(QIcon(":/icons/icons/zoomin.png"));
    m_zoomInAction->setToolTip(tr("Zoom In"));
    connect(m_zoomInAction, &QAction::triggered, this, &TimeLineNodeToolBar::zoomInClicked);

    m_zoomOutAction = new QAction(this);
    m_zoomOutAction->setIcon(QIcon(":/icons/icons/zoomout.png"));
    m_zoomOutAction->setToolTip(tr("Zoom Out"));
    connect(m_zoomOutAction, &QAction::triggered, this, &TimeLineNodeToolBar::zoomOutClicked);

}

void TimeLineNodeToolBar::setupUI()
{
    // 添加动作到工具栏
    addAction(m_previousMediaAction);
    m_allActions["previousMedia"]=m_previousMediaAction;
    addAction(m_previousFrameAction);
    m_allActions["previousFrame"]=m_previousFrameAction;
    addAction(m_playAction);
    m_allActions["play"]=m_playAction;
    // 注册play 外部控制
    registerOSCControl("/play",m_playAction);
    addAction(m_stopAction);
    m_allActions["stop"]=m_stopAction;
    // 注册stop 外部控制
    registerOSCControl("/stop",m_stopAction);
    addAction(m_nextFrameAction);
    m_allActions["nextFrame"]=m_nextFrameAction;
    addAction(m_nextMediaAction);
    m_allActions["nextMedia"]=m_nextMediaAction;

    addAction(m_loopAction);
    m_allActions["loop"]=m_loopAction;
    // 注册loop 外部控制
    registerOSCControl("/loop",m_loopAction);
    // addAction(m_outputAction);
    // BaseTimelineToolbar::registerOSCControl("/output",m_outputAction);
    addSeparator();
    addAction(m_moveClipLeftAction);
    m_allActions["moveClipLeft"]=m_moveClipLeftAction;

    addAction(m_moveClipRightAction);
    m_allActions["moveClipRight"]=m_moveClipRightAction;

    addAction(m_deleteClipAction);
    m_allActions["deleteClip"]=m_deleteClipAction;

    addAction(m_zoomInAction);
    m_allActions["zoomIn"]=m_zoomInAction;
    addAction(m_zoomOutAction);
    m_allActions["zoomOut"]=m_zoomOutAction;
    // 设置工具栏样式
    setMovable(false);
    setIconSize(QSize(toolbarButtonWidth, toolbarButtonWidth));
}

void TimeLineNodeToolBar::setPlaybackState(bool isPlaying)
{
    if (m_isPlaying == isPlaying) {
        return;
    }
    m_isPlaying = isPlaying;
    m_playAction->setIcon(QIcon(m_isPlaying ? ":/icons/icons/pause.png" : ":/icons/icons/play.png"));
    m_playAction->setToolTip(m_isPlaying ? tr("Pause") : tr("Play"));

}

void TimeLineNodeToolBar::setLoopState(bool isLooping)
{
    if (m_loopAction->isChecked() == isLooping) {
        return;
    }

    m_loopAction->blockSignals(true);
    m_loopAction->setChecked(isLooping);
    m_loopAction->blockSignals(false);
    m_loopAction->setToolTip(isLooping ? tr("Unloop") : tr("Loop"));

    publishState("/loop", isLooping);
}


std::unordered_map<QString, QAction *> TimeLineNodeToolBar::allActions() {
    return m_allActions;
}

void TimeLineNodeToolBar::startDrag(QAction* widget)
{
    // 找到对应的OSC地址
    QString oscAddress;
    for (const auto& pair : *_OscMapping) {
        if (pair.second == widget) {
            oscAddress = pair.first;
            break;
        }
    }

    if (oscAddress.isEmpty()) return;

    OSCMessage message;
    message.address = makeBusAddress(oscAddress);
    message.host = "127.0.0.1";
    message.port = 8991;
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(message.address);
    // 获取控件的值

    message.value = widget->isChecked();
    message.type = "Int";


    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << message.host << message.port << message.address << message.value<<message.type;

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-osc-address", itemData);

    QDrag* drag = new QDrag(widget);
    drag->setMimeData(mimeData);
    QPixmap pixmap(200, 30);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    QColor bgColor(40, 40, 40, 200);  // 半透明深灰色
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect(), 5, 5);  // 圆角矩形
    // 绘制文本
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    QRect textRect = pixmap.rect().adjusted(30, 0, -8, 0);  // 图标右侧的文本区域
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, message.address);

    // 设置拖拽预览
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));  // 热点在中心

    drag->exec(Qt::CopyAction);
}