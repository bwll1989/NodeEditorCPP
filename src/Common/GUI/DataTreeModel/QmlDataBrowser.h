#pragma once

#include <QVariantMap>
#include <QtQuickWidgets/QQuickWidget>

#include "DataTreeModel.h"

/**
 * @brief QML数据浏览器包装器，提供与QPropertyBrowser兼容的接口
 */
class QmlDataBrowser : public QQuickWidget
{
    Q_OBJECT

public:
    explicit QmlDataBrowser(QWidget *parent = nullptr);
    
    /**
     * @brief 构建属性从映射
     * @param map 数据映射
     */
    void buildPropertiesFromMap(const QVariantMap& map);
    
    /**
     * @brief 增量更新属性
     * @param newMap 新的数据映射
     */
    void updatePropertiesIncremental(const QVariantMap& newMap);
    
    /**
     * @brief 设置懒加载启用状态
     * @param enabled 是否启用
     */
    void setLazyLoadingEnabled(bool enabled);
    
    /**
     * @brief 设置最大可见项目数
     * @param maxItems 最大项目数
     */
    void setMaxVisibleItems(int maxItems);

signals:
    void nodeItemValueChanged(const QString& propertyName, const QVariant& newValue);

private slots:
    void onItemClicked(const QVariant& item);
    void onItemDoubleClicked(const QVariant& item);
    void onItemRightClicked(const QVariant& item);

private:
    /**
     * @brief 连接QML信号到C++槽
     */
    void connectQmlSignals();
    
    DataTreeModel* m_dataModel;
};
