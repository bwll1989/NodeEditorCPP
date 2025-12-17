#include "CalendarWidget.hpp"
#include <QPainter>
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonParseError>
#include <QTextStream>
#include <QTextCharFormat>  // 新增：用于设置周标题与日期的文本格式

OscCalendarWidget::OscCalendarWidget(ScheduledTaskModel* model,QWidget *parent)
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
     * 函数：OscCalendarWidget::paintCell
     * 作用：尽可能使用 QSS/调色板进行绘制，仅在必要处做叠加绘制。
     * 策略：
     * - 非本月：只填充基础背景，保持留空（不绘制日期与计数）
     * - 本月：使用基类默认绘制（受 QSS 与主题控制），叠加“今天”边框与右上角计数
     */
    painter->save();

    const int shownMonth = monthShown();
    const int shownYear  = yearShown();
    const bool inCurrentMonth = (date.month() == shownMonth && date.year() == shownYear);

    // 非本月：填充基础背景，留空
    if (!inCurrentMonth) {
        painter->fillRect(rect, palette().base());
        painter->restore();
        return;
    }

    const bool isSelected = (date == selectedDate());
    const bool isToday    = (date == QDate::currentDate());

    // 使用默认绘制（文本、选中样式等由样式/QSS控制）
    QCalendarWidget::paintCell(painter, rect, date);

    // 叠加“今天”边框（采用主题高亮色）
    if (isToday) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(palette().highlight().color(), 2));
        QRect borderRect = rect.adjusted(2, 2, -2, -2);
        painter->drawRoundedRect(borderRect, 4, 4);
    }

    // 右上角显示该日期的 OSC 任务数量（采用主题高亮色）
    if (hasOscMessageForDate(date)) {
        const QString countText = QString::number(m_model->itemsForDate(date).count());
        painter->setPen(palette().highlight().color());
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



