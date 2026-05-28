#pragma once

#include "ConditionMatch.hpp"

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>

namespace Nodes
{
    class DistributeInterface : public QWidget {
        Q_OBJECT

    public:
        explicit DistributeInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , tableView(new QTableView(this))
            , model(new QStandardItemModel(0, 3, this))
        {
            setupUI();
            connect(model, &QStandardItemModel::itemChanged, this, &DistributeInterface::onItemChanged);
            importRulesArray({});
            if (model->rowCount() == 0) {
                appendRow(QStringLiteral("== 0"), QStringLiteral("0"));
                appendRow(QStringLiteral("*"), QStringLiteral("0"));
                finishTableUpdate();
            }
        }

        QJsonArray exportRulesArray() const
        {
            QJsonArray rulesArray;
            for (int row = 0; row < model->rowCount(); ++row) {
                QJsonObject rule;
                rule["condition"] = conditionAt(row);
                rule["port"] = outputPortAt(row);
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
                    appendRow(rule["condition"].toString(), rule["port"].toString());
                }
            }
            m_batchEditing = false;

            finishTableUpdate();
            emit rulesChanged();
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

        QString outputPortAt(int row) const
        {
            if (row < 0 || row >= model->rowCount()) {
                return {};
            }
            QStandardItem *item = model->item(row, 1);
            return item ? item->text() : QString();
        }

        QList<int> findAllMatchingRows(const QVariant &input) const
        {
            QList<int> matchedRows;
            for (int row = 0; row < model->rowCount(); ++row) {
                if (ConditionMatch::matches(input, conditionAt(row))) {
                    matchedRows.append(row);
                }
            }
            return matchedRows;
        }

        int maxConfiguredOutputPort() const
        {
            int maxPort = 0;
            for (int row = 0; row < model->rowCount(); ++row) {
                bool ok = false;
                const int port = outputPortAt(row).toInt(&ok);
                if (ok && port > maxPort) {
                    maxPort = port;
                }
            }
            return maxPort;
        }

    signals:
        void rulesChanged();

    private slots:
        void deleteRow(int row)
        {
            if (row < 0 || row >= model->rowCount()) {
                return;
            }

            clearActionButtons();
            model->removeRow(row);
            finishTableUpdate();
            emit rulesChanged();
        }

        void addRow()
        {
            appendRow(QString(), QStringLiteral("0"));
            finishTableUpdate();
            emit rulesChanged();
        }

        void onItemChanged(QStandardItem *item)
        {
            if (m_batchEditing || !item) {
                return;
            }
            if (item->column() == 0 || item->column() == 1) {
                emit rulesChanged();
            }
        }

    private:
        QTableView *tableView;
        QStandardItemModel *model;
        QPushButton *addRowButton;
        bool m_batchEditing = false;

        void setupUI()
        {
            model->setHorizontalHeaderLabels({tr("Condition"), tr("Port"), tr("Action")});
            tableView->setModel(model);
            tableView->setSortingEnabled(false);
            tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
            tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
            tableView->verticalHeader()->setVisible(false);
            tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
            tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

            addRowButton = new QPushButton(tr("Add Rule"), this);
            connect(addRowButton, &QPushButton::clicked, this, &DistributeInterface::addRow);

            auto *mainLayout = new QVBoxLayout(this);
            mainLayout->addWidget(tableView);
            mainLayout->addWidget(addRowButton);
            setLayout(mainLayout);
        }

        void appendRow(const QString &condition, const QString &port)
        {
            const int row = model->rowCount();
            model->insertRow(row);
            model->setItem(row, 0, new QStandardItem(condition));
            model->setItem(row, 1, new QStandardItem(port));
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
