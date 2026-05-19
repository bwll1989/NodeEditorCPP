#include <QtWidgets/QApplication>
#include <QSharedMemory>
#include <QCommandLineParser>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include "Widget/MainWindow/MainWindowHeadLess.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Common/AppConfig/ConstantDefines.h"
#include "Widget/SplashWidget/CustomSplashScreen.hpp"

/**
 * @brief 生成单实例共享内存的唯一键
 * @return QString 格式为 "Organization_Product" 的稳定键值
 * @note 使用组织名与产品名组合，保证不同产品互不冲突，且应用名变化不影响单实例机制
 */
static QString makeSingleInstanceKey() {
    return QString(AppConstants::COMPANY_NAME) + "_" + QString(AppConstants::PRODUCT_NAME);
}

/**
 * @brief 设置应用程序基本信息
 * 配置应用程序的显示名称、版本号、组织名称和域名
 * @note 版本号从 PRODUCT_VERSION 宏获取，若未定义则使用默认值 "dev"
 */
void setupAppInfo() {
    QApplication::setApplicationDisplayName(AppConstants::PRODUCT_RUNTIME_NAME);
    QApplication::setApplicationVersion(PRODUCT_VERSION);
    QApplication::setOrganizationName(AppConstants::COMPANY_NAME);
    QApplication::setOrganizationDomain("www.qt.com");
}

/**
 * @brief 检查应用程序是否已经在运行
 * @param sharedMemory 共享内存对象的引用
 * @return bool 如果应用程序已经在运行则返回true
 * @note 无头模式不弹窗，仅输出日志
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
 * @brief 处理命令行参数（仅解析待加载的 flow 文件路径）
 * @param parser 命令行解析器
 * @return QString 解析到的文件路径（若为空表示未提供）
 * @note 仅使用位置参数 "file"
 */
QString parseFlowFileArg(const QCommandLineParser& parser) {
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        return args.first();
    }
    return QString();
}

/**
 * @brief 应用程序入口函数（无头模式）
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return int 程序退出码
 * @note 无任何界面；功能复用 GUI 版本的插件与节点执行逻辑
 */
int main(int argc, char *argv[])
{
    // 创建Qt应用程序实例
    QApplication app(argc, argv);
    // 无窗口也不自动退出（维持事件循环）
    QApplication::setQuitOnLastWindowClosed(false);

    // 设置工作目录为可执行文件所在目录
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // 应用程序基本信息
    setupAppInfo();

    // 解析命令行参数（flow 文件路径 + 自重启延迟）
    QCommandLineParser parser;
    parser.setApplicationDescription(AppConstants::FILE_DESCRIPTION);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "要打开的flow文件路径");
    QCommandLineOption restartDelayOpt("restart-delay-ms", "自重启启动前延迟毫秒（避免单实例冲突）", "ms");
    parser.addOption(restartDelayOpt);
    parser.process(app);
    const QString flowFile = parseFlowFileArg(parser);

    int restartDelayMs = 0;
    if (parser.isSet(restartDelayOpt)) {
        bool ok = false;
        restartDelayMs = parser.value(restartDelayOpt).toInt(&ok);
        if (!ok) restartDelayMs = 0;
    }
    restartDelayMs = qBound(0, restartDelayMs, 5000);
    if (restartDelayMs > 0) {
        QThread::msleep(static_cast<unsigned long>(restartDelayMs));
    }

    // 单实例检查（无头模式不弹窗，避免阻塞自重启流程）
    QSharedMemory sharedMemory(makeSingleInstanceKey());
    if (isApplicationRunning(sharedMemory)) {
        qWarning() << "应用程序已经在运行中。";
        return 1;
    }
    // 创建统一的启动画面（程序启动到文件加载完成期间共用）
    QScopedPointer<CustomSplashScreen> splashScreen(new CustomSplashScreen());
    splashScreen->updateStatus("Preparing to open the program...");

    // 创建主窗口（Headless），并注入统一的 SplashScreen
    QScopedPointer<MainWindowHeadLess> mainWindow(new MainWindowHeadLess(nullptr));
    //注入启动画面对象到主窗口
    mainWindow->setSplashScreen(splashScreen.data());
    // 将主窗口的初始化状态信号转发到统一的 SplashScreen
    QObject::connect(mainWindow.data(), &MainWindowHeadLess::initStatus,
                     splashScreen.data(), &CustomSplashScreen::updateStatus);
    // 初始化主窗口与插件
    mainWindow->init();
    // 加载插件与初始化节点列表（保持 GUI 版本顺序）
    if (mainWindow->pluginsManagerDlg) {
        mainWindow->pluginsManagerDlg->loadPluginsFromFolder();
    }
    // 加载 flow 文件（无视觉状态恢复）
    if (!flowFile.isEmpty()) {
        splashScreen->updateStatus( "Loading: " + flowFile);
        if (mainWindow->loadFileFromPath(flowFile)) {
            splashScreen->updateStatus( "Flow file loaded successfully: " + flowFile);
            splashScreen->finish(mainWindow.data());
        } else {
            splashScreen->updateStatus( "Flow file loaded fail,try open recent file... ");
            if (!mainWindow->loadRecentFile()) {
                splashScreen->updateStatus( "Failed to load recent file (headless)...");
                splashScreen->finish(mainWindow.data());
                qCritical()<<"Flow file loaded failed: " + flowFile;
                return 2;

            }
        }
    }else {
        splashScreen->updateStatus( " load recent file..." );
        if (!mainWindow->loadRecentFile()) {
            splashScreen->updateStatus( "Failed to load recent file (headless)...");
            splashScreen->finish(mainWindow.data());
            qCritical()<<"Failed to load recent file (headless)";
            return 3;
        }
        splashScreen->finish(mainWindow.data());
    }
    // 开始事件循环（无头模式：直至任务结束或外部终止）
    const int result = app.exec();

    // 结束时分离共享内存
    sharedMemory.detach();
    return result;
}
