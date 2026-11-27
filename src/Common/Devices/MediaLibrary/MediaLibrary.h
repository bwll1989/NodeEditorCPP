#pragma once
#include <QObject>
// #include <QStandardItemModel>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QtGui/QStandardItemModel>

#ifdef MEDIAMANGER_LIBRARY
#define MEDIAMANGER_EXPORT Q_DECL_EXPORT
#else
#define MEDIAMANGER_EXPORT Q_DECL_IMPORT
#endif

/**
 * @class MediaLibrary
 * @brief 媒体库数据模型。负责分类、增删、去重、序号维护以及JSON持久化。
 */
class MEDIAMANGER_EXPORT MediaLibrary : public QStandardItemModel
{
    Q_OBJECT
public:
    /**
     * @brief 媒体类别
     */
    enum class Category {
        Video,
        Audio,
        DMX,
        Image,
        Model,
        Document,
        Vioso,
        ISF,
        ChildFlow,
        Mpcdi,
        Fixture,
        Game,
        Gesture,
        Faust,
        Unknown
    };
    Q_ENUM(Category)

    /**
     * @brief 自定义数据角色
     * PathRole     文件绝对路径
     * CategoryRole 文件所属类别
     * OrdinalRole  序号（在类别中的位置，从1开始）
     */
    enum Roles {
        PathRole     = Qt::UserRole + 1,
        CategoryRole = Qt::UserRole + 2,
        OrdinalRole  = Qt::UserRole + 3,
        ParentRowRole= Qt::UserRole + 4,  // 父组的行号（顶层组为 -1）
        ProgressRole = Qt::UserRole + 5   // 进度百分比 0-100（-1/无效=无进度；导入复制与视频转码共用）
    };

    /**
     * @brief 构造函数：初始化五个分类的组节点
     */
    explicit MediaLibrary(QObject* parent = nullptr);

    static MediaLibrary* instance() {
        static MediaLibrary* sender = nullptr;
        if (!sender) {
            sender = new MediaLibrary();
        }
        return sender;
    }
    /**
     * @brief 确保类别根项存在并返回
     */
    QStandardItem* ensureCategory(Category cat);

    /**
     * @brief 根据文件路径添加单个文件，自动分类、去重并维护序号
     * @return 新增的索引；若重复则返回无效索引
     */
    QModelIndex addFile(const QString& absPath);

    /**
     * @brief 批量添加文件
     */
    void addFiles(const QStringList& absPaths);

    /**
     * @brief 添加文件夹（递归或扁平可按需扩展，这里仅一层）
     */
    void addFolder(const QString& folderPath);

    /**
     * @brief 删除选中索引（支持多选），自动重排序号
     */
    void removeIndexes(const QModelIndexList& indexes);

    /**
     * @brief 清空媒体库（保留分类组节点）
     */
    void clearLibrary();

    /**
     * @brief 外部通道（网页上传等）文件落盘后的统一处理：刷新模型入库，并对视频触发自动转码
     *        供 HttpServer 线程经 QMetaObject::invokeMethod(QueuedConnection) 调用，实际在主线程执行
     */
    Q_INVOKABLE void handleExternalImport(const QString& absPath);

    /**
     * @brief 返回某索引对应的文件路径（若为组节点则返回空）
     */
    QString filePathForIndex(const QModelIndex& index) const;

    /**
     * @brief 返回全部文件路径
     */
    QStringList allFiles() const;

    QStringList getFileList(Category cat) const;

    /**
     * @brief 返回类别对应的文件扩展名列表（不含点，全小写）
     *        作为自动分类与导入对话框过滤器的统一数据源，避免两处扩展名不同步
     */
    static QStringList categoryExtensions(Category cat);

    /**
     * @brief 类别图标（模型侧统一数据源）。
     *        addFileCore/addPendingEntry 写入条目 DecorationRole——图标作为模型数据，
     *        refresh() blockSignals 挡住 rowsInserted 时视图重绑仍能取到
     *        （原先靠视图侧 onRowsInserted 补挂，该路径被 blockSignals 挡掉即丢图标）
     */
    static QIcon categoryIcon(int cat);


    /**
     * @brief 导出为 JSON 对象
     */
    QJsonObject toJson() const;

    /**
     * @brief 从 JSON 对象加载
     */
    bool fromJson(const QJsonObject& obj);

    /**
     * @brief 保存到文件（UTF-8 JSON）
     */
    bool saveToFile(const QString& jsonPath) const;

    /**
     * @brief 从文件加载（UTF-8 JSON）
     */
    bool loadFromFile(const QString& jsonPath);

signals:
    /**
     * @brief 数据变更信号（用于通知视图层刷新或外部持久化）
     */
    void libraryChanged();
public slots:
    /**
     *
     */
    void refresh();
private:
    /**
     * @brief 根据扩展名判定类别
     */
    Category detectCategory(const QString& absPath) const;

    /**
     * @brief addFile 的静默核心（不 emit libraryChanged）。
     *        批量导入/refresh/fromJson 用它逐条插入、最后统一 emit 一次，
     *        避免视图被 libraryChanged 触发 N 次重绑
     */
    QModelIndex addFileCore(const QString& absPath);

    /**
     * @brief 类别中文名
     */
    QString categoryName(Category cat) const;

    /**
     * @brief 为指定类别组重排序号
     */
    void renumberCategory(QStandardItem* catItem);

    /**
     * @brief 判断该类别组内是否存在指定路径的文件（去重）
     */
    bool existsInCategory(QStandardItem* catItem, const QString& absPath) const;

    /**
     * @brief 全库按路径查条目（大小写不敏感）；找不到返回 nullptr
     *        供复制/转码进度回写定位条目
     */
    QStandardItem* entryForPath(const QString& absPath) const;

    /**
     * @brief 单行文本重排：序号 + 文件名 +（状态后缀）。renumberCategory 的单行版，
     *        供进度高频更新只刷新目标条目、不重排全组
     */
    void refreshEntryText(QStandardItem* item);

    /**
     * @brief 更新条目进度（ProgressRole + 状态后缀）。status 为空串则清除状态。
     *        导入复制与视频转码共用；条目不存在时静默返回
     */
    void setEntryProgress(const QString& path, int percent, const QString& status);

    /**
     * @brief 插入"导入复制中"占位条目（不做磁盘存在性检查——目标文件尚未复制完）
     *        状态挂"复制中…"，复制完成回调里清除状态/转码，失败则挂【复制失败】
     */
    void addPendingEntry(const QString& targetPath);

    /**
     * @brief 移除库内指向指定路径的全部条目（覆盖导入时清旧条目，让新条目重新编号）
     */
    void removeItemForPath(const QString& absPath);

    /**
     * @brief 后台线程块拷贝 srcPath→tmp，成功后改名为 targetPath；
     *        进度经 invokeMethod 排回主线程 setEntryProgress，完成走 finishBackgroundCopy
     */
    void startBackgroundCopy(const QString& srcPath, const QString& targetPath);

    /**
     * @brief 复制完成回调（恒主线程）：成功清状态+触发转码；失败删临时文件挂【复制失败】
     */
    void finishBackgroundCopy(const QString& targetPath, bool ok, const QString& err);

    /**
     * @brief 若为视频类文件则加入 HAP 自动转码队列（仅显式导入时调用）
     */
    void maybeTranscode(const QString& absPath);
private:
    QHash<Category, QStandardItem*> m_categories;

    /**
     * @brief 视频转码状态表（key=源文件绝对路径，value=状态文案）
     *        非空时 renumberCategory 在条目文本后附加【状态】后缀
     */
    QHash<QString, QString> m_statusByPath;
    void initializeStorageDir();
    QString uniqueFileNameInStorage(const QString& fileName) const;
    // 新增：判断路径是否位于存储目录
    bool isInStorageDir(const QString& absPath) const;

    /**
     * @brief 重写角色名映射，向 QML 暴露自定义角色
     */
    QHash<int, QByteArray> roleNames() const override;
    /**
     * @brief 支持从媒体库拖拽：提供文件URL等mime类型
     * 生成 text/uri-list，便于其他视图识别并创建对应节点
     */
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    /**
     * @brief 声明模型支持的拖拽mime类型（包含 text/uri-list）
     */
    QStringList mimeTypes() const override;

    /**
     * @brief 拖拽动作为复制（不移动媒体库条目）
     */
    Qt::DropActions supportedDragActions() const override;
};