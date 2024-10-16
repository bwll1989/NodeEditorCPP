#include <QtWidgets/QApplication>
#include <QProcessEnvironment>
#include <QObject>
#include <QtGui>
#include "QStyleFactory"
#include "Widget/MainWindow/MainWindow.h"
#include "Widget/SplashWidget/CustomSplashScreen.h"
#include "Widget/ExternalControl/ExternalControler.h"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#define Extra_Control_Port 8990
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
    // 获取当前的环境变量
//    QString path = QString::fromLocal8Bit(getenv("PATH"));
//
//    // 在 PATH 前添加自定义 DLL 路径
//    path.prepend(LUA_PATH);
//    // 使用 _putenv 设置新的 PATH
//    _putenv(("PATH=" + path).toLocal8Bit().data());
    //设置软件信息
}

int main(int argc, char *argv[])
{
    // 启用从应用程序的工作目录加载 DLL
//    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    // 添加多个 DLL 目录
//    AddDllDirectory(L"./lua");
//    SetDllDirectory(L"./lua");

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

    ExternalControler controller(&mainWindow,Extra_Control_Port);


    if(argc > 1)
 //    设置文件双击打开
    {
        QString strOpenPath = QString(argv[1]);
        mainWindow.loadFileFromPath(&strOpenPath);
    }
    mainWindow.showMaximized();
    // 默认最大化显示
    return app.exec();
}
