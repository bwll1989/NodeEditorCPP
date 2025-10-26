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
    /**
     * 设置菜单
     */
    void setupMenu();
    //文件菜单
    QMenu *Files_menu ;
    //视图菜单
    QMenu *views;
    //保存
    QAction *saveAction ;
    //保存为
    QAction *saveAsAction ;
    //加载
    QAction * loadAction ;
    //退出
    QAction *exitAction;
    //编辑菜单
    QMenu *Edit_menu;
    //新建数据流程
    QAction *New_dataflow;
    //清空所有数据流
    QAction *Clear_dataflows;
    //锁定
    QAction *lockAction ;
    //清除
    QAction *clearAction ;
    //设置菜单
    QMenu *Setting_menu;
    //设置1
    QAction *Setting_1 ;
    //设置2
    QAction *Setting_2 ;
    //视图菜单
    QMenu *View_menu;
    //恢复布局
    QAction *restoreLayout;
    //保存布局
    QAction *saveLayout;
    //插件菜单
    QMenu *Plugins_menu ;
    //插件管理器
    QAction *pluginsManagerAction;
    //插件文件夹
    QAction *pluginsFloderAction;
    //创建节点
    QAction *createNodeAction;
    //工具菜单
    QMenu *Tool_menu;
    //工具1
    QAction *tool1Action ;
    //工具2
    QAction *tool2Action ;
    //artnetrecoder工具
    QAction *ArtnetRecoderToolAction ;
    //关于菜单
    QMenu *About_menu;
    //关于
    QAction *aboutAction ;
    //关于Qt
    QAction *aboutQtAction ;
    //帮助
    QAction *helpAction ;
private slots:
    void openOSCInterface(const QString& exePath, const QStringList& args);
    void openToolWithArgs(const QString& exePath, const QStringList& args);
    //软件关于
    void showAboutDialog();
    //显示帮助
    void showHelp();
};

#endif //NODEEDITORCPP_MENUBARWIDGET_H
