#include <QtWidgets/QApplication>
#include <QProcessEnvironment>
#include <QObject>
#include <QtGui>
#include "QStyleFactory"
#include "Widget/MainWindow/MainWindow.hpp"
#include "Widget/SplashWidget/CustomSplashScreen.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"

// 外部控制端口号
#define Extra_Control_Port 8991
#define PRODUCT_VERSION        "1.0"   // 产品版本
#define COMPANY_NAME           "Bwll"  // 公司名称
#define PRODUCT_NAME          "NodeStudio" // 产品名称
#define LUA_PATH                "./lua;"   // Lua脚本路径
#define FILE_DESCRIPTION       "Qt Creator based on Qt 6.6 (MinGw, 64 bit)"  // 文件说明
#define LEGAL_COPYRIGHT        "Copyright 2008-2016 The Qt Company Ltd. All rights reserved." // 版权信息

// 设置应用程序基本信息
void setupApp(){
    QApplication::setApplicationDisplayName(PRODUCT_NAME);      // 设置显示名称
    QApplication::setApplicationVersion(PRODUCT_VERSION);       // 设置版本号
    QApplication::setOrganizationName(COMPANY_NAME);           // 设置组织名称
    QApplication::setOrganizationDomain("www.qt.com");         // 设置组织域名
}

int main(int argc, char *argv[])
{
    // 创建Qt应用程序实例
    QApplication app(argc, argv);

    // 设置工作目录为可执行文件所在目录
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    setupApp();
    qApp->setStyle(QStyleFactory::create("Fusion"));           // 设置应用程序风格为Fusion
    qApp->setAttribute(static_cast<Qt::ApplicationAttribute>(Qt::WA_StyledBackground)); // 启用 QSS 样式表支持

    // 创建启动画面
    CustomSplashScreen splashScreen;
    splashScreen.updateStatus("正在初始化...");

    // 创建主窗口
    MainWindow mainWindow;
    
    // 连接初始化状态信号到启动画面
    QObject::connect(&mainWindow, &MainWindow::initStatus, &splashScreen, &CustomSplashScreen::updateStatus);
    mainWindow.init();

    // 主窗口初始化完成后，连接插件管理器的状态信号
    QObject::connect(mainWindow.pluginsManagerDlg, &PluginsManagerWidget::loadPluginStatus, &splashScreen, &CustomSplashScreen::updateStatus);
    
    // 当收到初始化节点库成功的消息时，关闭启动画面
    QObject::connect(&mainWindow, &MainWindow::initStatus, &splashScreen, [&splashScreen, &mainWindow](const QString& message) {
        if (message == "Initialization nodes library success") {
            splashScreen.finish(&mainWindow);
        }
    });

    // 加载插件和初始化节点列表
    mainWindow.pluginsManagerDlg->loadPluginsFromFolder();     // 从目录中加载所有插件
    mainWindow.initNodelist();                                 // 初始化节点列表

    // 恢复窗口视觉状态
    mainWindow.restoreVisualState();

    // 处理命令行参数：如果有参数，则打开指定文件
    if(argc > 1)
    {
        QString strOpenPath = QString(argv[1]);
        // 加载文件
        mainWindow.loadFileFromPath(&strOpenPath);
        // 恢复窗口布局到上次保存的状态
        mainWindow.restoreVisualState();
    }

    // 最大化显示主窗口
    mainWindow.showMaximized();

    // 启动应用程序事件循环
    return app.exec();
}
