#pragma once

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTableView>

namespace Gui
{
inline constexpr int kCompactTableRowHeight = 26;

inline void applyCompactTableRows(QTableView *tableView, int rowHeight = kCompactTableRowHeight)
{
    if (!tableView) {
        return;
    }

    QHeaderView *header = tableView->verticalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);
    header->setDefaultSectionSize(rowHeight);
    header->setMinimumSectionSize(rowHeight);

    if (QAbstractItemModel *model = tableView->model()) {
        for (int row = 0; row < model->rowCount(); ++row) {
            tableView->setRowHeight(row, rowHeight);
        }
    }
}

inline void setupCompactTableView(QTableView *tableView, int rowHeight = kCompactTableRowHeight)
{
    if (!tableView) {
        return;
    }

    tableView->setWordWrap(false);
    applyCompactTableRows(tableView, rowHeight);
}

class CompactTableTextDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.features.setFlag(QStyleOptionViewItem::WrapText, false);
        opt.textElideMode = Qt::ElideRight;
        QStyledItemDelegate::paint(painter, opt, index);
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        return QSize(QStyledItemDelegate::sizeHint(option, index).width(), kCompactTableRowHeight);
    }
};

} // namespace Gui
