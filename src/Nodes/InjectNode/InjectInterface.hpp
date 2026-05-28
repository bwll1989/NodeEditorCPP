#pragma once

#include <cmath>

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QJsonArray>

namespace Nodes
{
    class InjectInterface : public QWidget {
        Q_OBJECT

    public:
        explicit InjectInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , tableView(new QTableView(this))
            , model(new QStandardItemModel(0, 3, this))
        {
            setupUI();
            connect(model, &QStandardItemModel::itemChanged, this, &InjectInterface::onItemChanged);
            importValuesArray({});
            ensureRowCount(5);
        }

        QJsonArray exportValuesArray() const
        {
            QJsonArray valuesArray;
            for (int row = 0; row < model->rowCount(); ++row) {
                valuesArray.append(valueAt(row));
            }
            return valuesArray;
        }

        void importValuesArray(const QJsonArray &valuesArray)
        {
            m_batchEditing = true;
            clearActionButtons();
            model->removeRows(0, model->rowCount());

            for (const QJsonValue &value : valuesArray) {
                appendRow(jsonValueToString(value));
            }
            m_batchEditing = false;

            finishTableUpdate();
        }

        int rowCount() const
        {
            return model->rowCount();
        }

        QString valueAt(int index) const
        {
            if (index < 0 || index >= model->rowCount()) {
                return {};
            }
            QStandardItem *item = model->item(index, 1);
            return item ? item->text() : QString();
        }

        void ensureRowCount(int count)
        {
            if (count < 0 || model->rowCount() >= count) {
                return;
            }

            m_batchEditing = true;
            while (model->rowCount() < count) {
                appendRow(QString());
            }
            m_batchEditing = false;

            finishTableUpdate();
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
            appendRow(QString());
            finishTableUpdate();
            emit listChanged();
        }

        void onItemChanged(QStandardItem *item)
        {
            if (m_batchEditing || !item || item->column() != 1) {
                return;
            }
            emit listChanged();
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
            model->setHorizontalHeaderLabels({tr("Index"), tr("Value"), tr("Action")});
            tableView->setModel(model);
            tableView->setSortingEnabled(false);
            tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
            tableView->verticalHeader()->setVisible(false);
            tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
            tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

            addRowButton = new QPushButton(tr("Add Item"), this);
            connect(addRowButton, &QPushButton::clicked, this, &InjectInterface::addRow);

            auto *mainLayout = new QVBoxLayout(this);
            mainLayout->addWidget(tableView);
            mainLayout->addWidget(addRowButton);
            setLayout(mainLayout);
        }

        void appendRow(const QString &value)
        {
            const int row = model->rowCount();
            model->insertRow(row);

            auto *indexItem = new QStandardItem(QString::number(row));
            indexItem->setEditable(false);
            indexItem->setTextAlignment(Qt::AlignCenter);
            model->setItem(row, 0, indexItem);

            model->setItem(row, 1, new QStandardItem(value));
            model->setItem(row, 2, new QStandardItem());
        }

        void refreshIndexColumn()
        {
            for (int row = 0; row < model->rowCount(); ++row) {
                QStandardItem *indexItem = model->item(row, 0);
                if (!indexItem) {
                    indexItem = new QStandardItem();
                    indexItem->setEditable(false);
                    indexItem->setTextAlignment(Qt::AlignCenter);
                    model->setItem(row, 0, indexItem);
                }
                indexItem->setText(QString::number(row));
            }
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
            refreshIndexColumn();
            rebuildActionButtons();
            tableView->resizeRowsToContents();
            tableView->viewport()->update();
        }
    };
}
