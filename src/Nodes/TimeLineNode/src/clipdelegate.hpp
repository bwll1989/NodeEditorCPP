#ifndef CLIPDELEGATE_H
#define CLIPDELEGATE_H

#include <QAbstractItemDelegate>
#include <QPainter>
#include "timelinemodel.hpp"
#include "timelinestyle.hpp"

#include "QHBoxLayout"
#include "QPushButton"
#include "QLabel"

class ClipDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit ClipDelegate(QObject *parent = nullptr):QAbstractItemDelegate{parent}
    {

    }

    ~ClipDelegate() = default;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        if(option.state & QStyle::State_MouseOver)
            clipColour = clipColour.darker(150);

        switch (index.data(TimelineRoles::ClipTypeRole).value<MediaType>()) {
            case MediaType::VIDEO:
//            text = "Video";
                clipColour = videoColour;
                break;
            case MediaType::AUDIO:
//            text = "Audio";
                clipColour = audioColour;
                break;

            case MediaType::CONTROL:
//            text = "Control";
                clipColour = controlColour;
                break;
        }
        index.parent();
        painter->save();
        painter->setClipRect(option.rect);
        painter->setClipping(true);

        //not a clip
        if(index.parent()==QModelIndex()){
            return;
        }
        int track = index.parent().row();
        painter->save();
        if(option.state & QStyle::State_Selected){

            int penWidth = 4;
            painter->setPen(QPen(Qt::white,penWidth));
            painter->setBrush(QBrush(clipColour));
            painter->drawRect(option.rect.left()+penWidth/2,option.rect.top()+penWidth/2,
                              option.rect.width()-penWidth,option.rect.height()-penWidth);

        }else {
            painter->setPen(clipColour.darker(300));
            if (option.state & QStyle::State_MouseOver)
                painter->setBrush(QBrush(clipColour.darker(150)));
            else
                painter->setBrush(QBrush(clipColour));
            painter->drawRect(option.rect);
        }
        painter->restore();

        // 显示时间或标题/
        QString text;
        if(!drawtime){
            text=index.data(TimelineRoles::ClipTitle).value<QString>();
        }else{
            int length=index.data(TimelineRoles::ClipLengthRole).value<int>();

            text=FramesToTimeString(length,fps);
            qDebug()<<length;
        }
        QRect   textRect = painter->fontMetrics().boundingRect(text);
        int textOffset = textRect.height();
        textRect.translate(0,textOffset);
        if(option.rect.left()<0){
            textRect.translate(0,option.rect.top());
        }else{
            textRect.translate(option.rect.topLeft());
        }
        painter->drawText(textRect, text);
        painter->setPen(Qt::red);
        //painter->drawRect(textRect);
        painter->restore();

        QPoint points[6] = {
                option.rect.topLeft(),
                QPoint(option.rect.left() + 5,option.rect.top()),
                QPoint(option.rect.left(),option.rect.top()+5),
                QPoint(option.rect.topRight().x()+1,option.rect.topRight().y()),
                QPoint(option.rect.right()+1 - 5,option.rect.top()),
                QPoint(option.rect.right()+1,option.rect.top()+5),
        };

        if(option.state &~ QStyle::State_Selected){
            painter->save();
            painter->setBrush(cornerColour);
            painter->setPen(Qt::NoPen);
            painter->drawConvexPolygon(points, 3);
            painter->drawConvexPolygon(points+3, 3);
            painter->restore();
        }

    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        return option.rect.size();
    }

    QWidget *createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const override{
        QPushButton *editor = new QPushButton(parent);
        editor->setText("test");
        qDebug()<<"test";
        return editor;
    }

    void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option,const QModelIndex &index) const override{
        editor->setGeometry(option.rect);
        QRegion mask = QRegion(0,rulerHeight,option.rect.right(),option.rect.bottom()-rulerHeight);
        editor->clearMask();
    }



private:
    QWidget* m_editor;
};

#endif // CLIPDELEGATE_H
