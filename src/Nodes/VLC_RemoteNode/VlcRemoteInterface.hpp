/**
 * @file VlcRemoteInterface.hpp
 * @brief VLC Remote 节点嵌入式 UI 面板
 *
 * 提供连接配置、播放控制、音量调节、播放列表展示等界面元素。
 * 所有控件均为 public 成员，由 VlcRemoteDataModel 负责信号绑定与数据同步。
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @class VlcRemoteInterface
     * @brief VLC 远程控制节点的可视化操作面板
     *
     * 布局结构（自上而下）：
     * 1. 连接配置：主机 / 端口 / 密码
     * 2. 控制按钮：播放暂停 / 停止 / 全屏 / 刷新列表
     * 3. 音量滑块：IntDragValueWidget，0～100%
     * 4. 状态栏：连接与播放摘要
     * 5. 播放列表：单击或双击切换条目
     */
    class VlcRemoteInterface : public QWidget
    {
        Q_OBJECT
    public:
        explicit VlcRemoteInterface(QWidget *parent = nullptr)
        {
            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(6);

            // 通用行布局：左侧标签 + 右侧编辑器
            const auto addRow = [layout](const QString &labelText, QWidget *editor) {
                auto *row = new QWidget();
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(new QLabel(labelText));
                rowLayout->addWidget(editor, 1);
                layout->addWidget(row);
            };

            // --- 连接配置区 ---
            addRow("主机:", hostEdit);
            addRow("端口:", portSpinBox);
            addRow("密码:", passwordEdit);
            passwordEdit->setEchoMode(QLineEdit::Password);

            portSpinBox->setRange(1, 65535);
            portSpinBox->setValue(8080);  // VLC HTTP 默认端口
            passwordEdit->setPlaceholderText("与 VLC Lua HTTP 密码一致");

            // --- 播放控制按钮区 ---
            {
                auto *row = new QWidget();
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(playPauseButton);
                rowLayout->addWidget(stopButton);
                rowLayout->addWidget(fullscreenButton);
                rowLayout->addWidget(refreshPlaylistButton);
                layout->addWidget(row);
            }

            // --- 音量控制 ---
            addRow("音量:", volumeEditor);
            volumeEditor->setRange(0, 100);
            volumeEditor->setSingleStep(1);
            volumeEditor->setSuffix(QStringLiteral("%"));
            volumeEditor->setValue(100);

            // --- 状态栏 ---
            statusLabel->setWordWrap(true);
            statusLabel->setText("状态: 未连接");
            layout->addWidget(statusLabel);

            // --- 播放列表 ---
            layout->addWidget(new QLabel("播放列表:"));
            playlistWidget->setMinimumHeight(120);
            layout->addWidget(playlistWidget, 1);
        }

        // ===== 连接配置 =====
        QLineEdit *hostEdit = new QLineEdit("127.0.0.1");
        QSpinBox *portSpinBox = new QSpinBox();
        QLineEdit *passwordEdit = new QLineEdit();

        // ===== 播放控制 =====
        QPushButton *playPauseButton = new QPushButton("播放/暂停");
        QPushButton *stopButton = new QPushButton("停止");
        QPushButton *fullscreenButton = new QPushButton("全屏");
        QPushButton *refreshPlaylistButton = new QPushButton("刷新列表");

        // ===== 音量 =====
        IntDragValueWidget *volumeEditor = new IntDragValueWidget();

        // ===== 状态与列表 =====
        QLabel *statusLabel = new QLabel();
        QListWidget *playlistWidget = new QListWidget();
    };
}
