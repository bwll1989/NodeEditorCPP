/**
 * @file ViosoInterface.hpp
 * @brief Vioso 节点内嵌界面（替代属性树反射）
 *
 * 控件：媒体库选 `.vwf`、AES Passkey、Reload。输出口数由 `.vwf` 通道数自动展开。
 */

#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"

namespace Nodes
{

/** @brief 节点上内嵌的参数面板 */
class ViosoInterface : public QWidget
{
public:
    explicit ViosoInterface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 2, 4, 4);
        layout->setSpacing(6);

        fileSelectComboBox->setPlaceholderText(QStringLiteral("Select .vwf from Media Library"));
        layout->addWidget(fileSelectComboBox);

        passkeyEdit->setPlaceholderText(QStringLiteral("Passkey (32 hex or 16 bytes, empty=off)"));
        passkeyEdit->setEchoMode(QLineEdit::Password);
        passkeyEdit->setClearButtonEnabled(true);
        layout->addWidget(passkeyEdit);

        reloadButton->setText(QStringLiteral("Reload"));
        layout->addWidget(reloadButton);

        layout->addStretch(1);
        setMinimumWidth(200);
    }

    SelectorComboBox* fileSelectComboBox =
        new SelectorComboBox(MediaLibrary::Category::Vioso, this);
    QLineEdit* passkeyEdit = new QLineEdit(this);
    QPushButton* reloadButton = new QPushButton(this);
};

} // namespace Nodes
