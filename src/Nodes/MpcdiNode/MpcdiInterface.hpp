/**
 * @file MpcdiInterface.hpp
 * @brief MPCDI 节点内嵌界面
 *
 * 控件对齐 mpcdi-viewer 的变形解释和融合开关。
 * 输出口数由文件里第一个 buffer 的 region 数决定。
 */

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"

namespace Nodes
{

class MpcdiInterface : public QWidget
{
public:
    explicit MpcdiInterface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 2, 4, 4);
        layout->setSpacing(4);

        fileSelectComboBox->setPlaceholderText(QStringLiteral("Select .mpcdi from Media Library"));
        layout->addWidget(fileSelectComboBox);

        auto* coordRow = new QHBoxLayout();
        coordRow->setContentsMargins(0, 0, 0, 0);
        auto* coordLabel = new QLabel(QStringLiteral("坐标"), this);
        coordModeCombo->addItem(QStringLiteral("自动"));
        coordModeCombo->addItem(QStringLiteral("归一化 [0,1]"));
        coordModeCombo->addItem(QStringLiteral("像素"));
        coordModeCombo->setToolTip(QStringLiteral(
            "自动：多数样本在 0–1 时当归一化，否则当像素。"
            "归一化直接当 UV。像素会先除以 buffer 分辨率。"));
        coordRow->addWidget(coordLabel);
        coordRow->addWidget(coordModeCombo, 1);
        layout->addLayout(coordRow);

        auto addCheck = [&](QCheckBox* box, const QString& text, bool on, const QString& tip) {
            box->setText(text);
            box->setChecked(on);
            box->setToolTip(tip);
            layout->addWidget(box);
        };
        addCheck(flipYCheck, QStringLiteral("翻转 Y（内容）"), true,
                 QStringLiteral("把变形图里的内容 V 反过来。画面上下颠倒时切换。"));
        addCheck(flipWarpYCheck, QStringLiteral("翻转 Y（变形图）"), false,
                 QStringLiteral("读变形图时上下对调。变形形状本身镜像时打开。"));
        addCheck(regionLocalCheck, QStringLiteral("Region 局部"), false,
                 QStringLiteral("把整幅 buffer 的 UV 收进当前 region，让这一路铺满输出。"));
        addCheck(blendCheck, QStringLiteral("Blend"), true,
                 QStringLiteral("用 alpha 图做边缘融合。关闭后只变形。"));
        addCheck(linearizeAlphaCheck, QStringLiteral("Alpha 线性化"), true,
                 QStringLiteral("按 gammaEmbedded 把 alpha 还原成线性权重后再相乘。"));
        addCheck(blackLevelCheck, QStringLiteral("黑电平"), true,
                 QStringLiteral("有 beta 图时按协议抬黑：颜色 = alpha×颜色×(1-beta)+beta。没有 beta 图时无效果。"));
        addCheck(boundsCheck, QStringLiteral("区域边界"), false,
                 QStringLiteral("边缘画绿线，未映射的像素涂成琥珀色。"));

        layout->addStretch(1);
        setMinimumWidth(230);
    }

    SelectorComboBox* fileSelectComboBox =
        new SelectorComboBox(MediaLibrary::Category::Mpcdi, this);
    QComboBox* coordModeCombo = new QComboBox(this);
    QCheckBox* flipYCheck = new QCheckBox(this);
    QCheckBox* flipWarpYCheck = new QCheckBox(this);
    QCheckBox* regionLocalCheck = new QCheckBox(this);
    QCheckBox* blendCheck = new QCheckBox(this);
    QCheckBox* linearizeAlphaCheck = new QCheckBox(this);
    QCheckBox* blackLevelCheck = new QCheckBox(this);
    QCheckBox* boundsCheck = new QCheckBox(this);
};

} // namespace Nodes
