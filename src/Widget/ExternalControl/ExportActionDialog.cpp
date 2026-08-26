#include "ExportActionDialog.h"

#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace Flow {

namespace {

/** 无边框对话框标题栏：支持拖拽移动窗口 */
class DialogTitleBar final : public QWidget {
public:
    explicit DialogTitleBar(QWidget* dialog, QWidget* parent = nullptr)
        : QWidget(parent)
        , _dialog(dialog)
    {
        setObjectName(QStringLiteral("exportActionDialogHeader"));
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

QString pickString(const QJsonObject& obj, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        const QString value = obj.value(QLatin1String(key)).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return {};
}

} // namespace

QString ExportActionDialog::defaultName(const QJsonObject& binding)
{
    const QString name = pickString(binding, {"suggestedName", "name", "nodeName", "remarks"});
    if (!name.isEmpty()) {
        return name;
    }
    return binding.value(QStringLiteral("entity")).toString();
}

ExportActionDialog::ExportActionDialog(const QJsonObject& binding, QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("exportActionDialog"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setModal(true);
    setMinimumWidth(440);
    setWindowTitle(tr("导出到网页控制台"));
    buildUi(binding);
}

void ExportActionDialog::buildUi(const QJsonObject& binding)
{
    const QString entity = binding.value(QStringLiteral("entity")).toString();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 14);
    root->setSpacing(12);

    auto* titleBar = new DialogTitleBar(this, this);
    auto* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(8);

    auto* titleLabel = new QLabel(tr("导出到网页控制台"), titleBar);
    titleLabel->setObjectName(QStringLiteral("exportActionDialogTitle"));
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleRow->addWidget(titleLabel);
    root->addWidget(titleBar);

    _addressEdit = new QLineEdit(entity, this);
    _addressEdit->setObjectName(QStringLiteral("exportActionDialogAddress"));
    _addressEdit->setReadOnly(true);

    _nameEdit = new QLineEdit(defaultName(binding), this);
    _nameEdit->setObjectName(QStringLiteral("exportActionDialogName"));
    _nameEdit->setPlaceholderText(tr("显示名称"));

    auto* form = new QFormLayout();
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->addRow(tr("控制地址"), _addressEdit);
    form->addRow(tr("显示名称"), _nameEdit);
    root->addLayout(form);

    auto* actionRow = new QHBoxLayout();
    actionRow->setContentsMargins(0, 4, 0, 0);
    actionRow->addStretch();

    auto* cancelButton = new QPushButton(tr("取消"), this);
    cancelButton->setObjectName(QStringLiteral("exportActionDialogCancel"));
    cancelButton->setCursor(Qt::PointingHandCursor);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    _okButton = new QPushButton(tr("确定"), this);
    _okButton->setObjectName(QStringLiteral("exportActionDialogOk"));
    _okButton->setDefault(true);
    _okButton->setAutoDefault(true);
    _okButton->setCursor(Qt::PointingHandCursor);
    connect(_okButton, &QPushButton::clicked, this, &QDialog::accept);

    actionRow->addWidget(cancelButton);
    actionRow->addWidget(_okButton);
    root->addLayout(actionRow);
}

ExportActionDraft ExportActionDialog::draft() const
{
    ExportActionDraft out;
    out.entity = _addressEdit ? _addressEdit->text().trimmed() : QString();
    out.name = _nameEdit ? _nameEdit->text().trimmed() : QString();
    return out;
}

void ExportActionDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

bool ExportActionDialog::prompt(QWidget* parent, const QJsonObject& binding, ExportActionDraft& out)
{
    ExportActionDialog dialog(binding, parent);
    dialog._nameEdit->setFocus();
    dialog._nameEdit->selectAll();

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    out = dialog.draft();
    if (out.entity.isEmpty()) {
        out.entity = binding.value(QStringLiteral("entity")).toString().trimmed();
    }
    if (out.name.isEmpty()) {
        out.name = defaultName(binding);
    }
    return !out.entity.isEmpty();
}

} // namespace Flow
