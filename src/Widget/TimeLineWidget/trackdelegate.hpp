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
    explicit TrackDelegate(QObject *parent = nullptr): QAbstractItemDelegate{parent}{};

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
        layout->setContentsMargins(0,0,0,0);
        QLabel* title=new QLabel();
        title->setText("Untitled");
        title->setStyleSheet("QLabel { background: rgba(255, 255, 255, 0); color: white; border: none; }");
        title->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->addWidget(title);


        QPushButton* muteButton = new QPushButton("M");
        muteButton->setFixedSize((trackHeight-10)/2,(trackHeight-10)/2);
        QPushButton* soloButton = new QPushButton("S");
        soloButton->setFixedSize((trackHeight-10)/2,(trackHeight-10)/2);

        layout->addWidget(muteButton);
        
        layout->addWidget(soloButton);
        editor->show();
        editor->setMouseTracking(true);
        return editor;
    };

    // QAbstractItemDelegate interface
public:
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        editor->setGeometry(option.rect);
        QRegion mask = QRegion(0,rulerHeight,option.rect.right(),option.rect.bottom()-rulerHeight-toolbarHeight);
        editor->clearMask();
    };

    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        QLabel *lineEdit = editor->findChild<QLabel*>();
        if (lineEdit) {
            QString trackType = index.data(TimelineRoles::TrackTypeRole).toString();
            lineEdit->setText(trackType);
        }
        // Remove or replace the following line if not applicable
        QAbstractItemDelegate::setEditorData(editor, index); // Call the correct base class method if needed
    }

};

#endif // TRACKDELEGATE_HPP
