#include "QmlDataBrowser.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

/**
 * @brief QmlDataBrowser构造函数
 * @param parent 父窗口部件
 */
QmlDataBrowser::QmlDataBrowser(QWidget *parent)
    : QQuickWidget(parent)
    , m_dataModel(new DataTreeModel(this))
{
    // 设置最小尺寸
    setMinimumSize(300, 200);
    
    // 直接将数据模型实例注册到QML上下文，不注册类型
    rootContext()->setContextProperty("dataModel", m_dataModel);
    
    // 设置QML源文件
    setSource(QUrl("qrc:/qml/qml/DataBrowser.qml"));
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 连接信号 - 延迟连接确保QML对象已加载
    connect(this, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status) {
        if (status == QQuickWidget::Ready) {
            connectQmlSignals();
        }
    });
    
    // 如果已经准备好，立即连接
    if (status() == QQuickWidget::Ready) {
        connectQmlSignals();
    }
    
    // 连接数据模型信号以更新统计信息
    connect(m_dataModel, &DataTreeModel::totalItemsChanged, this, [this]() {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "updateStatistics");
        }
    });
    
    connect(m_dataModel, &DataTreeModel::filteredItemsChanged, this, [this]() {
        QQuickItem* root = rootObject();
        if (root) {
            QMetaObject::invokeMethod(root, "updateStatistics");
        }
    });
}

/**
 * @brief 连接QML信号到C++槽
 */
void QmlDataBrowser::connectQmlSignals()
{
    QQuickItem* root = rootObject();
    if (root) {
        // 使用QMetaObject::invokeMethod来调用QML方法
        connect(root, SIGNAL(itemClicked(QVariant)), 
                this, SLOT(onItemClicked(QVariant)));
        connect(root, SIGNAL(itemDoubleClicked(QVariant)), 
                this, SLOT(onItemDoubleClicked(QVariant)));
        connect(root, SIGNAL(itemRightClicked(QVariant)), 
                this, SLOT(onItemRightClicked(QVariant)));
    }
}

/**
 * @brief 构建属性从映射
 * @param map 数据映射
 */
void QmlDataBrowser::buildPropertiesFromMap(const QVariantMap& map)
{
    m_dataModel->setRootData(map);
}

/**
 * @brief 增量更新属性
 * @param newMap 新的数据映射
 */
void QmlDataBrowser::updatePropertiesIncremental(const QVariantMap& newMap)
{
    m_dataModel->updateData(newMap);
}

/**
 * @brief 设置懒加载是否启用
 * @param enabled 是否启用懒加载
 */
void QmlDataBrowser::setLazyLoadingEnabled(bool enabled)
{
    m_dataModel->setLazyLoadingEnabled(enabled);
    
    // 更新QML中的属性
    QQuickItem* root = rootObject();
    if (root) {
        root->setProperty("lazyLoadingEnabled", enabled);
    }
}

/**
 * @brief 设置最大可见项目数
 * @param maxItems 最大可见项目数
 */
void QmlDataBrowser::setMaxVisibleItems(int maxItems)
{
    m_dataModel->setMaxVisibleItems(maxItems);
    
    // 更新QML中的属性
    QQuickItem* root = rootObject();
    if (root) {
        root->setProperty("maxVisibleItems", maxItems);
    }
}

/**
 * @brief 处理项目点击事件
 * @param item 被点击的项目数据
 */
void QmlDataBrowser::onItemClicked(const QVariant& item)
{
    // 处理项目点击事件
    QVariantMap itemMap = item.toMap();
    QString key = itemMap.value("key").toString();
    QVariant value = itemMap.value("value");
    
    emit nodeItemValueChanged(key, value);
}

/**
 * @brief 处理项目双击事件
 * @param item 被双击的项目数据
 */
void QmlDataBrowser::onItemDoubleClicked(const QVariant& item)
{
    // 处理双击事件，可以用于编辑
    onItemClicked(item);
}

/**
 * @brief 处理项目右键点击事件
 * @param item 被右键点击的项目数据
 */
void QmlDataBrowser::onItemRightClicked(const QVariant& item)
{
    // 处理右键点击事件，可以显示上下文菜单
    QVariantMap itemMap = item.toMap();
    QString key = itemMap.value("key").toString();
    QString valueType = itemMap.value("valueType").toString();
    
    // 这里可以添加右键菜单逻辑
    // 例如：复制值、编辑、删除等操作
}

