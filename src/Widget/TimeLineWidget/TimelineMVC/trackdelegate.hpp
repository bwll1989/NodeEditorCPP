#ifndef TRACKDELEGATE_HPP
#define TRACKDELEGATE_HPP

#include <QAbstractItemDelegate>
#include <QObject>

#include<QPainter>
#include <QPushButton>
#include <QLayout>
#include <QSpacerItem>
#include "QLabel"
#include "timelinemodel.hpp"
#include "timelinestyle.hpp"
class TrackDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    /**
     * 构造函数
     * @param QObject *parent 父窗口
     */
    explicit TrackDelegate(QObject *parent = nullptr): QAbstractItemDelegate{parent}{};


    // QAbstractItemDelegate interface
public:
    /**
     * 绘制
     * @param QPainter *painter 绘图器
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     */
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override{

        painter->setPen(fillColour);
        painter->setBrush(fillColour.lighter(150));

        if(option.state & QStyle::State_MouseOver)
            painter->setBrush(fillColour);
        painter->drawRect(option.rect);
        painter->save();
        if(option.state & QStyle::State_MouseOver && option.state & QStyle::State_Raised)
            painter->setPen(QPen(fillColour.darker(200),4));
        painter->drawLine(0, option.rect.top(), option.rect.width(), option.rect.top());
        painter->restore();


    };
    /**
     * 大小提示
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     * @return QSize 大小
     */
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        return option.rect.size();
    };
    /**
     * 创建编辑器
     * @param QWidget *parent 父窗口
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     * @return QWidget * 编辑器
     */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        
        QWidget* editor = new QWidget(parent);
        
        QHBoxLayout* layout = new QHBoxLayout(editor);
        layout->setContentsMargins(0,0,0,0);
        // QLabel* title=new QLabel();
        // title->setText("Untitled");
        // title->setStyleSheet("QLabel { background: rgba(255, 255, 255, 0); color: white; border: none; }");
        // title->setAlignment(Qt::AlignCenter);
        // layout->setContentsMargins(5, 5, 5, 5);
        // layout->addWidget(title);

        QLineEdit* lineEdit = new QLineEdit();
        lineEdit->setText(index.data(TimelineRoles::TrackNameRole).toString());
        editor->setToolTip(index.data(TimelineRoles::TrackTypeRole).toString());
        lineEdit->setAlignment(Qt::AlignCenter);
        lineEdit->setStyleSheet("QLineEdit { background: rgba(255, 255, 255, 0); color: white; border: none; }");
        connect(lineEdit, &QLineEdit::textChanged, this, [this, index](const QString& text){
            auto* track = static_cast<TrackModel*>(index.internalPointer());
            track->setName(text);
        });
        layout->addWidget(lineEdit);
        // 移动轨道按钮
        QLabel* moveLabel = new QLabel();
        moveLabel->setPixmap(QPixmap(":/icons/icons/move.png"));
        moveLabel->setFixedSize(20,10);
        moveLabel->setAlignment(Qt::AlignCenter);
        moveLabel->setStyleSheet("QLabel { background: rgba(255, 255, 255, 0); color: white; border: none; }");
        layout->addWidget(moveLabel);
        editor->show();
        editor->setMouseTracking(true);
        return editor;
    };

    // QAbstractItemDelegate interface
public:
    /**
     * 更新编辑器几何形状
     * @param QWidget *editor 编辑器
     * @param const QStyleOptionViewItem &option 选项
     * @param const QModelIndex &index 索引
     */
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        editor->setGeometry(option.rect);
        QRegion mask = QRegion(0,rulerHeight,option.rect.right(),option.rect.bottom()-rulerHeight-toolbarHeight);
        editor->clearMask();
    };

    /**
     * 设置编辑器数据
     * @param QWidget *editor 编辑器
     * @param const QModelIndex &index 索引
     */
    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        // QLabel *lineEdit = editor->findChild<QLabel*>();
        // if (lineEdit) {
        //     QString trackType = index.data(TimelineRoles::TrackTypeRole).toString();
        //     lineEdit->setText(trackType);
        // }
        // // Remove or replace the following line if not applicable
        // QAbstractItemDelegate::setEditorData(editor, index); // Call the correct base class method if needed
    }

};

#endif // TRACKDELEGATE_HPP
