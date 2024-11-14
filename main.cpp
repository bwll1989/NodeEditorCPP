#include <QtWidgets/QApplication>
#include <QProcessEnvironment>
#include <QObject>
#include <QtGui>
#include "QStyleFactory"
#include "Widget/MainWindow/MainWindow.h"
#include "Widget/SplashWidget/CustomSplashScreen.h"
#include "Widget/ExternalControl/ExternalControler.h"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#define Extra_Control_Port 8991
#define PRODUCT_VERSION        "1.0"   // 产品版本
#define COMPANY_NAME           "Bwll"
#define PRODUCT_NAME          "NodeEditorCPP" // 产品名称
#define LUA_PATH                "./lua;"
#define FILE_DESCRIPTION       "Qt Creator based on Qt 6.6 (MinGw, 64 bit)"  // 文件说明
#define LEGAL_COPYRIGHT        "Copyright 2008-2016 The Qt Company Ltd. All rights reserved." // 版权

void setupApp(){
    QApplication::setApplicationDisplayName(PRODUCT_NAME);
    QApplication::setApplicationVersion(PRODUCT_VERSION);
    QApplication::setOrganizationName(COMPANY_NAME);
    QApplication::setOrganizationDomain("www.qt.com");
}

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    setupApp();
    qApp->setStyle(QStyleFactory::create("Fusion"));
    qApp->setAttribute(static_cast<Qt::ApplicationAttribute>(Qt::WA_StyledBackground)); // 启用 QSS

    CustomSplashScreen splashScreen;
    // 启动画面
    splashScreen.updateStatus("正在初始化...");
    MainWindow mainWindow;
    QObject::connect(&mainWindow, &MainWindow::initStatus, &splashScreen, &CustomSplashScreen::updateStatus);
    mainWindow.init();
//    主窗口初始化，所有控件实例化后才能进行信号连接
    QObject::connect(mainWindow.pluginsManagerDlg, &PluginsManagerWidget::loadPluginStatus, &splashScreen, &CustomSplashScreen::updateStatus);
    QObject::connect(&mainWindow, &MainWindow::initStatus, &splashScreen, [&splashScreen, &mainWindow](const QString& message) {
        if (message == "Initialization nodes list success") {
            splashScreen.finish(&mainWindow);

        }
    });
    mainWindow.pluginsManagerDlg->loadPluginsFromFolder();
    //从目录中加载所有插件
    mainWindow.initNodelist();
    mainWindow.setWindowTitle("NodeEditorCPP");



    mainWindow.restoreVisualState();
    if(argc > 1)
 //    设置文件双击打开
    {
        QString strOpenPath = QString(argv[1]);
        mainWindow.loadFileFromPath(&strOpenPath);
    }
    mainWindow.showMaximized();
    // 默认最大化显示
     ExternalControler controller(&mainWindow,Extra_Control_Port);
    // controller.show();
    return app.exec();
}
