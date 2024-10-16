#ifndef TRACKDELEGATE_H
#define TRACKDELEGATE_H

#include <QAbstractItemDelegate>
#include <QObject>
#include "trackdelegate.hpp"
#include<QPainter>
#include <QPushButton>
#include <QLayout>
#include <QSpacerItem>
#include <QLineEdit>
#include "timelinemodel.hpp"
#include "timelinestyle.hpp"
class TrackDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TrackDelegate(QObject *parent = nullptr): QAbstractItemDelegate{parent}{

    };

signals:

    // QAbstractItemDelegate interface
public:
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

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        return option.rect.size();
    };

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        QWidget* editor = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(editor);
        QPushButton* a = new QPushButton();
        QPushButton* b = new QPushButton();
        QLineEdit* title=new QLineEdit();


        title->setText("Untitled");
        title->setStyleSheet("QLineEdit { background: rgba(255, 255, 255, 0); color: white; border: none; }");
        title->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(5, 5, 5, 5);
        a->setMaximumHeight(option.rect.height());
        a->setMaximumWidth(option.rect.height());
        b->setMaximumHeight(option.rect.height()*.9);
        b->setMaximumWidth(option.rect.height()*.9);

        a->setText("M");
        b->setText("S");

        layout->addWidget(title);
        layout->addWidget(a);
        layout->addWidget(b);
        editor->show();
        editor->setMouseTracking(true);
        return editor;
    };

    // QAbstractItemDelegate interface
public:
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        editor->setGeometry(option.rect);
        QRegion mask = QRegion(0,rulerHeight,option.rect.right(),option.rect.bottom()-rulerHeight);
        editor->clearMask();
    };

private:
    QWidget* m_editor;

};

#endif // TRACKDELEGATE_H
