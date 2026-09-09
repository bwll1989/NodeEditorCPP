#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>

#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief Nav Missions 面板：编辑 Navigate / Wait / Action 步骤
     * 到点判定在 Nav；本面板不配置到达距离。
     */
    class MissionSequencerInterface : public QWidget
    {
        Q_OBJECT

    public:
        explicit MissionSequencerInterface(QWidget *parent = nullptr)
        {
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(6);

            typeCombo->addItem(QStringLiteral("导航 Navigate"), QStringLiteral("navigate"));
            typeCombo->addItem(QStringLiteral("等待 Wait"), QStringLiteral("wait"));
            typeCombo->addItem(QStringLiteral("动作 Action"), QStringLiteral("action"));

            goalXSpin->setRange(-1000.0, 1000.0);
            goalXSpin->setDecimals(3);
            goalXSpin->setSingleStep(0.1);
            goalXSpin->setSuffix(QStringLiteral(" m"));
            goalXSpin->setValue(0.0);

            goalYSpin->setRange(-1000.0, 1000.0);
            goalYSpin->setDecimals(3);
            goalYSpin->setSingleStep(0.1);
            goalYSpin->setSuffix(QStringLiteral(" m"));
            goalYSpin->setValue(0.0);

            goalYawSpin->setRange(-3.1416, 3.1416);
            goalYawSpin->setDecimals(3);
            goalYawSpin->setSingleStep(0.05);
            goalYawSpin->setSuffix(QStringLiteral(" rad"));
            goalYawSpin->setValue(0.0);

            waitSpin->setRange(0.0, 3600.0);
            waitSpin->setDecimals(2);
            waitSpin->setSingleStep(0.5);
            waitSpin->setSuffix(QStringLiteral(" s"));
            waitSpin->setValue(2.0);

            actionEdit->setPlaceholderText(QStringLiteral("动作名，如 open_gripper"));
            paramsEdit->setPlaceholderText(QStringLiteral("可选；写入 ACTION.params（文本/数字/JSON）"));

            auto *btnRow = new QHBoxLayout();
            btnRow->addWidget(addButton);
            btnRow->addWidget(applyButton);
            btnRow->addWidget(removeButton);
            btnRow->addWidget(upButton);
            btnRow->addWidget(downButton);

            int row = 0;
            formLayout->addWidget(new QLabel(QStringLiteral("类型")), row, 0);
            formLayout->addWidget(typeCombo, row++, 1);
            formLayout->addWidget(new QLabel(QStringLiteral("备注")), row, 0);
            formLayout->addWidget(labelEdit, row++, 1);
            formLayout->addWidget(new QLabel(QStringLiteral("目标 X")), row, 0);
            formLayout->addWidget(goalXSpin, row++, 1);
            formLayout->addWidget(new QLabel(QStringLiteral("目标 Y")), row, 0);
            formLayout->addWidget(goalYSpin, row++, 1);
            formLayout->addWidget(new QLabel(QStringLiteral("目标航向")), row, 0);
            formLayout->addWidget(goalYawSpin, row++, 1);
            formLayout->addWidget(new QLabel(QStringLiteral("等待")), row, 0);
            formLayout->addWidget(waitSpin, row++, 1);
            formLayout->addWidget(new QLabel(QStringLiteral("动作")), row, 0);
            formLayout->addWidget(actionEdit, row++, 1);
            formLayout->addWidget(new QLabel(QStringLiteral("参数")), row, 0);
            formLayout->addWidget(paramsEdit, row++, 1);

            mainLayout->addWidget(stepList);
            mainLayout->addLayout(formLayout);
            mainLayout->addLayout(btnRow);
            mainLayout->addWidget(statusLabel);

            statusLabel->setWordWrap(true);
            statusLabel->setText(QStringLiteral("状态：空闲"));
            stepList->setMinimumHeight(120);

            connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &MissionSequencerInterface::updateEditorVisibility);
            connect(stepList, &QListWidget::currentRowChanged,
                    this, &MissionSequencerInterface::onSelectionChanged);
            connect(addButton, &QPushButton::clicked, this, &MissionSequencerInterface::onAdd);
            connect(applyButton, &QPushButton::clicked, this, &MissionSequencerInterface::onApply);
            connect(removeButton, &QPushButton::clicked, this, &MissionSequencerInterface::onRemove);
            connect(upButton, &QPushButton::clicked, this, &MissionSequencerInterface::onMoveUp);
            connect(downButton, &QPushButton::clicked, this, &MissionSequencerInterface::onMoveDown);

            updateEditorVisibility();
            setLayout(mainLayout);
        }

        QJsonArray steps() const { return m_steps; }

        void setSteps(const QJsonArray &steps)
        {
            m_steps = normalizeSteps(steps);
            rebuildList(-1);
            emit stepsChanged();
        }

        void setStatusText(const QString &text) { statusLabel->setText(text); }

        void setCurrentStepHighlight(int index)
        {
            const QSignalBlocker b(stepList);
            if (index >= 0 && index < stepList->count()) {
                stepList->setCurrentRow(index);
            } else {
                stepList->clearSelection();
            }
        }

    signals:
        void stepsChanged();

    private:
        /// 兼容旧版 type=emit / 字段 data
        static QJsonObject normalizeStep(QJsonObject step)
        {
            QString type = step.value(QStringLiteral("type")).toString(QStringLiteral("navigate"));
            if (type == QStringLiteral("emit")) {
                type = QStringLiteral("action");
                step.insert(QStringLiteral("type"), type);
            }
            if (type == QStringLiteral("action")) {
                if (!step.contains(QStringLiteral("action")) && step.contains(QStringLiteral("data"))) {
                    step.insert(QStringLiteral("action"), step.value(QStringLiteral("data")).toString());
                }
            }
            return step;
        }

        static QJsonArray normalizeSteps(const QJsonArray &steps)
        {
            QJsonArray out;
            for (const QJsonValue &v : steps) {
                out.append(normalizeStep(v.toObject()));
            }
            return out;
        }

        static QString stepSummary(const QJsonObject &step, int index)
        {
            const QJsonObject s = normalizeStep(step);
            const QString type = s.value(QStringLiteral("type")).toString(QStringLiteral("navigate"));
            const QString label = s.value(QStringLiteral("label")).toString();
            QString body;
            if (type == QStringLiteral("navigate")) {
                body = QStringLiteral("导航 (%1, %2, %3 rad)")
                           .arg(s.value(QStringLiteral("x")).toDouble(), 0, 'f', 3)
                           .arg(s.value(QStringLiteral("y")).toDouble(), 0, 'f', 3)
                           .arg(s.value(QStringLiteral("yaw")).toDouble(), 0, 'f', 3);
            } else if (type == QStringLiteral("wait")) {
                body = QStringLiteral("等待 %1 s")
                           .arg(s.value(QStringLiteral("seconds")).toDouble(1.0), 0, 'f', 2);
            } else {
                const QString act = s.value(QStringLiteral("action")).toString();
                body = act.isEmpty() ? QStringLiteral("动作") : QStringLiteral("动作 %1").arg(act);
            }
            if (!label.isEmpty()) {
                body += QStringLiteral(" — ") + label;
            }
            return QStringLiteral("%1. %2").arg(index).arg(body);
        }

        QJsonObject readEditorStep() const
        {
            QJsonObject step;
            const QString type = typeCombo->currentData().toString();
            step.insert(QStringLiteral("type"), type);
            step.insert(QStringLiteral("label"), labelEdit->text().trimmed());
            if (type == QStringLiteral("navigate")) {
                step.insert(QStringLiteral("x"), goalXSpin->value());
                step.insert(QStringLiteral("y"), goalYSpin->value());
                step.insert(QStringLiteral("yaw"), goalYawSpin->value());
            } else if (type == QStringLiteral("wait")) {
                step.insert(QStringLiteral("seconds"), waitSpin->value());
            } else {
                step.insert(QStringLiteral("action"), actionEdit->text().trimmed());
                step.insert(QStringLiteral("params"), paramsEdit->text());
            }
            return step;
        }

        void writeEditorStep(const QJsonObject &raw)
        {
            const QJsonObject step = normalizeStep(raw);
            const QSignalBlocker b1(typeCombo);
            const QString type = step.value(QStringLiteral("type")).toString(QStringLiteral("navigate"));
            const int ti = typeCombo->findData(type);
            typeCombo->setCurrentIndex(ti >= 0 ? ti : 0);
            labelEdit->setText(step.value(QStringLiteral("label")).toString());
            goalXSpin->setValue(step.value(QStringLiteral("x")).toDouble());
            goalYSpin->setValue(step.value(QStringLiteral("y")).toDouble());
            goalYawSpin->setValue(step.value(QStringLiteral("yaw")).toDouble());
            waitSpin->setValue(step.value(QStringLiteral("seconds")).toDouble(2.0));
            actionEdit->setText(step.value(QStringLiteral("action")).toString());
            paramsEdit->setText(step.value(QStringLiteral("params")).toString());
            updateEditorVisibility();
        }

        void rebuildList(int selectRow)
        {
            const QSignalBlocker b(stepList);
            stepList->clear();
            for (int i = 0; i < m_steps.size(); ++i) {
                stepList->addItem(stepSummary(m_steps.at(i).toObject(), i));
            }
            if (selectRow >= 0 && selectRow < stepList->count()) {
                stepList->setCurrentRow(selectRow);
            }
        }

        void updateEditorVisibility()
        {
            const QString type = typeCombo->currentData().toString();
            const bool nav = type == QStringLiteral("navigate");
            const bool wait = type == QStringLiteral("wait");
            const bool act = type == QStringLiteral("action");
            goalXSpin->setEnabled(nav);
            goalYSpin->setEnabled(nav);
            goalYawSpin->setEnabled(nav);
            waitSpin->setEnabled(wait);
            actionEdit->setEnabled(act);
            paramsEdit->setEnabled(act);
        }

        void onSelectionChanged(int row)
        {
            if (row < 0 || row >= m_steps.size()) {
                return;
            }
            writeEditorStep(m_steps.at(row).toObject());
        }

        void onAdd()
        {
            m_steps.append(readEditorStep());
            rebuildList(m_steps.size() - 1);
            emit stepsChanged();
        }

        void onApply()
        {
            const int row = stepList->currentRow();
            if (row < 0 || row >= m_steps.size()) {
                return;
            }
            m_steps.replace(row, readEditorStep());
            rebuildList(row);
            emit stepsChanged();
        }

        void onRemove()
        {
            const int row = stepList->currentRow();
            if (row < 0 || row >= m_steps.size()) {
                return;
            }
            m_steps.removeAt(row);
            rebuildList(qMin(row, m_steps.size() - 1));
            emit stepsChanged();
        }

        void onMoveUp()
        {
            const int row = stepList->currentRow();
            if (row <= 0 || row >= m_steps.size()) {
                return;
            }
            const QJsonValue a = m_steps.takeAt(row);
            m_steps.insert(row - 1, a);
            rebuildList(row - 1);
            emit stepsChanged();
        }

        void onMoveDown()
        {
            const int row = stepList->currentRow();
            if (row < 0 || row >= m_steps.size() - 1) {
                return;
            }
            const QJsonValue a = m_steps.takeAt(row);
            m_steps.insert(row + 1, a);
            rebuildList(row + 1);
            emit stepsChanged();
        }

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        QGridLayout *formLayout = new QGridLayout();
        QListWidget *stepList = new QListWidget();
        QComboBox *typeCombo = new QComboBox();
        QLineEdit *labelEdit = new QLineEdit();
        FloatDragValueWidget *goalXSpin = new FloatDragValueWidget();
        FloatDragValueWidget *goalYSpin = new FloatDragValueWidget();
        FloatDragValueWidget *goalYawSpin = new FloatDragValueWidget();
        FloatDragValueWidget *waitSpin = new FloatDragValueWidget();
        QLineEdit *actionEdit = new QLineEdit();
        QLineEdit *paramsEdit = new QLineEdit();
        QPushButton *addButton = new QPushButton(QStringLiteral("添加"));
        QPushButton *applyButton = new QPushButton(QStringLiteral("应用"));
        QPushButton *removeButton = new QPushButton(QStringLiteral("删除"));
        QPushButton *upButton = new QPushButton(QStringLiteral("上移"));
        QPushButton *downButton = new QPushButton(QStringLiteral("下移"));
        QLabel *statusLabel = new QLabel();

        QJsonArray m_steps;
    };
}
