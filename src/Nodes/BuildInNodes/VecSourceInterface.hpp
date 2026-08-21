#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVariantList>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * 可变长度向量编辑，视觉对齐 VectorDragValueWidget。
     */
    class VecSourceInterface : public QFrame
    {
        Q_OBJECT
    public:
        static constexpr int kMinSize = 1;
        static constexpr int kMaxSize = 16;

        explicit VecSourceInterface(QWidget *parent = nullptr)
            : QFrame(parent)
        {
            setFrameShape(QFrame::NoFrame);
            setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

            sizeSpin->setRange(kMinSize, kMaxSize);
            sizeSpin->setValue(3);
            sizeSpin->setToolTip(QStringLiteral("分量个数"));
            sizeSpin->setFixedHeight(25);

            auto *sizeLabel = new QLabel(QStringLiteral("N"), this);
            sizeLabel->setFixedWidth(14);
            sizeLabel->setAlignment(Qt::AlignCenter);
            sizeLabel->setStyleSheet(QStringLiteral("color:#95a5a6;font-weight:600;"));

            auto *sizeRow = new QWidget(this);
            auto *sizeLayout = new QHBoxLayout(sizeRow);
            sizeLayout->setContentsMargins(0, 0, 0, 0);
            sizeLayout->setSpacing(4);
            sizeLayout->addWidget(sizeLabel);
            sizeLayout->addWidget(sizeSpin, 1);

            listLayout->setContentsMargins(0, 0, 0, 0);
            listLayout->setSpacing(2);

            main_layout->setContentsMargins(0, 0, 0, 0);
            main_layout->setSpacing(2);
            main_layout->addWidget(sizeRow);
            main_layout->addLayout(listLayout);
            setLayout(main_layout);
            setFixedWidth(132);

            connect(sizeSpin, &IntDragValueWidget::valueChanged,
                    this, &VecSourceInterface::onSizeEdited);

            rebuildFields(3, QVector<float>(3, 0.0f));
        }

        int componentCount() const { return m_fields.size(); }

        QVector<float> values() const
        {
            QVector<float> out;
            out.reserve(m_fields.size());
            for (auto *field : m_fields) {
                out.push_back(field ? float(field->value()) : 0.0f);
            }
            return out;
        }

        /** 作为 VariableData.default 的主载荷：float 列表。 */
        QVariantList toVariantList() const
        {
            QVariantList list;
            list.reserve(m_fields.size());
            for (auto *field : m_fields) {
                list.append(field ? field->value() : 0.0);
            }
            return list;
        }

        void setValues(const QVector<float> &values, bool emitChange = false)
        {
            const int n = qBound(kMinSize, values.size(), kMaxSize);
            if (n != m_fields.size()) {
                rebuildFields(n, values);
            } else {
                for (int i = 0; i < n; ++i) {
                    if (!m_fields[i]) {
                        continue;
                    }
                    const QSignalBlocker fb(m_fields[i]);
                    m_fields[i]->setValue(values[i]);
                }
            }
            if (emitChange) {
                Q_EMIT valuesChanged(this->values());
            }
        }

        void setComponentCount(int count)
        {
            count = qBound(kMinSize, count, kMaxSize);
            if (count == m_fields.size()) {
                return;
            }
            QVector<float> next = values();
            next.resize(count);
            rebuildFields(count, next);
            Q_EMIT sizeChanged(count);
            Q_EMIT valuesChanged(values());
        }

        QSize sizeHint() const override
        {
            return QSize(132, 27 * (1 + m_fields.size()));
        }

        QSize minimumSizeHint() const override
        {
            return sizeHint();
        }

    signals:
        void sizeChanged(int size);
        void valuesChanged(const QVector<float> &values);

    private slots:
        void onSizeEdited(int count)
        {
            setComponentCount(count);
        }

        void onFieldChanged()
        {
            Q_EMIT valuesChanged(values());
        }

    private:
        void rebuildFields(int count, const QVector<float> &values)
        {
            const QSignalBlocker blocker(this);
            m_fields.clear();

            while (QLayoutItem *item = listLayout->takeAt(0)) {
                delete item->widget();
                delete item;
            }

            m_fields.reserve(count);
            for (int i = 0; i < count; ++i) {
                auto *row = new QWidget(this);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(4);

                auto *label = new QLabel(QString::number(i), row);
                label->setFixedWidth(14);
                label->setAlignment(Qt::AlignCenter);
                label->setStyleSheet(QStringLiteral("color:#95a5a6;font-weight:600;"));

                auto *field = new FloatDragValueWidget(row);
                field->setFixedHeight(25);
                field->setSingleStep(0.01);
                field->setDecimals(3);
                field->setValue(i < values.size() ? double(values[i]) : 0.0);
                connect(field, &FloatDragValueWidget::valueChanged,
                        this, &VecSourceInterface::onFieldChanged);

                rowLayout->addWidget(label);
                rowLayout->addWidget(field, 1);
                listLayout->addWidget(row);
                m_fields.push_back(field);
            }

            {
                const QSignalBlocker sb(sizeSpin);
                sizeSpin->setValue(count);
            }
            updateGeometry();
        }

        QVBoxLayout *main_layout = new QVBoxLayout(this);
        QVBoxLayout *listLayout = new QVBoxLayout();
        IntDragValueWidget *sizeSpin = new IntDragValueWidget(this);
        QVector<FloatDragValueWidget *> m_fields;
    };
}
