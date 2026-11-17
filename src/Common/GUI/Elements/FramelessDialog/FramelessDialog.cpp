#include "FramelessDialog/FramelessDialog.hpp"

#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

/** 无边框对话框标题栏：按住左键拖拽移动窗口（统一实现，供 FramelessDialog 使用） */
class FramelessTitleBar final : public QWidget {
public:
    explicit FramelessTitleBar(QWidget* dialog, QWidget* parent = nullptr)
        : QWidget(parent)
        , _dialog(dialog)
    {
        setObjectName(QStringLiteral("framelessDialogHeader"));
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

FramelessDialog::FramelessDialog(QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("framelessDialog"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setModal(true);
    setMinimumWidth(440);
    buildUi();
}

void FramelessDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 14);
    root->setSpacing(12);

    // 标题栏：可拖拽移动窗口
    _titleBar = new FramelessTitleBar(this, this);
    auto* titleRow = new QHBoxLayout(_titleBar);
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(8);

    _titleLabel = new QLabel(_titleBar);
    _titleLabel->setObjectName(QStringLiteral("framelessDialogTitle"));
    QFont titleFont = _titleLabel->font();
    titleFont.setBold(true);
    _titleLabel->setFont(titleFont);
    titleRow->addWidget(_titleLabel);
    titleRow->addStretch();

    _closeButton = new QPushButton(QStringLiteral("\u00d7"), _titleBar);
    _closeButton->setObjectName(QStringLiteral("framelessDialogClose"));
    _closeButton->setFlat(true);
    _closeButton->setCursor(Qt::PointingHandCursor);
    _closeButton->setFocusPolicy(Qt::NoFocus);
    _closeButton->setFixedSize(20, 20);
    _closeButton->setToolTip(tr("关闭"));
    _closeButton->hide();
    connect(_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    titleRow->addWidget(_closeButton);

    root->addWidget(_titleBar);

    // 内容区：子类填充
    _bodyLayout = new QVBoxLayout();
    _bodyLayout->setContentsMargins(0, 0, 0, 0);
    _bodyLayout->setSpacing(10);
    root->addLayout(_bodyLayout);

    // 按钮行：右对齐
    _buttonLayout = new QHBoxLayout();
    _buttonLayout->setContentsMargins(0, 4, 0, 0);
    _buttonLayout->setSpacing(10);
    _buttonLayout->addStretch();
    root->addLayout(_buttonLayout);
}

void FramelessDialog::setDialogTitle(const QString& title)
{
    setWindowTitle(title);
    if (_titleLabel) {
        _titleLabel->setText(title);
        _titleLabel->setVisible(!title.isEmpty());
    }
}

QString FramelessDialog::dialogTitle() const
{
    return _titleLabel ? _titleLabel->text() : QString();
}

void FramelessDialog::setCloseButtonVisible(bool visible)
{
    if (_closeButton) {
        _closeButton->setVisible(visible);
    }
}

bool FramelessDialog::closeButtonVisible() const
{
    return _closeButton && !_closeButton->isHidden();
}

QVBoxLayout* FramelessDialog::bodyLayout()
{
    return _bodyLayout;
}

QHBoxLayout* FramelessDialog::buttonLayout()
{
    return _buttonLayout;
}

QPushButton* FramelessDialog::addButton(const QString& text, ButtonRole role)
{
    auto* button = new QPushButton(text, this);
    button->setCursor(Qt::PointingHandCursor);

    switch (role) {
    case ButtonRole::Accept:
        button->setObjectName(QStringLiteral("framelessDialogPrimary"));
        if (!_defaultButton) {
            _defaultButton = button;
            button->setDefault(true);
            button->setAutoDefault(true);
        }
        break;
    case ButtonRole::Reject:
        button->setObjectName(QStringLiteral("framelessDialogSecondary"));
        connect(button, &QPushButton::clicked, this, &QDialog::reject);
        break;
    case ButtonRole::Plain:
    default:
        break;
    }

    if (_buttonLayout) {
        // stretch 之后追加，保持按钮靠右
        _buttonLayout->addWidget(button);
    }
    return button;
}

void FramelessDialog::setDefaultButton(QPushButton* button)
{
    if (!button) {
        return;
    }
    _defaultButton = button;
    button->setDefault(true);
    button->setAutoDefault(true);
    button->setFocus();
}

void FramelessDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}
