//
// Created by bwll1 on 2024/9/25.
//

#include "MenuBarWidget.h"
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFile>
#include "AboutWidget.hpp"
#include "ConstantDefines.h"
#include "SystemInfoWidget.h"

MenuBarWidget::MenuBarWidget(QWidget *parent) : QMenuBar(parent) {
    this->setFont(QApplication::font());
    setupMenu();
}

void MenuBarWidget::setupMenu() {
    Files_menu = this->addMenu("文件");
    Files_menu->setWindowFlags(Files_menu->windowFlags() | Qt::NoDropShadowWindowHint);
    Files_menu->setAttribute(Qt::WA_TranslucentBackground, false);
    loadAction = Files_menu->addAction(QIcon(":/icons/icons/open_flat.png"),"打开文件");
    recentFilesMenu = Files_menu->addMenu(QIcon(":/icons/icons/history.png"),"最近打开");
    recentFilesMenu->setWindowFlags(recentFilesMenu->windowFlags() | Qt::NoDropShadowWindowHint);
    recentFilesMenu->setAttribute(Qt::WA_TranslucentBackground, false);
    for (int i = 0; i < AppConstants::MaxRecentFiles; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        // 将动作加入子菜单
        recentFilesMenu->addAction(recentFileActs[i]);
        // 点击后发出路径信号，由 MainWindow 处理实际打开
        connect(recentFileActs[i], &QAction::triggered, this, [this, i]() {
            emit recentFileTriggered(recentFileActs[i]->data().toString());
        });
    }
    Files_menu->addSeparator();

    saveAction = Files_menu->addAction(QIcon(":/icons/icons/save.png"),"保存");
    saveAsAction = Files_menu->addAction(QIcon(":/icons/icons/saveAs.png"),"保存为");
    Files_menu->addSeparator();
    exitAction = Files_menu->addAction(QIcon(":/icons/icons/exit.png"),"退出");

    Edit_menu=this->addMenu("编辑");
    Edit_menu->setWindowFlags(Edit_menu->windowFlags() | Qt::NoDropShadowWindowHint);
    Edit_menu->setAttribute(Qt::WA_TranslucentBackground, false);
    //新建数据流程
    New_dataflow=Edit_menu->addAction(QIcon(":/icons/icons/add_database.png"),"新建数据流");
    //清空数据流
    Clear_dataflows=Edit_menu->addAction(QIcon(":/icons/icons/delete_database.png"),"清空所有数据流");
    lockAction = Edit_menu->addAction(QIcon(":/icons/icons/unlock.png"),"编辑模式");
    lockAction->setCheckable(true);
    lockAction->setIcon(lockAction->isChecked()?QIcon(":/icons/icons/lock.png"):QIcon(":/icons/icons/unlock.png"));
    connect(lockAction, &QAction::toggled, [this](bool checked) {
        this->lockAction->setIcon(checked?QIcon(":/icons/icons/lock.png"):QIcon(":/icons/icons/unlock.png"));
    });

    clearAction = Edit_menu->addAction(QIcon(":/icons/icons/clear.png"),"清空日志");

    View_menu=this->addMenu("视图");
    View_menu->setWindowFlags(View_menu->windowFlags() | Qt::NoDropShadowWindowHint);
    View_menu->setAttribute(Qt::WA_TranslucentBackground, false);
    //切换主题
    switchTheme=View_menu->addAction(QIcon(":/icons/icons/landscape.png"),"切换主题");

    views=View_menu->addMenu(QIcon(":/icons/icons/statistics.png"),"显示窗口");
    views->setWindowFlags(views->windowFlags() | Qt::NoDropShadowWindowHint);
    views->setAttribute(Qt::WA_TranslucentBackground, false);

    View_menu->addSeparator();
    restoreLayout=View_menu->addAction(QIcon(":/icons/icons/restore.png"),"重置默认布局");

    saveLayout=View_menu->addAction(QIcon(":/icons/icons/layout.png"),"保存为默认布局");

    // Setting_menu=this->addMenu("设置");
    // Setting_1=Setting_menu->addAction("设置1");
    // Setting_1->setIcon(QIcon(":/icons/icons/setting.png"));
    // Setting_2=Setting_menu->addAction("设置2");
    // Setting_2->setIcon(QIcon(":/icons/icons/setting.png"));

    Plugins_menu = this->addMenu("插件");
    Plugins_menu->setWindowFlags(Plugins_menu->windowFlags() | Qt::NoDropShadowWindowHint);
    Plugins_menu->setAttribute(Qt::WA_TranslucentBackground, false);
    pluginsManagerAction = Plugins_menu->addAction(QIcon(":/icons/icons/plugins.png"),"插件管理器");
    pluginsFloderAction =Plugins_menu->addAction(QIcon(":/icons/icons/open_flat.png"),"打开插件文件夹");

    createNodeAction=new QAction(QStringLiteral("Create node"));

    Tool_menu=this->addMenu("工具");
    Tool_menu->setWindowFlags(Tool_menu->windowFlags() | Qt::NoDropShadowWindowHint);
    Tool_menu->setAttribute(Qt::WA_TranslucentBackground, false);
    FormatConverter = Tool_menu->addAction(QIcon(":/icons/icons/converty.png"),"视频格式转换器");
    connect(FormatConverter, &QAction::triggered, this, [this]() {

        openToolWithArgs("FormatConverter.exe", QStringList());
    });


    WebInterface = Tool_menu->addAction(QIcon(":/icons/icons/home.png"),"打开网页控制台");
    connect(WebInterface, &QAction::triggered, this, &MenuBarWidget::openDashboard);

    ArtnetRecoderToolAction=Tool_menu->addAction(QIcon(":/icons/icons/recoder.png"),"Artnet记录器");
    connect(ArtnetRecoderToolAction, &QAction::triggered, this, [this]() {
        openToolWithArgs("ArtnetRecorder.exe", QStringList());
    });
    systemInfoAction=Tool_menu->addAction(QIcon(":/icons/icons/property.png"),"系统信息");
    connect(systemInfoAction, &QAction::triggered, this, [this]() {
        SystemInfoWidget systemInfoWidget;
        systemInfoWidget.exec();
    });


    About_menu=this->addMenu("关于");
    About_menu->setWindowFlags(About_menu->windowFlags() | Qt::NoDropShadowWindowHint);
    About_menu->setAttribute(Qt::WA_TranslucentBackground, false);
    helpAction=About_menu->addAction(QIcon(":/icons/icons/help.png"),"帮助");
    connect(helpAction, &QAction::triggered, this, &MenuBarWidget::showHelp);

    aboutAction = About_menu->addAction(QIcon(":/icons/icons/about.png"),"关于NodeStudio");

    connect(aboutAction, &QAction::triggered, this, &MenuBarWidget::showAboutDialog);
    //关于软件窗口
    aboutQtAction = About_menu->addAction(QIcon(":/icons/icons/about.png"),"关于QT");
    connect(aboutQtAction, &QAction::triggered, this, &QApplication::aboutQt);
    // 关于qt窗口
}


void MenuBarWidget::openToolWithArgs(const QString& exePath, const QStringList& args) {
    QString fullPath = QCoreApplication::applicationDirPath() + "/" + exePath;

    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(QCoreApplication::applicationDirPath());

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, &QProcess::deleteLater);
    
    if (QFile::exists(fullPath)) {
        process->start(fullPath, args);
    } else {
        QMessageBox::warning(this, "", QString("找不到可执行文件：%1").arg(fullPath));
        process->deleteLater();
    }
}

//软件关于
void MenuBarWidget::showAboutDialog() {

    AboutWidget aboutDialog(":/docs/README.md");
    //    关于窗口显示文档设置
    aboutDialog.exec();
}

//帮助文档
void MenuBarWidget::showHelp() {
    QString helpPath = QCoreApplication::applicationDirPath() + "/html/index.html";
    QDesktopServices::openUrl(QUrl::fromLocalFile(helpPath));
}

// 函数级注释：在默认浏览器中打开本地HTTP服务器页面（Dashboard）
void MenuBarWidget::openDashboard() {
    const QUrl url(QString("http://127.0.0.1:%1/").arg(AppConstants::HTTP_SERVER_PORT));
    QDesktopServices::openUrl(url);
}
/**
 * 函数级注释：更新“最近打开”菜单的显示内容
 * - 显示最多 MaxRecentFiles 项，文本为序号+文件名，tooltip 为完整路径
 * - 如果列表长度不足，隐藏多余条目
 */
void MenuBarWidget::updateRecentFileActions(const QStringList& files) {
    const int count = qMin(files.size(), 5);
    for (int i = 0; i < count; ++i) {
        const QString path = files.at(i);
        const QFileInfo fi(path);
        recentFileActs[i]->setText(QString::number(i + 1) + " " + fi.fileName());
        recentFileActs[i]->setData(path);
        recentFileActs[i]->setToolTip(path);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = count; j < AppConstants::MaxRecentFiles; ++j) {
        recentFileActs[j]->setVisible(false);
        recentFileActs[j]->setData(QString());
        recentFileActs[j]->setText(QString());
        recentFileActs[j]->setToolTip(QString());
    }
}
