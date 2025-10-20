#include "MediaLibrary.h"
#include <QJsonDocument>
#include <QFile>
// 新增：定位 Windows 文档库路径
#include <QStandardPaths>
#include <QtWidgets/QMessageBox>

/**
 * @brief 构造函数：初始化五个分类的组节点
 */
MediaLibrary::MediaLibrary(QObject* parent)
    : QStandardItemModel(parent)
{
    setColumnCount(1);
    // 初始化五个类别为顶层组节点
    for (Category cat : { Category::Video, Category::Audio, Category::DMX, Category::Image, Category::Model ,Category::Document,Category::Unknown }) {
        auto* group = new QStandardItem(categoryName(cat));
        group->setEditable(false);
        group->setData(static_cast<int>(cat), CategoryRole);
        group->setData(-1, ParentRowRole); // 顶层组的父行为 -1
        invisibleRootItem()->appendRow(group);
        m_categories.insert(cat, group);
    }
    // 新增：初始化存储目录（Windows 文档库下），并扫描已有文件入库
    initializeStorageDir();
}

/**
 * @brief 确保类别根项存在并返回
 */
QStandardItem* MediaLibrary::ensureCategory(Category cat)
{
    if (m_categories.contains(cat)) return m_categories.value(cat);
    auto* group = new QStandardItem(categoryName(cat));
    group->setEditable(false);
    group->setData(static_cast<int>(cat), CategoryRole);
    group->setData(-1, ParentRowRole); // 顶层组的父行为 -1
    invisibleRootItem()->appendRow(group);
    m_categories.insert(cat, group);
    return group;
}

/**
 * @brief 根据文件路径添加单个文件，自动分类、去重并维护序号
 * @return 新增的索引；若重复则返回无效索引
 */
QModelIndex MediaLibrary::addFile(const QString& absPath)
{
    const QFileInfo fi(absPath);
    if (!fi.exists() || !fi.isFile()) return QModelIndex();

    const Category cat = detectCategory(absPath);
    QStandardItem* group = ensureCategory(cat);
    if (!group) return QModelIndex();

    if (existsInCategory(group, absPath)) {
        return QModelIndex(); // 已存在，跳过
    }

    // 创建叶子项
    auto* item = new QStandardItem(fi.fileName());
    item->setEditable(false);
    item->setData(absPath, PathRole);
    item->setData(static_cast<int>(cat), CategoryRole);
    item->setData(group->row(), ParentRowRole); // 叶子项的父组行号
    // 暂不设置 OrdinalRole，统一在重排时赋值
    group->appendRow(item);

    // 重排序号
    renumberCategory(group);

    emit libraryChanged();
    return item->index();
}

/**
 * @brief 批量添加文件
 */
// 属于类 MediaLibrary
/**
 * @brief 批量添加文件（复制到存储目录，若同名则强制覆盖）
 */
void MediaLibrary::addFiles(const QStringList& absPaths)
{
    QDir storage(QDir(MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
    for (const QString& p : absPaths) {
        const QFileInfo fi(p);
        if (!fi.exists() || !fi.isFile()) continue;

        // 若源文件已在存储目录中，直接入库
        if (isInStorageDir(fi.absoluteFilePath())) {
            addFile(fi.absoluteFilePath());
            continue;
        }

        // 目标路径为存储目录下同名文件，若存在则先删除以实现强制覆盖
        const QString targetPath = storage.absoluteFilePath(fi.fileName());
        if (QFile::exists(targetPath)) {
            // 尝试删除已有目标文件
            if (!QFile::remove(targetPath)) {
                // 删除失败：提示并跳过该文件复制，避免不一致
                QMessageBox::warning(nullptr,
                                     QStringLiteral("删除失败"),
                                     QStringLiteral("无法覆盖目标文件（删除失败）：\n%1").arg(targetPath));
                continue;
            }
        }

        // 复制到存储目录
        if (QFile::copy(fi.absoluteFilePath(), targetPath)) {
            addFile(targetPath);
        } else {
            // 复制失败：如果你希望弹窗提示，请告知；当前策略为跳过，不入库
            // 可改为 QMessageBox::warning(...) 提示复制失败
        }
    }
}


/**
 * @brief 添加文件夹（仅添加该文件夹下的一层文件）
 */
void MediaLibrary::addFolder(const QString& folderPath)
{
    QDir dir(folderPath);
    if (!dir.exists()) return;

    const QFileInfoList infos = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    QStringList paths;
    for (const QFileInfo& fi : infos) {
        paths << fi.absoluteFilePath();
    }
    addFiles(paths);
}

/**
 * @brief 删除选中索引（支持多选），自动重排序号
 */
// 属于类 MediaLibrary
/**
 * @brief 删除选中索引（支持多选）。在移除模型项之前，若文件位于存储目录，则尝试删除磁盘文件。
 *        删除失败则弹窗提示并跳过该项的删除，保证模型与磁盘状态一致。
 */
void MediaLibrary::removeIndexes(const QModelIndexList& indexes)
{
    // 按行去重，避免重复删除
    QSet<QModelIndex> uniqueRows;
    for (const QModelIndex& idx : indexes) {
        if (!idx.isValid()) continue;
        uniqueRows.insert(idx);
    }
    // 收集每个类别组需要重排的集合
    QSet<QStandardItem*> needRenumber;

    for (const QModelIndex& idx : uniqueRows) {
        QStandardItem* item = this->itemFromIndex(idx);
        if (!item) continue;
        QStandardItem* parentGroup = item->parent();
        if (!parentGroup) {
            // 顶层组不允许删除（如需支持，可自行扩展）
            continue;
        }

        // 同步删除磁盘文件（仅当该文件位于存储目录）
        const QString path = item->data(PathRole).toString();
        if (!path.isEmpty() && isInStorageDir(path)) {
            if (!QFile::remove(path)) {
                // 删除失败：提示并跳过该项删除
                QMessageBox::warning(nullptr,
                                     QStringLiteral("删除失败"),
                                     QStringLiteral("无法删除文件：\n%1").arg(path));
                continue;
            }
        }

        // 从模型移除
        parentGroup->removeRow(item->row());
        needRenumber.insert(parentGroup);
    }

    for (QStandardItem* group : needRenumber) {
        renumberCategory(group);
    }
    emit libraryChanged();
}

/**
 * @brief 清空媒体库（保留分类组节点）。同时尝试删除位于存储目录的文件；
 *        对删除失败的文件逐一弹窗提示，并保留其项，避免库与磁盘不一致。
 */
void MediaLibrary::clearLibrary()
{
    for (auto it = m_categories.begin(); it != m_categories.end(); ++it) {
        QStandardItem* group = it.value();
        if (!group) continue;

        // 逐项处理以便同步删除磁盘文件
        for (int r = group->rowCount() - 1; r >= 0; --r) {
            QStandardItem* item = group->child(r);
            if (!item) continue;
            const QString path = item->data(PathRole).toString();
            if (!path.isEmpty() && isInStorageDir(path)) {
                if (!QFile::remove(path)) {
                    QMessageBox::warning(nullptr,
                                         QStringLiteral("删除失败"),
                                         QStringLiteral("无法删除文件：\n%1").arg(path));
                    // 删除失败则保留该项
                    continue;
                }
            }
            // 删除成功或非存储目录文件，移除项
            group->removeRow(r);
        }
    }
    emit libraryChanged();
}

/**
 * @brief 返回某索引对应的文件路径（若为组节点则返回空）
 */
QString MediaLibrary::filePathForIndex(const QModelIndex& index) const
{
    if (!index.isValid()) return QString();
    const QStandardItem* item = this->itemFromIndex(index);
    if (!item) return QString();
    const QStandardItem* parentGroup = item->parent();
    if (!parentGroup) return QString(); // 组节点
    return item->data(PathRole).toString();
}

/**
 * @brief 返回全部文件路径
 */
QStringList MediaLibrary::allFiles() const
{
    QStringList out;
    for (Category cat : m_categories.keys()) {
        const QStandardItem* group = m_categories.value(cat);
        if (!group) continue;
        for (int r = 0; r < group->rowCount(); ++r) {
            const QStandardItem* item = group->child(r);
            if (!item) continue;
            const QString path = item->data(PathRole).toString();
            if (!path.isEmpty()) out << path;
        }
    }
    return out;
}

/**
 * @brief 返回指定类别文件路径列表
 */
QStringList MediaLibrary::getFileList(Category cat) const
{
    QStringList out;
    const QStandardItem* group = m_categories.value(cat);
    if (!group) return out;
    for (int r = 0; r < group->rowCount(); ++r) {
        const QStandardItem* item = group->child(r);
        if (!item) continue;
        const QString path = item->data(PathRole).toString();
        if (!path.isEmpty()) out << QFileInfo(path).fileName();
    }
    return out;
}

/**
 * @brief 导出为 JSON 对象
 */
QJsonObject MediaLibrary::toJson() const
{
    QJsonObject root;
    auto collect = [&](Category cat, const char* key) {
        QJsonArray arr;
        const QStandardItem* group = m_categories.value(cat);
        if (group) {
            for (int r = 0; r < group->rowCount(); ++r) {
                const QStandardItem* item = group->child(r);
                arr.append(item->data(PathRole).toString());
            }
        }
        root.insert(QString::fromUtf8(key), arr);
    };

    collect(Category::Video,   "Video");
    collect(Category::Audio,   "Audio");
    collect(Category::DMX,     "DMX");
    collect(Category::Image, "Image");
    collect(Category::Model,   "Model");
    collect(Category::Document,    "Document");
    collect(Category::Unknown, "Unknown");
    return root;
}

/**
 * @brief 从 JSON 对象加载
 */
bool MediaLibrary::fromJson(const QJsonObject& obj)
{
    clearLibrary();
    auto load = [&](Category cat, const char* key) {
        const QJsonArray arr = obj.value(QString::fromUtf8(key)).toArray();
        for (const QJsonValue& v : arr) {
            addFile(v.toString());
        }
    };
    load(Category::Video,   "Video");
    load(Category::Audio,   "Audio");
    load(Category::DMX,     "DMX");
    load(Category::Image, "Picture");
    load(Category::Model,   "Model");
    load(Category::Document,    "Document");
    load(Category::Unknown, "Unknown");
    emit libraryChanged();
    return true;
}

/**
 * @brief 保存到文件（UTF-8 JSON）
 */
bool MediaLibrary::saveToFile(const QString& jsonPath) const
{
    QFile f(jsonPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const QJsonObject obj = toJson();
    const QJsonDocument doc(obj);
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

/**
 * @brief 从文件加载（UTF-8 JSON）
 */
bool MediaLibrary::loadFromFile(const QString& jsonPath)
{
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray data = f.readAll();
    f.close();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return false;
    return fromJson(doc.object());
}

/**
 * @brief 根据扩展名判定类别
 */
MediaLibrary::Category MediaLibrary::detectCategory(const QString& absPath) const
{
    const QString ext = QFileInfo(absPath).suffix().toLower();
    // 视频
    static const QSet<QString> video = { "mp4", "mov", "mkv", "avi", "wmv", "flv", "webm" };
    // 音频
    static const QSet<QString> sound = { "wav", "mp3", "flac", "aac", "ogg", "m4a" };
    // MIDI
    static const QSet<QString> dmx  = { "dmx" };
    // 图片
    static const QSet<QString> pic   = { "jpg", "jpeg", "png", "bmp", "gif", "webp", "tiff" };
    // 3D 模型（简单示例）
    static const QSet<QString> model = { "obj", "fbx", "stl", "gltf", "glb" };
    // other
    static const QSet<QString> other = { "txt", "json", "xml", "cfg", "log" , "md" , "csv" };
    if (video.contains(ext)) return Category::Video;
    if (sound.contains(ext)) return Category::Audio;
    if (dmx.contains(ext))  return Category::DMX;
    if (pic.contains(ext))   return Category::Image;
    if (model.contains(ext)) return Category::Model;
    // 其他文件（如脚本、配置等）
    if (other.contains(ext)) return Category::Document;
    return Category::Unknown;
}

/**
 * @brief 类别中文名
 */
QString MediaLibrary::categoryName(Category cat) const
{
    switch (cat) {
    case Category::Video:   return QStringLiteral("Video Files");
    case Category::Audio:   return QStringLiteral("Audio Files");
    case Category::DMX:     return QStringLiteral("DMX Files");
    case Category::Image: return QStringLiteral("Image Files");
    case Category::Model:   return QStringLiteral("3D Models");
    case Category::Document:   return QStringLiteral("Documents");
    case Category::Unknown: return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

/**
 * @brief 为指定类别组重排序号
 */
void MediaLibrary::renumberCategory(QStandardItem* catItem)
{
    if (!catItem) return;
    for (int r = 0; r < catItem->rowCount(); ++r) {
        QStandardItem* item = catItem->child(r);
        if (!item) continue;
        const int ordinal = r + 1;
        item->setData(ordinal, OrdinalRole);
        // 在显示名左侧添加序号（示例：1  filename.ext）
        const QString baseName = QFileInfo(item->data(PathRole).toString()).fileName();
        item->setText(QString::number(ordinal) + QStringLiteral("  ") + baseName);
    }
}

/**
 * @brief 判断该类别组内是否存在指定路径的文件（去重）
 */
bool MediaLibrary::existsInCategory(QStandardItem* catItem, const QString& absPath) const
{
    if (!catItem) return false;
    for (int r = 0; r < catItem->rowCount(); ++r) {
        const QStandardItem* item = catItem->child(r);
        if (!item) continue;
        if (item->data(PathRole).toString().compare(absPath, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 角色名映射，供 QML 使用（如 ListView/TreeView 的委托访问）
 * - path       文件路径
 * - category   类别枚举值（int）
 * - ordinal    在类别中的序号（从 1 开始）
 * - parentRow  父组在根节点下的行号（顶层组为 -1）
 */
QHash<int, QByteArray> MediaLibrary::roleNames() const {
    QHash<int, QByteArray> names = QStandardItemModel::roleNames();
    names.insert(PathRole,      "path");
    names.insert(CategoryRole,  "category");
    names.insert(OrdinalRole,   "ordinal");
    names.insert(ParentRowRole, "parentRow");
    return names;
}

/**
 * @brief 初始化存储目录（使用头文件常量 MEDIA_LIBRARY_STORAGE_DIR），并扫描已有文件入库
 *        - 路径来源：MediaLibrary.h#L16 的 MEDIA_LIBRARY_STORAGE_DIR
 *        - 若目录不存在则创建
 *        - 扫描该目录的一层文件并加入模型
 */
void MediaLibrary::initializeStorageDir()
{
    // 如果存储目录不存在则创建
    if (!QDir(MEDIA_LIBRARY_STORAGE_DIR).exists()) {
        QDir().mkpath(MEDIA_LIBRARY_STORAGE_DIR);
    }


    // 扫描存储目录中的所有文件（仅一层）
    QDir storage(QDir(MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
    const QFileInfoList infos = storage.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& fi : infos) {
        addFile(fi.absoluteFilePath());
    }
}

// 新增：在存储目录内生成不重名的文件名
QString MediaLibrary::uniqueFileNameInStorage(const QString& fileName) const
{
    QDir storage(QDir(MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
    const QFileInfo info(fileName);
    const QString base = info.completeBaseName();
    const QString ext = info.suffix();

    QString candidate = fileName;
    int counter = 1;
    while (storage.exists(candidate)) {
        candidate = QStringLiteral("%1 (%2)%3")
                        .arg(base)
                        .arg(counter)
                        .arg(ext.isEmpty() ? QString() : QStringLiteral(".") + ext);
        ++counter;
    }
    return candidate;
}

// 属于类 MediaLibrary
/**
 * @brief 判断给定绝对路径是否位于存储目录中（用于决定是否需要同步删除/覆盖）
 */
bool MediaLibrary::isInStorageDir(const QString& absPath) const
{
    QDir storage(QDir(MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
    const QString rel = storage.relativeFilePath(absPath);
    return !rel.startsWith("..");
}