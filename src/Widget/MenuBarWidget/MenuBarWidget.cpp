//
// Created by bwll1 on 2024/9/25.
//

#include "MenuBarWidget.h"
#include "QApplication"
#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include "Widget/AboutWidget/AboutWidget.hpp"
#include "ConstantDefines.h"
MenuBarWidget::MenuBarWidget(QWidget *parent) : QMenuBar(parent) {
    setupMenu();
}

void MenuBarWidget::setupMenu() {
    Files_menu = this->addMenu("文件");
    loadAction = Files_menu->addAction("打开文件");
    loadAction->setIcon(QIcon(":/icons/icons/open_flat.png"));
    recentFilesMenu = Files_menu->addMenu("最近打开");
    recentFilesMenu->setIcon(QIcon(":/icons/icons/history.png"));
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

    saveAction = Files_menu->addAction("保存");
    saveAction->setIcon(QIcon(":/icons/icons/save.png"));
    saveAsAction = Files_menu->addAction("保存为");
    saveAsAction->setIcon(QIcon(":/icons/icons/saveAs.png"));
    Files_menu->addSeparator();
    exitAction = Files_menu->addAction("退出");
    exitAction->setIcon(QIcon(":/icons/icons/exit.png"));

    Edit_menu=this->addMenu("编辑");
    //新建数据流程
    New_dataflow=Edit_menu->addAction("新建数据流");
    New_dataflow->setIcon(QIcon(":/icons/icons/add_database.png"));
    //清空数据流
    Clear_dataflows=Edit_menu->addAction("清空所有数据流");
    Clear_dataflows->setIcon(QIcon(":/icons/icons/delete_database.png"));
    lockAction = Edit_menu->addAction("编辑模式");
    lockAction->setCheckable(true);
    lockAction->setIcon(lockAction->isChecked()?QIcon(":/icons/icons/lock.png"):QIcon(":/icons/icons/unlock.png"));
    connect(lockAction, &QAction::toggled, [this](bool checked) {
        this->lockAction->setIcon(checked?QIcon(":/icons/icons/lock.png"):QIcon(":/icons/icons/unlock.png"));
    });

    clearAction = Edit_menu->addAction("清空日志");
    clearAction->setIcon(QIcon(":/icons/icons/clear.png"));

    View_menu=this->addMenu("视图");
    views=View_menu->addMenu("显示窗口");
    views->setIcon(QIcon(":/icons/icons/statistics.png"));
    View_menu->addSeparator();
    restoreLayout=View_menu->addAction("重置默认布局");
    restoreLayout->setIcon(QIcon(":/icons/icons/restore.png"));

    saveLayout=View_menu->addAction("保存为默认布局");
    saveLayout->setIcon(QIcon(":/icons/icons/layout.png"));

    // Setting_menu=this->addMenu("设置");
    // Setting_1=Setting_menu->addAction("设置1");
    // Setting_1->setIcon(QIcon(":/icons/icons/setting.png"));
    // Setting_2=Setting_menu->addAction("设置2");
    // Setting_2->setIcon(QIcon(":/icons/icons/setting.png"));

    Plugins_menu = this->addMenu("插件");
    pluginsManagerAction = Plugins_menu->addAction("插件管理器");
    pluginsManagerAction->setIcon(QIcon(":/icons/icons/plugins.png"));
    pluginsFloderAction =Plugins_menu->addAction("打开插件文件夹");
    pluginsFloderAction->setIcon(QIcon(":/icons/icons/open_flat.png"));
    createNodeAction=new QAction(QStringLiteral("Create node"));


    Tool_menu=this->addMenu("工具");
    tool1Action = Tool_menu->addAction("视频格式转换器");
    tool1Action->setIcon(QIcon(":/icons/icons/converty.png"));
    connect(tool1Action, &QAction::triggered, this, [this]() {

        openToolWithArgs("FormatConverter.exe", QStringList());
    });


    tool2Action = Tool_menu->addAction("OpenStageControl");
    tool2Action->setIcon(QIcon(":/icons/icons/osc.png"));
    connect(tool2Action, &QAction::triggered, this, [this]() {
            QStringList args;
            args << "--load"
                << "./extrnal-control-interface.json"             // 加载默认文件
                <<"--port"
                << QString::number(AppConstants::OSC_WEB_PORT)      //网页端口
                <<"--osc-port"
                << QString::number(AppConstants::EXTRA_FEEDBACK_PORT)      //osc端口
                << "--no-gui"                                   // 无gui运行
                <<"--theme"                                     //主题
                << "orange"
                << "--send"                                      // OSC发送端口
                << QString("%1:%2").arg(AppConstants::EXTRA_FEEDBACK_HOST).arg(AppConstants::EXTRA_CONTROL_PORT); //OSC网页服务主机


            openOSCInterface("open-stage-control/open-stage-control.exe", args);
       });

    ArtnetRecoderToolAction=Tool_menu->addAction("Artnet记录器");
    ArtnetRecoderToolAction->setIcon(QIcon(":/icons/icons/recoder.png"));
    connect(ArtnetRecoderToolAction, &QAction::triggered, this, [this]() {
        openToolWithArgs("ArtnetRecorder.exe", QStringList());
    });

    About_menu=this->addMenu("关于");
    helpAction=About_menu->addAction("帮助");
    helpAction->setIcon(QIcon(":/icons/icons/help.png"));
    connect(helpAction, &QAction::triggered, this, &MenuBarWidget::showHelp);

    aboutAction = About_menu->addAction("关于NodeStudio");
    aboutAction->setIcon(QIcon(":/icons/icons/about.png"));
    connect(aboutAction, &QAction::triggered, this, &MenuBarWidget::showAboutDialog);
    //关于软件窗口
    aboutQtAction = About_menu->addAction("关于QT");
    aboutQtAction->setIcon(QIcon(":/icons/icons/about.png"));
    connect(aboutQtAction, &QAction::triggered, this, &QApplication::aboutQt);
    // 关于qt窗口
}

void MenuBarWidget::openOSCInterface(const QString& exePath, const QStringList& args) {
     QString fullPath = QCoreApplication::applicationDirPath() + "/" + exePath;

    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(QCoreApplication::applicationDirPath());

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, &QProcess::deleteLater);
    if (QFile::exists(fullPath)) {
        process->start(fullPath, args);
        qDebug()<<"open stage control is running on http://localhost:"+QString::number(AppConstants::OSC_WEB_PORT);
    } else {
        QMessageBox::warning(this, "", QString("找不到可执行文件：%1").arg(fullPath));
        process->deleteLater();
    }
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
