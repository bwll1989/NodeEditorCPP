//
// Created by bwll1 on 2024/9/25.
//

#pragma once

#include <QMenuBar>
#include <QAction>
#include "ConstantDefines.h"
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
    //最近文件菜单
    QMenu *recentFilesMenu;
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
    //切换主题
    QAction *switchTheme;
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
    //格式转化器
    QAction *FormatConverter ;
    //网页入口
    QAction *WebInterface ;
    //系统信息
    QAction *systemInfoAction;
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
    //最近文件菜单项
    QAction* recentFileActs[AppConstants::MaxRecentFiles];
    //更新最近文件菜单项
    void updateRecentFileActions(const QStringList& files);

Q_SIGNALS:
    void recentFileTriggered(const QString& path);
private slots:
    void openToolWithArgs(const QString& exePath, const QStringList& args);
    //软件关于
    void showAboutDialog();
    //显示帮助
    void showHelp();
    /**
     * 函数级注释：在默认浏览器中打开本地HTTP服务器的页面
     * - 使用 `AppConstants::HTTP_SERVER_PORT` 组合URL
     * - 调用 `QDesktopServices::openUrl` 打开浏览器
     */
    void openDashboard();
};

