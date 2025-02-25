//
// Created by bwll1 on 2024/8/29.
//

#include "CustomSplashScreen.hpp"
CustomSplashScreen::CustomSplashScreen() {
    splash = new QSplashScreen(QPixmap(":/images/images/loading.png"));
    splash->show();
}


void CustomSplashScreen::updateStatus(const QString &status)
{
    splash->showMessage(status, Qt::AlignBottom | Qt::AlignCenter, Qt::black);
}

void CustomSplashScreen::finish(QMainWindow *mainWindow) {
    splash->finish(mainWindow);
}
