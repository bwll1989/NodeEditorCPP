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
#include <QCoreApplication>

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

        fileSelectComboBox->setPlaceholderText(QCoreApplication::translate("Nodes::ViosoInterface", "从媒体库选择 .vwf"));
        layout->addWidget(fileSelectComboBox);

        passkeyEdit->setPlaceholderText(QCoreApplication::translate("Nodes::ViosoInterface", "配对码（32 位十六进制或 16 字节，留空=关闭）"));
        passkeyEdit->setEchoMode(QLineEdit::Password);
        passkeyEdit->setClearButtonEnabled(true);
        layout->addWidget(passkeyEdit);

        reloadButton->setText(QCoreApplication::translate("Nodes::ViosoInterface", "重新加载"));
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
