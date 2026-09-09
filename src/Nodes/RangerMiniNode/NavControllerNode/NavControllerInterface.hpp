#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QDialog>
#include <QPointer>
#include <QVector>
#include <QtMath>
#include <algorithm>
#include <cmath>

#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "qcustomplot.h"

namespace Nodes
{
    /**
     * @brief NavController 嵌入式面板：目标点与控制参数
     *
     * 路径预览改在独立弹窗（按钮「路径预览」打开），不挤占节点面板。
     * 图坐标系与控制律一致（开启镜像时已是控制系）。
     */
    class NavControllerInterface : public QWidget
    {
        Q_OBJECT

    public:
        explicit NavControllerInterface(QWidget *parent = nullptr)
        {
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(6);
            mainLayout->setColumnStretch(0, 1);
            mainLayout->setColumnStretch(1, 2);

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

            // 目标航向（rad）：参与 RS 规划与到达判定
            goalYawSpin->setRange(-3.1416, 3.1416);
            goalYawSpin->setDecimals(3);
            goalYawSpin->setSingleStep(0.05);
            goalYawSpin->setSuffix(QStringLiteral(" rad"));
            goalYawSpin->setValue(0.0);

            yawTolSpin->setRange(0.02, 1.0);
            yawTolSpin->setDecimals(3);
            yawTolSpin->setSingleStep(0.02);
            yawTolSpin->setSuffix(QStringLiteral(" rad"));
            yawTolSpin->setValue(0.15);

            lookAheadSpin->setRange(0.15, 3.0);
            lookAheadSpin->setDecimals(3);
            lookAheadSpin->setSingleStep(0.05);
            lookAheadSpin->setSuffix(QStringLiteral(" m"));
            lookAheadSpin->setValue(0.45);

            maxVxSpin->setRange(0.0, 2.0);
            maxVxSpin->setDecimals(3);
            maxVxSpin->setSingleStep(0.05);
            maxVxSpin->setSuffix(QStringLiteral(" m/s"));
            maxVxSpin->setValue(0.300);

            maxSteerSpin->setRange(0.05, 1.571);
            maxSteerSpin->setDecimals(3);
            maxSteerSpin->setSingleStep(0.05);
            maxSteerSpin->setSuffix(QStringLiteral(" rad"));
            maxSteerSpin->setValue(0.500);

            arriveSpin->setRange(0.02, 5.0);
            arriveSpin->setDecimals(3);
            arriveSpin->setSingleStep(0.05);
            arriveSpin->setSuffix(QStringLiteral(" m"));
            arriveSpin->setValue(0.150);

            // SLAM 抖动时：离开到点区需超过 arrive + 滞回，避免 ARRIVED 反复触发
            arriveHystSpin->setRange(0.0, 1.0);
            arriveHystSpin->setDecimals(3);
            arriveHystSpin->setSingleStep(0.01);
            arriveHystSpin->setSuffix(QStringLiteral(" m"));
            arriveHystSpin->setValue(0.050);

            slowDistSpin->setRange(0.05, 10.0);
            slowDistSpin->setDecimals(3);
            slowDistSpin->setSingleStep(0.1);
            slowDistSpin->setSuffix(QStringLiteral(" m"));
            slowDistSpin->setValue(0.800);

            kpSteerSpin->setRange(0.1, 5.0);
            kpSteerSpin->setDecimals(2);
            kpSteerSpin->setSingleStep(0.1);
            kpSteerSpin->setSuffix(QStringLiteral(" 1/s"));
            kpSteerSpin->setValue(1.20);

            // Ranger Mini 3.0 物理轴距 494 mm；控制律内按前后阿克曼自动用 L/2
            wheelbaseSpin->setRange(0.05, 2.0);
            wheelbaseSpin->setDecimals(3);
            wheelbaseSpin->setSingleStep(0.01);
            wheelbaseSpin->setSuffix(QStringLiteral(" m"));
            wheelbaseSpin->setValue(0.494);

            pathPreviewButton->setText(QStringLiteral("路径预览…"));
            connect(pathPreviewButton, &QPushButton::clicked, this, &NavControllerInterface::showPathPreview);

            int row = 0;
            mainLayout->addWidget(goalXLabel, row, 0);
            mainLayout->addWidget(goalXSpin, row++, 1);
            mainLayout->addWidget(goalYLabel, row, 0);
            mainLayout->addWidget(goalYSpin, row++, 1);
            mainLayout->addWidget(goalYawLabel, row, 0);
            mainLayout->addWidget(goalYawSpin, row++, 1);
            mainLayout->addWidget(yawTolLabel, row, 0);
            mainLayout->addWidget(yawTolSpin, row++, 1);
            mainLayout->addWidget(lookAheadLabel, row, 0);
            mainLayout->addWidget(lookAheadSpin, row++, 1);
            mainLayout->addWidget(maxVxLabel, row, 0);
            mainLayout->addWidget(maxVxSpin, row++, 1);
            mainLayout->addWidget(maxSteerLabel, row, 0);
            mainLayout->addWidget(maxSteerSpin, row++, 1);
            mainLayout->addWidget(arriveLabel, row, 0);
            mainLayout->addWidget(arriveSpin, row++, 1);
            mainLayout->addWidget(arriveHystLabel, row, 0);
            mainLayout->addWidget(arriveHystSpin, row++, 1);
            mainLayout->addWidget(slowDistLabel, row, 0);
            mainLayout->addWidget(slowDistSpin, row++, 1);
            mainLayout->addWidget(wheelbaseLabel, row, 0);
            mainLayout->addWidget(wheelbaseSpin, row++, 1);
            mainLayout->addWidget(kpSteerLabel, row, 0);
            mainLayout->addWidget(kpSteerSpin, row++, 1);
            mainLayout->addWidget(mirrorFrameCheck, row++, 0, 1, 2);
            mainLayout->addWidget(pathPreviewButton, row++, 0, 1, 2);
            mainLayout->addWidget(statusLabel, row++, 0, 1, 2);

            statusLabel->setWordWrap(true);
            statusLabel->setText(QStringLiteral("状态：空闲"));
            setLayout(mainLayout);
        }

        ~NavControllerInterface() override
        {
            if (m_pathDialog) {
                m_pathDialog->close();
                delete m_pathDialog;
                m_pathDialog = nullptr;
            }
        }

        void setStatusText(const QString &text)
        {
            statusLabel->setText(text);
        }

        /// 弹窗是否正在显示（未打开时 DataModel 可跳过刷图）
        bool isPathPreviewVisible() const
        {
            return m_pathDialog && m_pathDialog->isVisible();
        }

        /**
         * @brief 刷新 RS 路径预览（控制系 XY）；仅弹窗可见时真正 replot
         */
        void updatePathPlot(const QVector<double> &pathX,
                            const QVector<double> &pathY,
                            const QVector<int> &pathGear,
                            double poseX,
                            double poseY,
                            double poseYaw,
                            double goalX,
                            double goalY,
                            double goalYaw,
                            bool forceRescale)
        {
            // 缓存最新一帧，打开弹窗时立刻画出来
            m_cachePathX = pathX;
            m_cachePathY = pathY;
            m_cachePathGear = pathGear;
            m_cachePoseX = poseX;
            m_cachePoseY = poseY;
            m_cachePoseYaw = poseYaw;
            m_cacheGoalX = goalX;
            m_cacheGoalY = goalY;
            m_cacheGoalYaw = goalYaw;
            if (forceRescale) {
                m_pendingRescale = true;
            }

            if (!isPathPreviewVisible() || !pathPlot || !m_fwdGraph) {
                return;
            }
            applyCachedPlot(forceRescale || m_pendingRescale);
        }

        void clearPathPlot()
        {
            updatePathPlot({}, {}, {}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, true);
        }

        QGridLayout *mainLayout = new QGridLayout(this);
        FloatDragValueWidget *goalXSpin = new FloatDragValueWidget();
        FloatDragValueWidget *goalYSpin = new FloatDragValueWidget();
        FloatDragValueWidget *goalYawSpin = new FloatDragValueWidget();
        FloatDragValueWidget *yawTolSpin = new FloatDragValueWidget();
        FloatDragValueWidget *lookAheadSpin = new FloatDragValueWidget();
        FloatDragValueWidget *maxVxSpin = new FloatDragValueWidget();
        FloatDragValueWidget *maxSteerSpin = new FloatDragValueWidget();
        FloatDragValueWidget *arriveSpin = new FloatDragValueWidget();
        FloatDragValueWidget *arriveHystSpin = new FloatDragValueWidget();
        FloatDragValueWidget *slowDistSpin = new FloatDragValueWidget();
        FloatDragValueWidget *wheelbaseSpin = new FloatDragValueWidget();
        FloatDragValueWidget *kpSteerSpin = new FloatDragValueWidget();
        /// 成对镜像 (y,yaw)→(-y,-yaw)：Aurora +Y右 ↔ Ranger +Y左 / 左转为正
        QCheckBox *mirrorFrameCheck = new QCheckBox(
            QStringLiteral("镜像坐标系（Y 与偏航同时取反）"));
        QPushButton *pathPreviewButton = new QPushButton();
        QLabel *statusLabel = new QLabel();

    public slots:
        void showPathPreview()
        {
            ensurePathDialog();
            if (!m_pathDialog) {
                return;
            }
            applyCachedPlot(true);
            m_pathDialog->show();
            m_pathDialog->raise();
            m_pathDialog->activateWindow();
        }

    private:
        void ensurePathDialog()
        {
            if (m_pathDialog) {
                return;
            }

            // 必须无父窗口：节点面板在 QGraphicsProxyWidget 内，
            // 若 parent=window()/this，弹窗会被嵌进节点图而不是系统独立窗。
            m_pathDialog = new QDialog(nullptr);
            m_pathDialog->setWindowTitle(QStringLiteral("Nav 路径预览"));
            m_pathDialog->setWindowFlags(Qt::Window
                                         | Qt::WindowTitleHint
                                         | Qt::WindowSystemMenuHint
                                         | Qt::WindowMinMaxButtonsHint
                                         | Qt::WindowCloseButtonHint);
            m_pathDialog->setWindowModality(Qt::NonModal);
            m_pathDialog->setAttribute(Qt::WA_DeleteOnClose, false);
            m_pathDialog->setAttribute(Qt::WA_QuitOnClose, false);
            m_pathDialog->resize(560, 500);

            pathPlot = new QCustomPlot(m_pathDialog);
            setupPathPlot();

            auto *lay = new QVBoxLayout(m_pathDialog);
            lay->setContentsMargins(8, 8, 8, 8);
            auto *hint = new QLabel(
                QStringLiteral("蓝=前进　橙=倒车　红=当前　绿=目标　可拖拽/滚轮缩放"),
                m_pathDialog);
            hint->setWordWrap(true);
            lay->addWidget(hint);
            lay->addWidget(pathPlot, 1);
        }

        void setupPathPlot()
        {
            pathPlot->setMinimumSize(360, 320);
            pathPlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            pathPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
            pathPlot->axisRect()->setupFullAxesBox(true);
            pathPlot->xAxis->setLabel(QStringLiteral("X"));
            pathPlot->yAxis->setLabel(QStringLiteral("Y"));
            pathPlot->legend->setVisible(true);
            pathPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
            pathPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

            m_fwdGraph = pathPlot->addGraph();
            m_fwdGraph->setName(QStringLiteral("前进"));
            m_fwdGraph->setPen(QPen(QColor(30, 120, 220), 2));

            m_revGraph = pathPlot->addGraph();
            m_revGraph->setName(QStringLiteral("倒车"));
            m_revGraph->setPen(QPen(QColor(230, 120, 20), 2, Qt::DashLine));

            m_poseGraph = pathPlot->addGraph();
            m_poseGraph->setName(QStringLiteral("当前"));
            m_poseGraph->setLineStyle(QCPGraph::lsNone);
            m_poseGraph->setScatterStyle(
                QCPScatterStyle(QCPScatterStyle::ssDisc, QColor(220, 40, 40), Qt::white, 9));

            // 航向用带箭头的 ItemLine，比两点折线更直观
            m_poseYawArrow = new QCPItemLine(pathPlot);
            m_poseYawArrow->setPen(QPen(QColor(220, 40, 40), 2));
            m_poseYawArrow->setHead(QCPLineEnding(QCPLineEnding::esSpikeArrow, 10, 8));
            m_poseYawArrow->setTail(QCPLineEnding(QCPLineEnding::esNone));

            m_goalGraph = pathPlot->addGraph();
            m_goalGraph->setName(QStringLiteral("目标"));
            m_goalGraph->setLineStyle(QCPGraph::lsNone);
            m_goalGraph->setScatterStyle(
                QCPScatterStyle(QCPScatterStyle::ssSquare, QColor(40, 160, 70), Qt::white, 9));

            m_goalYawArrow = new QCPItemLine(pathPlot);
            m_goalYawArrow->setPen(QPen(QColor(40, 160, 70), 2));
            m_goalYawArrow->setHead(QCPLineEnding(QCPLineEnding::esSpikeArrow, 10, 8));
            m_goalYawArrow->setTail(QCPLineEnding(QCPLineEnding::esNone));

            pathPlot->xAxis->setRange(-1.0, 1.0);
            pathPlot->yAxis->setRange(-1.0, 1.0);
        }

        /// 设置航向箭头：从位姿指向 yaw 方向
        static void setYawArrow(QCPItemLine *arrow, double x, double y, double yaw, double len)
        {
            if (!arrow) {
                return;
            }
            arrow->start->setCoords(x, y);
            arrow->end->setCoords(x + len * std::cos(yaw), y + len * std::sin(yaw));
        }

        void applyCachedPlot(bool forceRescale)
        {
            if (!pathPlot || !m_fwdGraph || !m_revGraph) {
                return;
            }

            QVector<double> fwdX;
            QVector<double> fwdY;
            QVector<double> revX;
            QVector<double> revY;
            fwdX.reserve(m_cachePathX.size());
            fwdY.reserve(m_cachePathY.size());
            revX.reserve(m_cachePathX.size());
            revY.reserve(m_cachePathY.size());

            int prevGear = 0;
            for (int i = 0; i < m_cachePathX.size(); ++i) {
                const int gear = (i < m_cachePathGear.size() && m_cachePathGear.at(i) >= 0) ? 1 : -1;
                if (i > 0 && gear != prevGear) {
                    fwdX.append(qQNaN());
                    fwdY.append(qQNaN());
                    revX.append(qQNaN());
                    revY.append(qQNaN());
                }
                if (gear >= 0) {
                    fwdX.append(m_cachePathX.at(i));
                    fwdY.append(m_cachePathY.at(i));
                } else {
                    revX.append(m_cachePathX.at(i));
                    revY.append(m_cachePathY.at(i));
                }
                prevGear = gear;
            }

            m_fwdGraph->setData(fwdX, fwdY, true);
            m_revGraph->setData(revX, revY, true);

            const double arrowLen = 0.25;
            m_poseGraph->setData(QVector<double>{m_cachePoseX}, QVector<double>{m_cachePoseY}, true);
            setYawArrow(m_poseYawArrow, m_cachePoseX, m_cachePoseY, m_cachePoseYaw, arrowLen);

            m_goalGraph->setData(QVector<double>{m_cacheGoalX}, QVector<double>{m_cacheGoalY}, true);
            setYawArrow(m_goalYawArrow, m_cacheGoalX, m_cacheGoalY, m_cacheGoalYaw, arrowLen);

            if (forceRescale || m_cachePathX.isEmpty()) {
                rescalePathPlot(m_cachePathX,
                                m_cachePathY,
                                m_cachePoseX,
                                m_cachePoseY,
                                m_cacheGoalX,
                                m_cacheGoalY);
                m_pendingRescale = false;
            }

            pathPlot->replot(QCustomPlot::rpQueuedReplot);
        }

        void rescalePathPlot(const QVector<double> &pathX,
                             const QVector<double> &pathY,
                             double poseX,
                             double poseY,
                             double goalX,
                             double goalY)
        {
            double minX = std::min({poseX, goalX});
            double maxX = std::max({poseX, goalX});
            double minY = std::min({poseY, goalY});
            double maxY = std::max({poseY, goalY});
            for (int i = 0; i < pathX.size(); ++i) {
                if (!qIsFinite(pathX.at(i)) || !qIsFinite(pathY.at(i))) {
                    continue;
                }
                minX = std::min(minX, pathX.at(i));
                maxX = std::max(maxX, pathX.at(i));
                minY = std::min(minY, pathY.at(i));
                maxY = std::max(maxY, pathY.at(i));
            }

            const double spanX = std::max(0.5, maxX - minX);
            const double spanY = std::max(0.5, maxY - minY);
            const double span = std::max(spanX, spanY) * 1.25;
            const double cx = 0.5 * (minX + maxX);
            const double cy = 0.5 * (minY + maxY);
            pathPlot->xAxis->setRange(cx - 0.5 * span, cx + 0.5 * span);
            pathPlot->yAxis->setRange(cy - 0.5 * span, cy + 0.5 * span);
        }

        QLabel *goalXLabel = new QLabel(QStringLiteral("目标 X"));
        QLabel *goalYLabel = new QLabel(QStringLiteral("目标 Y"));
        QLabel *goalYawLabel = new QLabel(QStringLiteral("目标航向"));
        QLabel *yawTolLabel = new QLabel(QStringLiteral("航向容差"));
        QLabel *lookAheadLabel = new QLabel(QStringLiteral("预瞄距离"));
        QLabel *maxVxLabel = new QLabel(QStringLiteral("最大线速度"));
        QLabel *maxSteerLabel = new QLabel(QStringLiteral("最大转角"));
        QLabel *arriveLabel = new QLabel(QStringLiteral("到达距离"));
        QLabel *arriveHystLabel = new QLabel(QStringLiteral("到点滞回"));
        QLabel *slowDistLabel = new QLabel(QStringLiteral("减速距离"));
        QLabel *wheelbaseLabel = new QLabel(QStringLiteral("轴距 L"));
        QLabel *kpSteerLabel = new QLabel(QStringLiteral("航向增益 Kp"));

        QPointer<QDialog> m_pathDialog;
        QCustomPlot *pathPlot = nullptr;
        QCPGraph *m_fwdGraph = nullptr;
        QCPGraph *m_revGraph = nullptr;
        QCPGraph *m_poseGraph = nullptr;
        QCPGraph *m_goalGraph = nullptr;
        QCPItemLine *m_poseYawArrow = nullptr;
        QCPItemLine *m_goalYawArrow = nullptr;

        QVector<double> m_cachePathX;
        QVector<double> m_cachePathY;
        QVector<int> m_cachePathGear;
        double m_cachePoseX = 0.0;
        double m_cachePoseY = 0.0;
        double m_cachePoseYaw = 0.0;
        double m_cacheGoalX = 0.0;
        double m_cacheGoalY = 0.0;
        double m_cacheGoalYaw = 0.0;
        bool m_pendingRescale = true;
    };
}
