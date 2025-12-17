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
        ParentRowRole= Qt::UserRole + 4  // 新增：父组的行号（顶层组为 -1）
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
     * @brief 返回某索引对应的文件路径（若为组节点则返回空）
     */
    QString filePathForIndex(const QModelIndex& index) const;

    /**
     * @brief 返回全部文件路径
     */
    QStringList allFiles() const;

    QStringList getFileList(Category cat) const;


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
private:
    QHash<Category, QStandardItem*> m_categories;
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