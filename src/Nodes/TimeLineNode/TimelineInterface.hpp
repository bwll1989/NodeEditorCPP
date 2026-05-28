//
// Created by Administrator on 2023/12/13.
//

#pragma once

#include "QWidget"
#include "QGridLayout"
#include "QPushButton"
#include "QTimeEdit"
#include "QFont"
#include "QEvent"
#include "QSignalBlocker"
#include "TimeCodeDefines.h"
#include "TimeLineNodeWidget.hpp"
using namespace std;
namespace Nodes
{
    class TimelineInterface: public QWidget{
        Q_OBJECT
        public:
        explicit TimelineInterface(TimeLineNodeModel* model=nullptr, QWidget *parent = nullptr) :QWidget(parent){
            timeline = new TimelineNodeWidget(model, this);
            timeline->hide();
            timeline->installEventFilter(this);

            this->setLayout(mainlayout);
            this->layout()->setContentsMargins(0,0,0,0);
            startButton = new QPushButton(tr("启动"), this);
            statusLabel = new QTimeEdit(this);
            statusLabel->setDisplayFormat("hh:mm:ss:zzz");
            statusLabel->setAlignment(Qt::AlignCenter);
            statusLabel->setTime(QTime(0,0,0,0));
            statusLabel->setReadOnly(true);
            statusLabel->setButtonSymbols(QAbstractSpinBox::NoButtons);
            QFont font = statusLabel->font();
            font.setBold(true);
            font.setPointSize(font.pointSize() + 8);
            statusLabel->setFont(font);

            mainlayout->addWidget(statusLabel,0,0,1,3);
            mainlayout->addWidget(startButton,1,0,1,3);
            startButton->setCheckable(true);

            editButton = new QPushButton(tr("显示编辑器"), this);
            editButton->setToolTip(tr("打开时间轴编辑窗口"));
            mainlayout->addWidget(editButton,2,0,1,3);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            connect(editButton, &QPushButton::clicked, this, &TimelineInterface::toggleEditorMode);
            connect(startButton, &QPushButton::toggled, this, &TimelineInterface::onStartToggled);

            if (timeline && timeline->model && timeline->model->getClock()) {
                connect(timeline->model->getClock(), &TimeLineNodeClock::timecodePlayingChanged,
                        this, &TimelineInterface::onPlaybackStateChanged);
                connect(timeline->model->getClock(), &TimeLineNodeClock::timecodeFinished,
                        this, &TimelineInterface::onPlaybackFinished);
                connect(timeline->model->getClock(), &TimeLineNodeClock::timecodeChanged,
                        this, &TimelineInterface::onTimeUpdated);
            }
        }

        TimelineNodeWidget* editorWidget() const { return timeline; }

        void toggleEditorMode() {
            if (!timeline) {
                return;
            }
            if (timeline->isVisible() && (timeline->windowFlags() & Qt::Window)) {
                hideEditorWindow();
                return;
            }
            timeline->setParent(nullptr);
            timeline->setWindowTitle(tr("时间轴编辑器"));
            timeline->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
            timeline->setAttribute(Qt::WA_DeleteOnClose, false);
            timeline->setAttribute(Qt::WA_QuitOnClose, false);
            timeline->resize(1000, 600);
            timeline->show();
            timeline->activateWindow();
            timeline->raise();
            editButton->setText(tr("隐藏编辑器"));
        }

        ~TimelineInterface() override {
            if (timeline) {
                timeline->hide();
                timeline->setParent(this);
            }
        }

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override {
            if (watched == timeline && event->type() == QEvent::Close) {
                hideEditorWindow();
                return true;
            }
            return QWidget::eventFilter(watched, event);
        }

    private slots:
        void onStartToggled(bool checked) {
            if (!timeline || !timeline->model) {
                return;
            }
            if (checked) {
                timeline->model->onStartPlay();
            } else {
                timeline->model->onStopPlay();
            }
        }

        void onTimeUpdated(const TimeCodeFrame& timecode) {
            if (!timeline || !timeline->model) {
                return;
            }
            statusLabel->setTime(timecode_frame_to_qtime(timecode, timeline->model->getTimeCodeType()));
        }
        /**
         * @brief 监听时钟的播放状态改变信号，自动更新状态文本与按钮
         * @param isPlaying 当前是否在播放
         */
        void onPlaybackStateChanged(bool isPlaying) {
            QSignalBlocker blocker(startButton);
            startButton->setChecked(isPlaying);
            startButton->setText(isPlaying ? tr("停止") : tr("启动"));
        }

        void onPlaybackFinished() {
            QSignalBlocker blocker(startButton);
            startButton->setChecked(false);
            startButton->setText(tr("启动"));
            statusLabel->setTime(QTime(0, 0, 0, 0));
        }

    private:
        void hideEditorWindow() {
            if (!timeline) {
                return;
            }
            timeline->hide();
            timeline->setParent(this);
            editButton->setText(tr("显示编辑器"));
        }

    public:
        QGridLayout *mainlayout = new QGridLayout();
        QPushButton *editButton; // 编辑按钮
        TimelineNodeWidget  *timeline;
        QPushButton *startButton;
        // QPushButton *stopButton;
        QTimeEdit *statusLabel;
    };
}


