#include "BaseTrackDelegate.h"
#include <QTimer>

BaseTrackDelegate::BaseTrackDelegate(QObject *parent): QAbstractItemDelegate{parent}{};

void BaseTrackDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {};

QSize BaseTrackDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    return option.rect.size();
};


QWidget *BaseTrackDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    /**
     * @brief 创建轨道行持久编辑器
     * @details
     * 重新导入/轨道移动后“显示的 TrackName 错误”的常见原因是：
     * - 旧代码在 textChanged 的 lambda 中捕获了 QModelIndex（临时索引）。
     * - 当行被 move/reset 后，该 index 可能已指向错误的行或失效，导致写回/显示错乱。
     *
     * 解决方案：
     * - 使用 QPersistentModelIndex 作为长期索引引用，能够在 beginMoveRows 等操作后自动跟随。
     */

    QWidget* editor = new QWidget(parent);

    QHBoxLayout* layout = new QHBoxLayout(editor);
    layout->setContentsMargins(0,0,0,0);

    QLineEdit* lineEdit = new QLineEdit();
    lineEdit->setText(index.data(TimelineRoles::TrackNameRole).toString());
    editor->setToolTip(index.data(TimelineRoles::TrackTypeRole).toString());
    lineEdit->setAlignment(Qt::AlignCenter);

    const QPersistentModelIndex pIndex(index);
    connect(lineEdit, &QLineEdit::textChanged, this, [pIndex](const QString& text){
        /**
         * @brief 轨道名编辑回写模型
         * @details
         * - 通过 QPersistentModelIndex 确保行移动/导入重载后仍写到正确轨道。
         * - 若索引失效（例如 model reset），则忽略本次回写。
         */
        if (!pIndex.isValid()) {
            return;
        }
        QAbstractItemModel* m = const_cast<QAbstractItemModel*>(pIndex.model());
        if (!m) {
            return;
        }
        if (m->data(pIndex, TimelineRoles::TrackNameRole).toString() == text) {
            return;
        }
        m->setData(pIndex, text, TimelineRoles::TrackNameRole);
    });

    layout->addWidget(lineEdit);
    editor->setFocusProxy(lineEdit);
    QTimer::singleShot(0, lineEdit, [lineEdit]() {
        lineEdit->setFocus();
        lineEdit->selectAll();
    });

    auto* self = const_cast<BaseTrackDelegate*>(this);
    connect(lineEdit, &QLineEdit::editingFinished, editor, [self, editor]() {
        if (editor->property("qtl_closing").toBool())
            return;
        editor->setProperty("qtl_closing", true);
        emit self->closeEditor(editor, QAbstractItemDelegate::NoHint);
    });

    // 移动轨道按钮
    QLabel* moveLabel = new QLabel();
    moveLabel->setFixedSize(20,20);
    moveLabel->setPixmap(QPixmap(":/icons/icons/move.png").scaled(moveLabel->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    moveLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(moveLabel);

    editor->show();
    editor->setMouseTracking(true);
    return editor;
};


void BaseTrackDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {

    // 编辑器对齐轨道名文本区（行内顶部 trackHeight，2026-11-06 用户定案
    // "三行顶对齐"）：固定高 trackHeight−2（上下各 1 与原 adjusted(5,1,0,-1)
    // 逐像素一致，零回归）——行高恒定（2026-11-15 折叠退役），编辑器与轨道
    // 名/grip 同位。右缘不动——编辑器自带 moveLabel 把手继续覆盖 paint
    // grip 位置。
    editor->setGeometry(option.rect.left() + 5, option.rect.top() + 1,
                        option.rect.width() - 5, trackHeight - 2);
    editor->clearMask();
};


void BaseTrackDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    /**
     * @brief 将模型数据同步到编辑器
     * @details
     * TrackListView 使用 openPersistentEditor 创建持久编辑器。
     * - 当模型在 load() 里 setData(TrackNameRole) 更新轨道名时，若 delegate 不实现 setEditorData，
     *   编辑器会一直显示“创建时的旧文本”，导致导入后显示错误。
     */
    if (!editor) {
        return;
    }

    QLineEdit* lineEdit = editor->findChild<QLineEdit*>();
    if (!lineEdit) {
        return;
    }

    const QString newText = index.data(TimelineRoles::TrackNameRole).toString();
    if (lineEdit->text() == newText) {
        return;
    }

    const QSignalBlocker blocker(lineEdit);
    lineEdit->setText(newText);
};
