#include <QtWidgets/QApplication>
#include <QProcessEnvironment>
#include <QObject>

#include <QStyleFactory>
#include <QMessageBox>
#include <QSharedMemory>
#include <QCommandLineParser>
#include <QScopedPointer>
#include <QScreen>
#include "Widget/MainWindow/MainWindow.hpp"
#include "Widget/SplashWidget/CustomSplashScreen.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
// 应用程序常量定义
namespace AppConstants {
    // 外部控制端口号
    constexpr int EXTRA_CONTROL_PORT = 8991;
    // 产品信息
    const QString PRODUCT_VERSION = "1.0";
    const QString COMPANY_NAME = "Bwll";
    const QString PRODUCT_NAME = "NodeStudio";
    const QString LUA_PATH = "./lua;";
    const QString FILE_DESCRIPTION = "Qt Creator based on Qt 6.6 (MinGw, 64 bit)";
    const QString LEGAL_COPYRIGHT = "Copyright 2008-2016 The Qt Company Ltd. All rights reserved.";
}

/**
 * @brief 设置应用程序基本信息
 * 配置应用程序的显示名称、版本号、组织名称和域名
 */
void setupAppInfo() {
    QApplication::setApplicationDisplayName(AppConstants::PRODUCT_NAME);
    QApplication::setApplicationVersion(AppConstants::PRODUCT_VERSION);
    QApplication::setOrganizationName(AppConstants::COMPANY_NAME);
    QApplication::setOrganizationDomain("www.qt.com");
}

/**
 * @brief 设置应用程序样式
 * @param app QApplication对象的引用
 * @return bool 样式设置是否成功
 */
bool setupAppStyle(QApplication& app) {
    // 设置应用程序风格为Fusion
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    // 启用 QSS 样式表支持
    QApplication::setAttribute(static_cast<Qt::ApplicationAttribute>(Qt::WA_StyledBackground));
    
    // 加载样式表
    QFile qssFile(":/styles/styles/DefaultDark.qss");
    // QFile qssFile(":/darkstyle/darkstyle/darkstyle.qss");
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load QSS file:" << qssFile.errorString();
        return false;
    }
    
    QTextStream stream(&qssFile);
    QString styleSheet = stream.readAll();
    qssFile.close();
    
    if (styleSheet.isEmpty()) {
        qWarning() << "QSS file is empty or could not be read properly";
        return false;
    }
    
    try {
        app.setStyleSheet(styleSheet);
    } catch (const std::exception& e) {
        qWarning() << "Error setting stylesheet:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "Unknown error setting stylesheet";
        return false;
    }
    
    return true;
}

/**
 * @brief 检查应用程序是否已经在运行
 * @param sharedMemory 共享内存对象的引用
 * @return bool 如果应用程序已经在运行则返回true
 */
bool isApplicationRunning(QSharedMemory& sharedMemory) {
    if (sharedMemory.attach()) {
        sharedMemory.detach();
        return true;
    }
    
    // 创建1byte大小的共享内存段
    if (!sharedMemory.create(1)) {
        qWarning() << "Failed to create shared memory:" << sharedMemory.errorString();
        return true; // 如果创建失败，也认为应用程序已经在运行
    }
    
    return false;
}

/**
 * @brief 处理命令行参数
 * @param parser 命令行解析器
 * @param mainWindow 主窗口引用
 */
void handleCommandLineArguments(const QCommandLineParser& parser, MainWindow& mainWindow) {
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        QString filePath = args.first();
        mainWindow.loadFileFromPath(&filePath);
    }
}

/**
 * @brief 应用程序入口函数
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码
 */
int main(int argc, char *argv[])
{
    // 设置高DPI支持
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6中高DPI缩放默认启用，不需要显式设置AA_EnableHighDpiScaling
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    // Qt 5中需要显式启用高DPI缩放
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    
    // 创建Qt应用程序实例
    QApplication app(argc, argv);
    app.setApplicationName(AppConstants::PRODUCT_NAME);
    
    // 设置工作目录为可执行文件所在目录
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    
    // 检查应用程序是否已经在运行
    QSharedMemory sharedMemory(app.applicationName());
    if (isApplicationRunning(sharedMemory)) {
        QMessageBox::warning(nullptr, "警告", "应用程序已经在运行中。");
        return 1;
    }
    
    // 设置应用程序基本信息
    setupAppInfo();
    
    // 设置应用程序样式
    if (!setupAppStyle(app)) {
        qWarning() << "应用程序样式设置失败，将使用默认样式。";
        QMessageBox::warning(nullptr, "样式警告", "应用程序样式加载失败，将使用默认样式。\n应用程序功能不受影响，但视觉效果可能不理想。");
    }
    
    // 解析命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription(AppConstants::FILE_DESCRIPTION);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "要打开的文件路径");
    parser.process(app);
    
    // 创建启动画面
    QScopedPointer<CustomSplashScreen> splashScreen(new CustomSplashScreen());
    splashScreen->updateStatus("正在初始化...");
    
    // 创建主窗口
    QScopedPointer<MainWindow> mainWindow(new MainWindow());
    
    // 连接初始化状态信号到启动画面
    QObject::connect(mainWindow.data(), &MainWindow::initStatus, 
                     splashScreen.data(), &CustomSplashScreen::updateStatus);
    
    // 初始化主窗口
    mainWindow->init();
    
    // 主窗口初始化完成后，连接插件管理器的状态信号
    QObject::connect(mainWindow->pluginsManagerDlg, &PluginsManagerWidget::loadPluginStatus, 
                     splashScreen.data(), &CustomSplashScreen::updateStatus);
    
    // 当收到初始化节点库成功的消息时，关闭启动画面
    QObject::connect(mainWindow.data(), &MainWindow::initStatus, 
                     [&splashScreen, &mainWindow](const QString& message) {
        if (message == "Initialization nodes library success") {
            splashScreen->finish(mainWindow.data());
        }
    });
    
    // 加载插件和初始化节点列表
    mainWindow->pluginsManagerDlg->loadPluginsFromFolder();
    mainWindow->initNodelist();
    

    
    // 处理命令行参数
    handleCommandLineArguments(parser, *mainWindow);
    
    // 最大化显示主窗口
    mainWindow->showMaximized();
    // 恢复窗口视觉状态
    mainWindow->restoreVisualState();
    // 启动应用程序事件循环
    int result = app.exec();
    
    // 完成后分离共享内存
    sharedMemory.detach();
    
    return result;
}
