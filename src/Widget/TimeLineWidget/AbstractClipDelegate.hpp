#ifndef ABSTRACTCLIPDELEGATE_HPP
#define ABSTRACTCLIPDELEGATE_HPP

#include <QAbstractItemDelegate>
#include <QPainter>
#include "timelinetypes.h"
#include "timelinestyle.hpp"
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QAbstractItemView>

class AbstractClipDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit AbstractClipDelegate(QObject *parent = nullptr) : QAbstractItemDelegate(parent) {}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override=0;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        return QSize(option.rect.width(), trackHeight);
    }

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override=0;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        // 设置编辑器的基本几何形状
        editor->setGeometry(option.rect.adjusted(clipoffset, clipoffset+clipround, -clipoffset, -clipoffset-clipround));
        
        // 获取父视图
        QAbstractItemView* view = qobject_cast<QAbstractItemView*>(editor->parent());
        if (view) {
            // 创建一个视口区域的遮罩
            QRegion mask = QRegion(view->viewport()->rect());
            
            // 应用遮罩到编辑器
            if (!mask.isEmpty()) {
                editor->setMask(mask.translated(-editor->pos()));
            }
            
            // 确保编辑器不会超出视口边界
            QRect editorRect = editor->geometry();
            QRect viewportRect = view->viewport()->rect();
            if (!viewportRect.contains(editorRect)) {
                editor->setGeometry(editorRect.intersected(viewportRect));
            }
        }
    }

    

protected:
    // Helper function to draw the basic clip rectangle with the given color
    void drawClipBackground(QPainter *painter, const QStyleOptionViewItem &option, const QColor &color) const {
        painter->save();  // 保存状态
        painter->setBrush(QBrush(color));
        painter->drawRect(option.rect.adjusted(1,1,-2,-2));
        painter->restore();  // 恢复状态
    }

    // Helper function to draw the clip title
    void drawClipTitle(QPainter *painter, const QStyleOptionViewItem &option, const QString &type, const QString &title) const {
        painter->save();  // 保存状态
        painter->setPen(Qt::white);
        painter->drawText(option.rect, Qt::AlignCenter, type + "\n" + title);
        painter->restore();  // 恢复状态
    }


};

#endif // ABSTRACTCLIPDELEGATE_HPP
