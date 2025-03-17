#ifndef TRIGGERCLIPDELEGATE_H
#define TRIGGERCLIPDELEGATE_H

#include "TimeLineWidget/TimelineAbstract/AbstractClipDelegate.hpp"
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
class TriggerClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit TriggerClipDelegate(QObject *parent = nullptr) : AbstractClipDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();  // 保存状态
        painter->setRenderHint(QPainter::Antialiasing);
        
        // 计算矩形区域，使其比轨道高度小一些
        QRect rect = option.rect.adjusted(1, trackHeight/3, 30, -trackHeight/3);
        
        // 设置画笔和画刷
        if (option.state & QStyle::State_Selected) {
            painter->setBrush(ClipSelectedColor);
        } else if (option.state & QStyle::State_MouseOver) {
            painter->setBrush(ClipHoverColor);
        } else {
            painter->setBrush(ClipColor);
        }
        
        // 设置细边框
        painter->setPen(QPen(ClipBorderColour, 1));

        // 绘制左侧竖线
        painter->setPen(QPen(ClipBorderColour, 4));  // 稍微粗一点的线
        painter->drawLine(option.rect.left() + 2, option.rect.top(), 
                         option.rect.left() + 2, option.rect.bottom());

        // 设置回细边框
        painter->setPen(QPen(ClipBorderColour, 1));
        
        // 绘制矩形
        painter->drawRect(rect);

        // 绘制文本
        painter->setPen(Qt::white);
        QFont font = painter->font();
        font.setPointSize(8);  // 设置较小的字体大小
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, "Trigger");
        painter->restore();  // 恢复状态
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QWidget* editor = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(editor);
        layout->setContentsMargins(0,0,0,0);
        // Add trigger configuration widgets
        layout->addWidget(new QLabel("Trigger"));
        // Set semi-transparent background
        QString bgColorWithAlpha = QString("background-color: rgba(%1, %2, %3, 180);")
            .arg(ClipColor.red())
            .arg(ClipColor.green())
            .arg(ClipColor.blue());
        editor->setStyleSheet(bgColorWithAlpha);
        editor->setAttribute(Qt::WA_TranslucentBackground);
        editor->setAutoFillBackground(true);

        return editor;
    }
};

#endif // TRIGGERCLIPDELEGATE_H 