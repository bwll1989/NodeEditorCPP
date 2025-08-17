#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <QHash>
#include <QTimer>
#include <QQmlEngine>

/**
 * @brief 高性能树形数据模型，支持虚拟化和懒加载
 * 专门为大数据量优化，支持增量更新和过滤
 */
class DataTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int totalItems READ totalItems NOTIFY totalItemsChanged)
    Q_PROPERTY(int filteredItems READ filteredItems NOTIFY filteredItemsChanged)
    Q_PROPERTY(bool lazyLoadingEnabled READ lazyLoadingEnabled WRITE setLazyLoadingEnabled NOTIFY lazyLoadingEnabledChanged)
    Q_PROPERTY(int maxVisibleItems READ maxVisibleItems WRITE setMaxVisibleItems NOTIFY maxVisibleItemsChanged)

public:
    explicit DataTreeModel(QObject *parent = nullptr);
    ~DataTreeModel() override;

    // QAbstractItemModel接口
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    // 属性访问器
    int totalItems() const { return m_totalItems; }
    int filteredItems() const { return m_filteredItems; }
    bool lazyLoadingEnabled() const { return m_lazyLoadingEnabled; }
    int maxVisibleItems() const { return m_maxVisibleItems; }

    // 公共方法
    Q_INVOKABLE void setRootData(const QVariantMap &data);
    Q_INVOKABLE void updateData(const QVariantMap &newData);
    Q_INVOKABLE void setFilter(const QString &filterText);
    Q_INVOKABLE void setLazyLoadingEnabled(bool enabled);
    Q_INVOKABLE void setMaxVisibleItems(int maxItems);

signals:
    void totalItemsChanged();
    void filteredItemsChanged();
    void lazyLoadingEnabledChanged();
    void maxVisibleItemsChanged();

private slots:
    /**
     * @brief 延迟更新处理，避免频繁的UI更新
     */
    void performDelayedUpdate();

private:
    struct TreeItem {
        QString key;
        QVariant value;
        QString valueType;
        QString displayValue;
        TreeItem* parent = nullptr;
        QList<TreeItem*> children;
        bool isLazyLoaded = false;
        bool isVisible = true;
        int depth = 0;
        
        ~TreeItem() {
            qDeleteAll(children);
        }
    };

    TreeItem* m_rootItem;
    QVariantMap m_currentData;
    QVariantMap m_pendingData;  // 添加这个缺失的成员变量
    QHash<TreeItem*, QVariant> m_lazyDataCache;  // 添加懒加载数据缓存
    QString m_filterText;
    bool m_lazyLoadingEnabled = true;
    int m_maxVisibleItems = 1000;
    int m_totalItems = 0;
    int m_filteredItems = 0;
    QTimer* m_updateTimer;

    /**
     * @brief 从QVariantMap创建树形结构
     * @param data 数据映射
     * @param parent 父项目
     * @param depth 当前深度
     */
    void createTreeFromMap(const QVariantMap &data, TreeItem* parent, int depth = 0);

    /**
     * @brief 增量更新树形结构
     * @param newData 新数据
     * @param oldData 旧数据
     * @param parent 父项目
     */
    void updateTreeIncremental(const QVariantMap &newData, const QVariantMap &oldData, TreeItem* parent);

    /**
     * @brief 应用过滤器
     * @param item 要过滤的项目
     * @param filterText 过滤文本
     * @return 是否匹配过滤条件
     */
    bool applyFilter(TreeItem* item, const QString &filterText);

    /**
     * @brief 懒加载子项目
     * @param item 要加载的项目
     */
    void lazyLoadChildren(TreeItem* item);

    /**
     * @brief 获取项目的显示值
     * @param value 原始值
     * @return 格式化后的显示值
     */
    QString getDisplayValue(const QVariant &value);

    /**
     * @brief 获取值的类型字符串
     * @param value 值
     * @return 类型字符串
     */
    QString getValueType(const QVariant &value);

    /**
     * @brief 计算总项目数
     * @param item 起始项目
     * @return 项目总数
     */
    int calculateTotalItems(TreeItem* item = nullptr);

    /**
     * @brief 计算可见项目数
     * @param item 起始项目
     * @return 可见项目数
     */
    int calculateVisibleItems(TreeItem* item = nullptr);

    /**
     * @brief 更新过滤后的项目数
     */
    void updateFilteredItemsCount();

    TreeItem* getItem(const QModelIndex &index) const;
    
    /**
     * @brief 优化的属性构建方法，支持懒加载
     * @param map 数据映射
     */
    void buildPropertiesFromMapOptimized(const QVariantMap& map);
    
    /**
     * @brief 懒加载版本的属性更新方法
     * @param map 数据映射
     * @param parent 父属性
     * @param readOnly 是否只读
     */
    void updatePropertiesFromMapLazy(const QVariantMap& map, TreeItem* parent = nullptr, bool readOnly = true);
    
    /**
     * @brief 创建简单属性
     * @param key 属性键
     * @param value 属性值
     * @param parent 父属性
     * @param readOnly 是否只读
     */
    void createSimpleProperty(const QString& key, const QVariant& value, TreeItem* parent, bool readOnly);
    
    /**
     * @brief 递归进行增量更新
     * @param newMap 新数据
     * @param oldMap 旧数据
     * @param parent 父属性
     */
    void updateMapIncremental(const QVariantMap& newMap, const QVariantMap& oldMap, TreeItem* parent);
};