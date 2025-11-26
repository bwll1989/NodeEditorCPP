#pragma once
//
// Created by Administrator on 2023/12/13.
//
#include "QWidget"
#include "QLayout"
#include "QPushButton"
#include "QCheckBox"
#include <QQuickWidget>
#include <QTimeEdit>
#include <QUrl>
using namespace std;
namespace Nodes {
    class CurveInterface : public QWidget {
        Q_OBJECT
    public:
        // 函数级注释：构造界面，使用 QQuickWidget 加载 qml 的 main.qml，
        // 其中 WebEngineView 渲染 main.html，所有按钮与状态文本均由 QML 控件实现。
        explicit CurveInterface(QWidget *parent = nullptr){
            this->setLayout(mainlayout);
            this->layout()->setContentsMargins(0,0,0,0);
            startButton = new QPushButton("Start");
            loopCheckBox = new QCheckBox("Loop"); // 循环播放复选框
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
            mainlayout->addWidget(startButton,1,0,1,2);

            mainlayout->addWidget(loopCheckBox,1,2,1,1); // 添加循环播放复选框

            startButton->setCheckable(true);
            // stopButton->setCheckable(true);
            // 创建编辑图标按钮（更小尺寸）
            editButton = new QPushButton("编辑曲线");
            mainlayout->addWidget(editButton,3,0,1,3);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            // 连接控制按钮到模型

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
        ~CurveInterface() {

        }

    public:
        QGridLayout *mainlayout = new QGridLayout();
        QPushButton *editButton; // 编辑按钮
        QPushButton *startButton; // 启动按钮
        // QPushButton *stopButton; // 停止按钮
        QCheckBox *loopCheckBox; // 循环播放复选框
        QTimeEdit *statusLabel;

    };
}