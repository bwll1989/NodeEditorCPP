#ifndef ABSTRACTCLIPDELEGATE_HPP
#define ABSTRACTCLIPDELEGATE_HPP

#include <QAbstractItemDelegate>
#include <QPainter>
#include "timelinetypes.h"
#include "Widget/TimeLineWidget/TimelineMVC/timelinestyle.hpp"
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QAbstractItemView>

class AbstractClipDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    /**
     * 构造函数
     * @param QObject *parent 父窗口
     */
    explicit AbstractClipDelegate(QObject *parent = nullptr) : QAbstractItemDelegate(parent) {}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override=0;
    /**
     * 大小提示
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     * @return QSize 大小
     */
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        return QSize(option.rect.width(), trackHeight);
    }
    /**
     * 创建编辑器
     * @param QWidget *parent 父窗口
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     * @return QWidget * 编辑器
     */
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override=0;
    /**
     * 更新编辑器几何形状
     * @param QWidget *editor 编辑器
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     */
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
    /**
     * 绘制剪辑背景
     * @param QPainter *painter 绘图器
     * @param const QStyleOptionViewItem &option 选项
     * @param const QColor &color 颜色
     */
    void drawClipBackground(QPainter *painter, const QStyleOptionViewItem &option, const QColor &color) const {
        painter->save();  // 保存状态
        painter->setBrush(QBrush(color));
        painter->drawRect(option.rect.adjusted(1,1,-2,-2));
        painter->restore();  // 恢复状态
    }

    /**
     * 绘制剪辑标题
     * @param QPainter *painter 绘图器
     * @param const QStyleOptionViewItem &option 选项
     * @param const QString &type 类型
     * @param const QString &title 标题
     */
    void drawClipTitle(QPainter *painter, const QStyleOptionViewItem &option, const QString &type, const QString &title) const {
        painter->save();  // 保存状态
        painter->setPen(Qt::white);
        painter->drawText(option.rect, Qt::AlignCenter, type + "\n" + title);
        painter->restore();  // 恢复状态
    }

    
};

#endif // ABSTRACTCLIPDELEGATE_HPP
