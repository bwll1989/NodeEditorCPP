#include "DataBrowserWidget.h"

#include <QTreeView>
#include <QVBoxLayout>
#include <QHeaderView>

/**
 * @brief DataBrowserWidget 构造函数
 * @param parent 父窗口部件
 */
DataBrowserWidget::DataBrowserWidget(QWidget *parent)
    : QWidget(parent)
    , m_dataModel(new DataTreeModel(this))
    , m_treeView(new QTreeView(this))
{
    // 设置最小尺寸（对齐原 QmlDataBrowser）
    setMinimumSize(300, 200);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);

    // 三列表头：属性 / 类型 / 值（列宽 0.4/0.2/0.4，对齐原 QML 布局）
    m_treeView->setModel(m_dataModel);
    QHeaderView *header = m_treeView->header();
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->resizeSection(0, 240);
    header->resizeSection(1, 120);
    header->setStretchLastSection(true);

    // 视觉与性能：斑马纹 + 统一行高（高频刷新降耗）+ 根级装饰线
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setFrameShape(QFrame::NoFrame);

    // 深色主题（2026-11-27 用户反馈：默认斑马纹 AlternateBase 白行太亮刺眼）
    // 两档深灰交替，柔和区分行；列头/悬停/选中配套深色
    m_treeView->setStyleSheet(QStringLiteral(
        "QTreeView {"
        "  background-color: #232527;"
        "  alternate-background-color: #2d3034;"
        "  color: #d8dcdc;"
        "  outline: 0;"
        "  border: none;"
        "}"
        "QTreeView::item { min-height: 21px; }"
        "QTreeView::item:hover { background-color: #34383d; }"
        "QTreeView::item:selected { background-color: #3a5f8a; color: #ffffff; }"
        "QHeaderView::section {"
        "  background-color: #191a1c;"
        "  color: #b8bcc0;"
        "  padding: 4px 6px;"
        "  border: none;"
        "  border-right: 1px solid #2a2c2e;"
        "  border-bottom: 1px solid #2a2c2e;"
        "}"
    ));

    // 模型重建（setRootData / performDelayedUpdate / setFilter）后自动展开全部，
    // 对齐 QML 版 autoExpandAll + expandAll 语义
    connect(m_dataModel, &QAbstractItemModel::modelReset, this, [this]() {
        m_treeView->expandAll();
    });

    // 点击/双击 → 对齐 QML 版 itemClicked/itemDoubleClicked → nodeItemValueChanged
    connect(m_treeView, &QTreeView::clicked, this, [this](const QModelIndex &index) {
        if (!index.isValid())
            return;
        emit nodeItemValueChanged(index.siblingAtColumn(0).data(Qt::UserRole).toString(),
                                  index.siblingAtColumn(0).data(Qt::UserRole + 1));
    });
    connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        if (!index.isValid())
            return;
        emit nodeItemValueChanged(index.siblingAtColumn(0).data(Qt::UserRole).toString(),
                                  index.siblingAtColumn(0).data(Qt::UserRole + 1));
    });
    // 右键：QML 版为空实现（预留上下文菜单），此处保持行为一致不额外处理
}

/**
 * @brief 构建属性从映射
 * @param map 数据映射
 */
void DataBrowserWidget::buildPropertiesFromMap(const QVariantMap& map)
{
    m_dataModel->setRootData(map);
}

/**
 * @brief 增量更新属性
 * @param newMap 新的数据映射
 */
void DataBrowserWidget::updatePropertiesIncremental(const QVariantMap& newMap)
{
    m_dataModel->updateData(newMap);
}

/**
 * @brief 设置懒加载是否启用
 * @param enabled 是否启用懒加载
 */
void DataBrowserWidget::setLazyLoadingEnabled(bool enabled)
{
    m_dataModel->setLazyLoadingEnabled(enabled);
}

/**
 * @brief 设置最大可见项目数
 * @param maxItems 最大可见项目数
 */
void DataBrowserWidget::setMaxVisibleItems(int maxItems)
{
    m_dataModel->setMaxVisibleItems(maxItems);
}
