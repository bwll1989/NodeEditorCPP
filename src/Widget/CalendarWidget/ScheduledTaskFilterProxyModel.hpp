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
        beginFilterChange();
        m_date = d;
        endFilterChange();
    }
    QDate getFilterDate() const {return m_date;}
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override {
        if (!sourceModel()) return true;
        const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        if (!idx.isValid()) return false;

        const QString schedType = sourceModel()->data(idx, ScheduledTaskModel::RoleScheduleType).toString();
        const QTime t = sourceModel()->data(idx, ScheduledTaskModel::RoleTime).toTime();

        if (schedType.compare(QStringLiteral("loop"), Qt::CaseInsensitive) == 0) {
            // 循环任务：列表中始终可见，便于在任何日期下编辑
            return true;
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