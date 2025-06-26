//
// Created by 吴斌 on 2023/11/21.
//

#ifndef NODEEDITORCPP_CUSTOMGRAPHICSVIEW_H
#define NODEEDITORCPP_CUSTOMGRAPHICSVIEW_H
#include "QtNodes/GraphicsView"
using QtNodes::GraphicsView;
class CustomGraphicsView: public GraphicsView{
    Q_OBJECT
public:
    CustomGraphicsView(QWidget *parent = nullptr);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    /**
     * 移动事件
     */
    void dragMoveEvent(QDragMoveEvent *event) override;
    /**
     * 离开事件
     */
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    /**
     * 放置事件
     */
    void dropEvent(QDropEvent *event) override;
};


#endif //NODEEDITORCPP_CUSTOMGRAPHICSVIEW_H
