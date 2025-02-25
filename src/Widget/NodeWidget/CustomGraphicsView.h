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
    /**
     * 拖入事件
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

};


#endif //NODEEDITORCPP_CUSTOMGRAPHICSVIEW_H
