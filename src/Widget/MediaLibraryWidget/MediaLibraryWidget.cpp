// class MediaLibraryWidget
#include "MediaLibraryWidget.h"
#include "Common/ProcessGuard/SpawnNoInherit.hpp"  // bInheritHandles=FALSE 派生（幽灵 socket 修复）
#include <QStandardItem>
#include <QApplication>
#include <QMessageBox>
#include <qevent.h>
#include <QHeaderView>
#include <QFileInfo>
#include <QMimeData>

/**
 * @brief 构造函数：搭建界面、连接信号、初始化树视图
 */
MediaLibraryWidget::MediaLibraryWidget(QWidget* parent)
    : QWidget(parent) {
    // 模型
    m_model =MediaLibrary::instance();

    // 树视图
    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    initializeTreeView();

    // [新增] 监听媒体库的整体变更信号，统一刷新视图
    connect(m_model, &MediaLibrary::libraryChanged,
            this,    &MediaLibraryWidget::onLibraryChanged);

    // ... existing code ...
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
    // 通用操作（空白、组、子项都显示）
    // QMenu ctx(m_tree);
    // 初始化右键菜单操作
    initializeContextMenu();
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
        // 分组默认折叠，文件多时避免一次性展开过长列表
        m_tree->setExpanded(group->index(), false);
    }
    // 视图侧：既支持拖也支持放置（内部拖拽）
    m_tree->setDragEnabled(true);
    // m_tree->setAcceptDrops(true);
    // m_tree->viewport()->setAcceptDrops(true);
    // m_tree->setDropIndicatorShown(true);
    // m_tree->setDragDropMode(QAbstractItemView::DragDrop);
    // m_tree->setDefaultDropAction(Qt::CopyAction);
}

void MediaLibraryWidget::initializeContextMenu()
{
    // 初始化右键菜单操作（导入、删除、清空、刷新等）
    m_ctxMenu = new QMenu(m_tree);
    // 通用操作（空白、组、子项都显示）
    m_actImportFiles   = m_ctxMenu->addAction(QIcon(":/icons/icons/import.png"),tr("导入文件"));
    m_actImportFolder  = m_ctxMenu->addAction(QIcon(":/icons/icons/folder.png"),tr("导入文件夹"));
    m_actDeleteChecked = m_ctxMenu->addAction(QIcon(":/icons/icons/remove.png"),tr("删除选中"));
    m_actClear         = m_ctxMenu->addAction(QIcon(":/icons/icons/clear.png"),tr("清空"));
    m_actRefresh       = m_ctxMenu->addAction(QIcon(":/icons/icons/reload.png"),tr("刷新"));
    m_ctxMenu->addSeparator();
    // 初始与"无右键上下文"状态一致：导入/删除仅在右键对应条目时显示
    // （本组动作同时被 MainWindow 挂到停靠面板标题栏菜单，需同步控制可见性）
    m_actDeleteChecked->setVisible(false);
    connect(m_actImportFiles,  &QAction::triggered, this, &MediaLibraryWidget::importFiles);
    connect(m_actImportFolder, &QAction::triggered, this, &MediaLibraryWidget::importFolder);
    // 连接“删除选中”到当前选择删除
    connect(m_actDeleteChecked, &QAction::triggered, this, [this] {
        deleteSelectedItems();
    });
    connect(m_actClear,        &QAction::triggered, this, &MediaLibraryWidget::clearLibrary);
    connect(m_actRefresh,      &QAction::triggered, this, [this] {
        MediaLibrary::instance()->refresh();
    });

    // 动态操作（展开/折叠、在资源管理器中显示）
    m_actToggle = m_ctxMenu->addAction(QIcon(":/icons/icons/expand.png"), tr("展开/折叠"));
    m_actToggle->setVisible(false);
    connect(m_actToggle, &QAction::triggered, this, [this] {
        if (m_contextIndex.isValid()) {
            m_tree->setExpanded(m_contextIndex, !m_tree->isExpanded(m_contextIndex));
        }
    });

    m_actReveal = m_ctxMenu->addAction(QIcon(":/icons/icons/explore.png"), tr("在资源管理器中显示"));
    m_actReveal->setVisible(false);
    connect(m_actReveal, &QAction::triggered, this, [this] {
        if (m_contextIndex.isValid()) {
            QString path = m_model->filePathForIndex(m_contextIndex);
            if (!path.isEmpty()) {
                // 【幽灵 socket 修复 2026-11-13】explorer 常驻桌面进程——
                // startDetached 句柄继承会把本进程 socket 句柄复制给
                // explorer 并长期存活（Flow 退出后端口仍被幽灵句柄占据）。
                // 统一走 bInheritHandles=FALSE 派生。
                const QString rawArgs = QStringLiteral("/select,\"%1\"")
                                            .arg(QDir::toNativeSeparators(path));
                ms::spawnNoInherit(QStringLiteral("explorer.exe"), rawArgs);
            }
        }
    });
}

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
 * @brief 类别的中文显示名（仅用于导入对话框过滤器，与树分组名解耦）
 */
namespace {
QString categoryDisplayLabel(MediaLibrary::Category cat)
{
    switch (cat) {
    case MediaLibrary::Category::Video:     return QObject::tr("视频");
    case MediaLibrary::Category::Audio:     return QObject::tr("音频");
    case MediaLibrary::Category::DMX:       return QObject::tr("DMX");
    case MediaLibrary::Category::Image:     return QObject::tr("图片");
    case MediaLibrary::Category::Model:     return QObject::tr("3D 模型");
    case MediaLibrary::Category::Document:  return QObject::tr("文档");
    case MediaLibrary::Category::Vioso:     return QStringLiteral("Vioso");
    case MediaLibrary::Category::ISF:       return QStringLiteral("ISF");
    case MediaLibrary::Category::ChildFlow: return QObject::tr("Child Flow");
    case MediaLibrary::Category::Mpcdi:     return QStringLiteral("MPCDI");
    case MediaLibrary::Category::Fixture:   return QStringLiteral("Fixture/GDTF");
    case MediaLibrary::Category::Game:      return QObject::tr("游戏 ROM");
    case MediaLibrary::Category::Gesture:   return QObject::tr("手势库");
    case MediaLibrary::Category::Unknown:   break;
    }
    return {};
}
} // namespace

/**
 * @brief 生成指定类别的文件对话框过滤器串；无扩展名的类别（Unknown）返回空串
 */
QString MediaLibraryWidget::categoryDialogFilter(MediaLibrary::Category cat) const
{
    const QStringList exts = MediaLibrary::categoryExtensions(cat);
    if (exts.isEmpty()) return {};
    QStringList patterns;
    for (const QString& e : exts) {
        patterns << QStringLiteral("*.") + e;
    }
    return QStringLiteral("%1 (%2)").arg(categoryDisplayLabel(cat),
                                         patterns.join(QLatin1Char(' ')));
}

/**
 * @brief 导入多个文件（打开文件对话框）。
 *        若右键选中了某分组/子项，则把该分组的过滤器置顶作为默认选中项（分组指定导入）；
 *        选中文件批量入库，仍由 MediaLibrary::detectCategory 按扩展名自动归类
 */
void MediaLibraryWidget::importFiles()
{
    // 全部支持格式与各分类过滤器；扩展名与 MediaLibrary::categoryExtensions 同源
    static const QString allFilter = tr(
        "全部支持格式 ("
        "*.mp4 *.mov *.mkv *.avi *.wmv *.flv *.webm "
        "*.wav *.mp3 *.flac *.aac *.ogg *.m4a "
        "*.jpg *.jpeg *.png *.bmp *.gif *.webp *.tiff "
        "*.dmx "
        "*.obj *.fbx *.stl *.gltf *.glb "
        "*.txt *.json *.xml *.cfg *.log *.md *.csv *.ini "
        "*.vwf *.fs *.childflow *.mpcdi *.gdtf "
        "*.gba *.gb *.gbc"
        ");;"
        "视频 (*.mp4 *.mov *.mkv *.avi *.wmv *.flv *.webm);;"
        "音频 (*.wav *.mp3 *.flac *.aac *.ogg *.m4a);;"
        "图片 (*.jpg *.jpeg *.png *.bmp *.gif *.webp *.tiff);;"
        "DMX (*.dmx);;"
        "3D 模型 (*.obj *.fbx *.stl *.gltf *.glb);;"
        "文档 (*.txt *.json *.xml *.cfg *.log *.md *.csv *.ini);;"
        "Vioso (*.vwf);;"
        "MPCDI (*.mpcdi);;"
        "ISF (*.fs);;"
        "Child Flow (*.childflow);;"
        "Fixture/GDTF (*.gdtf);;"
        "游戏 ROM (*.gba *.gb *.gbc);;"
        "所有文件 (*.*)");

    // 分组指定导入：该分组过滤器置顶，对话框默认选中它
    QString filter = allFilter;
    const QString catFilter = categoryDialogFilter(m_contextCategory);
    if (!catFilter.isEmpty()) {
        filter = catFilter + QStringLiteral(";;") + allFilter;
    }

    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("导入媒体文件"), QString(), filter);
    if (files.isEmpty()) return;
    m_model->addFiles(files);
}

/**
 * @brief 导入文件夹（仅添加该文件夹下的一层文件）。
 *        若右键选中了某分组/子项，则仅导入该分组扩展名匹配的文件（分组指定导入）
 */
void MediaLibraryWidget::importFolder()
{
    // 函数级注释：选择文件夹并将该目录下一层文件加入模型；分组上下文下按分组扩展名过滤
    const QString dir = QFileDialog::getExistingDirectory(this, tr("选择文件夹"));
    if (dir.isEmpty()) return;

    const QStringList exts = MediaLibrary::categoryExtensions(m_contextCategory);
    if (exts.isEmpty()) {
        // 无分组上下文或 Unknown：保持旧行为，导入全部文件
        m_model->addFolder(dir);
        return;
    }

    const QDir d(dir);
    const QFileInfoList infos = d.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    QStringList matched;
    matched.reserve(infos.size());
    for (const QFileInfo& fi : infos) {
        if (exts.contains(fi.suffix().toLower())) {
            matched << fi.absoluteFilePath();
        }
    }
    if (!matched.isEmpty()) {
        m_model->addFiles(matched);
    }
}

/**
 * @brief 删除当前条目；删除完成后清空选择高亮。
 *        优先使用右键所在条目（删除菜单仅对文件子项显示），无右键上下文时
 *        回退到树视图当前选中项，兼容外部动作触发。
 */
void MediaLibraryWidget::deleteSelectedItems()
{
    // 函数级注释：优先取右键上下文条目，否则取树视图当前选择；仅对子项执行删除
    QModelIndex idx = m_contextIndex;
    if (!idx.isValid()) {
        QItemSelectionModel* sel = m_tree->selectionModel();
        if (!sel) return;
        idx = sel->currentIndex();
    }
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
    // 函数级注释：清空媒体库会同步删除存储目录中的文件，必须先弹窗确认
    const QMessageBox::StandardButton ret = QMessageBox::question(
        this,
        tr("清空媒体库"),
        tr("确定要清空媒体库吗？\n\n存储目录中的媒体文件将被一并删除，此操作不可撤销。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    // 调用模型清空，同时重置所有复选框
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
    m_contextIndex = m_tree->indexAt(pos);
    const QString path = m_contextIndex.isValid() ? m_model->filePathForIndex(m_contextIndex) : QString();

    // Reset visibility
    m_actToggle->setVisible(false);
    m_actReveal->setVisible(false);
    m_actDeleteChecked->setVisible(false);

    // 分组指定导入：导入动作仅对有效条目（分组或其子项）显示，空白处隐藏；
    // 并记录该条目所属类别，供导入对话框过滤器使用
    const bool hasContextItem = m_contextIndex.isValid();
    m_actImportFiles->setVisible(hasContextItem);
    m_actImportFolder->setVisible(hasContextItem);
    m_contextCategory = hasContextItem
        ? static_cast<MediaLibrary::Category>(m_contextIndex.data(MediaLibrary::CategoryRole).toInt())
        : MediaLibrary::Category::Unknown;

    if (m_contextIndex.isValid()) {
        if (path.isEmpty()) {
            // Group node: show toggle
            m_actToggle->setVisible(true);
        } else {
            // Item node: show reveal + delete（删除仅作用于文件条目）
            m_actReveal->setVisible(true);
            m_actDeleteChecked->setVisible(true);
        }
    }

    m_ctxMenu->exec(m_tree->viewport()->mapToGlobal(pos));
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
    case Cat::Vioso:   return QIcon(":/icons/icons/vioso.png");
    case Cat::ISF:     return QIcon(":/icons/icons/isf.png");
    case Cat::ChildFlow: return QIcon(":/icons/icons/Flow.png");
    case Cat::Mpcdi:   return QIcon(":/icons/icons/image.png");
    case Cat::Fixture: return QIcon(":/icons/icons/gdtf.png");
    case Cat::Game: return QIcon(":/icons/icons/games.png");
    case Cat::Gesture: return QIcon(":/icons/icons/finger.png");
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

void MediaLibraryWidget::onLibraryChanged()
{
    // 函数级注释：
    // 1) refresh() 过程中屏蔽了模型信号，视图未收到 rowsInserted/rowsRemoved，
    //    因此此处强制重绑模型以刷新显示；
    // 2) 重新初始化组图标与默认折叠状态；
    // 3) 为所有子项补充类型图标，避免在刷新过程中遗漏。

    // 记录当前选中项（可选）
    const QModelIndex current = m_tree->currentIndex();

    // 强制刷新绑定，确保视图重新构建
    m_tree->setModel(nullptr);
    m_tree->setModel(m_model);

    // 重新初始化视图外观（组图标、默认折叠等）
    initializeTreeView();

    // 为所有组刷新子项图标
    const int topCount = m_model->rowCount();
    for (int r = 0; r < topCount; ++r) {
        QStandardItem* group = m_model->item(r);
        if (!group) continue;
        setChildrenFlag(group);
    }

    // 恢复当前选择（若仍有效）
    if (current.isValid()) {
        m_tree->setCurrentIndex(current);
    }

    // 刷新视口以更新绘制
    m_tree->viewport()->update();
}