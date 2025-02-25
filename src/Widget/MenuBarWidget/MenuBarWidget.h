//
// Created by bwll1 on 2024/9/25.
//

#ifndef NODEEDITORCPP_MENUBARWIDGET_H
#define NODEEDITORCPP_MENUBARWIDGET_H

#include <QMenuBar>
#include <QAction>

class MenuBarWidget:public QMenuBar {
Q_OBJECT

public:
    explicit MenuBarWidget(QWidget *parent = nullptr);
    void setupMenu();
    QMenu *Files_menu ;
    QMenu *views;
    QAction *saveAction ;
    QAction *saveAsAction ;
    QAction * loadAction ;
    QAction *exitAction;
    QMenu *Edit_menu;
    QAction *lockAction ;
    QAction *clearAction ;
    QMenu *Setting_menu;
    QAction *Setting_1 ;
    QAction *Setting_2 ;
    QMenu *View_menu;
    QAction *restoreLayout;
    QAction *saveLayout;
    QMenu *Plugins_menu ;
    QAction *pluginsManagerAction;
    QAction *pluginsFloderAction;
    QAction *createNodeAction;
    QMenu *Tool_menu;
    QAction *tool1Action ;
    QAction *tool2Action ;
    QMenu *About_menu;
    QAction *aboutAction ;
    QAction *aboutQtAction ;
};

#endif //NODEEDITORCPP_MENUBARWIDGET_H
