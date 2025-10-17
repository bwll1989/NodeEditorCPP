#include "CalendarWidget.hpp"
#include <QPainter>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonParseError>
#include <QTextStream>
#include <QTextCharFormat>  // 新增：用于设置周标题与日期的文本格式

OscCalendarWidget::OscCalendarWidget(    ScheduledTaskModel* model,QWidget *parent)
    :m_model(model),
    QCalendarWidget(parent)
{
    // 启用拖拽功能
    setAcceptDrops(true);
    
    // 连接日期选择变化信号
    connect(this, &QCalendarWidget::selectionChanged,
            this, &OscCalendarWidget::onDateSelectionChanged);
    
    // 设置日历样式
    setGridVisible(true);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);//不显示星期数
    // setHorizontalHeaderFormat(QCalendarWidget::NoHorizontalHeader);
    setNavigationBarVisible(true);
    setDateEditEnabled(false);
    setStyleSheet(

            "QCalendarWidget QAbstractItemView::item:enabled:selected:!alternate { background-color: #1976D2; }"
            "QCalendarWidget QAbstractItemView::item:enabled:!selected:!alternate { background-color: #333333; }"
            "QCalendarWidget QAbstractItemView::item:enabled:!selected:alternate { background-color: #323232; }");
}

OscCalendarWidget::~OscCalendarWidget()
{

}

bool OscCalendarWidget::hasOscMessageForDate(const QDate& date) const
{
    return m_model->itemsForDate(date).count()>0;
}



void OscCalendarWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // 获取右键点击位置对应的日期
    QDate date = getDateAtPosition(event->pos());
    if (!date.isValid()) {
        QCalendarWidget::contextMenuEvent(event);
        return;
    }
    
    m_contextMenuDate = date;
    
    // 创建并显示右键菜单
    QMenu* menu = createContextMenu(date);
    if (menu) {
        menu->exec(event->globalPos());
        menu->deleteLater();
    }
}

void OscCalendarWidget::paintCell(QPainter *painter, const QRect &rect, QDate date) const
{
    /**
     * @brief 自定义绘制单元格
     * 
     * 功能点：
     * - 非本月日期不显示（仅保留背景，留空）
     * - 选中日期：填充蓝色背景
     * - 今天：绿色边框
     * - 周末日期数字为红色、工作日为浅色
     * - 保留右上角 OSC 计数显示
     */
    painter->save();

    const int shownMonth = monthShown();
    const int shownYear  = yearShown();
    const bool inCurrentMonth = (date.month() == shownMonth && date.year() == shownYear);

    // 背景填充（保持当前主题基色）
    QColor baseBg = palette().base().color();
    painter->fillRect(rect, baseBg);

    // 非本月日期：填充深色背景后返回
    if (!inCurrentMonth) {
        // painter->fillRect(rect, QColor("#2A2A2A")); // 更深的背景色
        // 显示文字为灰色
        painter->setPen(QColor("#808080"));
        painter->drawText(rect.adjusted(6, 4, -4, -4),
                          Qt::AlignLeft | Qt::AlignTop,
                          QString::number(date.day()));
        painter->restore();
        return;
    }

    const bool isSelected = (date == selectedDate());
    const bool isToday    = (date == QDate::currentDate());

    // 选中日期背景
    if (isSelected) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(QColor("#1976D2")); // 选中蓝色
        painter->setPen(Qt::NoPen);
        QRect selRect = rect.adjusted(2, 2, -2, -2);
        painter->drawRoundedRect(selRect, 4, 4);
    }

    // 今天边框（绿色）
    if (isToday) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(QColor("#00C853"), 2));
        QRect borderRect = rect.adjusted(2, 2, -2, -2);
        painter->drawRoundedRect(borderRect, 4, 4);
    }

    // 日期数字颜色：周末红，其它浅色
    QColor textColor = QColor("#E0E0E0");
    const int dow = date.dayOfWeek(); // 1-7（周一=1）
    if (dow == 6 || dow == 7) {
        textColor = QColor("#FF3B30");
    }
    painter->setPen(textColor);
    painter->drawText(rect.adjusted(6, 4, -4, -4),
                      Qt::AlignLeft | Qt::AlignTop,
                      QString::number(date.day()));

    // 右上角显示该日期的 OSC 任务数量
    if (hasOscMessageForDate(date)) {
        const QString countText = QString::number(m_model->itemsForDate(date).count());
        painter->setPen(QColor(255, 0, 0, 200));
        const int textWidth = painter->fontMetrics().horizontalAdvance(countText);
        const int x = rect.right() - textWidth - 4;
        const int y = rect.top() + painter->fontMetrics().ascent() + 4;
        painter->drawText(x, y, countText);
    }

    painter->restore();
}


void OscCalendarWidget::onDateSelectionChanged()
{



}


QDate OscCalendarWidget::getDateAtPosition(const QPoint& pos)
{
    // 这是一个简化的实现，实际可能需要更复杂的计算
    // 获取当前显示的月份和年份
    QDate currentDate = selectedDate();
    int year = currentDate.year();
    int month = currentDate.month();
    
    // 计算单元格大小
    QRect calendarRect = rect();
    int cellWidth = calendarRect.width() / 7;  // 7天一周
    int cellHeight = (calendarRect.height() - 50) / 6;  // 大约6行，减去标题栏
    
    // 计算点击的是第几行第几列
    int col = pos.x() / cellWidth;
    int row = (pos.y() - 50) / cellHeight;  // 减去标题栏高度
    
    if (col < 0 || col >= 7 || row < 0 || row >= 6) {
        return QDate();
    }
    
    // 计算该月第一天是星期几
    QDate firstDay(year, month, 1);
    int firstDayOfWeek = firstDay.dayOfWeek() - 1;  // Qt中周一是1，我们需要周日是0
    
    // 计算日期
    int dayNumber = row * 7 + col - firstDayOfWeek + 1;
    
    if (dayNumber < 1 || dayNumber > firstDay.daysInMonth()) {
        return QDate();
    }
    
    return QDate(year, month, dayNumber);
}

QMenu* OscCalendarWidget::createContextMenu(const QDate& date)
{
    QMenu* menu = new QMenu(this);
    
    // 显示日期信息
    QAction* dateAction = menu->addAction(QString("日期: %1").arg(date.toString("yyyy-MM-dd")));
    dateAction->setEnabled(false);
    
    menu->addSeparator();
    
    if (hasOscMessageForDate(date)) {
        // 如果有OSC数据，显示编辑和删除选项
        // QAction* editAction = menu->addAction("编辑OSC数据");
        // connect(editAction, &QAction::triggered, this, &OscCalendarWidget::editCurrentDateOscData);
        
        // QAction* deleteAction = menu->addAction("删除OSC数据");
        // connect(deleteAction, &QAction::triggered, this, &OscCalendarWidget::deleteCurrentDateOscData);
        
        menu->addSeparator();

    } else {
        // 如果没有OSC数据，显示提示
        QAction* noDataAction = menu->addAction("no scheduled");
        noDataAction->setEnabled(false);
    }
    
    return menu;
}




