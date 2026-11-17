#include "ExportActionDialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace Flow {

QString ExportActionDialog::defaultName(const QJsonObject& binding)
{
    const QStringList keys = {QStringLiteral("suggestedName"),
                              QStringLiteral("name"),
                              QStringLiteral("nodeName"),
                              QStringLiteral("remarks")};
    for (const QString& key : keys) {
        const QString value = binding.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return binding.value(QStringLiteral("entity")).toString();
}

ExportActionDialog::ExportActionDialog(const QJsonObject& binding, QWidget* parent)
    : FramelessDialog(parent)
{
    setMinimumWidth(440);
    setDialogTitle(tr("导出到网页控制台"));
    buildUi(binding);
}

void ExportActionDialog::buildUi(const QJsonObject& binding)
{
    const QString entity = binding.value(QStringLiteral("entity")).toString();

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
    bodyLayout()->addLayout(form);

    addButton(tr("取消"), ButtonRole::Reject);
    auto* okButton = addButton(tr("确定"), ButtonRole::Accept);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
}

ExportActionDraft ExportActionDialog::draft() const
{
    ExportActionDraft out;
    out.entity = _addressEdit ? _addressEdit->text().trimmed() : QString();
    out.name = _nameEdit ? _nameEdit->text().trimmed() : QString();
    return out;
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
