#pragma once

#include "PortAlignedColumn/PortAlignedColumn.hpp"

#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

namespace Nodes
{
    /**
     * Distribute 单行：左侧删除 + 条件输入。
     * 行高 = step；1px 边框画在高度内部（layout margins=1），避免下边线被裁切。
     */
    class DistributeRuleRow : public QWidget
    {
        Q_OBJECT
    public:
        explicit DistributeRuleRow(int rowHeight, const QString &placeholder, QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setObjectName(QStringLiteral("DistributeRuleRow"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            setFixedHeight(rowHeight);
            setMinimumHeight(rowHeight);
            setMaximumHeight(rowHeight);

            const int inner = qMax(1, rowHeight - 2);

            auto *lay = new QHBoxLayout(this);
            lay->setContentsMargins(1, 1, 1, 1);
            lay->setSpacing(0);

            m_deleteBtn = new QPushButton(this);
            m_deleteBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/remove.png")));
            m_deleteBtn->setIconSize(QSize(14, 14));
            m_deleteBtn->setFixedSize(inner, inner);
            m_deleteBtn->setFlat(true);
            m_deleteBtn->setFocusPolicy(Qt::NoFocus);
            m_deleteBtn->setToolTip(QStringLiteral("删除"));
            connect(m_deleteBtn, &QPushButton::clicked, this, &DistributeRuleRow::deleteRequested);

            m_edit = new QLineEdit(this);
            m_edit->setPlaceholderText(placeholder);
            m_edit->setFixedHeight(inner);
            m_edit->setFrame(false);
            m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            connect(m_edit, &QLineEdit::editingFinished, this, &DistributeRuleRow::conditionChanged);
            connect(m_edit, &QLineEdit::textEdited, this, &DistributeRuleRow::conditionChanged);

            lay->addWidget(m_deleteBtn, 0);
            lay->addWidget(m_edit, 1);
        }

        QString condition() const { return m_edit ? m_edit->text() : QString(); }
        void setCondition(const QString &text)
        {
            if (m_edit) {
                m_edit->setText(text);
            }
        }

    signals:
        void conditionChanged();
        void deleteRequested();

    private:
        QLineEdit *m_edit = nullptr;
        QPushButton *m_deleteBtn = nullptr;
    };

    class DistributeInterface : public QWidget
    {
        Q_OBJECT

    public:
        /** 与 Inject 嵌入区同宽，保证两节点默认宽度一致 */
        static constexpr int kEmbeddedWidth = 260;

        explicit DistributeInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_column(new PortAlignedColumn(this))
            , m_rowHeight(PortAlignedColumn::rowPitch())
            , m_placeholder(QStringLiteral("JS Expression (e.g., \"$input['key']\")"))
        {
            setObjectName(QStringLiteral("DistributeInterface"));
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
            connect(m_addButton, &QPushButton::clicked, this, &DistributeInterface::onAddClicked);

            auto *left = new QVBoxLayout();
            left->setContentsMargins(1, 1, 0, 1);
            left->setSpacing(0);
            left->addWidget(m_addButton, 0, Qt::AlignTop);
            left->addStretch(1);

            auto *root = new QHBoxLayout(this);
            root->setContentsMargins(0, 0, 0, 0);
            root->setSpacing(0);
            root->addLayout(left, 0);
            root->addWidget(m_column, 1);
            setLayout(root);

            setMinimumRows(1);
            setRowCount(4);
        }

        QJsonArray exportRulesArray() const
        {
            QJsonArray rulesArray;
            for (int row = 0; row < m_column->rowCount(); ++row) {
                QJsonObject rule;
                rule["condition"] = conditionAt(row);
                rule["port"] = QString::number(row);
                rulesArray.append(rule);
            }
            return rulesArray;
        }

        void importRulesArray(const QJsonArray &rulesArray)
        {
            const QSignalBlocker blocker(this);
            QMap<int, QString> conditionsByPort;
            int maxPort = -1;
            int fallbackPort = 0;

            for (const QJsonValue &ruleValue : rulesArray) {
                if (!ruleValue.isObject()) {
                    continue;
                }
                const QJsonObject rule = ruleValue.toObject();
                bool ok = false;
                int port = rule["port"].toString().toInt(&ok);
                if (!ok || port < 0) {
                    port = fallbackPort;
                }
                conditionsByPort[port] = rule["condition"].toString();
                maxPort = qMax(maxPort, port);
                fallbackPort = qMax(fallbackPort, port + 1);
            }

            const int target = qMax(m_minimumRows, maxPort + 1);
            while (m_column->rowCount() < target) {
                appendRowInternal(QString());
            }
            while (m_column->rowCount() > target) {
                m_column->removeRow(m_column->rowCount() - 1);
            }
            for (int row = 0; row < m_column->rowCount(); ++row) {
                if (auto *ruleRow = rowAt(row)) {
                    ruleRow->setCondition(conditionsByPort.value(row));
                }
            }
            updateGeometry();
        }

        int rowCount() const { return m_column->rowCount(); }
        QString conditionAt(int row) const
        {
            if (auto *ruleRow = rowAt(row)) {
                return ruleRow->condition();
            }
            return {};
        }

        QString outputPortAt(int row) const
        {
            return row >= 0 && row < m_column->rowCount() ? QString::number(row) : QString();
        }

        int maxConfiguredOutputPort() const { return qMax(0, m_column->rowCount() - 1); }

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
        void rulesChanged();
        void rowAppended();
        void rowRemoved(int index);

    private slots:
        void onAddClicked()
        {
            appendRowInternal(QString());
            updateGeometry();
            emit rulesChanged();
            emit rowAppended();
        }

        void onRowDeleteRequested()
        {
            auto *row = qobject_cast<DistributeRuleRow *>(sender());
            if (!row) {
                return;
            }
            const int index = indexOfRow(row);
            if (index < 0 || m_column->rowCount() <= m_minimumRows) {
                return;
            }
            m_column->removeRow(index);
            updateGeometry();
            emit rulesChanged();
            emit rowRemoved(index);
        }

    private:
        DistributeRuleRow *rowAt(int index) const
        {
            return qobject_cast<DistributeRuleRow *>(m_column->rowContent(index));
        }

        int indexOfRow(DistributeRuleRow *row) const
        {
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (m_column->rowContent(i) == row) {
                    return i;
                }
            }
            return -1;
        }

        void appendRowInternal(const QString &condition)
        {
            auto *row = new DistributeRuleRow(m_rowHeight, m_placeholder, m_column);
            row->setCondition(condition);
            connect(row, &DistributeRuleRow::conditionChanged, this, &DistributeInterface::rulesChanged);
            connect(row, &DistributeRuleRow::deleteRequested, this, &DistributeInterface::onRowDeleteRequested);
            m_column->appendRow(row);
        }

        PortAlignedColumn *m_column = nullptr;
        QPushButton *m_addButton = nullptr;
        QString m_placeholder;
        int m_rowHeight = 23;
        int m_minimumRows = 1;
    };
}
