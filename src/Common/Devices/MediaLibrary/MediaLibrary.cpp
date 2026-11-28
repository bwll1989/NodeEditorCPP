#include "MediaLibrary.h"
#include "VideoTranscoder.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QSet>
#include <QThreadPool>
#include <functional>
// 新增：定位 Windows 文档库路径
#include <QMimeData>
#include <QStandardPaths>
#include <QtWidgets/QMessageBox>
#include "Common/AppConfig/ConstantDefines.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QIcon>

namespace {
/**
 * @brief 导入复制的临时文件后缀：先写 xxx.ext.copying，成功后改名回 xxx.ext。
 *        任何中断（关程序/断电）都不在存储目录留下半截正式名文件——
 *        否则下次启动 initializeStorageDir 扫描会把半截文件当素材入库。
 */
constexpr const char* kCopyTempSuffix = ".copying";

/**
 * @brief 正在后台复制的目标路径集合（key=targetPath）。
 *        文件级静态而非成员变量：instance() 内联在头文件、主程序 TU 也展开
 *        new MediaLibrary()——加成员变量会改变 sizeof，主程序按旧布局分配
 *        +库内新构造写穿 = 堆损坏（2026-11-25 实锤过的同款模式）。
 *        仅主线程访问（insert 在提交时、remove 在完成回调里，回调经
 *        invokeMethod 排回主线程），无锁安全。
 */
QSet<QString> s_activeCopies;

/**
 * @brief 带进度的块拷贝：1MiB 分块读写，percent 变化才回调（≤100 次/文件）。
 *        目标先写 dstPath（调用方传 .copying 临时名），成功后由调用方改名。
 */
bool copyFileWithProgress(const QString& srcPath, const QString& dstPath,
                          const std::function<void(int)>& onPercent, QString* err)
{
    QFile in(srcPath);
    QFile out(dstPath);
    if (!in.open(QIODevice::ReadOnly)) {
        *err = QCoreApplication::translate("MediaLibrary", "无法打开源文件：%1").arg(in.errorString());
        return false;
    }
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *err = QCoreApplication::translate("MediaLibrary", "无法写入目标文件：%1").arg(out.errorString());
        return false;
    }
    const qint64 total = in.size();
    constexpr qint64 kChunk = 1 << 20; // 1 MiB
    QByteArray buf(int(kChunk), Qt::Uninitialized);
    qint64 done = 0;
    int lastPct = -1;
    while (!in.atEnd()) {
        const qint64 n = in.read(buf.data(), kChunk);
        if (n < 0 || out.write(buf.constData(), n) != n) {
            *err = QCoreApplication::translate("MediaLibrary", "复制失败：%1").arg(out.errorString());
            out.close();
            QFile::remove(dstPath);
            return false;
        }
        done += n;
        if (total > 0) {
            const int pct = int(done * 100 / total);
            if (pct != lastPct) {
                lastPct = pct;
                if (onPercent) onPercent(pct);
            }
        }
    }
    return true;
}
} // namespace

/**
 * @brief 构造函数：初始化五个分类的组节点
 */
inline QString ImageFileNode = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 0, "internal-data": {"path": "%1"}, "output-count": 1, "position": {"x": %2, "y": %3}, "type": "Image File"}]})");
inline QString JsonFileNode  = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 0, "internal-data": {"path": "%1"}, "output-count": 1, "position": {"x": %2, "y": %3}, "type": "JSON File"}]})");
inline QString IniFileNode   = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 0, "internal-data": {"path": "%1"}, "output-count": 1, "position": {"x": %2, "y": %3}, "type": "INI File"}]})");
inline QString DmxFileNode   = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 5, "internal-data": {"UniverseSettings": { "isLooping": false, "net": 0, "subnet": 0, "universe": 0, "videoFilePath": "%1" } }, "output-count": 5, "port-editable": true, "position": {"x": %2, "y": %3}, "remarks": "Universe Playback", "type": "Universe Playback"}]})");
inline QString AudioFileNode = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 4, "internal-data": {"autoPlay": false, "filePath": "%1", "isLoop": false, "volume": 0 }, "output-count": 3, "port-editable": true, "position": {"x": %2, "y": %3},  "remarks": "Audio Decoder", "type": "Audio Decoder" } ] })");
inline QString VideoFileNode = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 5, "internal-data": {"autoPlay": false, "filePath": "%1", "isLoop": false, "volume": 0 }, "output-count": 4, "port-editable": true, "position": {"x": %2, "y": %3},  "remarks": "Video Decoder", "type": "Video Decoder" } ] })");
// internal-data 须与节点 save()/load() 一致：属性在 values 下；坐标占位符由画布 drop 填入
inline QString ViosoFileNode = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 1, "internal-data": {"values": {"configFile": "%1"}}, "output-count": 1, "position": {"x": %2, "y": %3}, "type": "Vioso"}]})");
inline QString MpcdiFileNode = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 1, "internal-data": {"values": {"configFile": "%1"}}, "output-count": 1, "position": {"x": %2, "y": %3}, "type": "MPCDI"}]})");
inline QString IsfFileNode   = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 0, "internal-data": {"values": {"shaderFile": "%1"}}, "output-count": 1, "position": {"x": %2, "y": %3}, "type": "ISF"}]})");
inline QString GdtfFileNode  = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 1, "internal-data": {"file": "%1"}, "output-count": 1, "position": {"x": %2, "y": %3}, "type": "GDTF Fixture"}]})");
// 拖拽 Game 文件 → Libretro（GBA）节点；romName 为媒体库文件名，节点内反查媒体库全路径
inline QString GameFileNode  = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 12, "internal-data": {"values": {"romName": "%1"}}, "output-count": 3, "position": {"x": %2, "y": %3}, "type": "Libretro"}]})");
// 拖拽 Gesture 文件 → Gesture 手势识别节点；libraryName 为媒体库文件名，节点内反查媒体库全路径（手势端口由节点按库内容动态生成）
inline QString GestureFileNode = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 2, "internal-data": {"values": {"libraryName": "%1"}}, "output-count": 2, "position": {"x": %2, "y": %3}, "type": "Gesture"}]})");
// 拖拽 Faust 文件 → Faust DSP 节点；file 为媒体库文件名（节点内反查媒体库全路径），
// internal-data 键与 FaustDataModel::save()/load() 的顶层键一致；端口由节点按 .dsp 动态生成
inline QString FaustFileNode = QStringLiteral(R"({"nodes": [{"id": 1, "input-count": 0, "internal-data": {"faust": {"file": "%1"}}, "output-count": 0, "position": {"x": %2, "y": %3}, "type": "Faust DSP"}]})");

MediaLibrary::MediaLibrary(QObject* parent)
    : QStandardItemModel(parent)
{
    setColumnCount(1);
    // 初始化五个类别为顶层组节点
    for (Category cat : { Category::Video, Category::Audio, Category::DMX, Category::Image,
                          Category::Model, Category::Document, Category::Vioso, Category::ISF,
                          Category::ChildFlow, Category::Mpcdi, Category::Fixture, Category::Game,
                          Category::Gesture, Category::Faust, Category::Unknown }) {
        auto* group = new QStandardItem(categoryName(cat));
        group->setEditable(false);
        group->setData(static_cast<int>(cat), CategoryRole);
        group->setData(-1, ParentRowRole); // 顶层组的父行为 -1
        invisibleRootItem()->appendRow(group);
        m_categories.insert(cat, group);
    }
    // 新增：初始化存储目录（Windows 文档库下），并扫描已有文件入库
    initializeStorageDir();

    // 连接视频自动转码器：条目状态后缀 + ProgressRole 进度条（MediaProgressDelegate 绘制），
    // 成功后自动入库 HAP 版本
    auto* transcoder = VideoTranscoder::instance();
    connect(transcoder, &VideoTranscoder::transcodeStarted, this,
            [this](const QString& src) {
        setEntryProgress(src, -1, QStringLiteral("[loading]"));
    });
    connect(transcoder, &VideoTranscoder::transcodeProgress, this,
            [this](const QString& src, int percent) {
        setEntryProgress(src, percent,
                         percent < 0 ? QStringLiteral("[loading]")
                                     : QStringLiteral("[loading %1%]").arg(percent));
    });
    connect(transcoder, &VideoTranscoder::transcodeFinished, this,
            [this](const QString& src, const QString& out, bool success, const QString& msg) {
        // 清源条目进度（无论成败：转码已结束）
        setEntryProgress(src, -1, QString());
        if (success && !out.isEmpty()) {
            addFile(out);  // HAP 版本入库（原始文件已被删除并改名为最终名）
            refresh();     // 清除指向已删除原文件的条目；mov 源条目路径==最终名会保留
        } else if (!success) {
            setEntryProgress(src, -1, tr("【转码失败】"));
            qCWarning(logTranscoder) << "转码失败:" << src << msg;
        }
        // 跳过（已是 HAP）时仅清除状态，无额外动作
    });
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
    const QModelIndex idx = addFileCore(absPath);
    if (idx.isValid()) {
        emit libraryChanged();
    }
    return idx;
}

/**
 * @brief addFile 的静默核心（不 emit libraryChanged）——批量插入统一在调用方
 *        尾部 emit 一次，避免批量导入期间视图被 libraryChanged 反复触发重绑
 */
QModelIndex MediaLibrary::addFileCore(const QString& absPath)
{
    const QFileInfo fi(absPath);
    if (!fi.exists() || !fi.isFile()) return QModelIndex();

    const Category cat = detectCategory(absPath);
    QStandardItem* group = ensureCategory(cat);
    if (!group) return QModelIndex();

    if (existsInCategory(group, absPath)) {
        return QModelIndex(); // 已存在，跳过
    }

    // 创建叶子项（图标写进模型数据，见 categoryIcon 头注）
    auto* item = new QStandardItem(fi.fileName());
    item->setEditable(false);
    item->setData(absPath, PathRole);
    item->setData(static_cast<int>(cat), CategoryRole);
    item->setData(group->row(), ParentRowRole); // 叶子项的父组行号
    item->setData(categoryIcon(static_cast<int>(cat)), Qt::DecorationRole);
    // 暂不设置 OrdinalRole，统一在重排时赋值
    group->appendRow(item);

    // 重排序号
    renumberCategory(group);
    return item->index();
}

/**
 * @brief 批量添加文件：库外源文件复制到存储目录（后台线程，条目占位显示进度条），
 *        库内源文件直接入库。失败项汇总为一次弹窗（不逐个轰炸）。
 */
void MediaLibrary::addFiles(const QStringList& absPaths)
{
    QDir storage(QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
    QStringList failures;

    for (const QString& p : absPaths) {
        const QFileInfo fi(p);
        if (!fi.exists() || !fi.isFile()) {
            failures << tr("%1：文件不存在或不可访问").arg(p);
            continue;
        }

        // 源文件已在存储目录中：直接入库（无需复制）
        if (isInStorageDir(fi.absoluteFilePath())) {
            addFileCore(fi.absoluteFilePath());
            maybeTranscode(fi.absoluteFilePath());
            continue;
        }

        // 目标路径为存储目录下同名文件；同名覆盖策略保持既有语义（先删旧文件）
        const QString targetPath = storage.absoluteFilePath(fi.fileName());
        if (s_activeCopies.contains(targetPath)) {
            failures << tr("%1：同名文件正在导入中，请稍后再试").arg(fi.fileName());
            continue;
        }
        if (QFile::exists(targetPath)) {
            if (!QFile::remove(targetPath)) {
                failures << tr("%1：无法覆盖库内同名文件").arg(fi.fileName());
                continue;
            }
            removeItemForPath(targetPath); // 旧条目一并移除，新条目重新编号
        }

        // 条目立即占位（状态"复制中…"），后台线程块拷贝、进度条实时推进
        addPendingEntry(targetPath);
        startBackgroundCopy(fi.absoluteFilePath(), targetPath);
    }

    // 全程 addFileCore/addPendingEntry 均不 emit（信号收敛），行级变更经
    // rowsInserted 实时送达视图（图标在模型数据上，见 addFileCore）；
    // 批量结束在此统一 emit libraryChanged 一次
    if (!failures.isEmpty()) {
        QMessageBox::warning(nullptr,
                             tr("导入失败"),
                             tr("以下文件导入失败：\n\n%1").arg(failures.join(QLatin1Char('\n'))));
    }
    emit libraryChanged();
}


void MediaLibrary::maybeTranscode(const QString& absPath)
{
    if (detectCategory(absPath) != Category::Video) return;
    VideoTranscoder::instance()->enqueue(absPath);
}

void MediaLibrary::handleExternalImport(const QString& absPath)
{
    // 刷新模型（扫描存储目录，把新落盘文件入库），再对视频类文件触发自动转码
    refresh();
    maybeTranscode(absPath);
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
                                     tr("删除失败"),
                                     tr("无法删除文件：\n%1").arg(path));
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
                                         tr("删除失败"),
                                         tr("无法删除文件：\n%1").arg(path));
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
    collect(Category::Vioso,   "Vioso");
    collect(Category::ISF,     "ISF");
    collect(Category::ChildFlow, "ChildFlow");
    collect(Category::Mpcdi,   "MPCDI");
    collect(Category::Fixture, "Fixture");
    collect(Category::Game, "Game");
    collect(Category::Gesture, "Gesture");
    collect(Category::Faust,   "Faust");
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
            addFileCore(v.toString());
        }
    };
    load(Category::Video,   "Video");
    load(Category::Audio,   "Audio");
    load(Category::DMX,     "DMX");
    load(Category::Image, "Picture");
    load(Category::Model,   "Model");
    load(Category::Document,    "Document");
    load(Category::Vioso,   "Vioso");
    load(Category::ISF,     "ISF");
    load(Category::ChildFlow, "ChildFlow");
    load(Category::Mpcdi,   "MPCDI");
    load(Category::Fixture, "Fixture");
    load(Category::Game, "Game");
    load(Category::Gesture, "Gesture");
    load(Category::Faust,   "Faust");
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
 * @brief 返回类别对应的文件扩展名列表（不含点，全小写）
 *        统一数据源：detectCategory 与 MediaLibraryWidget 导入对话框共用
 */
QStringList MediaLibrary::categoryExtensions(Category cat)
{
    switch (cat) {
    case Category::Video:    return { "mp4", "mov", "mkv", "avi", "wmv", "flv", "webm" };
    case Category::Audio:    return { "wav", "mp3", "flac", "aac", "ogg", "m4a" };
    case Category::DMX:      return { "dmx" };
    case Category::Image:    return { "jpg", "jpeg", "png", "bmp", "gif", "webp", "tif", "tiff" };
    case Category::Model:    return { "obj", "fbx", "stl", "gltf", "glb" };
    case Category::Document: return { "txt", "json", "xml", "cfg", "log", "md", "csv", "ini" };
    case Category::Vioso:    return { "vwf" };
    case Category::ISF:      return { "fs" };
    case Category::ChildFlow: return { "childflow" };
    case Category::Mpcdi:    return { "mpcdi" };
    case Category::Fixture:  return { "gdtf" };
    case Category::Game:     return { "gba", "gb", "gbc" };
    case Category::Gesture:  return { "gesture" };
    case Category::Faust:    return { "dsp" };
    case Category::Unknown:  break;
    }
    return {};
}

/**
 * @brief 根据扩展名判定类别
 */
MediaLibrary::Category MediaLibrary::detectCategory(const QString& absPath) const
{
    const QString ext = QFileInfo(absPath).suffix().toLower();
    // 扩展名→类别映射（与 categoryExtensions 同源，首次调用时构建一次）
    // 说明：各类别扩展名集合互不相交，判定顺序不影响结果
    static const QHash<QString, Category> extMap = [] {
        QHash<QString, Category> map;
        const QList<Category> cats = { Category::Video, Category::Audio, Category::DMX,
                                       Category::Image, Category::Model, Category::Vioso,
                                       Category::ISF, Category::ChildFlow, Category::Mpcdi,
                                       Category::Fixture, Category::Game, Category::Gesture,
                                       Category::Faust, Category::Document };
        for (Category cat : cats) {
            for (const QString& e : categoryExtensions(cat)) {
                map.insert(e, cat);
            }
        }
        return map;
    }();
    return extMap.value(ext, Category::Unknown);
}

/**
 * @brief 类别图标（模型侧统一数据源；MediaLibraryWidget 的组/子项共用）
 */
QIcon MediaLibrary::categoryIcon(int cat)
{
    using Cat = Category;
    switch (static_cast<Cat>(cat)) {
    case Cat::Video:     return QIcon(QStringLiteral(":/icons/icons/video.png"));
    case Cat::Audio:     return QIcon(QStringLiteral(":/icons/icons/audio.png"));
    case Cat::DMX:       return QIcon(QStringLiteral(":/icons/icons/stage.png"));
    case Cat::Image:     return QIcon(QStringLiteral(":/icons/icons/image.png"));
    case Cat::Model:     return QIcon(QStringLiteral(":/icons/icons/model_1.png"));
    case Cat::Document:  return QIcon(QStringLiteral(":/icons/icons/document.png"));
    case Cat::Vioso:     return QIcon(QStringLiteral(":/icons/icons/vioso.png"));
    case Cat::ISF:       return QIcon(QStringLiteral(":/icons/icons/isf.png"));
    case Cat::ChildFlow: return QIcon(QStringLiteral(":/icons/icons/Flow.png"));
    case Cat::Mpcdi:     return QIcon(QStringLiteral(":/icons/icons/image.png"));
    case Cat::Fixture:   return QIcon(QStringLiteral(":/icons/icons/gdtf.png"));
    case Cat::Game:      return QIcon(QStringLiteral(":/icons/icons/games.png"));
    case Cat::Gesture:   return QIcon(QStringLiteral(":/icons/icons/finger.png"));
    // Faust 暂无专属图标，先沿用音频图标（与 DSP 的音频属性一致）
    case Cat::Faust:     return QIcon(QStringLiteral(":/icons/icons/audio.png"));
    case Cat::Unknown:   return QIcon(QStringLiteral(":/icons/icons/unknown.png"));
    }
    return QIcon();
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
    case Category::Vioso:   return QStringLiteral("Vioso");
    case Category::ISF:     return QStringLiteral("ISF");
    case Category::ChildFlow: return QStringLiteral("Child Flows");
    case Category::Mpcdi:   return QStringLiteral("MPCDI");
    case Category::Fixture: return QStringLiteral("Fixtures");
    case Category::Game:    return QStringLiteral("Games");
    case Category::Gesture: return QStringLiteral("Gestures");
    case Category::Faust:   return QStringLiteral("Faust DSP");
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
        item->setData(r + 1, OrdinalRole);
        refreshEntryText(item);
    }
}

/**
 * @brief 单行文本重排：序号 + 文件名 + 状态后缀（复制中/转码中/失败等）。
 *        renumberCategory 与 setEntryProgress 共用，保证文本格式单一来源
 */
void MediaLibrary::refreshEntryText(QStandardItem* item)
{
    if (!item) return;
    const QString path = item->data(PathRole).toString();
    const int ordinal = item->data(OrdinalRole).toInt();
    QString text = QString::number(ordinal) + QStringLiteral("  ") + QFileInfo(path).fileName();
    const QString status = m_statusByPath.value(path);
    if (!status.isEmpty()) {
        text += QStringLiteral("  ") + status;
    }
    item->setText(text);
}

/**
 * @brief 全库按路径查条目（大小写不敏感，与 existsInCategory 同口径）
 */
QStandardItem* MediaLibrary::entryForPath(const QString& absPath) const
{
    for (auto it = m_categories.cbegin(); it != m_categories.cend(); ++it) {
        QStandardItem* group = it.value();
        if (!group) continue;
        for (int r = 0; r < group->rowCount(); ++r) {
            QStandardItem* child = group->child(r);
            if (child && child->data(PathRole).toString().compare(absPath, Qt::CaseInsensitive) == 0) {
                return child;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 更新条目进度（ProgressRole 供 MediaProgressDelegate 画背景进度条，
 *        status 供文本后缀）。percent<0 表示无进度（仅状态文案）。
 *        复制/转码进度高频更新走这里，只刷新目标条目不重排全组
 */
void MediaLibrary::setEntryProgress(const QString& path, int percent, const QString& status)
{
    QStandardItem* item = entryForPath(path);
    if (!item) return;
    if (status.isEmpty()) {
        m_statusByPath.remove(path);
    } else {
        m_statusByPath[path] = status;
    }
    item->setData(percent, ProgressRole);
    refreshEntryText(item);
}

/**
 * @brief 插入"导入复制中"占位条目：目标文件尚未落盘，跳过 addFileCore 的
 *        磁盘存在性检查；状态与进度挂条目上，完成回调里收尾
 */
void MediaLibrary::addPendingEntry(const QString& targetPath)
{
    const Category cat = detectCategory(targetPath);
    QStandardItem* group = ensureCategory(cat);
    if (!group) return;
    if (entryForPath(targetPath)) return; // 同路径条目已在（重复提交防御）

    auto* item = new QStandardItem(QFileInfo(targetPath).fileName());
    item->setEditable(false);
    item->setData(targetPath, PathRole);
    item->setData(static_cast<int>(cat), CategoryRole);
    item->setData(group->row(), ParentRowRole);
    item->setData(categoryIcon(static_cast<int>(cat)), Qt::DecorationRole);
    item->setData(-1, ProgressRole);
    group->appendRow(item);
    renumberCategory(group);

    m_statusByPath[targetPath] = tr("复制中…");
    refreshEntryText(item);
}

/**
 * @brief 移除库内指向指定路径的全部条目（覆盖导入时清旧条目）
 */
void MediaLibrary::removeItemForPath(const QString& absPath)
{
    for (auto it = m_categories.begin(); it != m_categories.end(); ++it) {
        QStandardItem* group = it.value();
        if (!group) continue;
        for (int r = group->rowCount() - 1; r >= 0; --r) {
            QStandardItem* child = group->child(r);
            if (child && child->data(PathRole).toString().compare(absPath, Qt::CaseInsensitive) == 0) {
                group->removeRow(r);
            }
        }
    }
}

/**
 * @brief 提交后台复制任务：QThreadPool 线程块拷贝到 .copying 临时名，
 *        进度经 invokeMethod 排回主线程（this 析构后自动丢弃），完成后改名收尾
 */
void MediaLibrary::startBackgroundCopy(const QString& srcPath, const QString& targetPath)
{
    s_activeCopies.insert(targetPath);
    const QString tmpPath = targetPath + QLatin1String(kCopyTempSuffix);
    QThreadPool::globalInstance()->start([srcPath, targetPath, tmpPath, this] {
        QString err;
        bool ok = copyFileWithProgress(srcPath, tmpPath,
            [this, targetPath](int percent) {
                QMetaObject::invokeMethod(this, [this, targetPath, percent] {
                    if (s_activeCopies.contains(targetPath)) {
                        setEntryProgress(targetPath, percent,
                                         tr("复制中 %1%").arg(percent));
                    }
                }, Qt::QueuedConnection);
            }, &err);
        if (ok) {
            // 复制成功：临时名改回正式名（同卷 rename；target 此刻不存在——
            // 提交前已删旧文件且 s_activeCopies 拒绝同名并发）
            if (!QFile::rename(tmpPath, targetPath)) {
                QFile::remove(tmpPath);
                err = tr("复制完成但落位失败（改名 %1 → %2）").arg(tmpPath, targetPath);
                ok = false;
            }
        } else {
            QFile::remove(tmpPath);
        }
        QMetaObject::invokeMethod(this, [this, targetPath, ok, err] {
            finishBackgroundCopy(targetPath, ok, err);
        }, Qt::QueuedConnection);
    });
}

/**
 * @brief 复制完成回调（恒主线程）：成功清状态+触发转码；失败删条目占位挂【复制失败】
 *        （条目保留可让用户看到哪个失败；下次 refresh 因文件不存在会自然清除）
 */
void MediaLibrary::finishBackgroundCopy(const QString& targetPath, bool ok, const QString& err)
{
    s_activeCopies.remove(targetPath);
    if (ok) {
        setEntryProgress(targetPath, -1, QString());
        maybeTranscode(targetPath);
    } else {
        qCWarning(logTranscoder) << "导入复制失败:" << targetPath << err;
        setEntryProgress(targetPath, -1, tr("【复制失败】"));
    }
    emit libraryChanged();
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
    names.insert(ProgressRole,  "progress");
    return names;
}

/**
 * @brief 生成拖拽的 mimeData（application/media-item）,生成节点创建 JSON 模板，并复制到剪贴板
 * 仅包含文件标签与文件名（不包含绝对路径），用于画布按标签创建节点
 */
QMimeData* MediaLibrary::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* md = new QMimeData();

    // 单次遍历：同时完成 mimeData 生成 与 剪贴板模板复制
    for (const QModelIndex& idx : indexes) {
        if (!idx.isValid()) continue;

        const QStandardItem* item = this->itemFromIndex(idx);
        if (!item) continue;

        const QStandardItem* parentGroup = item->parent();
        if (!parentGroup) continue;          // 顶层组不参与

        const QString path = item->data(PathRole).toString();
        if (path.isEmpty()) continue;

        const QString name = QFileInfo(path).fileName();
        const int catInt   = item->data(CategoryRole).toInt();
        const Category cat = static_cast<Category>(catInt);

        // 1. 生成 mimeData（仅一次）
        static const QHash<Category, QString> tagMap = {
            {Category::Video,    "Video"},
            {Category::Audio,    "Audio"},
            {Category::DMX,      "DMX"},
            {Category::Image,    "Image"},
            {Category::Model,    "Model"},
            {Category::Document, "Document"},
            {Category::Vioso,    "Vioso"},
            {Category::ISF,      "ISF"},
            {Category::ChildFlow,"ChildFlow"},
            {Category::Mpcdi,    "MPCDI"},
            {Category::Fixture,  "Fixture"},
            {Category::Game,     "Game"},
            {Category::Gesture,  "Gesture"},
            {Category::Faust,    "Faust"},
            {Category::Unknown,  "Unknown"}
        };
        const QString tagKey = tagMap.value(cat, "Unknown");

        QJsonObject obj;
        obj["tag"]  = tagKey;
        obj["name"] = name;
        md->setData("application/media-item",
                    QJsonDocument(obj).toJson(QJsonDocument::Compact));

        // 2. 复制节点模板到剪贴板（仅一次）
        const QFileInfo fi(path);
        const QString fileName = fi.fileName();
        QString tpl;
        switch (cat) {
            case Category::Video:
                tpl = VideoFileNode.arg(fileName);
                    break;
                case Category::Audio:
                    tpl = AudioFileNode.arg(fileName);
                    break;
                case Category::Image:
                    tpl = ImageFileNode.arg(fileName);
                    break;
                case Category::DMX:
                    tpl = DmxFileNode.arg(fileName);
                    break;
                case Category::Model:
                    break;
                case Category::Document: {
                    const QString ext = fi.suffix().toLower();
                    if (ext == "json") tpl = JsonFileNode.arg(fileName);
                    else if (ext == "ini") tpl = IniFileNode.arg(fileName);
                }
                    break;
                case Category::Vioso:
                    tpl = ViosoFileNode.arg(fileName);
                    break;
                case Category::Mpcdi:
                    tpl = MpcdiFileNode.arg(fileName);
                    break;
                case Category::Fixture:
                    // 拖拽生成 GDTF Fixture 节点；file 为媒体库文件名（节点内解析到存储目录），
                    // internal-data 键与 GDTFDataModel::save()/load() 顶层键一致
                    tpl = GdtfFileNode.arg(fileName);
                    break;
                case Category::Game:
                    // 拖拽生成 Libretro（GBA）节点；romName 为媒体库文件名（节点内反查）
                    tpl = GameFileNode.arg(fileName);
                    break;
                case Category::Gesture:
                    // 拖拽生成 Gesture 手势识别节点；libraryName 为媒体库文件名（节点内反查）
                    tpl = GestureFileNode.arg(fileName);
                    break;
                case Category::ISF:
                    tpl = IsfFileNode.arg(fileName);
                    break;
                case Category::Faust:
                    // 拖拽生成 Faust DSP 节点；file 为媒体库文件名（节点内反查）
                    tpl = FaustFileNode.arg(fileName);
                    break;
                case Category::ChildFlow: {
                    // 读取 .childflow 场景，包装为 Container 节点供画布 Paste
                    QFile cf(path);
                    if (!cf.open(QIODevice::ReadOnly))
                        break;
                    QJsonParseError perr;
                    const QJsonDocument sceneDoc = QJsonDocument::fromJson(cf.readAll(), &perr);
                    if (perr.error != QJsonParseError::NoError || !sceneDoc.isObject())
                        break;

                    QJsonObject internalData;
                    internalData.insert(QStringLiteral("inner-scene"), sceneDoc.object());

                    QJsonObject pos;
                    pos.insert(QStringLiteral("x"), 0);
                    pos.insert(QStringLiteral("y"), 0);

                    const QString remarks = fi.completeBaseName();
                    QJsonObject node;
                    node.insert(QStringLiteral("id"), 1);
                    node.insert(QStringLiteral("input-count"), 0);
                    node.insert(QStringLiteral("output-count"), 0);
                    node.insert(QStringLiteral("internal-data"), internalData);
                    node.insert(QStringLiteral("position"), pos);
                    node.insert(QStringLiteral("type"), QStringLiteral("Container"));
                    node.insert(QStringLiteral("remarks"), remarks);

                    QJsonObject pasteScene;
                    pasteScene.insert(QStringLiteral("nodes"), QJsonArray{node});
                    tpl = QString::fromUtf8(
                        QJsonDocument(pasteScene).toJson(QJsonDocument::Compact));
                }
                    break;
                case Category::Unknown:
                    tpl.clear();
                    break;
        }
        if (!tpl.isEmpty()) {
            if (QClipboard* cb = QGuiApplication::clipboard())
                cb->setText(tpl);
        }

        break;   // 仅处理第一个有效文件项
    }

    return md;
}

/**
 * @brief 声明支持的 mime 类型（包含 application/media-item）
 */
QStringList MediaLibrary::mimeTypes() const
{
    QStringList types = QStandardItemModel::mimeTypes();
    if (!types.contains("application/media-item"))
        types << "application/media-item";
    return types;
}

/**
 * @brief 拖拽动作为复制
 */
Qt::DropActions MediaLibrary::supportedDragActions() const
{
    return Qt::CopyAction;
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
    if (!QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).exists()) {
        QDir().mkpath(AppConstants::MEDIA_LIBRARY_STORAGE_DIR);
    }


    // 扫描存储目录中的所有文件（仅一层）；先清掉上次异常退出残留的复制临时文件
    QDir storage(QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
    const QFileInfoList all = storage.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& fi : all) {
        if (fi.fileName().endsWith(QLatin1String(kCopyTempSuffix), Qt::CaseInsensitive)) {
            qCWarning(logTranscoder) << "清理上次未完成的导入复制临时文件:" << fi.absoluteFilePath();
            QFile::remove(fi.absoluteFilePath());
        }
    }
    const QFileInfoList infos = storage.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& fi : infos) {
        addFileCore(fi.absoluteFilePath());
    }
}

// 新增：在存储目录内生成不重名的文件名
QString MediaLibrary::uniqueFileNameInStorage(const QString& fileName) const
{
    QDir storage(QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
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
/**
 * @brief 更稳健的库内路径判断（修复 Windows 跨盘符误判）
 *
 * 背景：
 * - 之前使用 QDir::relativeFilePath(absPath) 并判断是否以 ".." 开头，
 *   在 Windows 上当源文件与库目录不在同一盘符时，relativeFilePath 会返回带盘符的绝对路径（如 "D:/..."），
 *   该字符串不以 ".." 开头，导致误判为“在库目录内”，从而跳过复制。
 *
 * 策略：
 * - 使用 canonicalPath/canonicalFilePath 获取规范绝对路径（消除符号链接与相对路径）
 * - 若 canonical 获取失败则回退到 absolutePath/absoluteFilePath
 * - 规范化分隔符并追加尾随斜杠后，做前缀匹配判断包含关系
 * - Windows 上大小写不敏感
 */
const QString storageCanonical = QDir(QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absolutePath()).canonicalPath();
QString fileCanonical = QFileInfo(absPath).canonicalFilePath();

QString storagePath = storageCanonical.isEmpty()
    ? QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absolutePath()
    : storageCanonical;
QString filePath = fileCanonical.isEmpty()
    ? QFileInfo(absPath).absoluteFilePath()
    : fileCanonical;

storagePath = QDir::cleanPath(storagePath);
filePath = QDir::cleanPath(filePath);

// 确保库路径有尾随斜杠，避免前缀误匹配
QString storagePrefix = storagePath;
if (!storagePrefix.endsWith('/')) storagePrefix += '/';

#ifdef Q_OS_WIN
// Windows：大小写不敏感匹配
return filePath.startsWith(storagePrefix, Qt::CaseInsensitive);
#else
return filePath.startsWith(storagePrefix);
#endif
}



void MediaLibrary::refresh() {
    // 屏蔽信号，避免过程中产生大量中间状态通知
    this->blockSignals(true);
    bool anyChange = false;

    // 确保存储目录存在
    if (!QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).exists()) {
        QDir().mkpath(AppConstants::MEDIA_LIBRARY_STORAGE_DIR);
    }
    // 当前存储目录文件集合（跳过导入复制中的 .copying 临时文件——它们不是素材）
    QDir storage(QDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR).absolutePath());
    const QFileInfoList infos = storage.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    QSet<QString> storageFiles;
    for (const QFileInfo& fi : infos) {
        if (fi.fileName().endsWith(QLatin1String(kCopyTempSuffix), Qt::CaseInsensitive)) continue;
        storageFiles.insert(fi.absoluteFilePath());
    }

    // 清理失效项，并按扩展名重新归类（例如原 Documents 下的 .vwf/.fs）
    QSet<QString> modelPaths;
    QSet<Category> dirtyGroups;
    QVector<QPair<Category, QString>> pendingReadd;
    for (auto it = m_categories.begin(); it != m_categories.end(); ++it) {
        const Category storedCat = it.key();
        QStandardItem* group = it.value();
        if (!group) continue;
        for (int r = group->rowCount() - 1; r >= 0; --r) {
            QStandardItem* item = group->child(r);
            if (!item) continue;
            const QString path = item->data(PathRole).toString();
            if (path.isEmpty()) continue;

            // 正在后台复制的占位条目跳过失效检查（目标文件尚未落盘，非失效）
            if (s_activeCopies.contains(path)) {
                modelPaths.insert(path);
                continue;
            }

            if (!QFileInfo(path).exists()) {
                group->removeRow(r);
                dirtyGroups.insert(storedCat);
                anyChange = true;
                continue;
            }

            const Category detected = detectCategory(path);
            if (detected != storedCat) {
                group->removeRow(r);
                dirtyGroups.insert(storedCat);
                pendingReadd.append({detected, path});
                anyChange = true;
                continue;
            }

            modelPaths.insert(path);
        }
    }
    for (Category cat : dirtyGroups) {
        renumberCategory(m_categories.value(cat));
    }
    for (const auto& entry : pendingReadd) {
        if (modelPaths.contains(entry.second)) {
            continue;
        }
        if (addFileCore(entry.second).isValid()) {
            modelPaths.insert(entry.second);
            anyChange = true;
        }
    }

    // 将存储目录中新出现的文件加入模型
    for (const QString& absPath : storageFiles) {
        if (!modelPaths.contains(absPath)) {
            if (addFileCore(absPath).isValid()) {
                anyChange = true;
            }
        }
    }

    // 重新开启信号，并统一通知一次
    this->blockSignals(false);
    if (anyChange) {
        emit libraryChanged();
    }
}
