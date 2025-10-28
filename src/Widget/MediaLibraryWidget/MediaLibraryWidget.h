// class MediaLibraryWidget
#pragma once

#include <QWidget>
#include <QTreeView>
#include <QMenu>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include "MediaLibrary/MediaLibrary.h"

/**
 * @class MediaLibraryWidget
 * @brief 媒体库控件（Qt Widgets版本）。使用 QTreeView 与 MediaLibraryModel 展示媒体分类与文件，
 *        提供右键菜单操作（导入、删除、清空、打开、定位等）。
 */
class MediaLibraryWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数：搭建界面、连接信号、初始化树视图
     */
    explicit MediaLibraryWidget(QWidget* parent = nullptr);


signals:
    /**
     * @brief 文件被激活（双击或菜单“打开”）
     */
    void fileActivated(const QString& path);

private slots:
    /**
     * @brief 导入多个文件（打开文件对话框）
     */
    void importFiles();

    /**
     * @brief 导入文件夹（仅添加该文件夹下的一层文件）
     */
    void importFolder();

    /**
     * @brief 删除复选框选中的条目（多选）
     */
    void deleteSelectedItems();

    /**
     * @brief 清空媒体库（保留分组）
     */
    void clearLibrary();

    /**
     * @brief 处理双击：子项打开文件；组节点切换展开
     */
    void onDoubleClicked(const QModelIndex& index);

    /**
     * @brief 右键菜单请求：在光标处弹出菜单（空白、组、子项均支持）
     */
    void onContextMenuRequested(const QPoint& pos);

    /**
     * @brief 行插入后：为新子项设置复选框
     */
    void onRowsInserted(const QModelIndex& parent, int first, int last);

    /**
     * @brief 监听媒体库数据变更并刷新所有显示
     * 函数级注释：当 MediaLibrary 发出 libraryChanged 时，强制重绑模型以刷新树视图，
     *            重新设置组图标与展开状态，并为子项补充类型图标，确保 UI 同步。
     */
    void onLibraryChanged();

private:
    /**
     * @brief 初始化树视图外观与行为（单选、隐藏表头、图标等）
     */
    void initializeTreeView();

    /**
     * @brief 为所有顶层组与其子项设置复选框：组不勾选，子项可勾选
     */
    void initAllFlag();

    /**
     * @brief 为指定组的所有子项设置复选框
     */
    void setChildrenFlag(QStandardItem* groupItem);

    /**
     * @brief 根据类别值返回组图标（存在则加载）
     */
    QIcon categoryIcon(int cat) const;

private:
    MediaLibrary* m_model = nullptr;
    QTreeView* m_tree = nullptr;
    QAction* m_actImportFiles = nullptr;
    QAction* m_actImportFolder = nullptr;
    QAction* m_actDeleteChecked = nullptr;
    QAction* m_actClear = nullptr;
    QAction* m_actRefresh = nullptr;
    QMenu* m_ctxMenu = nullptr;

protected:
    /**
     * @brief 拖拽进入事件：当从资源管理器拖入 URLs 时接受拖拽（Copy）
     */
    void dragEnterEvent(QDragEnterEvent* event) override;

    /**
     * @brief 拖拽移动事件：当拖拽在控件内移动且包含 URLs 时持续接受动作
     */
    void dragMoveEvent(QDragMoveEvent* event) override;

    /**
     * @brief 放置事件：解析 URLs，文件调用 addFiles，文件夹调用 addFolder
     */
    void dropEvent(QDropEvent* event) override;
};

