//
// Created by WuBin on 2025/10/15.
//

#pragma once
#include <QStyledItemDelegate>
#include <QPainter>

class TaskItemDelegate : public QStyledItemDelegate {
public:
    explicit TaskItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    /**
     * @brief 创建持久编辑器（TaskItemWidget）
     */
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;

    /**
     * @brief 将模型数据填充到编辑器
     */
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;

    /**
     * @brief 将编辑器数据写回模型
     */
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    /**
     * @brief 编辑器几何更新
     */
    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;

    /**
     * @brief 返回每一行的推荐尺寸（用于防止编辑器被挤压）
     * 根据模型数据（once/loop）动态计算较为合理的高度
     */
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    /**
     * @brief 自定义绘制：不绘制文本内容，避免与持久编辑器叠加
     * 保留背景和选中高亮
     */
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};


