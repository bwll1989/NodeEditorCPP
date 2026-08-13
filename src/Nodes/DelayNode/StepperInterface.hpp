#pragma once

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace Nodes
{
    /**
     * @brief Stepper 节点界面：当前输出端口 index 与 Reset 上下排列
     */
    class StepperInterface : public QWidget
    {
        Q_OBJECT
    public:
        explicit StepperInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_stepLabel(new QLabel(this))
            , m_resetButton(new QPushButton(tr("Reset"), this))
        {
            m_stepLabel->setAlignment(Qt::AlignCenter);
            setStepDisplay(-1);

            auto *mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(4, 4, 4, 4);
            mainLayout->setSpacing(4);
            mainLayout->addWidget(m_stepLabel);
            mainLayout->addWidget(m_resetButton);
            setLayout(mainLayout);

            connect(m_resetButton, &QPushButton::clicked, this, &StepperInterface::resetClicked);
        }

        void setStepDisplay(int portIndex)
        {
            m_stepLabel->setText(QString::number(portIndex));
        }

        QPushButton *resetButton() const { return m_resetButton; }

    signals:
        void resetClicked();

    private:
        QLabel *m_stepLabel = nullptr;
        QPushButton *m_resetButton = nullptr;
    };
}
