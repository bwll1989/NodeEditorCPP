#include "LogWidget.hpp"
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QActionGroup>
LogWidget::LogWidget() {
    setColumnCount(4);  // 时间、级别、文件、消息
    QStringList headers = {"Timestamp", "Level", "File", "Message"};
    setHorizontalHeaderLabels(headers);
    horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
    setEditTriggers(QAbstractItemView::DoubleClicked);
    
    // 设置右键菜单策略
    setContextMenuPolicy(Qt::DefaultContextMenu);
    
    // 设置选择行为，使整行选中
    setSelectionBehavior(QAbstractItemView::SelectRows);
    // 移除菜单阴影，避免圆角显示不一致
    if (menu) {
        menu->setWindowFlags(menu->windowFlags() | Qt::NoDropShadowWindowHint);
        menu->setAttribute(Qt::WA_TranslucentBackground, false);
    }
    // 初始化右键菜单
    initializeActions();
    
}

void LogWidget::initializeActions() {

    // 创建清除日志的动作
    QAction *clearAction = menu->addAction("清除日志 ");
    clearAction->setIcon(QIcon(":/icons/icons/clear.png"));
    connect(clearAction, &QAction::triggered, this, &LogWidget::clearTableWidget);

    // 添加导出日志功能
    QAction *exportAction = menu->addAction("导出日志 ");
    exportAction->setIcon(QIcon(":/icons/icons/export.png"));
    connect(exportAction, &QAction::triggered, this, &LogWidget::exportLog);

    // 添加日志过滤子菜单
    QMenu *filterMenu = menu->addMenu( "过滤日志  ");
    filterMenu->setIcon(QIcon(":/icons/icons/filter.png"));
    QStringList filterLevels = {"All", "Debug", "Info", "Warn", "Critical", "Fatal"};
    QActionGroup *filterGroup = new QActionGroup(this);
    filterGroup->setExclusive(true);

    for (const QString &level : filterLevels) {
        QAction *levelAction = filterMenu->addAction(level);
        levelAction->setCheckable(true);
        levelAction->setChecked(level == m_currentFilter);
        filterGroup->addAction(levelAction);

        connect(levelAction, &QAction::triggered, [this, level]() {
            this->setLogFilter(level);
        });
    }
}
QList<QAction*> LogWidget::getActions() {
    return menu->actions();
}

void LogWidget::contextMenuEvent(QContextMenuEvent *event)
{
    menu->exec(event->globalPos());
}

void LogWidget::clearTableWidget() {
    clearContents();  // 仅清空内容，不清空表头
    setRowCount(0);   // 清空所有行
}

void LogWidget::setLogFilter(const QString &level) {
    m_currentFilter = level;
    
    // 应用过滤器
    for (int row = 0; row < rowCount(); ++row) {
        bool show = true;
        if (level != "All") {
            QTableWidgetItem *levelItem = item(row, 1);
            if (levelItem && levelItem->text() != level) {
                show = false;
            }
        }
        setRowHidden(row, !show);
    }
}

void LogWidget::exportLog() {
    QString fileName = QFileDialog::getSaveFileName(this, "导出日志",
                                                 QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + "_log.csv",
                                                 "CSV文件 (*.csv);;文本文件 (*.txt)");
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        
        // 写入表头
        QStringList headers;
        for (int col = 0; col < columnCount(); ++col) {
            headers << horizontalHeaderItem(col)->text();
        }
        stream << headers.join(",") << "\n";
        
        // 写入数据行
        for (int row = 0; row < rowCount(); ++row) {
            if (!isRowHidden(row)) {
                QStringList rowData;
                for (int col = 0; col < columnCount(); ++col) {
                    QTableWidgetItem *item = this->item(row, col);
                    rowData << (item ? "\"" + item->text().replace("\"", "\"\"") + "\"" : "");
                }
                stream << rowData.join(",") << "\n";
            }
        }
        
        file.close();
    }
}
