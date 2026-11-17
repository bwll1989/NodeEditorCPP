#include "FramelessConfirmDialog.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

FramelessConfirmDialog::FramelessConfirmDialog(const QString& title,
                                               const QString& message,
                                               const ButtonTexts& buttons,
                                               Result defaultButton,
                                               QWidget* parent)
    : FramelessDialog(parent)
{
    setMinimumWidth(480);
    setMinimumHeight(160);
    setDialogTitle(title);
    buildUi(message, buttons, defaultButton);
}

void FramelessConfirmDialog::buildUi(const QString& message,
                                     const ButtonTexts& buttons,
                                     Result defaultButton)
{
    auto* messageLabel = new QLabel(message, this);
    messageLabel->setObjectName(QStringLiteral("framelessDialogMessage"));
    messageLabel->setWordWrap(true);
    messageLabel->setMinimumWidth(420);
    bodyLayout()->addWidget(messageLabel);

    _cancelButton = addButton(buttons.cancel, ButtonRole::Reject);
    connect(_cancelButton, &QPushButton::clicked, this, [this]() {
        _result = Result::Cancel;
    });

    _secondaryButton = addButton(buttons.secondary, ButtonRole::Plain);
    connect(_secondaryButton, &QPushButton::clicked, this, [this]() {
        _result = Result::Secondary;
        accept();
    });

    _primaryButton = addButton(buttons.primary, ButtonRole::Accept);
    connect(_primaryButton, &QPushButton::clicked, this, [this]() {
        _result = Result::Primary;
        accept();
    });

    switch (defaultButton) {
    case Result::Primary:
        setDefaultButton(_primaryButton);
        break;
    case Result::Secondary:
        setDefaultButton(_secondaryButton);
        break;
    case Result::Cancel:
    default:
        setDefaultButton(_cancelButton);
        break;
    }
}

FramelessConfirmDialog::Result FramelessConfirmDialog::question(QWidget* parent,
                                                               const QString& title,
                                                               const QString& message,
                                                               const ButtonTexts& buttons,
                                                               Result defaultButton)
{
    FramelessConfirmDialog dialog(title, message, buttons, defaultButton, parent);
    if (dialog.exec() == QDialog::Accepted) {
        return dialog._result;
    }
    return Result::Cancel;
}
