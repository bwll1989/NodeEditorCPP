//
// Created by WuBin on 24-11-3.
//

#ifndef STATUSBAR_H
#define STATUSBAR_H



#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>

class StatusBar : public QStatusBar {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);

    // 更新进度条值
    void updateProgress(int value);

    // 更新状态标签
    void updateStatus(const QString &status);

private:
    QProgressBar *progressBar; // 进度条
    QLabel *statusLabel; // 状态标签
};



#endif //STATUSBAR_H
