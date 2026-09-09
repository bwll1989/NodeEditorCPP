#pragma once

#include <QtCore/QVariantMap>
#include <QtCore/QVariantList>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"

namespace Nodes
{
    /// Aurora S 节点嵌入式面板 UI
    class AuroraSInterface : public QWidget
    {
    public:
        explicit AuroraSInterface(QWidget* parent = nullptr)
        {
            auto* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            mainLayout->setSpacing(5);

            createConnectionGroup(mainLayout);
            createLocalizationGroup(mainLayout);
            createPoseGroup(mainLayout);

            hostEdit->setText(QStringLiteral("192.168.11.1"));
            mapFileSelector->setCurrentValue(QStringLiteral("auroramap.stcm"));
            setMinimumSize(320, 300);
        }

        /// 更新连接状态指示；断开时清空位姿/定位信息
        void updateConnectionStatus(bool connected)
        {
            connectionStatus->setChecked(connected);
            if (connected) {
                connectionStatus->setText(QStringLiteral("已连接"));
                connectionStatus->setStyleSheet(QStringLiteral("color: green; font-weight: bold;"));
            } else {
                connectionStatus->setText(QStringLiteral("未连接"));
                connectionStatus->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
                clearPoseInfo();
                clearLocalizationInfo();
            }
        }

        /// 更新定位状态文字与颜色
        void updateLocalizationStatus(const QVariantMap& status)
        {
            const QString stateText = status.value(QStringLiteral("state_text")).toString();
            localizationStatusLabel->setText(stateText);

            const QString state = status.value(QStringLiteral("state")).toString();
            if (state == QStringLiteral("localized")) {
                localizationStatusLabel->setStyleSheet(
                    QStringLiteral("color: green; font-weight: bold;"));
            } else if (state == QStringLiteral("relocalizing")
                       || state == QStringLiteral("reconnecting")) {
                localizationStatusLabel->setStyleSheet(
                    QStringLiteral("color: #4aa3ff; font-weight: bold;"));
            } else if (state == QStringLiteral("tracking_lost")
                       || state == QStringLiteral("relocalization_failed")) {
                localizationStatusLabel->setStyleSheet(
                    QStringLiteral("color: red; font-weight: bold;"));
            } else {
                localizationStatusLabel->setStyleSheet(
                    QStringLiteral("color: orange; font-weight: bold;"));
            }
        }

        /// 显示 Roll/Pitch/Yaw：优先用 map.deg 向量（度）
        void updateOrientation(const QVariantMap& orientation)
        {
            const QVariantList deg = orientation.value(QStringLiteral("deg")).toList();
            orientationLabel->setText(
                QStringLiteral("R:%1° P:%2° Y:%3°")
                    .arg(deg.value(0).toDouble(), 0, 'f', 2)
                    .arg(deg.value(1).toDouble(), 0, 'f', 2)
                    .arg(deg.value(2).toDouble(), 0, 'f', 2));
        }

        /// 显示当前 X/Y/Z 位置（米，列表 [x,y,z]）
        void updatePosition(const QVariantList& position)
        {
            positionLabel->setText(
                QStringLiteral("(%1, %2, %3) m")
                    .arg(position.value(0).toDouble(), 0, 'f', 3)
                    .arg(position.value(1).toDouble(), 0, 'f', 3)
                    .arg(position.value(2).toDouble(), 0, 'f', 3));
        }

        void showError(const QString& message)
        {
            errorLabel->setText(message);
        }

        void clearStatusMessage()
        {
            errorLabel->clear();
        }

        QLineEdit* hostEdit = new QLineEdit();
        /// 媒体库 Unknown 分类下的 .stcm 地图选择器
        SelectorComboBox* mapFileSelector =
            new SelectorComboBox(MediaLibrary::Category::Unknown, this);
        QPushButton* connectionStatus = new QPushButton(QStringLiteral("未连接"));
        QPushButton* reinitializeButton = new QPushButton(QStringLiteral("重新初始化"));

    private:
        void clearPoseInfo()
        {
            orientationLabel->setText(QStringLiteral("--"));
            positionLabel->setText(QStringLiteral("--"));
        }

        void clearLocalizationInfo()
        {
            localizationStatusLabel->setText(QStringLiteral("--"));
            localizationStatusLabel->setStyleSheet(QStringLiteral("color: orange;"));
        }

        void createConnectionGroup(QVBoxLayout* mainLayout)
        {
            auto* group = new QGroupBox(QStringLiteral("连接设置"), this);
            auto* layout = new QGridLayout(group);

            layout->addWidget(new QLabel(QStringLiteral("设备地址:")), 0, 0);
            layout->addWidget(hostEdit, 0, 1);
            layout->addWidget(new QLabel(QStringLiteral("地图数据:")), 1, 0);
            layout->addWidget(mapFileSelector, 1, 1);

            connectionStatus->setCheckable(true);
            connectionStatus->setEnabled(false);
            connectionStatus->setFlat(true);
            connectionStatus->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));
            layout->addWidget(connectionStatus, 2, 0, 1, 2);

            reinitializeButton->setFlat(false);
            layout->addWidget(reinitializeButton, 3, 0, 1, 2);

            errorLabel = new QLabel(this);
            errorLabel->setStyleSheet(QStringLiteral("color: orange; font-size: 10px;"));
            errorLabel->setWordWrap(true);
            layout->addWidget(errorLabel, 4, 0, 1, 2);

            mainLayout->addWidget(group);
        }

        void createLocalizationGroup(QVBoxLayout* mainLayout)
        {
            auto* group = new QGroupBox(QStringLiteral("定位状态"), this);
            auto* layout = new QGridLayout(group);

            localizationStatusLabel = createValueLabel();
            addInfoRow(layout, 0, QStringLiteral("状态"), localizationStatusLabel);

            mainLayout->addWidget(group);
        }

        void createPoseGroup(QVBoxLayout* mainLayout)
        {
            auto* group = new QGroupBox(QStringLiteral("位姿 (50 Hz)"), this);
            auto* layout = new QGridLayout(group);

            orientationLabel = createValueLabel();
            positionLabel = createValueLabel();

            int row = 0;
            addInfoRow(layout, row++, QStringLiteral("姿态 (Roll/Pitch/Yaw)"), orientationLabel);
            addInfoRow(layout, row++, QStringLiteral("位置 (X/Y/Z)"), positionLabel);

            mainLayout->addWidget(group);
            mainLayout->addStretch();
        }

        static QLabel* createValueLabel()
        {
            auto* label = new QLabel(QStringLiteral("--"));
            label->setStyleSheet(QStringLiteral("color: orange;"));
            label->setWordWrap(true);
            return label;
        }

        static void addInfoRow(QGridLayout* layout, int row, const QString& title, QLabel* valueLabel)
        {
            auto* titleLabel = new QLabel(title + QStringLiteral(":"));
            layout->addWidget(titleLabel, row, 0);
            layout->addWidget(valueLabel, row, 1);
        }

        QLabel* localizationStatusLabel = nullptr;
        QLabel* orientationLabel = nullptr;
        QLabel* positionLabel = nullptr;
        QLabel* errorLabel = nullptr;
    };
}
