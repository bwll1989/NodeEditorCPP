#include "DataTreeModel.h"

#include <QColor>
#include <QDebug>
#include <QFont>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap>
#include <qquaternion.h>
#include <QRectF>
#include <QSizeF>
#include <qvectornd.h>

DataTreeModel::DataTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootItem(new TreeItem)
    , m_lazyLoadingEnabled(true)
    , m_maxVisibleItems(1000)
    , m_totalItems(0)
    , m_filteredItems(0)
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(16); // 减少到16ms（约60fps）以提高实时性
    connect(m_updateTimer, &QTimer::timeout, this, &DataTreeModel::performDelayedUpdate);
}

DataTreeModel::~DataTreeModel()
{
    delete m_rootItem;
}

QModelIndex DataTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    TreeItem *childItem = parentItem->children.value(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex DataTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(child);
    TreeItem *parentItem = childItem ? childItem->parent : nullptr;

    if (parentItem == m_rootItem || !parentItem)
        return QModelIndex();

    TreeItem *grandParentItem = parentItem->parent;
    if (!grandParentItem)
        return QModelIndex();

    int row = grandParentItem->children.indexOf(parentItem);
    return createIndex(row, 0, parentItem);
}

int DataTreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem = getItem(parent);
    return parentItem ? parentItem->children.count() : 0;
}

int DataTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1; // 单列显示
}

QVariant DataTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeItem *item = getItem(index);
    if (!item)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        // 返回空字符串，让自定义委托完全控制显示
        return QString();
    case Qt::UserRole: // key
        return item->key;
    case Qt::UserRole + 1: // value
        return item->value;
    case Qt::UserRole + 2: // valueType
        return item->valueType;
    case Qt::UserRole + 3: // displayValue
        return item->displayValue;
    case Qt::UserRole + 4: // hasChildren
        return !item->children.isEmpty();
    case Qt::UserRole + 5: // depth
        return item->depth;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DataTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[Qt::UserRole] = "key";
    roles[Qt::UserRole + 1] = "value";
    roles[Qt::UserRole + 2] = "valueType";
    roles[Qt::UserRole + 3] = "displayValue";
    roles[Qt::UserRole + 4] = "hasChildren";
    roles[Qt::UserRole + 5] = "depth";
    return roles;
}

/**
 * @brief 检查指定项目是否有子项目
 * @param parent 父项目索引
 * @return 是否有子项目
 */
bool DataTreeModel::hasChildren(const QModelIndex &parent) const
{
    TreeItem *parentItem = getItem(parent);
    return parentItem && !parentItem->children.isEmpty();
}

/**
 * @brief 设置根数据并重建模型
 * @param data 要显示的数据
 */
void DataTreeModel::setRootData(const QVariantMap &data)
{
    beginResetModel();
    
    delete m_rootItem;
    m_rootItem = new TreeItem;
    m_currentData = data;
    
    createTreeFromMap(data, m_rootItem);
    
    m_totalItems = calculateTotalItems();
    updateFilteredItemsCount();
    
    endResetModel();
    
    emit totalItemsChanged();
    emit filteredItemsChanged();
}

/**
 * @brief 增量更新数据
 * @param newData 新的数据
 */
void DataTreeModel::updateData(const QVariantMap &newData)
{
    if (m_currentData == newData) {
        return; // 数据未变化
    }
    
    // 延迟更新以避免频繁的UI刷新
    m_pendingData = newData;
    m_updateTimer->start();
}

/**
 * @brief 设置过滤文本
 * @param filterText 过滤条件
 */
void DataTreeModel::setFilter(const QString &filterText)
{
    if (m_filterText == filterText)
        return;
    
    beginResetModel();
    m_filterText = filterText;
    
    // 应用过滤器
    applyFilter(m_rootItem, filterText);
    updateFilteredItemsCount();
    
    endResetModel();
    emit filteredItemsChanged();
}

void DataTreeModel::setLazyLoadingEnabled(bool enabled)
{
    if (m_lazyLoadingEnabled == enabled)
        return;
    
    m_lazyLoadingEnabled = enabled;
    emit lazyLoadingEnabledChanged();
}

void DataTreeModel::setMaxVisibleItems(int maxItems)
{
    if (m_maxVisibleItems == maxItems)
        return;
    
    m_maxVisibleItems = maxItems;
    emit maxVisibleItemsChanged();
}

/**
 * @brief 延迟更新处理
 */
void DataTreeModel::performDelayedUpdate()
{
    beginResetModel();
    
    updateMapIncremental(m_pendingData, m_currentData, m_rootItem);
    m_currentData = m_pendingData;
    
    m_totalItems = calculateTotalItems();
    updateFilteredItemsCount();
    
    endResetModel();
    
    emit totalItemsChanged();
    emit filteredItemsChanged();
}

/**
 * @brief 从QVariantMap创建树形结构
 * @param data 数据映射
 * @param parent 父项目
 * @param depth 当前深度
 */
void DataTreeModel::createTreeFromMap(const QVariantMap &data, TreeItem* parent, int depth)
{
    int itemCount = 0;
    
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        // 懒加载检查
        if (m_lazyLoadingEnabled && itemCount >= m_maxVisibleItems && depth == 0) {
            // 创建"显示更多"项目
            TreeItem* moreItem = new TreeItem;
            moreItem->key = QString("... 还有 %1 项").arg(data.size() - itemCount);
            moreItem->value = QVariant();
            moreItem->valueType = "More";
            moreItem->displayValue = "点击加载更多";
            moreItem->parent = parent;
            moreItem->depth = depth;
            parent->children.append(moreItem);
            break;
        }
        
        TreeItem* item = new TreeItem;
        item->key = it.key();
        item->value = it.value();
        item->valueType = getValueType(it.value());
        item->displayValue = getDisplayValue(it.value());
        item->parent = parent;
        item->depth = depth;
        
        // 递归处理子项目
        if (it.value().typeId() == QMetaType::QVariantMap) {
            QVariantMap childMap = it.value().toMap();
            if (m_lazyLoadingEnabled && childMap.size() > 50) {
                // 大数据量时使用懒加载
                item->isLazyLoaded = false;
                m_lazyDataCache[item] = it.value();
            } else {
                createTreeFromMap(childMap, item, depth + 1);
            }
        } else if (it.value().typeId() == QMetaType::QVariantList) {
            QVariantList list = it.value().toList();
            QVariantMap listMap;
            for (int i = 0; i < list.size(); ++i) {
                listMap.insert(QString::number(i), list.at(i));
            }
            if (m_lazyLoadingEnabled && list.size() > 50) {
                item->isLazyLoaded = false;
                m_lazyDataCache[item] = QVariant(listMap);
            } else {
                createTreeFromMap(listMap, item, depth + 1);
            }
        }
        
        parent->children.append(item);
        itemCount++;
    }
}

/**
 * @brief 递归进行增量更新
 * @param newMap 新数据
 * @param oldMap 旧数据
 * @param parent 父属性
 */
void DataTreeModel::updateMapIncremental(const QVariantMap& newMap, const QVariantMap& oldMap, TreeItem* parent)
{
    // 简化实现：对于大数据量，直接重建可能更高效
    // 清空现有子项
    qDeleteAll(parent->children);
    parent->children.clear();
    
    // 重新创建
    createTreeFromMap(newMap, parent, parent->depth);
}

/**
 * @brief 增量更新树形结构
 * @param newData 新数据
 * @param oldData 旧数据
 * @param parent 父项目
 */
void DataTreeModel::updateTreeIncremental(const QVariantMap &newData, const QVariantMap &oldData, TreeItem* parent)
{
    updateMapIncremental(newData, oldData, parent);
}

/**
 * @brief 应用过滤器
 * @param item 要过滤的项目
 * @param filterText 过滤文本
 * @return 是否匹配过滤条件
 */
bool DataTreeModel::applyFilter(TreeItem* item, const QString &filterText)
{
    if (filterText.isEmpty()) {
        item->isVisible = true;
        for (TreeItem* child : item->children) {
            applyFilter(child, filterText);
        }
        return true;
    }
    
    bool hasVisibleChild = false;
    for (TreeItem* child : item->children) {
        if (applyFilter(child, filterText)) {
            hasVisibleChild = true;
        }
    }
    
    bool matchesFilter = item->key.contains(filterText, Qt::CaseInsensitive) ||
                        item->displayValue.contains(filterText, Qt::CaseInsensitive);
    
    item->isVisible = matchesFilter || hasVisibleChild;
    return item->isVisible;
}

/**
 * @brief 懒加载子项目
 * @param item 要加载的项目
 */
void DataTreeModel::lazyLoadChildren(TreeItem* item)
{
    if (item->isLazyLoaded || !m_lazyDataCache.contains(item)) {
        return;
    }
    
    QVariant data = m_lazyDataCache.value(item);
    if (data.typeId() == QMetaType::QVariantMap) {
        createTreeFromMap(data.toMap(), item, item->depth + 1);
    }
    
    item->isLazyLoaded = true;
    m_lazyDataCache.remove(item);
}

/**
 * @brief 获取显示值，支持更多Qt数据类型的格式化显示
 * @param value 原始值
 * @return 格式化后的显示值
 */
QString DataTreeModel::getDisplayValue(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::QVariantMap:
        // 对于对象类型，不显示项目数量，因为已经在类型列显示了
        return "";
    case QMetaType::QVariantList:
        // 对于数组类型，不显示项目数量，因为已经在类型列显示了
        return "";
    case QMetaType::QStringList: {
        QStringList list = value.toStringList();
        if (list.size() <= 3) {
            return QString("[ %1 ]").arg(list.join(", "));
        } else {
            return QString("[ %1, ... ]").arg(list.mid(0, 3).join(", "));
        }
    }
    case QMetaType::QString:
        return QString("\"%1\"").arg(value.toString());
    case QMetaType::QByteArray:
        return QString("0x%1").arg(QString(value.toByteArray().toHex()));
    
    // Qt几何类型
    case QMetaType::QSize: {
        QSize size = value.toSize();
        return QString("(%1, %2)").arg(size.width()).arg(size.height());
    }
    case QMetaType::QSizeF: {
        QSizeF size = value.toSizeF();
        return QString("(%1, %2)").arg(size.width()).arg(size.height());
    }
    case QMetaType::QRect: {
        QRect rect = value.toRect();
        return QString("(%1, %2, %3, %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
    }
    case QMetaType::QRectF: {
        QRectF rect = value.toRectF();
        return QString("(%1, %2, %3, %4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
    }
    case QMetaType::QPoint: {
        QPoint point = value.toPoint();
        return QString("(%1, %2)").arg(point.x()).arg(point.y());
    }
    case QMetaType::QPointF: {
        QPointF point = value.toPointF();
        return QString("(%1, %2)").arg(point.x()).arg(point.y());
    }
    
    // Qt图形类型
    case QMetaType::QColor: {
        QColor color = value.value<QColor>();
        return QString("RGBA(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    }
    case QMetaType::QFont: {
        QFont font = value.value<QFont>();
        return QString("%1, %2pt").arg(font.family()).arg(font.pointSize());
    }
    case QMetaType::QPixmap: {
        QPixmap pixmap = value.value<QPixmap>();
        return QString("Pixmap %1x%2").arg(pixmap.width()).arg(pixmap.height());
    }
    case QMetaType::QImage: {
        QImage image = value.value<QImage>();
        return QString("Image %1x%2").arg(image.width()).arg(image.height());
    }
    
    // Qt时间类型
    case QMetaType::QDateTime:
        return value.toDateTime().toString("yyyy-MM-dd hh:mm:ss");
    case QMetaType::QDate:
        return value.toDate().toString("yyyy-MM-dd");
    case QMetaType::QTime:
        return value.toTime().toString("hh:mm:ss");
    
    // Qt其他类型
    case QMetaType::QUrl:
        return value.toUrl().toString();
    
    // Qt数学类型
    case QMetaType::QVector2D: {
        QVector2D vec = value.value<QVector2D>();
        return QString("(%1, %2)").arg(vec.x()).arg(vec.y());
    }
    case QMetaType::QVector3D: {
        QVector3D vec = value.value<QVector3D>();
        return QString("(%1, %2, %3)").arg(vec.x()).arg(vec.y()).arg(vec.z());
    }
    case QMetaType::QVector4D: {
        QVector4D vec = value.value<QVector4D>();
        return QString("(%1, %2, %3, %4)").arg(vec.x()).arg(vec.y()).arg(vec.z()).arg(vec.w());
    }
    case QMetaType::QQuaternion: {
        QQuaternion quat = value.value<QQuaternion>();
        return QString("(%1, %2, %3, %4)").arg(quat.scalar()).arg(quat.x()).arg(quat.y()).arg(quat.z());
    }
    
    default:
        return value.toString();
    }
}

/**
 * @brief 获取值类型，支持更多Qt数据类型
 * @param value 值
 * @return 类型字符串
 */
QString DataTreeModel::getValueType(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::QString: return "String";
    case QMetaType::Int: return "Int";
    case QMetaType::Double: return "Double";
    case QMetaType::Float: return "Float";
    case QMetaType::Bool: return "Bool";
    case QMetaType::QVariantMap: return "QVariantMap";
    case QMetaType::QVariantList: return "QVariantList";
    case QMetaType::QStringList: return "QStringList";
    case QMetaType::QByteArray: return "QByteArray";
    
    // Qt几何类型
    case QMetaType::QSize: return "QSize";
    case QMetaType::QSizeF: return "QSizeF";
    case QMetaType::QRect: return "QRect";
    case QMetaType::QRectF: return "QRectF";
    case QMetaType::QPoint: return "QPoint";
    case QMetaType::QPointF: return "QPointF";
    
    // Qt图形类型
    case QMetaType::QColor: return "QColor";
    case QMetaType::QFont: return "QFont";
    case QMetaType::QPixmap: return "QPixmap";
    case QMetaType::QImage: return "QImage";
    case QMetaType::QBrush: return "QBrush";
    case QMetaType::QPen: return "QPen";
    
    // Qt时间类型
    case QMetaType::QDateTime: return "QDateTime";
    case QMetaType::QDate: return "QDate";
    case QMetaType::QTime: return "QTime";
    
    // Qt其他类型
    case QMetaType::QUrl: return "QUrl";
    case QMetaType::QTransform: return "QTransform";
    
    // Qt数学类型
    case QMetaType::QVector2D: return "QVector2D";
    case QMetaType::QVector3D: return "QVector3D";
    case QMetaType::QVector4D: return "QVector4D";
    case QMetaType::QQuaternion: return "QQuaternion";
    
    // 基础数据类型
    case QMetaType::Char: return "Char";
    case QMetaType::UChar: return "UChar";
    case QMetaType::Short: return "Short";
    case QMetaType::UShort: return "UShort";
    case QMetaType::UInt: return "UInt";
    case QMetaType::Long: return "Long";
    case QMetaType::ULong: return "ULong";
    case QMetaType::LongLong: return "LongLong";
    case QMetaType::ULongLong: return "ULongLong";
    
    default: 
        // 尝试获取类型名称
        QString typeName = value.typeName();
        if (!typeName.isEmpty()) {
            return typeName;
        }
        return "Unknown";
    }
}

/**
 * @brief 计算总项目数
 * @param item 起始项目
 * @return 项目总数
 */
int DataTreeModel::calculateTotalItems(TreeItem* item)
{
    if (!item) {
        item = m_rootItem;
    }
    
    int count = item->children.size();
    for (TreeItem* child : item->children) {
        count += calculateTotalItems(child);
    }
    
    return count;
}

/**
 * @brief 更新过滤后的项目数
 */
void DataTreeModel::updateFilteredItemsCount()
{
    m_filteredItems = calculateVisibleItems(m_rootItem);
}

/**
 * @brief 计算可见项目数
 * @param item 起始项目
 * @return 可见项目数
 */
int DataTreeModel::calculateVisibleItems(TreeItem* item)
{
    if (!item) {
        return 0;
    }
    
    int count = 0;
    for (TreeItem* child : item->children) {
        if (child->isVisible) {
            count++;
            count += calculateVisibleItems(child);
        }
    }
    
    return count;
}

DataTreeModel::TreeItem* DataTreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return m_rootItem;
}

/**
 * @brief 优化的属性构建方法，支持懒加载
 * @param map 数据映射
 */
void DataTreeModel::buildPropertiesFromMapOptimized(const QVariantMap& map)
{
    setRootData(map);
}

/**
 * @brief 懒加载版本的属性更新方法
 * @param map 数据映射
 * @param parent 父属性
 * @param readOnly 是否只读
 */
void DataTreeModel::updatePropertiesFromMapLazy(const QVariantMap& map, TreeItem* parent, bool readOnly)
{
    Q_UNUSED(readOnly)
    if (!parent) {
        parent = m_rootItem;
    }
    createTreeFromMap(map, parent, parent->depth);
}

/**
 * @brief 创建简单属性
 * @param key 属性键
 * @param value 属性值
 * @param parent 父属性
 * @param readOnly 是否只读
 */
void DataTreeModel::createSimpleProperty(const QString& key, const QVariant& value, TreeItem* parent, bool readOnly)
{
    Q_UNUSED(readOnly)
    TreeItem* item = new TreeItem;
    item->key = key;
    item->value = value;
    item->valueType = getValueType(value);
    item->displayValue = getDisplayValue(value);
    item->parent = parent;
    item->depth = parent ? parent->depth + 1 : 0;
    
    if (parent) {
        parent->children.append(item);
    }
}