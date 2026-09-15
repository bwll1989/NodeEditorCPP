//
// Created by Administrator on 2023/12/13.
//

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QGroupBox"
#include "QVBoxLayout"
#include "QHBoxLayout"
#include "QGridLayout"
#include "QDateTime"
#include "QListWidget"
#include "QAbstractItemView"

using namespace std;

namespace Nodes
{
    /**
     * @brief DAW控制器界面类
     *
     * 提供DAW播放列表控制的用户界面，包括：
     * - 连接设置（IP地址，端口固定为2004）
     * - 刷新播放列表按钮
     * - 播放列表显示（每行图标按钮控制播放/停止）
     * - 连接状态与消息状态显示
     */
    class DAWControllerInterface: public QGroupBox{
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit DAWControllerInterface(QWidget *parent = nullptr){
            setTitle(QStringLiteral("DAW 播放控制器"));

            auto *mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(8, 8, 8, 8);
            mainLayout->setSpacing(6);

            const auto addRow = [mainLayout](const QString &labelText, QWidget *editor) {
                auto *row = new QWidget();
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(new QLabel(labelText));
                rowLayout->addWidget(editor, 1);
                mainLayout->addWidget(row);
            };

            hostEdit->setText(QStringLiteral("127.0.0.1"));

            refreshButton->setText(QStringLiteral("刷新列表"));
            refreshButton->setToolTip(QStringLiteral("GET /players 获取当前播放列表"));

            connectionStatusLabel->setFlat(true);
            connectionStatusLabel->setCheckable(true);
            connectionStatusLabel->setEnabled(false);
            connectionStatusLabel->setText(QStringLiteral("连接: 未连接"));
            connectionStatusLabel->setStyleSheet(QStringLiteral("color: red; font-weight: bold;"));

            statusLabel->setWordWrap(true);
            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            statusLabel->setText(QStringLiteral("状态: —"));

            auto *hostRow = new QWidget();
            auto *hostRowLayout = new QHBoxLayout(hostRow);
            hostRowLayout->setContentsMargins(0, 0, 0, 0);
            hostRowLayout->setSpacing(8);
            hostRowLayout->addWidget(new QLabel(QStringLiteral("主机:")));
            hostRowLayout->addWidget(hostEdit, 1);
            mainLayout->addWidget(hostRow);

            mainLayout->addWidget(refreshButton);

            mainLayout->addWidget(connectionStatusLabel);
            mainLayout->addWidget(statusLabel);

            mainLayout->addWidget(new QLabel(QStringLiteral("播放器列表 (点击图标切换播放/停止):")));
            playlistWidget->setMinimumHeight(160);
            playlistWidget->setSelectionMode(QAbstractItemView::SingleSelection);
            mainLayout->addWidget(playlistWidget, 1);

            setMinimumSize(320, 460);
        }

    public:
        QLineEdit* hostEdit = new QLineEdit("127.0.0.1");

        /**
         * @brief 更新连接状态显示
         * @param connected 连接状态
         */
        void updateConnectionStatus(bool connected) {
            connectionStatusLabel->setChecked(connected);
            connectionStatusLabel->setText(connected
                ? QStringLiteral("连接: 已连接")
                : QStringLiteral("连接: 未连接"));
            connectionStatusLabel->setStyleSheet(connected
                ? QStringLiteral("color: green; font-weight: bold;")
                : QStringLiteral("color: red; font-weight: bold;"));
        }

    private:
        QLabel *hostLabel = new QLabel("主机地址:");

    public:
        QPushButton* refreshButton = new QPushButton();
        QPushButton *connectionStatusLabel = new QPushButton("Disconnect");
        QLabel *statusLabel = new QLabel();
        QListWidget *playlistWidget = new QListWidget();
    };
}
