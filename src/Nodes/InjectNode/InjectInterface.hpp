#pragma once

#include "PortAlignedColumn/PortAlignedColumn.hpp"

#include <cmath>

#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonValue>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace Nodes
{
    /**
     * Inject 单行：输入框 + 右侧删除。
     * 行高 = step；1px 边框含在高度内（margins=1），避免下边线被裁切。
     */
    class InjectPresetRow : public QWidget
    {
        Q_OBJECT
    public:
        explicit InjectPresetRow(int rowHeight, const QString &placeholder, QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setObjectName(QStringLiteral("InjectPresetRow"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            setFixedHeight(rowHeight);
            setMinimumHeight(rowHeight);
            setMaximumHeight(rowHeight);

            const int inner = qMax(1, rowHeight - 2);

            auto *lay = new QHBoxLayout(this);
            lay->setContentsMargins(1, 1, 1, 1);
            lay->setSpacing(0);

            m_edit = new QLineEdit(this);
            m_edit->setPlaceholderText(placeholder);
            m_edit->setFixedHeight(inner);
            m_edit->setFrame(false);
            m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            connect(m_edit, &QLineEdit::editingFinished, this, &InjectPresetRow::valueChanged);
            connect(m_edit, &QLineEdit::textEdited, this, &InjectPresetRow::valueChanged);

            m_deleteBtn = new QPushButton(this);
            m_deleteBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/remove.png")));
            m_deleteBtn->setIconSize(QSize(14, 14));
            m_deleteBtn->setFixedSize(inner, inner);
            m_deleteBtn->setFlat(true);
            m_deleteBtn->setFocusPolicy(Qt::NoFocus);
            m_deleteBtn->setToolTip(QStringLiteral("删除"));
            connect(m_deleteBtn, &QPushButton::clicked, this, &InjectPresetRow::deleteRequested);

            lay->addWidget(m_edit, 1);
            lay->addWidget(m_deleteBtn, 0);
        }

        QString text() const { return m_edit ? m_edit->text() : QString(); }
        void setText(const QString &text)
        {
            if (m_edit) {
                m_edit->setText(text);
            }
        }

    signals:
        void valueChanged();
        void deleteRequested();

    private:
        QLineEdit *m_edit = nullptr;
        QPushButton *m_deleteBtn = nullptr;
    };

    /**
     * Inject 嵌入界面：PortAlignedColumn 行槽 + 右侧添加按钮 + 每行右侧删除。
     */
    class InjectInterface : public QWidget
    {
        Q_OBJECT

    public:
        /** 与 Distribute 嵌入区同宽，保证两节点默认宽度一致 */
        static constexpr int kEmbeddedWidth = 260;

        explicit InjectInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_column(new PortAlignedColumn(this))
            , m_rowHeight(PortAlignedColumn::rowPitch())
            , m_placeholder(QStringLiteral("触发后输出的值"))
        {
            setObjectName(QStringLiteral("InjectInterface"));
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
            connect(m_addButton, &QPushButton::clicked, this, &InjectInterface::onAddClicked);

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

        QString valueAt(int index) const
        {
            if (auto *row = rowAt(index)) {
                return row->text();
            }
            return {};
        }

        void setRowCount(int count)
        {
            count = qMax(m_minimumRows, count);
            const QSignalBlocker blocker(this);
            while (m_column->rowCount() < count) {
                appendRowInternal(QString());
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

        QJsonArray exportValuesArray() const
        {
            QJsonArray arr;
            for (int i = 0; i < m_column->rowCount(); ++i) {
                arr.append(valueAt(i));
            }
            return arr;
        }

        void importValuesArray(const QJsonArray &valuesArray)
        {
            const QSignalBlocker blocker(this);
            QStringList values;
            values.reserve(valuesArray.size());
            for (const QJsonValue &v : valuesArray) {
                values.append(jsonValueToString(v));
            }

            const int target = qMax(m_minimumRows, values.size());
            while (m_column->rowCount() < target) {
                appendRowInternal(QString());
            }
            while (m_column->rowCount() > target) {
                m_column->removeRow(m_column->rowCount() - 1);
            }
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (auto *row = rowAt(i)) {
                    row->setText(i < values.size() ? values.at(i) : QString());
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
        void listChanged();
        void rowAppended();
        void rowRemoved(int index);

    private slots:
        void onAddClicked()
        {
            appendRowInternal(QString());
            updateGeometry();
            emit listChanged();
            emit rowAppended();
        }

        void onRowDeleteRequested()
        {
            auto *row = qobject_cast<InjectPresetRow *>(sender());
            if (!row) {
                return;
            }
            const int index = indexOfRow(row);
            if (index < 0 || m_column->rowCount() <= m_minimumRows) {
                return;
            }
            m_column->removeRow(index);
            updateGeometry();
            emit listChanged();
            emit rowRemoved(index);
        }

    private:
        InjectPresetRow *rowAt(int index) const
        {
            return qobject_cast<InjectPresetRow *>(m_column->rowContent(index));
        }

        int indexOfRow(InjectPresetRow *row) const
        {
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (m_column->rowContent(i) == row) {
                    return i;
                }
            }
            return -1;
        }

        void appendRowInternal(const QString &value)
        {
            auto *row = new InjectPresetRow(m_rowHeight, m_placeholder, m_column);
            row->setText(value);
            connect(row, &InjectPresetRow::valueChanged, this, &InjectInterface::listChanged);
            connect(row, &InjectPresetRow::deleteRequested, this, &InjectInterface::onRowDeleteRequested);
            m_column->appendRow(row);
        }

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

        PortAlignedColumn *m_column = nullptr;
        QPushButton *m_addButton = nullptr;
        QString m_placeholder;
        int m_rowHeight = 23;
        int m_minimumRows = 1;
    };
}
