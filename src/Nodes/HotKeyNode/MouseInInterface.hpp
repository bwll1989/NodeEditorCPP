#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QCursor>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QScreen>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTimer>
#include <QVariantList>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Nodes
{
    /**
     * Mouse In 界面：监视鼠标位置与左/右键（不消费事件）。
     */
    class MouseInInterface : public QFrame
    {
        Q_OBJECT
    public:
        enum CoordMode {
            Absolute = 0,   // 屏幕像素
            Normalized = 1  // 主屏 0–1
        };

        enum OutPort : int {
            Tx = 0,
            Ty,
            Left,
            Right,
            Pos,
            PortCount
        };

        explicit MouseInInterface(QWidget *parent = nullptr)
            : QFrame(parent)
        {
            activeCheck->setChecked(true);
            activeCheck->setText(QStringLiteral("Active"));
            activeCheck->setToolTip(QStringLiteral("开启后监视鼠标"));

            coordCombo->addItem(QStringLiteral("Absolute"), Absolute);
            coordCombo->addItem(QStringLiteral("Normalized"), Normalized);
            coordCombo->setToolTip(QStringLiteral("坐标：像素 / 主屏归一化 0–1"));

            setupValueWidget(txSpin, 2);
            setupValueWidget(tySpin, 2);
            setupValueWidget(leftSpin, 0);
            setupValueWidget(rightSpin, 0);
            leftSpin->setRange(0.0, 1.0);
            rightSpin->setRange(0.0, 1.0);

            int row = 0;
            addValueRow(row++, QStringLiteral("tx"), txSpin);
            addValueRow(row++, QStringLiteral("ty"), tySpin);
            addValueRow(row++, QStringLiteral("left"), leftSpin);
            addValueRow(row++, QStringLiteral("right"), rightSpin);

            main_layout->addWidget(activeCheck, row++, 0, 1, 2);
            main_layout->addWidget(new QLabel(QStringLiteral("Coord"), this), row, 0);
            main_layout->addWidget(coordCombo, row, 1);

            main_layout->setContentsMargins(4, 2, 4, 4);
            main_layout->setHorizontalSpacing(4);
            main_layout->setVerticalSpacing(2);
            main_layout->setColumnStretch(1, 1);
            setLayout(main_layout);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            setFixedSize(sizeHint());

            pollTimer->setInterval(16);
            connect(pollTimer, &QTimer::timeout, this, &MouseInInterface::pollMouse);
            connect(activeCheck, &QCheckBox::toggled, this, &MouseInInterface::onActiveToggled);
            connect(coordCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int) {
                        applyCoordDecimals();
                        pollMouse();
                    });

            applyCoordDecimals();

            if (activeCheck->isChecked()) {
                pollTimer->start();
            }

            pollMouse();
        }

        QSize sizeHint() const override { return QSize(168, 180); }
        QSize minimumSizeHint() const override { return sizeHint(); }

        bool isActive() const { return activeCheck->isChecked(); }

        CoordMode coordMode() const
        {
            return static_cast<CoordMode>(coordCombo->currentData().toInt());
        }

        void setCoordMode(int mode)
        {
            const int idx = coordCombo->findData(mode);
            if (idx >= 0) {
                coordCombo->setCurrentIndex(idx);
            }
        }

        double valueAt(int port) const
        {
            switch (port) {
            case Tx: return m_tx;
            case Ty: return m_ty;
            case Left: return m_left ? 1.0 : 0.0;
            case Right: return m_right ? 1.0 : 0.0;
            default: return 0.0;
            }
        }

        /** pos 口主载荷：VariableData.default = [tx, ty] */
        QVariantList posList() const
        {
            return {m_tx, m_ty};
        }

    signals:
        void mouseUpdated(int port);
        void mouseUpdatedAll();

    private slots:
        void onActiveToggled(bool active)
        {
            if (active) {
                pollTimer->start();
                pollMouse();
            } else {
                pollTimer->stop();
            }
        }

        void pollMouse()
        {
            if (!activeCheck->isChecked()) {
                return;
            }

            const QPoint pos = QCursor::pos();
            double tx = pos.x();
            double ty = pos.y();

            if (coordMode() == Normalized) {
                QScreen *screen = QGuiApplication::screenAt(pos);
                if (!screen) {
                    screen = QGuiApplication::primaryScreen();
                }
                if (screen) {
                    const QRect g = screen->geometry();
                    tx = g.width() > 0 ? (pos.x() - g.x()) / double(g.width()) : 0.0;
                    ty = g.height() > 0 ? (pos.y() - g.y()) / double(g.height()) : 0.0;
                }
            }

            bool left = false;
            bool right = false;
#ifdef Q_OS_WIN
            left = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            right = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
#else
            const Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
            left = buttons.testFlag(Qt::LeftButton);
            right = buttons.testFlag(Qt::RightButton);
#endif

            bool any = false;
            if (!qFuzzyCompare(m_tx + 1.0, tx + 1.0)) {
                m_tx = tx;
                any = true;
                Q_EMIT mouseUpdated(Tx);
            }
            if (!qFuzzyCompare(m_ty + 1.0, ty + 1.0)) {
                m_ty = ty;
                any = true;
                Q_EMIT mouseUpdated(Ty);
            }
            if (any) {
                Q_EMIT mouseUpdated(Pos);
            }
            if (m_left != left) {
                m_left = left;
                any = true;
                Q_EMIT mouseUpdated(Left);
            }
            if (m_right != right) {
                m_right = right;
                any = true;
                Q_EMIT mouseUpdated(Right);
            }

            if (any) {
                updateDisplays();
                Q_EMIT mouseUpdatedAll();
            }
        }

    private:
        static void setupValueWidget(FloatDragValueWidget *w, int decimals)
        {
            w->setFixedHeight(25);
            w->setDecimals(decimals);
            w->setSingleStep(decimals == 0 ? 1.0 : 0.01);
            w->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            w->setFocusPolicy(Qt::NoFocus);
        }

        void addValueRow(int row, const QString &caption, FloatDragValueWidget *spin)
        {
            auto *label = new QLabel(caption, this);
            label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            main_layout->addWidget(label, row, 0);
            main_layout->addWidget(spin, row, 1);
        }

        void applyCoordDecimals()
        {
            const int decimals = coordMode() == Normalized ? 4 : 0;
            txSpin->setDecimals(decimals);
            tySpin->setDecimals(decimals);
            txSpin->setSingleStep(decimals == 0 ? 1.0 : 0.001);
            tySpin->setSingleStep(decimals == 0 ? 1.0 : 0.001);
            updateDisplays();
        }

        void updateDisplays()
        {
            const QSignalBlocker b0(txSpin);
            const QSignalBlocker b1(tySpin);
            const QSignalBlocker b2(leftSpin);
            const QSignalBlocker b3(rightSpin);
            txSpin->setValue(m_tx);
            tySpin->setValue(m_ty);
            leftSpin->setValue(m_left ? 1.0 : 0.0);
            rightSpin->setValue(m_right ? 1.0 : 0.0);
        }

    public:
        QGridLayout *main_layout = new QGridLayout(this);
        FloatDragValueWidget *txSpin = new FloatDragValueWidget(this);
        FloatDragValueWidget *tySpin = new FloatDragValueWidget(this);
        FloatDragValueWidget *leftSpin = new FloatDragValueWidget(this);
        FloatDragValueWidget *rightSpin = new FloatDragValueWidget(this);
        QCheckBox *activeCheck = new QCheckBox(this);
        QComboBox *coordCombo = new QComboBox(this);
        QTimer *pollTimer = new QTimer(this);

    private:
        double m_tx = 0.0;
        double m_ty = 0.0;
        bool m_left = false;
        bool m_right = false;
    };
}
