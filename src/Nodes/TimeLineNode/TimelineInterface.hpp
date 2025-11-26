//
// Created by Administrator on 2023/12/13.
//

#pragma once

#include "QWidget"
#include "QVBoxLayout"
#include "QGridLayout"
#include "QSplitter"
#include "QToolButton"
#include "QIcon"
#include "QStyle"
#include "TimeLineNodeWidget.hpp"
using namespace std;
namespace Nodes
{
    class TimelineInterface: public QWidget{
        Q_OBJECT
        public:
        explicit TimelineInterface(TimeLineNodeModel* model=nullptr, QWidget *parent = nullptr) :QWidget(parent){
            timeline=new TimelineNodeWidget(model);
            this->setLayout(mainlayout);
            this->layout()->setContentsMargins(0,0,0,0);
            startButton = new QPushButton("启动");
            stopButton  = new QPushButton("停止");
            statusLabel = new QTimeEdit(this);
            statusLabel->setDisplayFormat("hh:mm:ss:zzz");
            statusLabel->setAlignment(Qt::AlignCenter);
            statusLabel->setTime(QTime(0,0,0,0));
            statusLabel->setReadOnly(true);
            statusLabel->setButtonSymbols(QAbstractSpinBox::NoButtons);
            // 设置字体加粗并放大
            QFont font = statusLabel->font();
            font.setBold(true);
            font.setPointSize(font.pointSize() + 8);
            statusLabel->setFont(font);
            
            mainlayout->addWidget(statusLabel,0,0,1,3);
            mainlayout->addWidget(startButton,1,0,1,1);
            mainlayout->addWidget(stopButton,1,1,1,1);
            startButton->setCheckable(true);
            stopButton->setCheckable(true);
            // 创建编辑图标按钮（更小尺寸）
            editButton = new QPushButton("显示编辑器");
            editButton->setToolTip("编辑脚本");
            mainlayout->addWidget(editButton,2,0,1,3);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            // 连接信号和槽
            connect(editButton, &QToolButton::clicked, this, &TimelineInterface::toggleEditorMode);
            // 连接控制按钮到模型
            connect(startButton, &QPushButton::clicked, this, &TimelineInterface::onStartClicked);
            connect(stopButton,  &QPushButton::clicked, this, &TimelineInterface::onStopClicked);

            // 监听时钟播放状态变化，自动更新状态
            if (timeline && timeline->model && timeline->model->getClock()) {
                connect(timeline->model->getClock(), &TimeLineNodeClock::timecodePlayingChanged,
                        this, &TimelineInterface::onPlaybackStateChanged);
                connect(timeline->model->getClock(), &TimeLineNodeClock::timecodeFinished,
                        this, &TimelineInterface::onPlaybackFinished);
                connect(timeline->model->getClock(), &TimeLineNodeClock::timecodeChanged,
                        this, &TimelineInterface::onTimeUpdated);
            }
        }

        /**
         * @brief 切换编辑器显示模式（嵌入/独立窗口）
         * 将编辑器显示为独立窗口，并确保跟随主程序退出
         */
        void toggleEditorMode() {
            // 移除父子关系，使其成为独立窗口
            timeline->setParent(nullptr);
            
            // 设置为独立窗口
            timeline->setWindowTitle("JS脚本编辑器");
            
            // 设置窗口图标
            timeline->setWindowIcon(QIcon(":/icons/icons/js.png"));
            
            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            timeline->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
            
            // 设置窗口属性：当关闭时自动删除
            timeline->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            timeline->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序
            
            // 设置窗口大小和显示
            timeline->resize(800, 600);
            timeline->show();
            // 激活窗口并置于前台
            timeline->activateWindow();
            timeline->raise();
        }

        /**
         * @brief 析构函数，确保在销毁时强制关闭独立窗口
         */
        ~TimelineInterface() {
            // 强制关闭编辑器窗口
            if (timeline && timeline->isVisible()) {
                timeline->close();
            }
            // 如果窗口仍然存在，强制隐藏
            if (timeline) {
                timeline->hide();
                timeline->setParent(this); // 重新设置父子关系，确保随对象销毁
            }
        }

    private slots:
        /**
         * @brief 启动播放按钮的槽函数
         * 调用模型的 onStartPlay 并更新按钮启用状态
         */
        void onStartClicked() {
            if (timeline && timeline->model) {
                timeline->model->onStartPlay();
            }
        }

        /**
         * @brief 停止播放按钮的槽函数
         * 调用模型的 onStopPlay 并更新状态与按钮
         */
        void onStopClicked() {
            if (timeline && timeline->model) {
                timeline->model->onStopPlay();
                QThread::msleep(100);
                statusLabel->setTime(QTime(0,0,0,0));
            }
        }
        void onTimeUpdated(const TimeCodeFrame& timecode) {
            statusLabel->setTime( timecode_frame_to_qtime(timecode,timeline->model->getTimeCodeType()));
        }
        /**
         * @brief 监听时钟的播放状态改变信号，自动更新状态文本与按钮
         * @param isPlaying 当前是否在播放
         */
        void onPlaybackStateChanged(bool isPlaying) {
            if (isPlaying) {
                startButton->setChecked(true);
                stopButton->setChecked(false);
            } else {
                startButton->setChecked(false);
                stopButton->setChecked(true);
                QThread::msleep(10);
                statusLabel->setTime(QTime(0,0,0,0));
            }
        }

        /**
         * @brief 监听播放完成事件，自动更新状态为停止
         */
        void onPlaybackFinished() {
            startButton->setChecked(false);
            stopButton->setChecked(true);

            statusLabel->setTime(QTime(0,0,0,0));

        }

    public:
        QGridLayout *mainlayout = new QGridLayout();
        QPushButton *editButton; // 编辑按钮
        TimelineNodeWidget  *timeline;
        QPushButton *startButton;
        QPushButton *stopButton;
        QTimeEdit *statusLabel;
    };
}


