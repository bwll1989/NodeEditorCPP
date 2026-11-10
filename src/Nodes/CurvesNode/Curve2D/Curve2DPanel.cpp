#include "Curve2DPanel.hpp"

#include "Curve2DDataModel.hpp"

#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <cmath>

namespace Nodes
{
    Curve2DPanel::Curve2DPanel(Curve2DDataModel* model, QWidget* parent)
        : QWidget(parent)
        , m_model(model)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(3);

        // 大号时间读数（hh:mm:ss:zzz，随播放推进，上限为一个周期）
        m_timeReadout = new QTimeEdit(this);
        m_timeReadout->setDisplayFormat(QStringLiteral("hh:mm:ss:zzz"));
        m_timeReadout->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_timeReadout->setReadOnly(true);
        m_timeReadout->setAlignment(Qt::AlignCenter);
        QFont timeFont = m_timeReadout->font();
        timeFont.setBold(true);
        timeFont.setPointSize(8);
        m_timeReadout->setFont(timeFont);
        layout->addWidget(m_timeReadout);

        // 进度条：可拖拽定位播放头（播放时跟随移动）
        m_progressSlider = new QSlider(Qt::Horizontal, this);
        m_progressSlider->setRange(0, 10000);
        connect(m_progressSlider, &QSlider::valueChanged, this, [this](int v) {
            m_model->setProgress(v / 10000.0);   // slider int 0~10000 归一化为 0.0~1.0
        });
        layout->addWidget(m_progressSlider);

        auto* controlRow = new QHBoxLayout();
        m_playButton = new QPushButton(QStringLiteral("启动"), this);
        m_playButton->setCheckable(true);
        controlRow->addWidget(m_playButton);
        m_loopBox = new QCheckBox(QStringLiteral("循环"), this);
        controlRow->addWidget(m_loopBox);
        controlRow->addStretch();
        layout->addLayout(controlRow);

        auto* editRow = new QHBoxLayout();
        auto* editButton = new QPushButton(QStringLiteral("编辑曲线"), this);
        editButton->setToolTip(QStringLiteral("打开二维曲线编辑窗口"));
        editRow->addWidget(editButton, 1);
        layout->addLayout(editRow);

        connect(m_playButton, &QPushButton::toggled, m_model, &Curve2DDataModel::setPlay);
        connect(m_loopBox, &QCheckBox::toggled, m_model, &Curve2DDataModel::setLoop);
        connect(editButton, &QPushButton::clicked, m_model, &Curve2DDataModel::openEditor);

        // 模型播放态变化 → 启动按钮勾选与文字（连线/OSC/同步源起停都经 playChanged）
        connect(m_model, &Curve2DDataModel::playChanged, this, [this](bool playing) {
            const QSignalBlocker blocker(m_playButton);
            m_playButton->setChecked(playing);
            m_playButton->setText(playing ? QStringLiteral("停止") : QStringLiteral("启动"));
        });

        // 模型循环态变化 → 循环复选框（load / OSC / 弹窗复选框改动）
        connect(m_model, &Curve2DDataModel::loopChanged, this, [this](bool loop) {
            const QSignalBlocker blocker(m_loopBox);
            m_loopBox->setChecked(loop);
        });
    }

    void Curve2DPanel::setProgressView(double progress)
    {
        if (!m_progressSlider) {
            return;
        }
        const QSignalBlocker blocker(m_progressSlider);
        m_progressSlider->setValue(int(std::round(progress * 10000.0)));
    }

    void Curve2DPanel::setTimeView(double progress, double period)
    {
        if (!m_timeReadout) {
            return;
        }
        const qint64 ms = qint64(progress * period * 1000.0);
        const int clamped = int(qBound(qint64(0), ms, qint64(86399999)));
        const QSignalBlocker blocker(m_timeReadout);
        m_timeReadout->setTime(QTime(0, 0, 0).addMSecs(clamped));
    }
}
