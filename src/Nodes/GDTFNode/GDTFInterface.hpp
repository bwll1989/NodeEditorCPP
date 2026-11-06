/**
 * @file GDTFInterface.hpp
 * @brief GDTF 节点内嵌界面
 *
 * 组成：
 *  - .gdtf 文件选择（Fixture 媒体库分类）
 *  - 起始地址（DMX Break 1 起点，1-512）
 *  - DMX 模式选择（解析后填充）
 *  - 设备图片（zip 内 thumbnail.png 放大显示）
 *  - 通道值控制区（按映射自动生成，每物理地址一个 IntDragValueWidget）
 *  - 摘要
 */
#pragma once

#include <algorithm>

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"
#include "../GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{

class GDTFInterface : public QWidget
{
public:
    explicit GDTFInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 2, 4, 4);
        layout->setSpacing(4);

        // 文件行
        fileSelectComboBox->setPlaceholderText(QStringLiteral("Select .gdtf from Media Library"));
        layout->addWidget(fileSelectComboBox);

        // 起始地址 + DMX 模式（同一行）
        auto *addrRow = new QHBoxLayout();
        addrRow->setContentsMargins(0, 0, 0, 0);
        startAddressSpin->setRange(1, 512);
        startAddressSpin->setValue(1);
        startAddressSpin->setToolTip(
            QStringLiteral("DMX 起始地址（1-512）；后续断点按前断点占用顺序接续"));
        addrRow->addWidget(new QLabel(QStringLiteral("起始"), this));
        addrRow->addWidget(startAddressSpin);
        addrRow->addWidget(rangeLabel);
        addrRow->addSpacing(8);
        addrRow->addWidget(new QLabel(QStringLiteral("模式"), this));
        addrRow->addWidget(modeCombo, 1);
        layout->addLayout(addrRow);

        // 设备图片（zip 内 thumbnail.png，放大显示在映射信息区）
        thumbnailLabel->setMinimumHeight(140);
        thumbnailLabel->setAlignment(Qt::AlignCenter);
        thumbnailLabel->setText(QStringLiteral("No Thumbnail"));
        layout->addWidget(thumbnailLabel, 1);

        // 通道值控制区（按映射结果自动生成，每物理地址一个拖动控件）
        controlGroup->setTitle(QStringLiteral("通道控制"));
        auto *cgLayout = new QVBoxLayout(controlGroup);
        cgLayout->setContentsMargins(0, 2, 0, 2);
        controlScroll->setWidget(controlInner);
        controlScroll->setWidgetResizable(true);
        controlScroll->setMinimumHeight(104);
        controlScroll->setFrameShape(QFrame::NoFrame);
        cgLayout->addWidget(controlScroll);
        layout->addWidget(controlGroup);

        layout->addStretch(1);
        setMinimumSize(320, 360);
    }

    SelectorComboBox *fileSelectComboBox =
        new SelectorComboBox(MediaLibrary::Category::Fixture, this);
    QSpinBox     *startAddressSpin = new QSpinBox(this);
    QComboBox    *modeCombo = new QComboBox(this);
    QGroupBox    *controlGroup = new QGroupBox(this);
    QScrollArea  *controlScroll = new QScrollArea(this);
    QWidget      *controlInner = new QWidget;
    QVBoxLayout  *controlLayout = new QVBoxLayout(controlInner);
    QLabel       *thumbnailLabel = new QLabel(this);
    QLabel       *rangeLabel = new QLabel(this);

    /**
     * 按映射结果重建通道值控制行（每物理地址一行：标签 + IntDragValueWidget）。
     * @param labels    每行标签（如 "513 Dimmer MSB"）
     * @param addresses 每行对应绝对 DMX 地址（与 labels 平行）
     * @param defaults  每行初值（0-255）
     * @return 生成的行数
     */
    int rebuildValueControls(const QStringList &labels,
                             const QVector<int> &addresses,
                             const QVector<int> &defaults)
    {
        QLayoutItem *item = nullptr;
        while ((item = controlLayout->takeAt(0)) != nullptr) {
            if (item->widget())
                item->widget()->deleteLater();
            delete item;
        }
        valueSliders.clear();

        const int count = qMin(qMin(labels.size(), addresses.size()), defaults.size());
        for (int i = 0; i < count; ++i) {
            auto *rowWidget = new QWidget(controlInner);
            auto *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 1, 0, 1);
            rowLayout->setSpacing(4);

            auto *lab = new QLabel(labels[i], rowWidget);
            lab->setStyleSheet(QStringLiteral("color: orange; font-weight: bold;"));

            auto *slider = new IntDragValueWidget(rowWidget);
            slider->setRange(0, 255);
            slider->setValue(defaults[i]);
            slider->setToolTip(QStringLiteral("DMX 地址 %1").arg(addresses[i]));

            rowLayout->addWidget(lab);
            rowLayout->addWidget(slider, 1);
            controlLayout->addWidget(rowWidget);
            valueSliders.append(slider);
        }
        controlLayout->addStretch(1);
        return count;
    }

    int sliderCount() const { return valueSliders.size(); }

    IntDragValueWidget *sliderAt(int row)
    {
        return (row >= 0 && row < valueSliders.size()) ? valueSliders.at(row) : nullptr;
    }

private:
    QVector<IntDragValueWidget *> valueSliders;

public:
    /** 起始地址旁的占用范围提示 */
    void updateRange(int startAddress, int footprint)
    {
        rangeLabel->setText(footprint > 0
            ? QStringLiteral("%1-%2").arg(startAddress).arg(startAddress + footprint - 1)
            : QStringLiteral("-"));
    }
};

} // namespace Nodes
