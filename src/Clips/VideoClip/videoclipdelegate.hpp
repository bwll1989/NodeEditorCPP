#ifndef VIDEOCLIPDELEGATE_H
#define VIDEOCLIPDELEGATE_H

#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipDelegate.hpp"
#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include "videoclipmodel.hpp"

class VideoClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit VideoClipDelegate(QObject *parent = nullptr) 
        : AbstractClipDelegate(parent)
        , m_model(nullptr)  // 在构造函数中初始化
        , m_xSpinBox(new QSpinBox())
        , m_ySpinBox(new QSpinBox())
        , m_sizeXSpinBox(new QSpinBox())
        , m_sizeYSpinBox(new QSpinBox())
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {}
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        return m_model->propertyWidget();
    }
 
private:
    VideoClipModel* m_model;  // 使用 m_ 前缀表示成员变量
    
};

#endif // VIDEOCLIPDELEGATE_H 