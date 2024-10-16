//
// Created by 吴斌 on 2023/11/21.
//

#include <QMimeData>
#include "CustomGraphicsView.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>

CustomGraphicsView::CustomGraphicsView(QWidget *parent) {
//    setAcceptDrops(true);
}

void CustomGraphicsView::dragEnterEvent(QDragEnterEvent *event) {
    event->ignore();

}


