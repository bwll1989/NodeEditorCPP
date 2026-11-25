#include "FramelessDialog/FramelessDialog.hpp"

#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow> // startSystemMove（系统级窗口拖动）

namespace {

/** 无边框对话框标题栏：按住左键拖拽移动窗口（统一实现，供 FramelessDialog 使用）
 *
 * 【2026-11-25 拖拽断手修复】旧实现手写 press/move（无 mouseGrab）——
 * 鼠标快速移动时 move 事件投递给光标下的其他控件、标题栏收不到 → 拖动
 * 稍快即断手（用户报"弹窗拖拽很难选中"，片段属性同样症状）。改
 * QWindow::startSystemMove()：系统级窗口拖动（WM_SYSCOMMAND），事件
 * 由 OS 全程接管永不丢；不支持系统拖动的环境回退手写路径 + grabMouse。
 */
class FramelessTitleBar final : public QWidget {
public:
    explicit FramelessTitleBar(QWidget* dialog, QWidget* parent = nullptr)
        : QWidget(parent)
        , _dialog(dialog)
    {
        setObjectName(QStringLiteral("framelessDialogHeader"));
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(false);
        // 【2026-11-25 用户定案】拖动热区光标 = 手型：悬停张开手（可拖
        // affordance），手写回退路径拖动中变握拳；startSystemMove 接管后
        // 光标由系统绘制（Windows 拖窗移动光标），无需 Qt 侧干预。
        setCursor(Qt::OpenHandCursor);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && _dialog) {
            if (auto* wh = _dialog->windowHandle(); wh && wh->startSystemMove()) {
                event->accept();
                return; // 系统接管拖动（含后续 release，无事件回传依赖）
            }
            // 回退：手写拖拽 + 独占鼠标（防 move 丢失断手）
            _dragging   = true;
            _pressGlobal = event->globalPosition().toPoint();
            _windowPos   = _dialog->frameGeometry().topLeft();
            setCursor(Qt::ClosedHandCursor);
            grabMouse();
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
            if (_dragging) {
                _dragging = false;
                setCursor(Qt::OpenHandCursor);
                releaseMouse();
            }
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
    // 【2026-11-25】不参与"最后窗口关闭→退出"判定：主程序退出语义由主
    // 窗口独占——若弹窗（非模态 show）也计入窗口集合，主窗口关闭后应用
    // 不会退出，弹窗孤儿化飘在桌面上。模态用法不受影响（先于主窗口关闭）。
    setAttribute(Qt::WA_QuitOnClose, false);
    setModal(true);
    setMinimumWidth(440);
    buildUi();
    followRootWindow();
}

void FramelessDialog::followRootWindow()
{
    // 【2026-11-25 主程序关闭→弹窗随行（用户报"主程序关闭后弹窗不自动
    // 关闭"）】弹窗是独立顶层窗口，主窗 hide 时不会自动带走它们：托盘
    // 最小化路径 closeEvent→hide() 主窗+ignore，进程继续运行 → 属性弹窗
    // 孤儿化悬浮。跟随策略：根窗 Hide→弹窗 hide（hide 而非 close：close
    // 会走 reject 取消还原链，丢弃未确认编辑态）；根窗 Show→弹窗还原置前。
    // 状态记在动态属性上（不加成员——零类布局变更，消费者无需重编）。
    //
    // 【13:58 教训：严禁在根窗 destroyed 里 close() 弹窗】Qt 源码
    //（QWidget::~QWidget）destroyed 信号发射于 deleteChildren **之前**——
    // 此时 ~MainWindow 成员（scene 宿主 DataflowViewsManger 等）已析构、
    // 子树尚未回收；此处 close()→QDialog::closeEvent→rejected→宿主的
    // 取消还原槽访问 scene_/refreshViews = 悬垂解引用（用户实测：指针
    // 悬挂+进程挂起+鼠标繁忙）。弹窗是 QObject 子级，父链析构自然回收，
    // destroyed 路径无需（也不得）干预。
    if (!parentWidget())
        return;
    QWidget* root = parentWidget()->window();
    if (!root || root == this)
        return;
    root->installEventFilter(this);
}

bool FramelessDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (parentWidget()) {
        QWidget* root = parentWidget()->window();
        if (root && watched == root) {
            if (event->type() == QEvent::Hide && isVisible()) {
                setProperty("followRootShown", true);
                hide();
            } else if (event->type() == QEvent::Show
                       && property("followRootShown").toBool()) {
                setProperty("followRootShown", false);
                show();
                raise();
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void FramelessDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 14);
    root->setSpacing(12);

    // 标题栏：可拖拽移动窗口
    _titleBar = new FramelessTitleBar(this, this);
    auto* titleRow = new QHBoxLayout(_titleBar);
    // 上下各 6px 内边距：热区加高到 ~32px（原只有一行字高，光标很难
    // 按进拖动区——用户报"不好选中"第二主因；手型光标是第一 affordance）
    titleRow->setContentsMargins(0, 6, 0, 6);
    titleRow->setSpacing(8);

    _titleLabel = new QLabel(_titleBar);
    _titleLabel->setObjectName(QStringLiteral("framelessDialogTitle"));
    QFont titleFont = _titleLabel->font();
    titleFont.setBold(true);
    _titleLabel->setFont(titleFont);
    titleRow->addWidget(_titleLabel);
    titleRow->addStretch();

    // 【2026-11-25 关闭按钮整体移除（用户定案"彻底去掉"）】按钮无图标
    // 只剩灰块（QSS 对文字 × 的渲染问题），且按钮行本就有"取消"——
    // 关闭语义由 Esc / 取消按钮 / X 窗口菜单承担，标题栏不放按钮。
    // （_closeButton 成员与 setCloseButtonVisible API 一并删除，
    // AbstractClipModel 两处 setCloseButtonVisible(false) 调用同步清理）

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

QVBoxLayout* FramelessDialog::bodyLayout()
{
    return _bodyLayout;
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
