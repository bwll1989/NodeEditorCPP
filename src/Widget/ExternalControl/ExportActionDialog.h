#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QString>

class QLineEdit;
class QPushButton;

namespace Flow {

/** 导出到网页控制台时的动作配置 */
struct ExportActionDraft {
    QString entity;
    QString name;
};

/**
 * @brief 节点导出到网页前的配置对话框（控制地址只读，名称可编辑）
 */
class ExportActionDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ExportActionDialog(const QJsonObject& binding, QWidget* parent = nullptr);

    ExportActionDraft draft() const;

    /** @return 用户确认则 true，并填充 out */
    static bool prompt(QWidget* parent, const QJsonObject& binding, ExportActionDraft& out);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildUi(const QJsonObject& binding);

    static QString defaultName(const QJsonObject& binding);

    QLineEdit* _addressEdit = nullptr;
    QLineEdit* _nameEdit = nullptr;
    QPushButton* _okButton = nullptr;
};

} // namespace Flow
