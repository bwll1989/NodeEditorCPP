// class MediaLibraryWidget
#include "MediaLibraryWidget.h"
#include <QStandardItem>
#include <QApplication>
#include <qevent.h>
#include <QHeaderView>
#include <QFileInfo>
#include <QMimeData>

/**
 * @brief 构造函数：搭建界面、连接信号、初始化树视图
 */
MediaLibraryWidget::MediaLibraryWidget(QWidget* parent)
    : QWidget(parent)
{
    // 模型
    m_model =MediaLibrary::instance();

    // 树视图
    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    initializeTreeView();

    // [更新] 为宿主控件开启接收外部拖拽（资源管理器）
    setAcceptDrops(true);

    // 右键菜单
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeView::customContextMenuRequested,
            this,  &MediaLibraryWidget::onContextMenuRequested);

    // 双击
    connect(m_tree, &QTreeView::doubleClicked,
            this,  &MediaLibraryWidget::onDoubleClicked);

    // 监听模型变化：新增行与整体变更
    connect(m_model, &QStandardItemModel::rowsInserted,
            this,    &MediaLibraryWidget::onRowsInserted);

    // 初始化所有标志位
    initAllFlag();

    // 总体布局（去除工具栏，仅放置树视图）
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(m_tree);
    setLayout(lay);
}

/**
 * @brief 初始化树视图外观与行为（单选、隐藏表头、图标等）
 */
void MediaLibraryWidget::initializeTreeView()
{
    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection); // 默认单选
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setExpandsOnDoubleClick(false); //  ourselves handle double click
    m_tree->setAllColumnsShowFocus(true);
    m_tree->setIconSize(QSize(18, 18));

    // 顶层组的样式（图标、加粗）
    const int topCount = m_model->rowCount();
    for (int r = 0; r < topCount; ++r) {
        QStandardItem* group = m_model->item(r);
        if (!group) continue;
        // 加粗
        QFont f = group->font();
        f.setBold(true);
        group->setFont(f);
        // 图标（存在则设置）
        const int cat = group->data(MediaLibrary::CategoryRole).toInt();
        QIcon ic = categoryIcon(cat);
        if (!ic.isNull()) {
            group->setIcon(ic);
        }
        // 展开
        m_tree->setExpanded(group->index(), true);
    }
    // 视图侧：既支持拖也支持放置（内部拖拽）
    m_tree->setDragEnabled(true);
    // m_tree->setAcceptDrops(true);
    // m_tree->viewport()->setAcceptDrops(true);
    // m_tree->setDropIndicatorShown(true);
    // m_tree->setDragDropMode(QAbstractItemView::DragDrop);
    // m_tree->setDefaultDropAction(Qt::CopyAction);
}

/**
 * @brief 为所有顶层组与其子项设置复选框：组不勾选，子项可勾选
 */

/**
 * @brief 初始化所有项的“勾选”状态（已改为无操作：不再使用复选框）
 */
void MediaLibraryWidget::initAllFlag()
{
    // 现在不再使用复选框，保留空实现以兼容既有调用
    const int topCount = m_model->rowCount();
    for (int r = 0; r < topCount; ++r) {
        QStandardItem* group = m_model->item(r);
        if (!group) continue;
        // 子项允许勾选
        setChildrenFlag(group);
    }
}

/**
 * @brief 为指定组的子项设置标志位
 */
void MediaLibraryWidget::setChildrenFlag(QStandardItem* groupItem)
{
    Q_UNUSED(groupItem);
    // 现在不再使用复选框，保留空实现以兼容既有调用
    if (!groupItem) return;
    for (int r = 0; r < groupItem->rowCount(); ++r) {
        QStandardItem* child = groupItem->child(r);
        if (!child) continue;
        Qt::ItemFlags flags = child->flags();
        flags |= Qt::ItemIsUserCheckable;
        child->setFlags(flags);
        child->setIcon(categoryIcon(child->data(MediaLibrary::CategoryRole).toInt()));

    }
}

/**
 * @brief 导入多个文件（打开文件对话框）
 */
void MediaLibraryWidget::importFiles()
{
    // 函数级注释：打开多文件选择对话框，把选中的绝对路径批量加入模型
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("选择媒体文件"), QString(),
        tr("所有文件 (*.*)"));
    if (files.isEmpty()) return;
    m_model->addFiles(files);
}

/**
 * @brief 导入文件夹（仅添加该文件夹下的一层文件）
 */
void MediaLibraryWidget::importFolder()
{
    // 函数级注释：选择文件夹并将该目录下一层文件加入模型
    const QString dir = QFileDialog::getExistingDirectory(this, tr("选择文件夹"));
    if (dir.isEmpty()) return;
    m_model->addFolder(dir);
}

/**
 * @brief 删除当前鼠标选中的条目；删除完成后清空选择高亮
 */
void MediaLibraryWidget::deleteSelectedItems()
{
    // 函数级注释：从树视图当前选择中获取索引，仅对子项执行删除；成功后清空选择
    QItemSelectionModel* sel = m_tree->selectionModel();
    if (!sel) return;

    const QModelIndex idx = sel->currentIndex();
    if (!idx.isValid()) return;

    // 顶层组不删除，仅对子项（有文件路径）进行删除
    const QString path = m_model->filePathForIndex(idx);
    if (path.isEmpty()) {
        // 若选中的是组节点则不处理
        return;
    }

    // 执行删除
    m_model->removeIndexes(QList<QModelIndex>() << idx);

    // 清空当前选择项，避免删除后仍保留高亮
    m_tree->clearSelection();
}

/**
 * @brief 清空媒体库（保留分组）
 */
void MediaLibraryWidget::clearLibrary()
{
    // 函数级注释：调用模型清空，同时重置所有复选框
    m_model->clearLibrary();
    // initCheckableForAll();
}

/**
 * @brief 处理双击：子项打开文件；组节点切换展开
 */
void MediaLibraryWidget::onDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    // 组节点：切换展开
    const QString path = m_model->filePathForIndex(index);
    if (path.isEmpty()) {
        const bool isExp = m_tree->isExpanded(index);
        m_tree->setExpanded(index, !isExp);
        return;
    }
    // 子项：打开文件 + 发信号
    emit fileActivated(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

/**
 * @brief 右键菜单请求：在光标处弹出菜单（空白、组、子项均支持）
 */

void MediaLibraryWidget::onContextMenuRequested(const QPoint& pos)
{
    const QModelIndex idx = m_tree->indexAt(pos);
    const QString path = idx.isValid() ? m_model->filePathForIndex(idx) : QString();

    QMenu ctx(m_tree);

    // 通用操作（空白、组、子项都显示）
    QAction* actImportFiles   = ctx.addAction(tr("导入文件"));
    QAction* actImportFolder  = ctx.addAction(tr("导入文件夹"));
    QAction* actDeleteChecked = ctx.addAction(tr("删除选中"));
    QAction* actClear         = ctx.addAction(tr("清空"));
    ctx.addSeparator();

    connect(actImportFiles,  &QAction::triggered, this, &MediaLibraryWidget::importFiles);
    connect(actImportFolder, &QAction::triggered, this, &MediaLibraryWidget::importFolder);
    // 连接“删除选中”到当前选择删除
    connect(actDeleteChecked, &QAction::triggered, this, [this] {
        deleteSelectedItems();
    });
    connect(actClear,        &QAction::triggered, this, &MediaLibraryWidget::clearLibrary);

    if (!idx.isValid()) {
        // 空白处：只显示通用操作
        ctx.popup(m_tree->viewport()->mapToGlobal(pos));
        return;
    }

    if (path.isEmpty()) {
        // 组节点：展开/折叠
        QAction* actToggle = ctx.addAction(tr("展开/折叠"));
        connect(actToggle, &QAction::triggered, this, [this, idx] {
            m_tree->setExpanded(idx, !m_tree->isExpanded(idx));
        });
    } else {
        // 子项：打开/定位/删除
        // QAction* actOpen   = ctx.addAction(tr("打开"));
        QAction* actReveal = ctx.addAction(tr("在资源管理器中显示"));

        // connect(actOpen, &QAction::triggered, this, [this, path] {
        //     emit fileActivated(path);
        //     QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        // });
        connect(actReveal, &QAction::triggered, this, [path] {
            // Windows 资源管理器定位
            QString cmd = "explorer";
            QStringList args;
            args << "/select," << QDir::toNativeSeparators(path);
            QProcess::startDetached(cmd, args);
        });

    }

    ctx.popup(m_tree->viewport()->mapToGlobal(pos));
    ctx.exec();
}

/**
 * @brief 行插入后：为新子项设置必要的外观（不再设置复选框，补充图标）
 */
void MediaLibraryWidget::onRowsInserted(const QModelIndex& parent, int first, int last)
{
    // 函数级注释：当某个组下增加了子项，为其设置类型图标；不再设置勾选
    QStandardItem* parentItem = m_model->itemFromIndex(parent);
    if (!parentItem) return;
    for (int r = first; r <= last; ++r) {
        QStandardItem* child = parentItem->child(r);
        if (!child) continue;
        // 不再设置 Qt::ItemIsUserCheckable / checkState
        // 为子项补充类型图标
        const int cat = child->data(MediaLibrary::CategoryRole).toInt();
        child->setIcon(categoryIcon(cat));

    }
}

/**
 * @brief 根据类别值返回组图标（存在则加载）
 */
QIcon MediaLibraryWidget::categoryIcon(int cat) const
{
    using Cat = MediaLibrary::Category;
    switch (static_cast<Cat>(cat)) {
    case Cat::Video:   return QIcon(":/icons/icons/video.png");
    case Cat::Audio:   return QIcon(":/icons/icons/audio.png");
    case Cat::DMX:     return QIcon(":/icons/icons/stage.png");
    case Cat::Image: return QIcon(":/icons/icons/image.png");
    case Cat::Model:   return QIcon(":/icons/icons/model_1.png");
    case Cat::Document:   return QIcon(":/icons/icons/document.png");
    case Cat::Unknown: return QIcon(":/icons/icons/unknown.png");
    default:           return QIcon();
    }
}

/**
 * @brief 事件过滤器：接收 Windows 资源管理器拖拽到 TreeView 空白区域
 * - DragEnter / DragMove：若包含 URL 则接受提出的动作
 * - Drop：解析 URL，为文件调用 addFiles；为文件夹调用 addFolder
 */
void MediaLibraryWidget::dragEnterEvent(QDragEnterEvent* event)
{
    // 函数级注释：当拖拽进入控件区域，如果包含本地文件/文件夹 URL，则接受拖拽（复制动作）
    const QMimeData* md = event ? event->mimeData() : nullptr;
    if (md && md->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QWidget::dragEnterEvent(event);
    }
}

void MediaLibraryWidget::dragMoveEvent(QDragMoveEvent* event)
{
    // 函数级注释：拖拽在控件内移动，若为本地 URL 则保持接受状态，显示放置指示
    const QMimeData* md = event ? event->mimeData() : nullptr;
    if (md && md->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QWidget::dragMoveEvent(event);
    }
}

void MediaLibraryWidget::dropEvent(QDropEvent* event)
{
    // 函数级注释：放置时解析所有 URL；文件批量导入，文件夹逐个导入
    const QMimeData* md = event ? event->mimeData() : nullptr;
    if (!md || !md->hasUrls()) {
        QWidget::dropEvent(event);
        return;
    }

    QStringList files;
    QStringList folders;

    for (const QUrl& u : md->urls()) {
        if (!u.isLocalFile()) continue;
        const QString p = u.toLocalFile();
        if (p.isEmpty()) continue;
        QFileInfo fi(p);
        if (fi.isDir()) folders << p;
        else if (fi.isFile()) files << p;
    }

    if (!files.isEmpty()) m_model->addFiles(files);
    for (const QString& dir : folders) m_model->addFolder(dir);

    event->acceptProposedAction();
}