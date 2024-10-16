//
// Created by bwll1 on 2024/9/25.
//

#include "MenuBarWidget.h"
#include "QApplication"

MenuBarWidget::MenuBarWidget(QWidget *parent) : QMenuBar(parent) {
    awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    setupMenu();
}

void MenuBarWidget::setupMenu() {
    Files_menu = this->addMenu("文件");
    saveAction = Files_menu->addAction("保存场景");
    saveAction->setIcon(awesome->icon("fa-solid fa-floppy-disk"));
    loadAction = Files_menu->addAction("打开场景");
    loadAction->setIcon(awesome->icon("fa-solid fa-file-pen"));
    Files_menu->addSeparator();
    exitAction = Files_menu->addAction("退出");
    exitAction->setIcon(awesome->icon("fa-solid fa-arrow-right-from-bracket"));
    Edit_menu=this->addMenu("编辑");
    lockAction = Edit_menu->addAction("编辑模式");
    lockAction->setCheckable(true);
    lockAction->setIcon(lockAction->isChecked()?awesome->icon("fa-solid fa-lock"):awesome->icon("fa-solid fa-unlock"));
    connect(lockAction, &QAction::toggled, [this](bool checked) {
        this->lockAction->setIcon(checked?awesome->icon("fa-solid fa-lock"):awesome->icon("fa-solid fa-unlock"));
    });
    clearAction = Edit_menu->addAction("清空日志");
    clearAction->setIcon(awesome->icon("fa-solid fa-broom"));

    View_menu=this->addMenu("视图");
    views=View_menu->addMenu("显示窗口");
    views->setIcon(awesome->icon("fa-regular fa-clone"));
    View_menu->addSeparator();
    restoreLayout=View_menu->addAction("恢复布局");
    restoreLayout->setIcon(awesome->icon("fa-solid fa-download"));
    saveLayout=View_menu->addAction("保存布局");
    saveLayout->setIcon(awesome->icon("fa-solid fa-upload"));

    Setting_menu=this->addMenu("设置");
    Setting_1=Setting_menu->addAction("setting_1");
    Setting_1->setIcon(awesome->icon("fa-solid fa-gears"));
    Setting_2=Setting_menu->addAction("setting_2");
    Setting_2->setIcon(awesome->icon("fa-solid fa-gears"));

    Plugins_menu = this->addMenu("插件");
    pluginsManagerAction = Plugins_menu->addAction("插件管理");
    pluginsManagerAction->setIcon(awesome->icon("fa-solid fa-plug-circle-check"));
    pluginsFloderAction =Plugins_menu->addAction("打开插件目录");
    pluginsFloderAction->setIcon(awesome->icon("fa-solid fa-folder-open"));
    createNodeAction=new QAction(QStringLiteral("Create node"));


    Tool_menu=this->addMenu("工具");
    tool1Action = Tool_menu->addAction("tool-1");
    tool1Action->setIcon(awesome->icon("fa-solid fa-wrench"));
    tool2Action = Tool_menu->addAction("tool-2");
    tool2Action->setIcon(awesome->icon("fa-solid fa-wrench"));

    About_menu=this->addMenu("关于");
    aboutAction = About_menu->addAction("关于NodeEditorCPP");
    aboutAction->setIcon(awesome->icon("fa-solid fa-info"));
    aboutQtAction = About_menu->addAction("关于QT");
    aboutQtAction->setIcon(awesome->icon("fa-solid fa-q"));
    connect(aboutQtAction, &QAction::triggered, this, &QApplication::aboutQt);
    // 关于qt窗口
}
