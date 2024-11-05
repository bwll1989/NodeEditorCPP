//
// Created by WuBin on 24-11-3.
//

#include "StatusBar.h"
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>


StatusBar:: StatusBar(QWidget *parent) : QStatusBar(parent) {
        // 创建进度条
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100); // 进度条范围
    addWidget(progressBar); // 将进度条添加到状态栏

    statusLabel = new QLabel("CPU Usage: 0%", this);
    addWidget(statusLabel); // 将标签添加到状态栏
    }

// 更新进度条值
void StatusBar::updateProgress(int value) {
    progressBar->setValue(value);
    statusLabel->setText(QString("CPU Usage: %1%").arg(value));
}

// 更新状态标签
void StatusBar::updateStatus(const QString &status) {
    statusLabel->setText(status);
}


