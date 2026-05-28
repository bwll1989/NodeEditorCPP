#pragma once

#include "../InjectNode/ConditionMatch.hpp"

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QJsonArray>
#include <QJsonObject>

namespace Nodes
{
    class ValueLookupInterface : public QWidget {
        Q_OBJECT

    public:
        explicit ValueLookupInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , tableView(new QTableView(this))
            , model(new QStandardItemModel(0, 3, this))
        {
            setupUI();
            connect(model, &QStandardItemModel::itemChanged, this, &ValueLookupInterface::onItemChanged);
            importRulesArray({});
            if (model->rowCount() == 0) {
                appendRow(QString(), QString());
                finishTableUpdate();
            }
        }

        QJsonArray exportRulesArray() const
        {
            QJsonArray rulesArray;
            for (int row = 0; row < model->rowCount(); ++row) {
                QJsonObject rule;
                rule["condition"] = conditionAt(row);
                rule["value"] = outputValueAt(row);
                rulesArray.append(rule);
            }
            return rulesArray;
        }

        void importRulesArray(const QJsonArray &rulesArray)
        {
            m_batchEditing = true;
            clearActionButtons();
            model->removeRows(0, model->rowCount());

            for (const QJsonValue &ruleValue : rulesArray) {
                if (ruleValue.isObject()) {
                    const QJsonObject rule = ruleValue.toObject();
                    appendRow(rule["condition"].toString(), rule["value"].toString());
                } else {
                    appendRow(QString(), jsonValueToString(ruleValue));
                }
            }
            m_batchEditing = false;

            finishTableUpdate();
        }

        int rowCount() const
        {
            return model->rowCount();
        }

        QString conditionAt(int row) const
        {
            if (row < 0 || row >= model->rowCount()) {
                return {};
            }
            QStandardItem *item = model->item(row, 0);
            return item ? item->text() : QString();
        }

        QString outputValueAt(int row) const
        {
            if (row < 0 || row >= model->rowCount()) {
                return {};
            }
            QStandardItem *item = model->item(row, 1);
            return item ? item->text() : QString();
        }

        int findMatchingRow(const QVariant &input) const
        {
            for (int row = 0; row < model->rowCount(); ++row) {
                if (ConditionMatch::matches(input, conditionAt(row))) {
                    return row;
                }
            }
            return -1;
        }

    signals:
        void listChanged();

    private slots:
        void deleteRow(int row)
        {
            if (row < 0 || row >= model->rowCount()) {
                return;
            }

            clearActionButtons();
            model->removeRow(row);
            finishTableUpdate();
            emit listChanged();
        }

        void addRow()
        {
            appendRow(QString(), QString());
            finishTableUpdate();
            emit listChanged();
        }

        void onItemChanged(QStandardItem *item)
        {
            if (m_batchEditing || !item) {
                return;
            }
            if (item->column() == 0 || item->column() == 1) {
                emit listChanged();
            }
        }

    private:
        QTableView *tableView;
        QStandardItemModel *model;
        QPushButton *addRowButton;
        bool m_batchEditing = false;

        static QString jsonValueToString(const QJsonValue &value)
        {
            if (value.isString()) {
                return value.toString();
            }
            if (value.isBool()) {
                return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            }
            if (value.isDouble()) {
                const double number = value.toDouble();
                if (qFuzzyCompare(number, std::round(number))) {
                    return QString::number(static_cast<qlonglong>(std::llround(number)));
                }
                return QString::number(number, 'g', 16);
            }
            if (value.isNull() || value.isUndefined()) {
                return {};
            }
            return value.toVariant().toString();
        }

        void setupUI()
        {
            model->setHorizontalHeaderLabels({tr("Condition"), tr("Value"), tr("Action")});
            tableView->setModel(model);
            tableView->setSortingEnabled(false);
            tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
            tableView->verticalHeader()->setVisible(false);
            tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
            tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

            addRowButton = new QPushButton(tr("Add Rule"), this);
            connect(addRowButton, &QPushButton::clicked, this, &ValueLookupInterface::addRow);

            auto *mainLayout = new QVBoxLayout(this);
            mainLayout->addWidget(tableView);
            mainLayout->addWidget(addRowButton);
            setLayout(mainLayout);
        }

        void appendRow(const QString &condition, const QString &value)
        {
            const int row = model->rowCount();
            model->insertRow(row);
            model->setItem(row, 0, new QStandardItem(condition));
            model->setItem(row, 1, new QStandardItem(value));
            model->setItem(row, 2, new QStandardItem());
        }

        void clearActionButtons()
        {
            for (int row = 0; row < model->rowCount(); ++row) {
                tableView->setIndexWidget(model->index(row, 2), nullptr);
            }
        }

        void rebuildActionButtons()
        {
            clearActionButtons();
            for (int row = 0; row < model->rowCount(); ++row) {
                auto *deleteButton = new QPushButton(tr("Delete"), this);
                connect(deleteButton, &QPushButton::clicked, this, [this, row]() {
                    deleteRow(row);
                });
                tableView->setIndexWidget(model->index(row, 2), deleteButton);
            }
        }

        void finishTableUpdate()
        {
            rebuildActionButtons();
            tableView->resizeRowsToContents();
            tableView->viewport()->update();
        }
    };
}
