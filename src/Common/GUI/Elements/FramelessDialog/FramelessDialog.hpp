#pragma once

#include <QDialog>
#include <QString>

class QKeyEvent;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;

#ifndef GUI_ELEMENTS_EXPORT
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif
#endif

/**
 * @brief 统一风格的无边框弹窗基类（GUIElements）
 *
 * 统一软件内所有弹窗的外观与交互（对齐 ExportActionDialog 的风格）：
 * - 无边框 + 细描边外观（QSS 侧统一 objectName：framelessDialog*）
 * - 标题栏按住左键拖拽移动窗口（系统级 startSystemMove——鼠标快速甩动
 *   不断手，2026-11-25；旧手写 move 因缺 grab 在快速拖动时丢 move 事件）
 * - Esc 关闭（默认 reject()）
 * - bodyLayout() 供子类填充内容；addButton() 生成风格统一的按钮
 *
 * 子类典型用法：
 * @code
 *   MyDialog::MyDialog(QWidget* parent) : FramelessDialog(parent) {
 *       setDialogTitle(tr("标题"));
 *       bodyLayout()->addWidget(...);
 *       auto* ok = addButton(tr("确定"), ButtonRole::Accept);
 *       connect(ok, &QPushButton::clicked, this, &QDialog::accept);
 *       addButton(tr("取消"), ButtonRole::Reject)
 *           ->clicked... // Reject 默认连 reject()
 *   }
 * @endcode
 */
class GUI_ELEMENTS_EXPORT FramelessDialog : public QDialog {
    Q_OBJECT
public:
    /** 按钮角色：决定 QSS 样式与默认行为 */
    enum class ButtonRole {
        Accept,  ///< 主操作按钮（绿色主样式；首个自动成为对话框 default 按钮）
        Reject,  ///< 取消/关闭按钮（次要样式；自动连接 reject()）
        Plain    ///< 中性按钮（走全局 QPushButton 样式；信号由调用方连接）
    };

    explicit FramelessDialog(QWidget* parent = nullptr);

    /** @brief 设置标题（同步到窗口标题与标题栏文字） */
    void setDialogTitle(const QString& title);

    /** @brief 内容区布局（标题栏之下、按钮行之上），子类往这里放控件 */
    QVBoxLayout* bodyLayout();

    /**
     * @brief 追加一个风格统一的按钮到按钮行末尾
     *
     * Accept：主样式（framelessDialogPrimary），首个自动 setDefault(true)；
     * Reject：次要样式并自动连接 reject()；
     * Plain：不设 objectName，由调用方连接信号。
     */
    QPushButton* addButton(const QString& text, ButtonRole role = ButtonRole::Accept);

    /** @brief 指定默认按钮（回车触发并抢焦点；通常传 addButton 返回值） */
    void setDefaultButton(QPushButton* button);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    /// 根窗口（主窗）Hide/Show 跟随：主窗隐藏（托盘最小化）→ 弹窗隐藏
    ///（保编辑态），主窗恢复 → 弹窗还原置前。零布局变更（虚槽覆写）。
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUi();
    /// 根窗口（主窗）Hide/Show 跟随挂接（2026-11-25 主程序关闭→弹窗随行，
    /// 策略见 cpp 实现；根窗 destroyed 路径严禁干预弹窗——cpp 注释有案）
    void followRootWindow();

    QWidget* _titleBar = nullptr;
    QLabel* _titleLabel = nullptr;
    QVBoxLayout* _bodyLayout = nullptr;
    QHBoxLayout* _buttonLayout = nullptr;
    QPushButton* _defaultButton = nullptr;
};
