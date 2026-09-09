#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSpacerItem>
#include <QtMath>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "LFOWaveMath.hpp"

namespace Nodes
{
    /** 内嵌波形预览：随参数刷新，可横向拖相位、纵向拖偏移 */
    class LFOWavePreview : public QWidget
    {
        Q_OBJECT
    public:
        explicit LFOWavePreview(QWidget* parent = nullptr)
            : QWidget(parent)
        {
            setMinimumHeight(110);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            setCursor(Qt::CrossCursor);
            setToolTip(QStringLiteral("横向拖动调整相位，纵向拖动调整偏移"));
        }

        void setWave(int method,
                     double period,
                     double amplitude,
                     double offset,
                     double phase,
                     double bias,
                     double sampleRate)
        {
            m_method = static_cast<LFOWaveType>(method);
            m_period = qMax(0.01, period);
            m_amplitude = amplitude;
            m_offset = offset;
            m_phase = phase;
            m_bias = bias;
            m_sampleRate = qMax(0.1, sampleRate);
            update();
        }

        void setPlayhead(double time, bool visible)
        {
            m_time = time;
            m_showPlayhead = visible;
            update();
        }

    signals:
        void phaseEdited(double phase);
        void offsetEdited(double offset);

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, true);

            const QRectF r = rect().adjusted(1.5, 1.5, -1.5, -1.5);
            p.fillRect(rect(), QColor(40, 42, 46));
            p.setPen(QPen(QColor(70, 74, 80), 1));
            p.drawRect(r);

            const double padL = 4.0;
            const double padR = 4.0;
            const double padT = 6.0;
            const double padB = 6.0;
            const QRectF plot(r.left() + padL, r.top() + padT,
                              r.width() - padL - padR, r.height() - padT - padB);
            if (plot.width() < 8 || plot.height() < 8) {
                return;
            }

            double yMin = 0.0;
            double yMax = 0.0;
            if (lfoWaveIsBipolar(m_method)) {
                const double a = qAbs(m_amplitude);
                yMin = m_offset - a;
                yMax = m_offset + a;
            } else {
                // 单极性：允许负振幅做垂直翻转
                yMin = m_offset;
                yMax = m_offset + m_amplitude;
            }
            if (yMax < yMin) {
                std::swap(yMin, yMax);
            }
            const double ySpan = qMax(1e-6, yMax - yMin);

            auto toY = [&](double v) {
                return plot.bottom() - (v - yMin) / ySpan * plot.height();
            };

            p.setPen(QPen(QColor(100, 140, 180), 1, Qt::DashLine));
            p.drawLine(QPointF(plot.left(), plot.top()), QPointF(plot.left(), plot.bottom()));

            p.setPen(QPen(QColor(58, 62, 68), 1, Qt::DotLine));
            const double mid = lfoWaveIsBipolar(m_method) ? m_offset : (m_offset + 0.5 * m_amplitude);
            p.drawLine(QPointF(plot.left(), toY(mid)), QPointF(plot.right(), toY(mid)));

            const int samples = qMax(64, int(plot.width()));
            const double pulseW = qBound(1.0 / samples, 1.0 / (m_period * m_sampleRate), 0.25);
            QPainterPath path;
            for (int i = 0; i <= samples; ++i) {
                const double tNorm = double(i) / double(samples);
                const double t = tNorm * m_period;
                const double v = lfoSample(m_method, m_period, t, m_phase,
                                          m_amplitude, m_offset, m_bias, pulseW);
                const double x = plot.left() + tNorm * plot.width();
                const double y = toY(v);
                if (i == 0) {
                    path.moveTo(x, y);
                } else {
                    path.lineTo(x, y);
                }
            }

            p.setPen(QPen(QColor(120, 200, 120), 1.8));
            p.drawPath(path);

            if (m_showPlayhead && m_period > 0.0) {
                double phaseInView = std::fmod(m_time / m_period, 1.0);
                if (phaseInView < 0.0) {
                    phaseInView += 1.0;
                }
                const double x = plot.left() + phaseInView * plot.width();
                p.setPen(QPen(QColor(255, 196, 72), 1.2));
                p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
                const double v = lfoSample(m_method, m_period, m_time, m_phase,
                                           m_amplitude, m_offset, m_bias, pulseW);
                p.setBrush(QColor(255, 196, 72));
                p.setPen(Qt::NoPen);
                p.drawEllipse(QPointF(x, toY(v)), 3.0, 3.0);
            }

            p.setPen(QColor(160, 164, 170));
            QFont f = p.font();
            f.setPointSize(7);
            p.setFont(f);
            p.drawText(QRectF(plot.left(), plot.top(), plot.width(), 12),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QString::number(yMax, 'f', 2));
            p.drawText(QRectF(plot.left(), plot.bottom() - 12, plot.width(), 12),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QString::number(yMin, 'f', 2));
        }

        void mousePressEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::LeftButton) {
                m_dragging = true;
                m_lastPos = event->pos();
                event->accept();
                return;
            }
            QWidget::mousePressEvent(event);
        }

        void mouseMoveEvent(QMouseEvent* event) override
        {
            if (!m_dragging) {
                QWidget::mouseMoveEvent(event);
                return;
            }

            const QPoint delta = event->pos() - m_lastPos;
            m_lastPos = event->pos();

            if (qAbs(delta.x()) >= qAbs(delta.y())) {
                const double dPhase = double(delta.x()) / qMax(1.0, double(width()));
                double next = m_phase + dPhase;
                next = next - std::floor(next);
                m_phase = next;
                Q_EMIT phaseEdited(m_phase);
            } else {
                const double ySpan = qMax(1.0, 2.0 * qAbs(m_amplitude));
                const double dOffset = -double(delta.y()) / qMax(1.0, double(height())) * ySpan;
                m_offset += dOffset;
                Q_EMIT offsetEdited(m_offset);
            }
            update();
            event->accept();
        }

        void mouseReleaseEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::LeftButton) {
                m_dragging = false;
                event->accept();
                return;
            }
            QWidget::mouseReleaseEvent(event);
        }

    private:
        LFOWaveType m_method = LFOWaveType::Sine;
        double m_period = 1.0;
        double m_amplitude = 10.0;
        double m_offset = 0.0;
        double m_phase = 0.0;
        double m_bias = 0.0;
        double m_sampleRate = 10.0;
        double m_time = 0.0;
        bool m_showPlayhead = false;
        bool m_dragging = false;
        QPoint m_lastPos;
    };

    class LFOInterface : public QFrame
    {
    public:
        explicit LFOInterface(QWidget* parent = nullptr)
            : QFrame(parent)
        {
            preview = new LFOWavePreview(this);
            method = new QComboBox(this);
            period = new FloatDragValueWidget(this);
            amplitude = new FloatDragValueWidget(this);
            offset = new FloatDragValueWidget(this);
            bias = new FloatDragValueWidget(this);
            phase = new FloatDragValueWidget(this);
            sampleRate = new FloatDragValueWidget(this);
            loop = new QCheckBox(QStringLiteral("限制周期"), this);
            loopCount = new FloatDragValueWidget(this);
            play = new QPushButton(QStringLiteral("Play"), this);
            play->setCheckable(true);
            play->setChecked(false);
            reset = new QPushButton(QStringLiteral("Reset"), this);

            // 顺序对齐 TouchDesigner LFO CHOP
            method->addItem(QStringLiteral("Sine"), static_cast<int>(LFOWaveType::Sine));
            method->addItem(QStringLiteral("Gaussian"), static_cast<int>(LFOWaveType::Gaussian));
            method->addItem(QStringLiteral("Triangle"), static_cast<int>(LFOWaveType::Triangle));
            method->addItem(QStringLiteral("Ramp"), static_cast<int>(LFOWaveType::Ramp));
            method->addItem(QStringLiteral("Square"), static_cast<int>(LFOWaveType::Square));
            method->addItem(QStringLiteral("Pulse"), static_cast<int>(LFOWaveType::Pulse));

            period->setRange(0.01, 3600.0);
            period->setValue(1.0);

            amplitude->setRange(-10000.0, 10000.0);
            amplitude->setValue(10.0);

            offset->setRange(-10000.0, 10000.0);
            offset->setValue(0.0);

            bias->setRange(-1.0, 1.0);
            bias->setValue(0.0);

            phase->setRange(0.0, 1.0);
            phase->setValue(0.0);

            sampleRate->setRange(0.1, 1000.0);
            sampleRate->setValue(10.0);

            loop->setChecked(false);
            loopCount->setRange(0.01, 100000.0);
            loopCount->setValue(1.0);
            loopCount->setSuffix(QStringLiteral(" 周期"));
            loopCount->setEnabled(false);

            connect(loop, &QCheckBox::toggled, loopCount, &QWidget::setEnabled);

            auto refreshPreview = [this]() {
                preview->setWave(method->currentData().toInt(),
                                 period->value(),
                                 amplitude->value(),
                                 offset->value(),
                                 phase->value(),
                                 bias->value(),
                                 sampleRate->value());
            };

            auto updateBiasEnabled = [this]() {
                const auto t = static_cast<LFOWaveType>(method->currentData().toInt());
                const bool needBias = (t == LFOWaveType::Triangle
                                       || t == LFOWaveType::Square
                                       || t == LFOWaveType::Gaussian);
                bias->setEnabled(needBias);
            };

            connect(method, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [refreshPreview, updateBiasEnabled](int) {
                        updateBiasEnabled();
                        refreshPreview();
                    });
            connect(period, &FloatDragValueWidget::valueChanged, this, [refreshPreview](double) { refreshPreview(); });
            connect(amplitude, &FloatDragValueWidget::valueChanged, this, [refreshPreview](double) { refreshPreview(); });
            connect(offset, &FloatDragValueWidget::valueChanged, this, [refreshPreview](double) { refreshPreview(); });
            connect(bias, &FloatDragValueWidget::valueChanged, this, [refreshPreview](double) { refreshPreview(); });
            connect(phase, &FloatDragValueWidget::valueChanged, this, [refreshPreview](double) { refreshPreview(); });
            connect(sampleRate, &FloatDragValueWidget::valueChanged, this, [refreshPreview](double) { refreshPreview(); });

            connect(preview, &LFOWavePreview::phaseEdited, this, [this](double v) {
                phase->setValue(v);
            });
            connect(preview, &LFOWavePreview::offsetEdited, this, [this](double v) {
                offset->setValue(v);
            });

            auto* layout = new QGridLayout();
            layout->setContentsMargins(4, 2, 4, 4);
            layout->setHorizontalSpacing(6);
            layout->setVerticalSpacing(4);

            int row = 0;
            layout->addWidget(preview, row++, 0, 1, 3);

            layout->addWidget(new QLabel(QStringLiteral("波形"), this), row, 0);
            layout->addWidget(method, row++, 1, 1, 2);

            layout->addWidget(new QLabel(QStringLiteral("周期 (s)"), this), row, 0);
            layout->addWidget(period, row++, 1, 1, 2);

            layout->addWidget(new QLabel(QStringLiteral("振幅"), this), row, 0);
            layout->addWidget(amplitude, row++, 1, 1, 2);

            layout->addWidget(new QLabel(QStringLiteral("偏移"), this), row, 0);
            layout->addWidget(offset, row++, 1, 1, 2);

            layout->addWidget(new QLabel(QStringLiteral("偏置"), this), row, 0);
            layout->addWidget(bias, row++, 1, 1, 2);

            layout->addWidget(new QLabel(QStringLiteral("相位"), this), row, 0);
            layout->addWidget(phase, row++, 1, 1, 2);

            layout->addWidget(new QLabel(QStringLiteral("采样率"), this), row, 0);
            layout->addWidget(sampleRate, row++, 1, 1, 2);

            layout->addWidget(loop, row, 0);
            layout->addWidget(loopCount, row++, 1, 1, 2);

            auto* btnRow = new QHBoxLayout();
            btnRow->setContentsMargins(0, 0, 0, 0);
            btnRow->setSpacing(4);
            btnRow->addWidget(play);
            btnRow->addWidget(reset);
            layout->addLayout(btnRow, row++, 0, 1, 3);

            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), row, 0, 1, 3);
            layout->setRowStretch(row, 1);
            layout->setColumnStretch(2, 1);

            setLayout(layout);
            setMinimumSize(240, 350);
            updateBiasEnabled();
            refreshPreview();
        }

        LFOWavePreview* preview = nullptr;
        QComboBox* method = nullptr;
        FloatDragValueWidget* period = nullptr;
        FloatDragValueWidget* amplitude = nullptr;
        FloatDragValueWidget* offset = nullptr;
        FloatDragValueWidget* bias = nullptr;
        FloatDragValueWidget* phase = nullptr;
        FloatDragValueWidget* sampleRate = nullptr;
        QCheckBox* loop = nullptr;
        FloatDragValueWidget* loopCount = nullptr;
        QPushButton* play = nullptr;
        QPushButton* reset = nullptr;
    };
}
