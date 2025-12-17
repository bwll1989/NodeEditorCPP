#include <QtWidgets/QApplication>
#include <QProcessEnvironment>
#include <QObject>

#include <QStyleFactory>
#include <QMessageBox>
#include <QSharedMemory>
#include <QCommandLineParser>
#include <QScopedPointer>
#include <QScreen>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include "Widget/MainWindow/MainWindow.hpp"
#include "Widget/SplashWidget/CustomSplashScreen.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "ConstantDefines.h"
#include <QtWebEngineQuick/QtWebEngineQuick>

/**
 * @brief 生成单实例共享内存的唯一键
 * @return QString 格式为 "Organization_Product" 的稳定键值
 * @note 使用组织名与产品名组合，保证不同产品互不冲突，且应用名变化不影响单实例机制
 */
static QString makeSingleInstanceKey() {
    const QString org = QApplication::organizationName().isEmpty()
        ? AppConstants::COMPANY_NAME
        : QApplication::organizationName();
    const QString prod = QApplication::applicationName().isEmpty()
        ? AppConstants::PRODUCT_NAME
        : QApplication::applicationName();
    return org + "_" + prod;
}

/**
 * @brief 设置应用程序基本信息
 * 配置应用程序的显示名称、版本号、组织名称和域名
 * @note 版本号从 PRODUCT_VERSION 宏获取，若未定义则使用默认值 "dev"
 */
void setupAppInfo() {
    QApplication::setApplicationDisplayName(AppConstants::PRODUCT_NAME);
    QApplication::setApplicationVersion(PRODUCT_VERSION);
    QApplication::setOrganizationName(AppConstants::COMPANY_NAME);
    QApplication::setOrganizationDomain("www.qt.com");
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
        const QString& filePath = args.first();
        mainWindow.loadFileFromPath(filePath);
    }
}

/**
 * @brief 安全关闭启动界面并断开所有相关信号
 * @param splashScreen 启动画面指针
 * @param mainWindow 主窗口指针
 * @note 统一封装关闭逻辑，防止在关闭后仍然接收状态更新导致的多余渲染
 */
static void closeSplashSafely(const QScopedPointer<CustomSplashScreen>& splashScreen,
                              const QScopedPointer<MainWindow>& mainWindow)
{
    if (!splashScreen || !mainWindow) return;

    // 断开状态更新信号，避免关闭后仍绘制
    QObject::disconnect(mainWindow.data(), &MainWindow::initStatus,
                        splashScreen.data(), &CustomSplashScreen::updateStatus);
    QObject::disconnect(mainWindow->pluginsManagerDlg, &PluginsManagerWidget::loadPluginStatus,
                        splashScreen.data(), &CustomSplashScreen::updateStatus);

    // 关闭启动界面
    splashScreen->finish(mainWindow.data());
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
        QMessageBox::warning(nullptr, "", "应用程序已经在运行中。");
        return 1;
    }
    
    // 设置应用程序基本信息
    setupAppInfo();

    // 设置应用程序样式
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // 解析命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription(AppConstants::FILE_DESCRIPTION);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "要打开的文件路径");
    parser.process(app);
    
    // 创建启动画面
    QScopedPointer<CustomSplashScreen> splashScreen(new CustomSplashScreen());
    splashScreen->updateStatus("Preparing to open the program...");
    
    // 创建主窗口
    QScopedPointer<MainWindow> mainWindow(new MainWindow());
    
    // 连接初始化状态信号到启动画面（仅用于启动阶段的文字提示）
    QObject::connect(mainWindow.data(), &MainWindow::initStatus,
                     splashScreen.data(), &CustomSplashScreen::updateStatus);
    
    // 初始化主窗口
    mainWindow->init();
    
    // 插件加载状态也显示在启动画面
    QObject::connect(mainWindow->pluginsManagerDlg, &PluginsManagerWidget::loadPluginStatus,
                     splashScreen.data(), &CustomSplashScreen::updateStatus);
    
    // 加载插件和初始化节点列表（保持原有顺序）
    mainWindow->pluginsManagerDlg->loadPluginsFromFolder();
    mainWindow->initNodelist();
    
    // 如果有命令行参数，则处理命令行参数，由于处理命令行时也会有恢复视觉效果，所以这里不需要再恢复视觉状态
    if (!parser.positionalArguments().isEmpty()) {
        // 处理命令行参数可能触发文件加载
        handleCommandLineArguments(parser, *mainWindow);
    }
    // else{
    //     // 如果没有命令行参数，则恢复视觉状态
    //     mainWindow->restoreVisualState();
    // }
    // 所有都处理完成后再显示主窗口，并关闭启动界面
    mainWindow->showMaximized();

    closeSplashSafely(splashScreen, mainWindow);

    // 启动应用程序事件循环
    int result = app.exec();

    // 完成后分离共享内存
    sharedMemory.detach();

    return result;
}
