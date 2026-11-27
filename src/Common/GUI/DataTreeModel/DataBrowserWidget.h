#pragma once

#include <QWidget>
#include <QVariantMap>

#include "DataTreeModel.h"

class QTreeView;

/**
 * @brief 纯 QWidget 数据浏览器（QTreeView 实现），提供与 QPropertyBrowser 兼容的接口
 *
 * 【2026-11-26 QML→QWidget 迁移】替代 QmlDataBrowser（QQuickWidget + DataBrowser.qml）：
 * - 同一 DataTreeModel（QAbstractItemModel）直接喂给 QTreeView，数据逻辑零改动；
 * - 三列显示（属性 40% / 类型 20% / 值 40%），对齐原 QML 布局；
 * - 模型 reset 后自动展开全部（对齐 QML 的 autoExpandAll/expandAll）；
 * - uniformRowHeights + 仅可见行渲染，高频增量刷新资源占用低于 QQuickWidget 路径。
 */
class DataBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataBrowserWidget(QWidget *parent = nullptr);

    /**
     * @brief 构建属性从映射（全量重建）
     * @param map 数据映射
     */
    void buildPropertiesFromMap(const QVariantMap& map);

    /**
     * @brief 增量更新属性（内部经 16ms 延迟合并高频刷新）
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

private:
    DataTreeModel* m_dataModel;
    QTreeView* m_treeView;
};
