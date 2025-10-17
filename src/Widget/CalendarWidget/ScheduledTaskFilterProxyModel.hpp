#pragma once

#include <QSortFilterProxyModel>
#include <QDate>
#include "ScheduledTaskModel.hpp"

class ScheduledTaskFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ScheduledTaskFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent) {}

    void setFilterDate(const QDate& d) {
        m_date = d;
        invalidateFilter();
    }
    QDate getFilterDate() const {return m_date;}
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override {
        if (!sourceModel()) return true;
        const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        if (!idx.isValid()) return false;

        const QString schedType = sourceModel()->data(idx, ScheduledTaskModel::RoleScheduleType).toString();
        const QTime t = sourceModel()->data(idx, ScheduledTaskModel::RoleTime).toTime();
        const QStringList loopDays = sourceModel()->data(idx, ScheduledTaskModel::RoleLoopDays).toStringList();

        if (schedType.compare("loop", Qt::CaseInsensitive) == 0) {
            // 映射星期几
            const int dow = m_date.dayOfWeek(); // 1=Mon ... 7=Sun
            static const QStringList names = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
            const QString name = names.value(dow - 1);
            return loopDays.contains(name, Qt::CaseInsensitive);
        } else {
            // 一次性任务：需要模型提供日期。若暂未提供，则默认不过滤（保留显示）
            const QVariant dVar = sourceModel()->data(idx, ScheduledTaskModel::RoleDate);
            if (dVar.isValid()) {
                const QDate date = dVar.toDate();
                return date.isValid() ? (date == m_date) : true;
            }
            // 回退策略：无日期时不过滤，避免误隐藏
            Q_UNUSED(t);
            return true;
        }
    }

private:
    QDate m_date = QDate::currentDate();
};