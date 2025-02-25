//
// Created by bwll1 on 2024/8/29.
//

#ifndef NODEEDITORCPP_CUSTOMSPLASHSCREEN_HPP
#define NODEEDITORCPP_CUSTOMSPLASHSCREEN_HPP
#include <QSplashScreen>
#include <QTimer>
#include <QPainter>
#include<QMainWindow>
class CustomSplashScreen :public QObject {
    Q_OBJECT

public:
    /**
     * 构造函数
     */
    CustomSplashScreen();
  
public slots:
    /**
     * 更新状态
     */
    void updateStatus(const QString &status) ;
    /**
     * 完成
     */
    void finish(QMainWindow *mainWindow);
private:
    //开屏动画
    QSplashScreen *splash;
};


#endif //NODEEDITORCPP_CUSTOMSPLASHSCREEN_HPP
