#include "TimeLineToolBar.h"
#include "TimeLineStyle.h"
#include <QStyle>
#include <QApplication>
#include <QClipboard>
#include <QSignalBlocker>

TimeLineToolBar::TimeLineToolBar(QWidget* parent)
    : BaseTimelineToolbar(parent)
{
    createActions();
    setupUI();
}

TimeLineToolBar::~TimeLineToolBar()
{
    delete m_playAction;
    delete m_stopAction;
    delete m_loopAction;
    delete m_nextFrameAction;
}

void TimeLineToolBar::bindBus(const QString& modelAlias)
{
    if (m_busBound) {
        return;
    }
    //首先校验起始，保证以/开头
    m_modelAlias = modelAlias.startsWith('/') ? modelAlias : ("/" + modelAlias);
    m_busBound = true;

    for (const auto& pair : *_OscMapping) {
        const QString& rel = pair.first;
        QAction* act = pair.second;
        QWidget* widget = widgetForAction(act);
        StatusContainer::instance()->registerWidget(widget, makeFullOscAddress(rel));
    }

    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/pause"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/output"), this, SLOT(onGlobalEvent(GlobalEvent)));

    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/previousMedia"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/previousFrame"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/nextFrame"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/nextMedia"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/fullscreen"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/settings"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/moveClipLeft"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/moveClipRight"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/deleteClip"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/zoomIn"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/zoomOut"), this, SLOT(onGlobalEvent(GlobalEvent)));
    //发布初始状态
    publishCurrentState();
}

void TimeLineToolBar::publishCurrentState()
{
    if (!m_busBound) {
        return;
    }
    stateFeedBack(makeFullOscAddress("/play"), m_isPlaying);
    stateFeedBack(makeFullOscAddress("/stop"), !m_isPlaying);
    stateFeedBack(makeFullOscAddress("/loop"), m_loopAction ? m_loopAction->isChecked() : false);

}

void TimeLineToolBar::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    if (ev.address == makeFullOscAddress("/play")) {
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
        stateFeedBack(makeFullOscAddress("/play"), m_isPlaying);
        return;

    }

    if (ev.address == makeFullOscAddress("/pause")) {
        const bool wantPause = !ev.payload.isValid() || ev.payload.toBool();
        if (wantPause && m_isPlaying && m_playAction) {
            m_playAction->trigger();
        }
        stateFeedBack(makeFullOscAddress("/play"), m_isPlaying);
        return;
    }

    if (ev.address == makeFullOscAddress("/stop")) {
        const bool wantStop = !ev.payload.isValid() || ev.payload.toBool();
        if (wantStop && m_stopAction) {
            m_stopAction->trigger();
        }
        stateFeedBack(makeFullOscAddress("/stop"), !m_isPlaying);
        return;
    }

    if (ev.address == makeFullOscAddress("/loop")) {
        const bool enabled = ev.payload.toBool();
        if (m_loopAction && m_loopAction->isChecked() != enabled) {
            setLoopState(enabled);
            emit loopToggled(enabled);
        }
        stateFeedBack(makeFullOscAddress("/loop"), enabled);
        return;
    }

    if (ev.address == makeFullOscAddress("/output")) {
        const bool enabled = ev.payload.toBool();
        if (m_outputAction && m_outputAction->isChecked() != enabled) {
            {
                QSignalBlocker blocker(m_outputAction);
                m_outputAction->setChecked(enabled);
            }
            emit outputWindowToggled(enabled);
            stateFeedBack(makeFullOscAddress("/output"), enabled);
        }
        return;
    }

    const bool doTrigger = !ev.payload.isValid() || ev.payload.toBool();
    if (!doTrigger) {
        return;
    }

    if (ev.address == makeFullOscAddress("/previousMedia") && m_previousMediaAction) {
        m_previousMediaAction->trigger();
    } else if (ev.address == makeFullOscAddress("/previousFrame") && m_previousFrameAction) {
        m_previousFrameAction->trigger();
    } else if (ev.address == makeFullOscAddress("/nextFrame") && m_nextFrameAction) {
        m_nextFrameAction->trigger();
    } else if (ev.address == makeFullOscAddress("/nextMedia") && m_nextMediaAction) {
        m_nextMediaAction->trigger();
    } else if (ev.address == makeFullOscAddress("/fullscreen") && m_fullscreenAction) {
        m_fullscreenAction->trigger();
    } else if (ev.address == makeFullOscAddress("/settings") && m_settingsAction) {
        m_settingsAction->trigger();
    } else if (ev.address == makeFullOscAddress("/moveClipLeft") && m_moveClipLeftAction) {
        m_moveClipLeftAction->trigger();
    } else if (ev.address == makeFullOscAddress("/moveClipRight") && m_moveClipRightAction) {
        m_moveClipRightAction->trigger();
    } else if (ev.address == makeFullOscAddress("/deleteClip") && m_deleteClipAction) {
        m_deleteClipAction->trigger();
    } else if (ev.address == makeFullOscAddress("/zoomIn") && m_zoomInAction) {
        m_zoomInAction->trigger();
    } else if (ev.address == makeFullOscAddress("/zoomOut") && m_zoomOutAction) {
        m_zoomOutAction->trigger();
    }
}

void TimeLineToolBar::createActions()
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
        stateFeedBack(makeFullOscAddress("/play"), m_isPlaying);
        stateFeedBack(makeFullOscAddress("/stop"), !m_isPlaying);
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
        stateFeedBack(makeFullOscAddress("/play"), m_isPlaying);
        stateFeedBack(makeFullOscAddress("/stop"), !m_isPlaying);
    });

    // 创建循环动作
    m_loopAction = new QAction(this);
    m_loopAction->setIcon(QIcon(":/icons/icons/repeat.png"));
    m_loopAction->setToolTip(tr("Loop"));
    m_loopAction->setCheckable(true);
    connect(m_loopAction, &QAction::toggled, [this]() {
        emit loopToggled(m_loopAction->isChecked());
        stateFeedBack(makeFullOscAddress("/loop"), m_loopAction->isChecked());
    });

    m_nextFrameAction = new QAction(this);
    m_nextFrameAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right));
    m_nextFrameAction->setIcon(QIcon(":/icons/icons/rewind-forward.png"));
    m_nextFrameAction->setToolTip(tr("Next Frame"));
    connect(m_nextFrameAction, &QAction::triggered, this, &TimeLineToolBar::nextFrameClicked);

    m_previousFrameAction = new QAction(this);
    m_previousFrameAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left));
    m_previousFrameAction->setIcon(QIcon(":/icons/icons/rewind-back.png"));
    m_previousFrameAction->setToolTip(tr("Previous Frame"));
    connect(m_previousFrameAction, &QAction::triggered, this, &TimeLineToolBar::prevFrameClicked);

    m_nextMediaAction = new QAction(this);
    m_nextMediaAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Right));
    m_nextMediaAction->setIcon(QIcon(":/icons/icons/play-next.png"));
    m_nextMediaAction->setToolTip(tr("Next Media"));
    // connect(m_nextMediaAction, &QAction::triggered, this, &TimeLineToolBar::nextMediaClicked);

    m_previousMediaAction = new QAction(this);
    m_previousMediaAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Left));
    m_previousMediaAction->setIcon(QIcon(":/icons/icons/play-previous.png"));
    m_previousMediaAction->setToolTip(tr("Previous Media"));
    // connect(m_previousMediaAction, &QAction::triggered, this, &TimeLineToolBar::previousMediaClicked);

    m_fullscreenAction = new QAction(this);
    m_fullscreenAction->setIcon(QIcon(":/icons/icons/fullscreen-enter.png"));
    m_fullscreenAction->setToolTip(tr("Fullscreen"));
    // connect(m_fullscreenAction, &QAction::triggered, this, &TimeLineToolBar::fullscreenClicked);
    
    m_settingsAction = new QAction(this);
    m_settingsAction->setIcon(QIcon(":/icons/icons/settings.png"));
    m_settingsAction->setToolTip(tr("Settings"));
    connect(m_settingsAction, &QAction::triggered, this, [this]() {
        emit settingsClicked();
    });
    
    m_outputAction = new QAction(this);
    m_outputAction->setIcon(QIcon(":/icons/icons/views.png"));
    m_outputAction->setToolTip(tr("Output Window"));
    m_outputAction->setCheckable(true);
    connect(m_outputAction, &QAction::toggled, this, &TimeLineToolBar::outputWindowToggled);
    
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
    connect(m_deleteClipAction, &QAction::triggered, this, &TimeLineToolBar::deleteClipClicked);
    // 创建缩放动作
    m_zoomInAction = new QAction(this);
    m_zoomInAction->setIcon(QIcon(":/icons/icons/zoomin.png"));
    m_zoomInAction->setToolTip(tr("Zoom In"));
    connect(m_zoomInAction, &QAction::triggered, this, &TimeLineToolBar::zoomInClicked);

    m_zoomOutAction = new QAction(this);
    m_zoomOutAction->setIcon(QIcon(":/icons/icons/zoomout.png"));
    m_zoomOutAction->setToolTip(tr("Zoom Out"));
    connect(m_zoomOutAction, &QAction::triggered, this, &TimeLineToolBar::zoomOutClicked);

    m_locationAction=new QAction(this);
    m_locationAction->setIcon(QIcon(":/icons/icons/location.png"));
    m_locationAction->setToolTip(tr("Focus cursor"));
    connect(m_locationAction, &QAction::triggered, this, &TimeLineToolBar::locationClicked);
}

void TimeLineToolBar::setupUI()
{
    // 添加动作到工具栏
    addAction(m_previousMediaAction);

    registerOSCControl("/previousMedia",m_previousMediaAction);
    addAction(m_previousFrameAction);
    registerOSCControl("/previousFrame",m_previousFrameAction);
    addAction(m_playAction);
    registerOSCControl("/play",m_playAction);
    addAction(m_stopAction);
    registerOSCControl("/stop",m_stopAction);
    addAction(m_nextFrameAction);
    registerOSCControl("/nextFrame",m_nextFrameAction);
    addAction(m_nextMediaAction);
    registerOSCControl("/nextMedia",m_nextMediaAction);
    addAction(m_fullscreenAction);
    registerOSCControl("/fullscreen",m_fullscreenAction);
    addAction(m_settingsAction);
    registerOSCControl("/settings",m_settingsAction);
    addAction(m_loopAction);
    registerOSCControl("/loop",m_loopAction);
    addAction(m_outputAction);
    registerOSCControl("/output",m_outputAction);
    addSeparator();
    addAction(m_moveClipLeftAction);
    registerOSCControl("/moveClipLeft",m_moveClipLeftAction);
    addAction(m_moveClipRightAction);
    registerOSCControl("/moveClipRight",m_moveClipRightAction);
    addAction(m_deleteClipAction);
    registerOSCControl("/deleteClip",m_deleteClipAction);
    addAction(m_zoomInAction);
    registerOSCControl("/zoomIn",m_zoomInAction);
    addAction(m_zoomOutAction);
    registerOSCControl("/zoomOut",m_zoomOutAction);
    addAction(m_locationAction);
    // 设置工具栏样式
    setMovable(false);
    setIconSize(QSize(toolbarButtonWidth, toolbarButtonWidth));
} 

void TimeLineToolBar::setPlaybackState(bool isPlaying)
{
    if (m_isPlaying == isPlaying) {
        return;
    }
    m_isPlaying = isPlaying;
    m_playAction->setIcon(QIcon(m_isPlaying ? ":/icons/icons/pause.png" : ":/icons/icons/play.png"));
    m_playAction->setToolTip(m_isPlaying ? tr("Pause") : tr("Play"));
    stateFeedBack(makeFullOscAddress("/play"), m_isPlaying);
    stateFeedBack(makeFullOscAddress("/stop"), !m_isPlaying);
}

void TimeLineToolBar::setLoopState(bool isLooping)
{
    if (m_loopAction->isChecked() == isLooping) {
        return;
    }

    m_loopAction->blockSignals(true);
    m_loopAction->setChecked(isLooping);
    m_loopAction->blockSignals(false);
    m_loopAction->setToolTip(isLooping ? tr("Unloop") : tr("Loop"));

}
void TimeLineToolBar::startDrag(QAction* widget)
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
    message.address = makeFullOscAddress(oscAddress);
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