#pragma once
//
// Created by Administrator on 2023/12/13.
//
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "QCheckBox"
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QVariant>
#include <QQuickWidget>
#include <QTimeEdit>
#include <QtWebEngineQuick/QtWebEngineQuick>
#include <QUrl>
using namespace std;
namespace Nodes {
    class QMLInterface : public QWidget {
        Q_OBJECT
    public:
        // 函数级注释：构造界面，使用 QQuickWidget 加载 qml 的 main.qml，
        // 其中 WebEngineView 渲染 main.html，所有按钮与状态文本均由 QML 控件实现。
        explicit QMLInterface(QWidget *parent = nullptr){
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
            // 连接控制按钮到模型
            connect(startButton, &QPushButton::clicked, this, &QMLInterface::onStartClicked);
            connect(stopButton,  &QPushButton::clicked, this, &QMLInterface::onStopClicked);

            // // 监听时钟播放状态变化，自动更新状态
            // if (timeline && timeline->model && timeline->model->getClock()) {
            //     connect(timeline->model->getClock(), &TimeLineNodeClock::timecodePlayingChanged,
            //             this, &TimelineInterface::onPlaybackStateChanged);
            //     connect(timeline->model->getClock(), &TimeLineNodeClock::timecodeFinished,
            //             this, &TimelineInterface::onPlaybackFinished);
            //     connect(timeline->model->getClock(), &TimeLineNodeClock::timecodeChanged,
            //             this, &TimelineInterface::onTimeUpdated);
            // }
        }
        /**
         * @brief 析构函数，确保在销毁时强制关闭独立窗口
         */
        ~QMLInterface() {

        }

    private slots:
        /**
         * @brief 启动播放按钮的槽函数
         * 调用模型的 onStartPlay 并更新按钮启用状态
         */
        void onStartClicked() {

        }

        /**
         * @brief 停止播放按钮的槽函数
         * 调用模型的 onStopPlay 并更新状态与按钮
         */
        void onStopClicked() {

        }
        // void onTimeUpdated(const TimeCodeFrame& timecode) {
        //     statusLabel->setTime( timecode_frame_to_qtime(timecode,timeline->model->getTimeCodeType()));
        // }
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
            }
        }

        /**
         * @brief 监听播放完成事件，自动更新状态为停止
         */
        void onPlaybackFinished() {
            startButton->setChecked(false);
            stopButton->setChecked(true);
        }

    public:
        QGridLayout *mainlayout = new QGridLayout();
        QPushButton *editButton; // 编辑按钮
        QPushButton *startButton;
        QPushButton *stopButton;
        QTimeEdit *statusLabel;

    };
}