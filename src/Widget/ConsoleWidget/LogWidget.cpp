#include "LogWidget.h"
#include <QHeaderView>
#include <QContextMenuEvent>

LogWidget::LogWidget() {
//    setColumnCount(3);
//    setHorizontalHeaderLabels({"Time", "Level", "Message"});
//    setAutoScroll(true);
    setColumnCount(4);  // 假设有5列：时间、级别、文件名、行号、消息
    QStringList headers = {"Timestamp", "Level", "File", "Message"};
    setHorizontalHeaderLabels(headers);
    horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
    setEditTriggers(QAbstractItemView::DoubleClicked);
    
    // 设置右键菜单策略
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void LogWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    
    // 创建清除日志的动作
    QAction *clearAction = menu.addAction(QIcon(":/icons/icons/clear.png"), "Clear Log");
    connect(clearAction, &QAction::triggered, this, &LogWidget::clearTableWidget);
    
    // 在鼠标位置显示菜单
    menu.exec(event->globalPos());
}

void LogWidget::clearTableWidget() {
    clearContents();  // 仅清空内容，不清空表头
    setRowCount(0);   // 清空所有行
}
