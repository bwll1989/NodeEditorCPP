#pragma once

#include <QDialog>
#include <QString>

class QKeyEvent;
class QPushButton;

/**
 * @brief 无边框三按钮确认框，样式对齐 ExportActionDialog
 *
 * Result 语义由调用方按钮文案决定（如「最小化到托盘 / 退出程序 / 取消」）。
 */
class FramelessConfirmDialog final : public QDialog {
    Q_OBJECT
public:
    enum class Result {
        Primary,   ///< 主操作按钮（蓝色）
        Secondary, ///< 次操作按钮
        Cancel
    };

    struct ButtonTexts {
        QString primary = QStringLiteral("确定");
        QString secondary = QStringLiteral("否");
        QString cancel = QStringLiteral("取消");
    };

    explicit FramelessConfirmDialog(const QString& title,
                                    const QString& message,
                                    const ButtonTexts& buttons = {},
                                    Result defaultButton = Result::Cancel,
                                    QWidget* parent = nullptr);

    /** @brief 模态询问；关闭窗口或 Esc 视为 Cancel */
    static Result question(QWidget* parent,
                           const QString& title,
                           const QString& message,
                           const ButtonTexts& buttons = {},
                           Result defaultButton = Result::Cancel);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildUi(const QString& title,
                 const QString& message,
                 const ButtonTexts& buttons,
                 Result defaultButton);

    Result _result = Result::Cancel;
    QPushButton* _primaryButton = nullptr;
    QPushButton* _secondaryButton = nullptr;
    QPushButton* _cancelButton = nullptr;
};
