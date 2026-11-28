#pragma once

#include "FramelessDialog/FramelessDialog.hpp"

#include <QString>

class QPushButton;

/**
 * @brief 无边框三按钮确认框（继承统一弹窗基类 FramelessDialog）
 *
 * Result 语义由调用方按钮文案决定（如「最小化到托盘 / 退出程序 / 取消」）。
 * 按钮样式与交互（拖拽 / Esc / 模态）全部由基类提供：
 * - cancel  → ButtonRole::Reject（次要样式，自动 reject()）
 * - secondary → ButtonRole::Plain（中性样式，调用方决定语义）
 * - primary → ButtonRole::Accept（绿色主样式）
 */
class FramelessConfirmDialog final : public FramelessDialog {
    Q_OBJECT
public:
    enum class Result {
        Primary,   ///< 主操作按钮（绿色主样式）
        Secondary, ///< 次操作按钮
        Cancel
    };

    struct ButtonTexts {
        QString primary = tr("确定");
        QString secondary = tr("否");
        QString cancel = tr("取消");
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

private:
    void buildUi(const QString& message,
                 const ButtonTexts& buttons,
                 Result defaultButton);

    Result _result = Result::Cancel;
    QPushButton* _primaryButton = nullptr;
    QPushButton* _secondaryButton = nullptr;
    QPushButton* _cancelButton = nullptr;
};
