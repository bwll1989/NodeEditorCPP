#pragma once

#include "PortAlignedColumn/PortAlignedColumn.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

#include <algorithm>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMap>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

namespace Nodes
{
    /** 延迟规则：输出端口 + 相对 TRIGGER 的绝对毫秒 */
    struct DelayRule
    {
        int port = 0;
        int time = 0;
    };

    /**
     * Delay 单行：左侧删除 + 延迟毫秒。
     * 行索引即输出端口。
     */
    class DelayRuleRow : public QWidget
    {
        Q_OBJECT
    public:
        explicit DelayRuleRow(int rowHeight, QWidget *parent = nullptr)
            : QWidget(parent)
            , m_rowHeight(qMax(1, rowHeight))
        {
            setObjectName(QStringLiteral("DelayRuleRow"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            setFixedHeight(m_rowHeight);
            setMinimumHeight(m_rowHeight);
            setMaximumHeight(m_rowHeight);

            const int inner = qMax(1, m_rowHeight - 2);
            const int icon = qBound(10, 14, inner - 2);

            auto *lay = new QHBoxLayout(this);
            lay->setContentsMargins(2, 0, 2, 0);
            lay->setSpacing(2);

            m_deleteBtn = new QPushButton(this);
            m_deleteBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/remove.png")));
            m_deleteBtn->setIconSize(QSize(icon, icon));
            m_deleteBtn->setFixedSize(inner, inner);
            m_deleteBtn->setFlat(true);
            m_deleteBtn->setFocusPolicy(Qt::NoFocus);
            m_deleteBtn->setCursor(Qt::PointingHandCursor);
            m_deleteBtn->setToolTip(QStringLiteral("删除"));
            connect(m_deleteBtn, &QPushButton::clicked, this, &DelayRuleRow::deleteRequested);

            m_timeEdit = new IntDragValueWidget(this);
            m_timeEdit->setRange(0, 24 * 60 * 60 * 1000);
            m_timeEdit->setSuffix(QStringLiteral(" ms"));
            m_timeEdit->setMinimumHeight(0);
            m_timeEdit->setMaximumHeight(inner);
            m_timeEdit->setFixedHeight(inner);
            m_timeEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            m_timeEdit->setToolTip(QStringLiteral("相对 TRIGGER 的延迟毫秒（拖动调节）"));
            connect(m_timeEdit, &IntDragValueWidget::valueChanged,
                    this, &DelayRuleRow::timeChanged);

            lay->addWidget(m_deleteBtn, 0, Qt::AlignVCenter);
            lay->addWidget(m_timeEdit, 1, Qt::AlignVCenter);
        }

        int timeMs() const { return m_timeEdit ? m_timeEdit->value() : 0; }

        void setTimeMs(int ms)
        {
            if (m_timeEdit) {
                const QSignalBlocker blocker(m_timeEdit);
                m_timeEdit->setValue(qMax(0, ms));
            }
        }

    signals:
        void timeChanged();
        void deleteRequested();

    private:
        QPushButton *m_deleteBtn = nullptr;
        IntDragValueWidget *m_timeEdit = nullptr;
        int m_rowHeight = 23;
    };

    /**
     * Delay 嵌入界面：左侧添加/选项/进度；右侧一行对应一个输出端口。
     */
    class DelayInterface : public QWidget
    {
        Q_OBJECT
    public:
        static constexpr int kEmbeddedWidth = 300;

        explicit DelayInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_column(new PortAlignedColumn(this))
            , m_rowHeight(PortAlignedColumn::rowPitch())
        {
            setObjectName(QStringLiteral("DelayInterface"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumWidth(kEmbeddedWidth);

            m_addButton = new QPushButton(this);
            m_addButton->setIcon(QIcon(QStringLiteral(":/icons/icons/add.png")));
            m_addButton->setIconSize(QSize(14, 14));
            m_addButton->setFlat(true);
            m_addButton->setToolTip(QStringLiteral("添加输出延迟行"));
            connect(m_addButton, &QPushButton::clicked, this, &DelayInterface::onAddClicked);

            m_ignoreRepeatCheckBox = new QCheckBox(QStringLiteral("忽略重复"), this);
            m_ignoreRepeatCheckBox->setChecked(true);
            m_ignoreRepeatCheckBox->setToolTip(
                QStringLiteral("选中：保持期间忽略新信号\n取消：立即停止当前保持并处理新信号"));

            m_progressBar = new QProgressBar(this);
            m_progressBar->setRange(0, 100);
            m_progressBar->setValue(0);
            m_progressBar->setTextVisible(true);
            m_progressBar->setFormat(QStringLiteral("空闲"));
            m_progressBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

            auto *left = new QVBoxLayout();
            left->setContentsMargins(4, 2, 6, 2);
            left->setSpacing(6);
            left->addWidget(m_addButton, 0, Qt::AlignLeft);
            left->addWidget(m_ignoreRepeatCheckBox, 0, Qt::AlignLeft);
            left->addWidget(m_progressBar, 0);
            left->addStretch(1);

            auto *leftWrap = new QWidget(this);
            leftWrap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            leftWrap->setLayout(left);

            auto *root = new QHBoxLayout(this);
            root->setContentsMargins(2, 2, 2, 2);
            root->setSpacing(6);
            root->addWidget(leftWrap, 0);
            root->addWidget(m_column, 1);
            setLayout(root);

            setMinimumRows(1);
            setRowCount(2);
        }

        int rowCount() const { return m_column->rowCount(); }

        bool isIgnoreRepeat() const
        {
            return m_ignoreRepeatCheckBox && m_ignoreRepeatCheckBox->isChecked();
        }

        void setIgnoreRepeat(bool ignore)
        {
            if (m_ignoreRepeatCheckBox) {
                m_ignoreRepeatCheckBox->setChecked(ignore);
            }
        }

        QVector<DelayRule> getDelayMessages() const
        {
            QVector<DelayRule> messages;
            messages.reserve(m_column->rowCount());
            for (int i = 0; i < m_column->rowCount(); ++i) {
                DelayRule rule;
                rule.port = i;
                rule.time = timeAt(i);
                messages.append(rule);
            }
            return messages;
        }

        void setRowCount(int count)
        {
            count = qMax(m_minimumRows, count);
            const QSignalBlocker blocker(this);
            while (m_column->rowCount() < count) {
                appendRowInternal(0);
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

        QJsonObject exportRules() const
        {
            QJsonArray messagesArray;
            for (int i = 0; i < m_column->rowCount(); ++i) {
                QJsonObject obj;
                obj.insert(QStringLiteral("port"), i);
                obj.insert(QStringLiteral("time"), timeAt(i));
                messagesArray.append(obj);
            }
            QJsonObject json;
            json.insert(QStringLiteral("messages"), messagesArray);
            return json;
        }

        void importRules(const QJsonObject &json)
        {
            const QSignalBlocker blocker(this);
            const QJsonArray messagesArray = json.value(QStringLiteral("messages")).toArray();

            QMap<int, int> timeByPort;
            int maxPort = -1;
            int fallback = 0;
            for (const QJsonValue &value : messagesArray) {
                if (!value.isObject()) {
                    continue;
                }
                const QJsonObject obj = value.toObject();
                int port = obj.value(QStringLiteral("port")).toInt(-1);
                if (port < 0) {
                    port = fallback;
                }
                timeByPort.insert(port, qMax(0, obj.value(QStringLiteral("time")).toInt(0)));
                maxPort = qMax(maxPort, port);
                fallback = qMax(fallback, port + 1);
            }

            const int target = qMax(m_minimumRows, maxPort + 1);
            while (m_column->rowCount() < target) {
                appendRowInternal(0);
            }
            while (m_column->rowCount() > target) {
                m_column->removeRow(m_column->rowCount() - 1);
            }
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (auto *row = rowAt(i)) {
                    row->setTimeMs(timeByPort.value(i, 0));
                }
            }
            updateGeometry();
        }

        void setProgressPercent(int percent)
        {
            if (!m_progressBar) {
                return;
            }
            m_progressBar->setFormat(QStringLiteral("%p%"));
            m_progressBar->setValue(std::max(0, std::min(100, percent)));
        }

        void setProgressIdle()
        {
            if (!m_progressBar) {
                return;
            }
            m_progressBar->setValue(0);
            m_progressBar->setFormat(QStringLiteral("空闲"));
        }

        void setProgressStatus(const QString &text, int percent = 0)
        {
            if (!m_progressBar) {
                return;
            }
            m_progressBar->setFormat(text);
            m_progressBar->setValue(std::max(0, std::min(100, percent)));
        }

        QSize sizeHint() const override
        {
            const int rows = qMax(1, m_column->rowCount());
            const int h = qMax(rows * m_rowHeight, m_rowHeight * 3);
            return QSize(kEmbeddedWidth, h);
        }

        QSize minimumSizeHint() const override
        {
            const int rows = qMax(1, m_column->rowCount());
            const int h = qMax(rows * m_rowHeight, m_rowHeight * 3);
            return QSize(kEmbeddedWidth, h);
        }

    signals:
        void listChanged();
        void rowAppended();
        void rowRemoved(int index);

    private slots:
        void onAddClicked()
        {
            appendRowInternal(0);
            updateGeometry();
            Q_EMIT listChanged();
            Q_EMIT rowAppended();
        }

        void onRowDeleteRequested()
        {
            auto *row = qobject_cast<DelayRuleRow *>(sender());
            if (!row) {
                return;
            }
            const int index = indexOfRow(row);
            if (index < 0 || m_column->rowCount() <= m_minimumRows) {
                return;
            }
            m_column->removeRow(index);
            updateGeometry();
            Q_EMIT listChanged();
            Q_EMIT rowRemoved(index);
        }

    private:
        DelayRuleRow *rowAt(int index) const
        {
            return qobject_cast<DelayRuleRow *>(m_column->rowContent(index));
        }

        int indexOfRow(DelayRuleRow *row) const
        {
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (m_column->rowContent(i) == row) {
                    return i;
                }
            }
            return -1;
        }

        int timeAt(int index) const
        {
            if (auto *row = rowAt(index)) {
                return row->timeMs();
            }
            return 0;
        }

        void appendRowInternal(int timeMs)
        {
            auto *row = new DelayRuleRow(m_rowHeight, m_column);
            row->setTimeMs(timeMs);
            connect(row, &DelayRuleRow::timeChanged, this, &DelayInterface::listChanged);
            connect(row, &DelayRuleRow::deleteRequested, this, &DelayInterface::onRowDeleteRequested);
            m_column->appendRow(row);
        }

        PortAlignedColumn *m_column = nullptr;
        QPushButton *m_addButton = nullptr;
        QCheckBox *m_ignoreRepeatCheckBox = nullptr;
        QProgressBar *m_progressBar = nullptr;
        int m_rowHeight = 23;
        int m_minimumRows = 1;
    };
}
