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
    class DataVisualInterface : public QWidget {
        Q_OBJECT
    public:
        // 函数级注释：构造界面，使用 QQuickWidget 加载 qml 的 main.qml，
        // 其中 WebEngineView 渲染 main.html，所有按钮与状态文本均由 QML 控件实现。
        explicit DataVisualInterface(QWidget *parent = nullptr){
            this->setLayout(mainlayout);
            this->layout()->setContentsMargins(0,0,0,0);
            // stopButton->setCheckable(true);
            // 创建编辑图标按钮（更小尺寸）
            editButton = new QPushButton("Show");
            mainlayout->addWidget(editButton,0,0,1,3);
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
        ~DataVisualInterface() {

        }

    public:
        QGridLayout *mainlayout = new QGridLayout();
        QPushButton *editButton; // 编辑按钮
    };
}