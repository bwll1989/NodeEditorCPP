//
// Created by bwll1 on 2024/9/25.
//

#include "MenuBarWidget.h"
#include "QApplication"

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
    tool1Action = Tool_menu->addAction("tool-1");
    tool1Action->setIcon(QIcon(":/icons/icons/tools.png"));
    tool2Action = Tool_menu->addAction("tool-2");
    tool2Action->setIcon(QIcon(":/icons/icons/tools.png"));

    About_menu=this->addMenu("关于");
    aboutAction = About_menu->addAction("关于NodeEditorCPP");
    aboutAction->setIcon(QIcon(":/icons/icons/about.png"));
    aboutQtAction = About_menu->addAction("关于QT");
    aboutQtAction->setIcon(QIcon(":/icons/icons/about.png"));
    connect(aboutQtAction, &QAction::triggered, this, &QApplication::aboutQt);
    // 关于qt窗口
}
