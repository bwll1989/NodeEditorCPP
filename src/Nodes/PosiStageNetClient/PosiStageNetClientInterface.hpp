/**
 * @file PosiStageNetClientInterface.hpp
 * @brief PosiStageNet(PSN)客户端节点界面：组播地址、端口、启用开关、状态显示
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
namespace Nodes
{
    class PosiStageNetClientInterface : public QWidget
    {
    public:
        explicit PosiStageNetClientInterface(QWidget *parent = nullptr)
        {
            // 使用QGridLayout重构整体布局，保持原有的边距和间距
            auto *mainGridLayout = new QGridLayout(this);
            mainGridLayout->setContentsMargins(0, 0, 0, 0);
            mainGridLayout->setSpacing(6);

            // 组播地址行 占据网格第一行的两列
            auto *addressLabel = new QLabel(QStringLiteral("组播地址:"));
            mainGridLayout->addWidget(addressLabel, 0, 0);
            multicastAddressEdit->setPlaceholderText(QStringLiteral("236.10.10.10"));
            multicastAddressEdit->setText(QStringLiteral("236.10.10.10"));
            multicastAddressEdit->setToolTip(QStringLiteral("PSN 默认组播地址 236.10.10.10"));
            mainGridLayout->addWidget(multicastAddressEdit, 0, 1);

            // 端口行 占据网格第二行的两列
            auto *portLabel = new QLabel(QStringLiteral("端口:"));
            mainGridLayout->addWidget(portLabel, 1, 0);
            portSpinBox->setRange(1, 65535);
            portSpinBox->setValue(56565);
            portSpinBox->setToolTip(QStringLiteral("PSN 默认端口 56565"));
            mainGridLayout->addWidget(portSpinBox, 1, 1);
            // 为端口行添加空白拉伸，保持原有布局效果
            mainGridLayout->setColumnStretch(1, 1);

            // 启用接收复选框 跨两列放置在第三行
            enableCheckBox->setText(QStringLiteral("启用接收"));
            enableCheckBox->setChecked(false);
            enableCheckBox->setToolTip(QStringLiteral("开启后加入组播并接收PSN数据"));
            mainGridLayout->addWidget(enableCheckBox, 2, 0, 1, 2);

            // 状态组框 跨两列放置在第四行，保持内部原有垂直布局不变
            auto *statusBox = new QGroupBox(QStringLiteral("状态"));
            auto *statusLayout = new QVBoxLayout(statusBox);
            statusLayout->setContentsMargins(8, 12, 8, 8);
            statusLayout->setSpacing(4);

            connectionLabel->setFlat(true);
            connectionLabel->setCheckable(true);
            connectionLabel->setEnabled(false);
            connectionLabel->setText(QStringLiteral("未连接"));
            connectionLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
            statusLayout->addWidget(connectionLabel);

            systemNameLabel->setText(QStringLiteral("系统名: —"));
            systemNameLabel->setStyleSheet(QStringLiteral("color: #888;"));
            statusLayout->addWidget(systemNameLabel);

            frameInfoLabel->setText(QStringLiteral("帧: — | 追踪器: 0"));
            frameInfoLabel->setStyleSheet(QStringLiteral("color: #888;"));
            statusLayout->addWidget(frameInfoLabel);

            trackerListWidget->setViewMode(QListView::ListMode);
            trackerListWidget->setSelectionMode(QAbstractItemView::NoSelection);
            trackerListWidget->setMaximumHeight(120);
            trackerListWidget->setStyleSheet(QStringLiteral(
                "QListWidget {"
                "  background: transparent;"
                "  border: 1px solid #333;"
                "  border-radius: 3px;"
                "  font-size: 11px;"
                "}"
                "QListWidget::item {"
                "  padding: 2px 4px;"
                "}"));
            statusLayout->addWidget(trackerListWidget);

            mainGridLayout->addWidget(statusBox, 3, 0, 1, 2);

            setMinimumWidth(260);
        }

        /**
         * @brief 更新连接状态显示
         * @param connected 是否已连接（正在接收数据）
         */
        void updateConnectionStatus(bool connected)
        {
            connectionLabel->setChecked(connected);
            connectionLabel->setText(connected
                                         ? QStringLiteral("已连接 (接收中)")
                                         : QStringLiteral("未连接"));
            connectionLabel->setStyleSheet(connected
                                               ? QStringLiteral("color: green; font-weight: bold;")
                                               : QStringLiteral("color: red; font-weight: bold;"));
        }

        /**
         * @brief 更新系统名显示
         * @param name PSN系统名
         */
        void updateSystemName(const QString &name)
        {
            if (name.isEmpty()) {
                systemNameLabel->setText(QStringLiteral("系统名: —"));
            } else {
                systemNameLabel->setText(QStringLiteral("系统名: %1").arg(name));
            }
        }

        /**
         * @brief 更新帧信息显示
         * @param frameId 帧ID
         * @param trackerCount 追踪器数量
         */
        void updateFrameInfo(int frameId, int trackerCount)
        {
            if (frameId < 0) {
                frameInfoLabel->setText(QStringLiteral("帧: — | 追踪器: 0"));
            } else {
                frameInfoLabel->setText(QStringLiteral("帧: %1 | 追踪器: %2")
                                            .arg(frameId)
                                            .arg(trackerCount));
            }
        }

        /**
         * @brief 更新追踪器列表显示
         * @param trackers 追踪器信息映射：key=ID, value=(name, posX, posY, posZ)
         */
        void updateTrackerList(const QMap<int, QString> &trackers)
        {
            trackerListWidget->clear();
            for (auto it = trackers.begin(); it != trackers.end(); ++it) {
                const QString text = QStringLiteral("[%1] %2")
                                         .arg(it.key())
                                         .arg(it.value().isEmpty() ? QStringLiteral("—") : it.value());
                trackerListWidget->addItem(text);
            }
        }

        /**
         * @brief 重置所有状态显示为未连接状态
         */
        void resetStatus()
        {
            updateConnectionStatus(false);
            updateSystemName(QString());
            updateFrameInfo(-1, 0);
            trackerListWidget->clear();
        }

        QLineEdit *multicastAddressEdit = new QLineEdit();
        IntDragValueWidget *portSpinBox = new IntDragValueWidget();
        QCheckBox *enableCheckBox = new QCheckBox();
        QPushButton *connectionLabel = new QPushButton();
        QLabel *systemNameLabel = new QLabel();
        QLabel *frameInfoLabel = new QLabel();
        QListWidget *trackerListWidget = new QListWidget();
    };
}
