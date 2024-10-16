#include "LogWidget.h"

LogWidget::LogWidget() {
//    setColumnCount(3);
//    setHorizontalHeaderLabels({"Time", "Level", "Message"});
//    setAutoScroll(true);
    setColumnCount(4);  // 假设有5列：时间、级别、文件名、行号、消息
    QStringList headers = {"Timestamp", "Level", "File", "Message"};
    setHorizontalHeaderLabels(headers);
    horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
    setEditTriggers(QAbstractItemView::DoubleClicked);
}

void LogWidget::clearTableWidget() {

        clearContents();  // 仅清空内容，不清空表头
        setRowCount(0);   // 清空所有行
    }
