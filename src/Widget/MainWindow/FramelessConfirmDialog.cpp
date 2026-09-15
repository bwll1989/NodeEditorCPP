#include "FramelessConfirmDialog.hpp"

#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

/** 无边框对话框标题栏：支持拖拽移动窗口 */
class DialogTitleBar final : public QWidget {
public:
    explicit DialogTitleBar(QWidget* dialog, QWidget* parent = nullptr)
        : QWidget(parent)
        , _dialog(dialog)
    {
        setObjectName(QStringLiteral("framelessConfirmDialogHeader"));
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(false);
        setCursor(Qt::ArrowCursor);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && _dialog) {
            _dragging = true;
            _pressGlobal = event->globalPosition().toPoint();
            _windowPos = _dialog->frameGeometry().topLeft();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (_dragging && _dialog && (event->buttons() & Qt::LeftButton)) {
            const QPoint delta = event->globalPosition().toPoint() - _pressGlobal;
            _dialog->move(_windowPos + delta);
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            _dragging = false;
        }
        QWidget::mouseReleaseEvent(event);
    }

private:
    QWidget* _dialog = nullptr;
    bool _dragging = false;
    QPoint _pressGlobal;
    QPoint _windowPos;
};

} // namespace

FramelessConfirmDialog::FramelessConfirmDialog(const QString& title,
                                               const QString& message,
                                               const ButtonTexts& buttons,
                                               Result defaultButton,
                                               QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("framelessConfirmDialog"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setModal(true);
    setMinimumWidth(480);
    setMinimumHeight(160);
    setWindowTitle(title);
    buildUi(title, message, buttons, defaultButton);
}

void FramelessConfirmDialog::buildUi(const QString& title,
                                     const QString& message,
                                     const ButtonTexts& buttons,
                                     Result defaultButton)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 18);
    root->setSpacing(16);

    auto* titleBar = new DialogTitleBar(this, this);
    auto* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(8);

    auto* titleLabel = new QLabel(title, titleBar);
    titleLabel->setObjectName(QStringLiteral("framelessConfirmDialogTitle"));
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleRow->addWidget(titleLabel);
    root->addWidget(titleBar);

    auto* messageLabel = new QLabel(message, this);
    messageLabel->setObjectName(QStringLiteral("framelessConfirmDialogMessage"));
    messageLabel->setWordWrap(true);
    messageLabel->setMinimumWidth(420);
    root->addWidget(messageLabel);

    auto* actionRow = new QHBoxLayout();
    actionRow->setContentsMargins(0, 8, 0, 0);
    actionRow->setSpacing(10);
    actionRow->addStretch();

    _cancelButton = new QPushButton(buttons.cancel, this);
    _cancelButton->setObjectName(QStringLiteral("framelessConfirmDialogCancel"));
    _cancelButton->setCursor(Qt::PointingHandCursor);
    connect(_cancelButton, &QPushButton::clicked, this, [this]() {
        _result = Result::Cancel;
        reject();
    });

    _secondaryButton = new QPushButton(buttons.secondary, this);
    _secondaryButton->setObjectName(QStringLiteral("framelessConfirmDialogNo"));
    _secondaryButton->setCursor(Qt::PointingHandCursor);
    connect(_secondaryButton, &QPushButton::clicked, this, [this]() {
        _result = Result::Secondary;
        accept();
    });

    _primaryButton = new QPushButton(buttons.primary, this);
    _primaryButton->setObjectName(QStringLiteral("framelessConfirmDialogYes"));
    _primaryButton->setCursor(Qt::PointingHandCursor);
    connect(_primaryButton, &QPushButton::clicked, this, [this]() {
        _result = Result::Primary;
        accept();
    });

    actionRow->addWidget(_cancelButton);
    actionRow->addWidget(_secondaryButton);
    actionRow->addWidget(_primaryButton);
    root->addLayout(actionRow);

    const auto setDefault = [](QPushButton* button) {
        if (!button) {
            return;
        }
        button->setDefault(true);
        button->setAutoDefault(true);
        button->setFocus();
    };

    switch (defaultButton) {
    case Result::Primary:
        setDefault(_primaryButton);
        break;
    case Result::Secondary:
        setDefault(_secondaryButton);
        break;
    case Result::Cancel:
    default:
        setDefault(_cancelButton);
        break;
    }
}

void FramelessConfirmDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        _result = Result::Cancel;
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
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
