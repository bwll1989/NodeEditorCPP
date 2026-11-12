#pragma once

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QWidget>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "GestureFeature.hpp"

namespace Nodes
{
class GestureInterface final : public QWidget
{
    Q_OBJECT
public:
    explicit GestureInterface(QWidget* parent = nullptr)
    {
        auto* layout = new QGridLayout(this);
        layout->setContentsMargins(4, 2, 4, 4);
        layout->setSpacing(6);

        auto* introLabel = new QLabel(
            QStringLiteral("从媒体库 Gestures 分类选择动作库（GestureRecorder.exe 录制后导入），对画面中的手实时匹配。"), this);
        introLabel->setWordWrap(true);
        layout->addWidget(introLabel, 0, 0, 1, 2);

        librarySelector = new SelectorComboBox(MediaLibrary::Category::Gesture, this);
        librarySelector->setPlaceholderText(QStringLiteral("从媒体库选择 .gesture"));
        layout->addWidget(new QLabel(QStringLiteral("动作库:"), this), 1, 0);
        layout->addWidget(librarySelector, 1, 1);

        auto* libRow = new QHBoxLayout;
        libRow->addStretch(1);
        reloadBtn = new QPushButton(QStringLiteral("重新加载"), this);
        libRow->addWidget(reloadBtn);
        layout->addLayout(libRow, 2, 0, 1, 2);

        layout->addWidget(new QLabel(QStringLiteral("库内标签:"), this), 3, 0);
        gestureList = new QListWidget(this);
        gestureList->setMinimumHeight(64);
        layout->addWidget(gestureList, 4, 0, 1, 2);

        threshold = new FloatDragValueWidget(this);
        threshold->setRange(0, 1);
        threshold->setSingleStep(0.01);
        threshold->setDecimals(2);
        threshold->setValue(GestureFeature::kDefaultThreshold);
        layout->addWidget(new QLabel(QStringLiteral("相似度阈值:"), this), 5, 0);
        layout->addWidget(threshold, 5, 1);

        stableFrames = new FloatDragValueWidget(this);
        stableFrames->setRange(1, 30);
        stableFrames->setSingleStep(1);
        stableFrames->setDecimals(0);
        stableFrames->setValue(3);
        layout->addWidget(new QLabel(QStringLiteral("确认帧数:"), this), 6, 0);
        layout->addWidget(stableFrames, 6, 1);

        maxFps = new FloatDragValueWidget(this);
        maxFps->setRange(1, 30);
        maxFps->setSingleStep(1);
        maxFps->setDecimals(0);
        maxFps->setValue(15);
        layout->addWidget(new QLabel(QStringLiteral("最大帧率 (FPS):"), this), 7, 0);
        layout->addWidget(maxFps, 7, 1);

        drawOverlayCheck = new QCheckBox(QStringLiteral("绘制手势标签"), this);
        drawOverlayCheck->setChecked(true);
        layout->addWidget(drawOverlayCheck, 8, 0, 1, 2);

        layout->addWidget(new QLabel(QStringLiteral("识别开关:"), this), 9, 0);
        enableBtn = new QPushButton(QStringLiteral("开始识别"), this);
        enableBtn->setCheckable(true);
        layout->addWidget(enableBtn, 9, 1);

        statusLabel = new QLabel(QStringLiteral("等待动作库"), this);
        statusLabel->setWordWrap(true);
        layout->addWidget(statusLabel, 10, 0, 1, 2);

        layout->setRowStretch(4, 1);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 2);

        setMinimumWidth(260);
        setMaximumWidth(300);
    }

    SelectorComboBox* librarySelector = nullptr;
    QPushButton* reloadBtn = nullptr;
    QListWidget* gestureList = nullptr;
    FloatDragValueWidget* threshold = nullptr;
    FloatDragValueWidget* stableFrames = nullptr;
    FloatDragValueWidget* maxFps = nullptr;
    QCheckBox* drawOverlayCheck = nullptr;
    QPushButton* enableBtn = nullptr;
    QLabel* statusLabel = nullptr;

public slots:
    /** 模型→UI 推送：阻断 selectionChanged 回环 */
    void setLibraryFile(const QString& name)
    {
        QSignalBlocker blocker(librarySelector);
        librarySelector->setCurrentValue(name);
    }
};
} // namespace Nodes
