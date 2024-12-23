//
// Created by bwll1 on 2024/5/30.
//

#ifndef NODEEDITORCPP_CLIPVIEW_H
#define NODEEDITORCPP_CLIPVIEW_H

#include <QAbstractItemView>
#include "clipdelegate.hpp" // 假设 ClipDelegate 类已经定义
#include "clipmodel.hpp"    // 假设 ClipModel 类已经定义

class clipview : public QAbstractItemView {
    Q_OBJECT
public:
    explicit clipview(QWidget *parent = nullptr): QAbstractItemView{parent}{
    }

    void setModel(QAbstractItemModel *model) override {
        QAbstractItemView::setModel(model);

        // 设置代理控件为 ClipDelegate
        if (model) {
            clipDelegate = new ClipDelegate(this);
            setItemDelegate(clipDelegate);
        }


    }
private:
    ClipDelegate *clipDelegate;
};


#endif //NODEEDITORCPP_CLIPVIEW_H
