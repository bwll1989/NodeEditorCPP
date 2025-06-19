//
// Created by bwll1 on 2024/9/25.
//

#include "MenuBarWidget.h"
#include "QApplication"
#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>
#include <QFile>
#include "Widget/AboutWidget/AboutWidget.hpp"
MenuBarWidget::MenuBarWidget(QWidget *parent) : QMenuBar(parent) {
    

    setupMenu();
}

void MenuBarWidget::setupMenu() {
    Files_menu = this->addMenu("文件");
    saveAction = Files_menu->addAction("保存场景");
    saveAction->setIcon(QIcon(":/icons/icons/save.png"));
    saveAsAction = Files_menu->addAction("另存为");
    saveAsAction->setIcon(QIcon(":/icons/icons/saveAs.png"));
    loadAction = Files_menu->addAction("打开场景");
    loadAction->setIcon(QIcon(":/icons/icons/open_flat.png"));
    Files_menu->addSeparator();
    exitAction = Files_menu->addAction("退出");
    exitAction->setIcon(QIcon(":/icons/icons/exit.png"));
    Edit_menu=this->addMenu("编辑");
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
    views->setIcon(QIcon(":/icons/icons/clone.png"));
    View_menu->addSeparator();
    restoreLayout=View_menu->addAction("恢复布局");
    restoreLayout->setIcon(QIcon(":/icons/icons/restore.png"));
    saveLayout=View_menu->addAction("保存布局");
    saveLayout->setIcon(QIcon(":/icons/icons/layout.png"));

    Setting_menu=this->addMenu("设置");
    Setting_1=Setting_menu->addAction("setting_1");
    Setting_1->setIcon(QIcon(":/icons/icons/setting.png"));
    Setting_2=Setting_menu->addAction("setting_2");
    Setting_2->setIcon(QIcon(":/icons/icons/setting.png"));

    Plugins_menu = this->addMenu("插件");
    pluginsManagerAction = Plugins_menu->addAction("插件管理");
    pluginsManagerAction->setIcon(QIcon(":/icons/icons/plugins.png"));
    pluginsFloderAction =Plugins_menu->addAction("打开插件目录");
    pluginsFloderAction->setIcon(QIcon(":/icons/icons/open_flat.png"));
    createNodeAction=new QAction(QStringLiteral("Create node"));


    Tool_menu=this->addMenu("工具");
    tool1Action = Tool_menu->addAction("FFmpeg转码工具");
    tool1Action->setIcon(QIcon(":/icons/icons/tools.png"));
    connect(tool1Action, &QAction::triggered, this, [this]() {
        QStringList args;
        args << "-version";  // 示例参数，显示FFmpeg版本信息
        openToolWithArgs("ffmpeg.exe", args);
    });


    tool2Action = Tool_menu->addAction("open-stage-control");
    tool2Action->setIcon(QIcon(":/icons/icons/osc.png"));
    connect(tool2Action, &QAction::triggered, this, [this]() {
            QStringList args;
            args << "--load"
                << "./extrnal-control-interface.json" // 无gui运行
                    << "--no-gui"
                << "--send"
                <<"127.0.0.1:8991";  // 主机地址
            openToolWithArgs("open-stage-control/open-stage-control.exe", args);
       });
    About_menu=this->addMenu("关于");
    aboutAction = About_menu->addAction("关于NodeEditorCPP");
    aboutAction->setIcon(QIcon(":/icons/icons/about.png"));
    connect(aboutAction, &QAction::triggered, this, &MenuBarWidget::showAboutDialog);
    //关于软件窗口
    aboutQtAction = About_menu->addAction("关于QT");
    aboutQtAction->setIcon(QIcon(":/icons/icons/about.png"));
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
        qDebug()<<"open stage control is running on port 8080";
    } else {
        QMessageBox::warning(this, "错误", QString("找不到可执行文件：%1").arg(fullPath));
        process->deleteLater();
    }
}

//软件关于
void MenuBarWidget::showAboutDialog() {

    AboutWidget aboutDialog(":/docs/README.txt");
    //    关于窗口显示文档设置
    aboutDialog.exec();
}