#pragma once

#include "PortAlignedColumn/PortAlignedColumn.hpp"

#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

namespace Nodes
{
    /**
     * Merge 单行：Name + Rename + 右侧删除。
     * 行索引即对应输入端口索引；行高 = step。
     */
    class MergeRuleRow : public QWidget
    {
        Q_OBJECT
    public:
        explicit MergeRuleRow(int rowHeight, QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setObjectName(QStringLiteral("MergeRuleRow"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            setFixedHeight(rowHeight);
            setMinimumHeight(rowHeight);
            setMaximumHeight(rowHeight);

            const int inner = qMax(1, rowHeight - 2);

            auto *lay = new QHBoxLayout(this);
            lay->setContentsMargins(1, 1, 1, 1);
            lay->setSpacing(2);

            m_nameEdit = new QLineEdit(this);
            m_nameEdit->setPlaceholderText(QStringLiteral("Name"));
            m_nameEdit->setFixedHeight(inner);
            m_nameEdit->setFrame(false);
            m_nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            connect(m_nameEdit, &QLineEdit::editingFinished, this, &MergeRuleRow::valueChanged);
            connect(m_nameEdit, &QLineEdit::textEdited, this, &MergeRuleRow::valueChanged);

            m_renameEdit = new QLineEdit(this);
            m_renameEdit->setPlaceholderText(QStringLiteral("Rename"));
            m_renameEdit->setFixedHeight(inner);
            m_renameEdit->setFrame(false);
            m_renameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            connect(m_renameEdit, &QLineEdit::editingFinished, this, &MergeRuleRow::valueChanged);
            connect(m_renameEdit, &QLineEdit::textEdited, this, &MergeRuleRow::valueChanged);

            m_deleteBtn = new QPushButton(this);
            m_deleteBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/remove.png")));
            m_deleteBtn->setIconSize(QSize(14, 14));
            m_deleteBtn->setFixedSize(inner, inner);
            m_deleteBtn->setFlat(true);
            m_deleteBtn->setFocusPolicy(Qt::NoFocus);
            m_deleteBtn->setToolTip(QStringLiteral("删除"));
            connect(m_deleteBtn, &QPushButton::clicked, this, &MergeRuleRow::deleteRequested);

            lay->addWidget(m_nameEdit, 1);
            lay->addWidget(m_renameEdit, 1);
            lay->addWidget(m_deleteBtn, 0);
        }

        QString name() const { return m_nameEdit ? m_nameEdit->text() : QString(); }
        QString rename() const { return m_renameEdit ? m_renameEdit->text() : QString(); }

        void setName(const QString &text)
        {
            if (m_nameEdit) {
                m_nameEdit->setText(text);
            }
        }

        void setRename(const QString &text)
        {
            if (m_renameEdit) {
                m_renameEdit->setText(text);
            }
        }

    signals:
        void valueChanged();
        void deleteRequested();

    private:
        QLineEdit *m_nameEdit = nullptr;
        QLineEdit *m_renameEdit = nullptr;
        QPushButton *m_deleteBtn = nullptr;
    };

    /**
     * Merge 嵌入界面：PortAlignedColumn 行槽 + 右侧添加；每行对应一个 In 端口。
     */
    class MergeInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kEmbeddedWidth = 280;

        explicit MergeInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_column(new PortAlignedColumn(this))
            , m_rowHeight(PortAlignedColumn::rowPitch())
        {
            setObjectName(QStringLiteral("MergeInterface"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumWidth(kEmbeddedWidth);

            const int inner = qMax(1, m_rowHeight - 2);
            m_addButton = new QPushButton(this);
            m_addButton->setIcon(QIcon(QStringLiteral(":/icons/icons/add.png")));
            m_addButton->setIconSize(QSize(14, 14));
            m_addButton->setFixedSize(inner, inner);
            m_addButton->setFlat(true);
            m_addButton->setFocusPolicy(Qt::NoFocus);
            m_addButton->setToolTip(QStringLiteral("添加"));
            connect(m_addButton, &QPushButton::clicked, this, &MergeInterface::onAddClicked);

            auto *right = new QVBoxLayout();
            right->setContentsMargins(0, 1, 1, 1);
            right->setSpacing(0);
            right->addWidget(m_addButton, 0, Qt::AlignTop);
            right->addStretch(1);

            auto *root = new QHBoxLayout(this);
            root->setContentsMargins(0, 0, 0, 0);
            root->setSpacing(0);
            root->addWidget(m_column, 1);
            root->addLayout(right, 0);
            setLayout(root);

            setMinimumRows(1);
            setRowCount(4);
        }

        int rowCount() const { return m_column->rowCount(); }

        QString nameAt(int index) const
        {
            if (auto *row = rowAt(index)) {
                return row->name();
            }
            return {};
        }

        QString renameAt(int index) const
        {
            if (auto *row = rowAt(index)) {
                return row->rename();
            }
            return {};
        }

        /** 兼容旧 DataModel：返回 [Name, Rename]；端口索引即行号，不再带 ID。 */
        QStringList getRowValues(int row) const
        {
            if (row < 0 || row >= m_column->rowCount()) {
                return {};
            }
            return {nameAt(row), renameAt(row)};
        }

        void setRowCount(int count)
        {
            count = qMax(m_minimumRows, count);
            const QSignalBlocker blocker(this);
            while (m_column->rowCount() < count) {
                appendRowInternal(QStringLiteral("default"), QString());
            }
            while (m_column->rowCount() > count) {
                m_column->removeRow(m_column->rowCount() - 1);
            }
            updateGeometry();
        }

        void setMinimumRows(int count)
        {
            m_minimumRows = qMax(1, count);
            if (m_column->rowCount() < m_minimumRows) {
                setRowCount(m_minimumRows);
            }
        }

        QJsonObject exportToJson() const
        {
            QJsonArray rowsArray;
            for (int row = 0; row < m_column->rowCount(); ++row) {
                QJsonObject rowObject;
                rowObject.insert(QStringLiteral("ID"), QString::number(row));
                rowObject.insert(QStringLiteral("Name"), nameAt(row));
                rowObject.insert(QStringLiteral("Rename"), renameAt(row));
                rowsArray.append(rowObject);
            }
            QJsonObject jsonObject;
            jsonObject.insert(QStringLiteral("rows"), rowsArray);
            return jsonObject;
        }

        void importFromJson(const QJsonObject &jsonObject)
        {
            const QSignalBlocker blocker(this);
            const QJsonArray rowsArray = jsonObject.value(QStringLiteral("rows")).toArray();

            const int target = qMax(m_minimumRows, rowsArray.size());
            while (m_column->rowCount() < target) {
                appendRowInternal(QString(), QString());
            }
            while (m_column->rowCount() > target) {
                m_column->removeRow(m_column->rowCount() - 1);
            }

            for (int row = 0; row < m_column->rowCount(); ++row) {
                auto *item = rowAt(row);
                if (!item) {
                    continue;
                }
                if (row < rowsArray.size()) {
                    const QJsonObject rowObject = rowsArray.at(row).toObject();
                    item->setName(rowObject.value(QStringLiteral("Name")).toString());
                    item->setRename(rowObject.value(QStringLiteral("Rename")).toString());
                } else {
                    item->setName(QStringLiteral("default"));
                    item->setRename(QString());
                }
            }
            updateGeometry();
        }

        QSize sizeHint() const override
        {
            const int rows = qMax(1, m_column->rowCount());
            return QSize(kEmbeddedWidth, rows * m_rowHeight);
        }

        QSize minimumSizeHint() const override
        {
            const int rows = qMax(1, m_column->rowCount());
            return QSize(kEmbeddedWidth, rows * m_rowHeight);
        }

    signals:
        void tableChanged();
        void rowChanged(int row);
        void rowAppended();
        void rowRemoved(int index);

    private slots:
        void onAddClicked()
        {
            appendRowInternal(QStringLiteral("default"), QString());
            updateGeometry();
            emit tableChanged();
            emit rowAppended();
        }

        void onRowDeleteRequested()
        {
            auto *row = qobject_cast<MergeRuleRow *>(sender());
            if (!row) {
                return;
            }
            const int index = indexOfRow(row);
            if (index < 0 || m_column->rowCount() <= m_minimumRows) {
                return;
            }
            m_column->removeRow(index);
            updateGeometry();
            emit tableChanged();
            emit rowRemoved(index);
        }

        void onRowValueChanged()
        {
            auto *row = qobject_cast<MergeRuleRow *>(sender());
            if (!row) {
                return;
            }
            const int index = indexOfRow(row);
            if (index < 0) {
                return;
            }
            emit rowChanged(index);
            emit tableChanged();
        }

    private:
        MergeRuleRow *rowAt(int index) const
        {
            return qobject_cast<MergeRuleRow *>(m_column->rowContent(index));
        }

        int indexOfRow(MergeRuleRow *row) const
        {
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (m_column->rowContent(i) == row) {
                    return i;
                }
            }
            return -1;
        }

        void appendRowInternal(const QString &name, const QString &rename)
        {
            auto *row = new MergeRuleRow(m_rowHeight, m_column);
            row->setName(name);
            row->setRename(rename);
            connect(row, &MergeRuleRow::valueChanged, this, &MergeInterface::onRowValueChanged);
            connect(row, &MergeRuleRow::deleteRequested, this, &MergeInterface::onRowDeleteRequested);
            m_column->appendRow(row);
        }

        PortAlignedColumn *m_column = nullptr;
        QPushButton *m_addButton = nullptr;
        int m_rowHeight = 23;
        int m_minimumRows = 1;
    };
}
