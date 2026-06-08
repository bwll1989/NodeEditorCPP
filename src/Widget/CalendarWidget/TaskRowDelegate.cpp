#include "TaskRowDelegate.h"

#include "ScheduledTaskModel.hpp"

#include <QApplication>
#include <QLineEdit>
#include <QPainter>
#include <QStyle>

namespace {

constexpr int kRowHeight = 32;
constexpr int kHMargin = 10;

} // namespace

TaskRowDelegate::TaskRowDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void TaskRowDelegate::paint(QPainter* painter,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index) const
{
    if (!index.isValid()) {
        return;
    }

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    if (opt.state.testFlag(QStyle::State_Editing)) {
        return;
    }

    const QRect rect = opt.rect.adjusted(kHMargin, 0, -kHMargin, 0);
    painter->save();
    painter->setClipRect(rect);

    const QPalette& palette = opt.palette;
    const bool selected = opt.state.testFlag(QStyle::State_Selected);
    const QColor primary = selected ? palette.color(QPalette::HighlightedText)
                                    : palette.color(QPalette::Text);
    const QColor secondary = selected ? primary
                                      : palette.color(QPalette::PlaceholderText);

    const QString remark = index.data(ScheduledTaskModel::RoleRemarks).toString();
    const QString displayText = remark.isEmpty() ? tr("(点击编辑备注)") : remark;

    painter->setFont(opt.font);
    painter->setPen(remark.isEmpty() ? secondary : primary);
    painter->drawText(rect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                      opt.fontMetrics.elidedText(displayText, Qt::ElideRight, rect.width()));

    painter->restore();
}

QSize TaskRowDelegate::sizeHint(const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(120, kRowHeight);
}

QWidget* TaskRowDelegate::createEditor(QWidget* parent,
                                       const QStyleOptionViewItem& /*option*/,
                                       const QModelIndex& /*index*/) const
{
    auto* edit = new QLineEdit(parent);
    edit->setFrame(false);
    edit->setPlaceholderText(tr("备注（例如：早上开机）"));
    edit->setMinimumHeight(kRowHeight - 4);
    return edit;
}

void TaskRowDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* edit = qobject_cast<QLineEdit*>(editor);
    if (!edit || !index.isValid()) {
        return;
    }
    edit->setText(index.data(ScheduledTaskModel::RoleRemarks).toString());
    edit->selectAll();
}

void TaskRowDelegate::setModelData(QWidget* editor,
                                   QAbstractItemModel* model,
                                   const QModelIndex& index) const
{
    auto* edit = qobject_cast<QLineEdit*>(editor);
    if (!edit || !index.isValid() || !model) {
        return;
    }
    model->setData(index, edit->text(), ScheduledTaskModel::RoleRemarks);
}

void TaskRowDelegate::updateEditorGeometry(QWidget* editor,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& /*index*/) const
{
    editor->setGeometry(option.rect.adjusted(kHMargin, 2, -kHMargin, -2));
}
