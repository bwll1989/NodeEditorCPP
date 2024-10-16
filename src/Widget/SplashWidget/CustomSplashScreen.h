//
// Created by bwll1 on 2024/8/29.
//

#ifndef NODEEDITORCPP_CUSTOMSPLASHSCREEN_H
#define NODEEDITORCPP_CUSTOMSPLASHSCREEN_H
#include <QSplashScreen>
#include <QTimer>
#include <QPainter>
#include<QMainWindow>
class CustomSplashScreen :public QObject {
    Q_OBJECT

public:
    CustomSplashScreen();

public slots:
    void updateStatus(const QString &status) ;
    void finish(QMainWindow *mainWindow);
private:
    QSplashScreen *splash;
};


#endif //NODEEDITORCPP_CUSTOMSPLASHSCREEN_H
