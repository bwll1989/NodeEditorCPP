// ============================================================================
//  ScreenWarpPanel.cpp —— 屏幕输出变形编辑面板实现（2026-11-11 自
//  ScreenWarpDialog 原样平移；逻辑不变，仅宿主容器从 QDialog 改 QWidget）
//
//  坐标约定（三域对齐）：
//    * 数据域：节点位置归一化 [0,1]、y 向下、行主序（SceneModel::ScreenWarp）；
//    * 编辑区 widget 坐标：y 同样向下——直接线性映射（margin 内缩），
//      无 GL 域的翻转问题；
//    * 渲染端（GlRenderer::buildWarpMesh）：同一数据域 × 输出像素 +
//      gl_y 翻转——编辑器所见即 display 所出（同一归一化曲面）。
// ============================================================================

#include "ScreenWarpPanel.hpp"

#include <algorithm>
#include <cmath>

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "model/SceneModel.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// WarpEditArea —— 自绘拖拽编辑区（无 Q_OBJECT：纯绘制 + 鼠标交互）
// ---------------------------------------------------------------------------
class ScreenWarpPanel::WarpEditArea : public QWidget {
public:
    explicit WarpEditArea(QWidget* parent) : QWidget(parent) {
        setMinimumSize(460, 280);
        setMouseTracking(false); // 仅按住拖拽，不需要 hover 跟踪
        setFocusPolicy(Qt::ClickFocus); // 点击获得焦点 → 接收方向键微调
    }

    // ---- 数据访问（面板侧重采样/写回用）-----------------------------------
    void setGrid(int cols, int rows, std::vector<double> pts) {
        cols_ = cols;
        rows_ = rows;
        pts_  = std::move(pts);
        sel_idx_ = -1; // 网格重建（密度/重置）：选中下标语义失效
        update();
    }
    int cols() const noexcept { return cols_; }
    int rows() const noexcept { return rows_; }
    const std::vector<double>& points() const noexcept { return pts_; }

    /// 【实时预览】编辑变化通知（拖拽 move/release 后由面板发实时回调）
    void setOnChange(std::function<void()> fn) { on_change_ = std::move(fn); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        // 底色 = 输出屏隐喻（深灰面板）
        p.fillRect(rect(), QColor(28, 28, 32));

        const QRectF area = contentArea();

        // ---- 恒等参考网格（虚线；未变形时与实线重合）-----------------------
        QPen ghost(QColor(255, 255, 255, 46), 1.0, Qt::DashLine);
        p.setPen(ghost);
        for (int c = 0; c < cols_; ++c) {
            const double u = cols_ == 1 ? 0.0
                                        : static_cast<double>(c) / (cols_ - 1);
            const qreal x = area.left() + u * area.width();
            p.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
        }
        for (int r = 0; r < rows_; ++r) {
            const double v = rows_ == 1 ? 0.0
                                        : static_cast<double>(r) / (rows_ - 1);
            const qreal y = area.top() + v * area.height();
            p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        }

        // ---- 变形网格（实线折线 = 节点位置的分片双线性曲面的边）------------
        p.setPen(QPen(QColor(90, 200, 250), 1.6));
        for (int r = 0; r < rows_; ++r)
            for (int c = 0; c + 1 < cols_; ++c)
                p.drawLine(nodePos(c, r, area), nodePos(c + 1, r, area));
        for (int c = 0; c < cols_; ++c)
            for (int r = 0; r + 1 < rows_; ++r)
                p.drawLine(nodePos(c, r, area), nodePos(c, r + 1, area));

        // ---- 节点：拖拽中高亮放大 > 选中描边 > 普通白色实心 ---------------
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                const int idx = r * cols_ + c;
                const QPointF q = nodePos(c, r, area);
                if (idx == drag_idx_) {
                    p.setPen(QPen(QColor(90, 200, 250), 2.0));
                    p.setBrush(QColor(255, 255, 255));
                    p.drawEllipse(q, 6.5, 6.5);
                } else if (idx == sel_idx_) {
                    // 选中（常驻醒目标记，供方向键微调定位）：
                    // 半透明橙色光晕 + 橙粗描边——密集网格中一眼可辨
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(255, 140, 0, 70));
                    p.drawEllipse(q, 9.0, 9.0);
                    p.setPen(QPen(QColor(255, 140, 0), 2.4));
                    p.setBrush(QColor(255, 255, 255));
                    p.drawEllipse(q, 5.8, 5.8);
                } else {
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(235, 235, 235));
                    p.drawEllipse(q, 4.0, 4.0);
                }
            }
        }
    }

    void mousePressEvent(QMouseEvent* e) override {
        const QRectF area = contentArea();
        drag_idx_ = -1;
        // 命中 = 距最近节点中心 < 14px（手感域；不中则无事发生）
        constexpr qreal kHitR2 = 14.0 * 14.0;
        qreal best = kHitR2;
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                const QPointF q = nodePos(c, r, area);
                const qreal dx  = q.x() - e->position().x();
                const qreal dy  = q.y() - e->position().y();
                const qreal dd  = dx * dx + dy * dy;
                if (dd <= best) {
                    best     = dd;
                    drag_idx_ = r * cols_ + c;
                }
            }
        }
        if (drag_idx_ >= 0) {
            sel_idx_ = drag_idx_; // 点击即选中（松手后保持，供方向键微调）
            setFocus();           // 接管键盘：方向键送达 keyPressEvent
        } else {
            sel_idx_ = -1; // 点空白取消选中
        }
        update();
    }

    // 方向键微调选中点（Shift = 大步长）；与拖拽同权触发实时预览。
    // 数据域 y 向下与编辑区同向：Up = y 减、Down = y 加（视觉直觉一致）。
    void keyPressEvent(QKeyEvent* e) override {
        if (sel_idx_ < 0) { QWidget::keyPressEvent(e); return; }
        const double step =
            (e->modifiers() & Qt::ShiftModifier) ? 0.02 : 0.002;
        const std::size_t i = static_cast<std::size_t>(sel_idx_) * 2;
        double x = pts_[i], y = pts_[i + 1];
        switch (e->key()) {
            case Qt::Key_Left:  x -= step; break;
            case Qt::Key_Right: x += step; break;
            case Qt::Key_Up:    y -= step; break;
            case Qt::Key_Down:  y += step; break;
            default: QWidget::keyPressEvent(e); return;
        }
        const double cx = std::clamp(x, 0.0, 1.0);
        const double cy = std::clamp(y, 0.0, 1.0);
        if (cx == pts_[i] && cy == pts_[i + 1]) return; // 已顶边界无变化
        pts_[i]     = cx;
        pts_[i + 1] = cy;
        update();
        if (on_change_) on_change_(); // 微调也实时（与拖拽同链路）
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (drag_idx_ < 0) return;
        const QRectF area = contentArea();
        // widget 坐标 → 归一化 [0,1]（y 向下两域同向，直接映射）
        const double nx = (e->position().x() - area.left()) / area.width();
        const double ny = (e->position().y() - area.top()) / area.height();
        const std::size_t i = static_cast<std::size_t>(drag_idx_) * 2;
        const double cx = std::clamp(nx, 0.0, 1.0);
        const double cy = std::clamp(ny, 0.0, 1.0);
        if (pts_[i] == cx && pts_[i + 1] == cy) return; // 未动不发实时回调
        pts_[i]     = cx;
        pts_[i + 1] = cy;
        update();
        if (on_change_) on_change_(); // 拖拽中实时预览（宿主写场景+下发）
    }

    void mouseReleaseEvent(QMouseEvent*) override {
        if (drag_idx_ >= 0) {
            drag_idx_ = -1;
            update();
            if (on_change_) on_change_(); // 松手终态（拖拽中已实时发过）
        }
    }

private:
    /// 内容区（四周留白；节点/网格的映射域）
    QRectF contentArea() const {
        constexpr qreal m = 22.0;
        return QRectF(m, m, width() - 2 * m, height() - 2 * m);
    }

    /// 节点 (c,r) 数据域 → widget 像素
    QPointF nodePos(int c, int r, const QRectF& area) const {
        const std::size_t i =
            (static_cast<std::size_t>(r) * cols_ + c) * 2;
        return QPointF(area.left() + pts_[i] * area.width(),
                       area.top() + pts_[i + 1] * area.height());
    }

    int cols_ = 4;
    int rows_ = 4;
    std::vector<double> pts_;
    int drag_idx_ = -1; ///< 按住的节点（行主序下标；-1 = 无）
    int sel_idx_ = -1;  ///< 选中的节点（点击后常驻，方向键微调对象）
    std::function<void()> on_change_; ///< 编辑变化通知（面板 → 实时预览）
};

// ---------------------------------------------------------------------------
// 构造：模式/密度/编辑区 + 预填（主体控件与原对话框逐一同构，objectName
// 不变——test_screen_warp_dialog 经薄壳 findChild 继续命中）
// ---------------------------------------------------------------------------
ScreenWarpPanel::ScreenWarpPanel(const ScreenWarp& initial,
                                 QWidget* parent)
    : QWidget(parent) {
    // ---- 预填基准：模式初值取自 initial；编辑区起点恒为恒等网格 ----------
    // （initial 无变形时显示 4×4 恒等网格作为"切到手动网格"的起点，
    //  但模式下拉仍为"无变形"——写回不清空用户未动过的变形状态）
    ScreenWarp seed = initial;
    const int seed_mode = (seed.mode == 1) ? 1 : 0; // 2=MPCDI 预留回落 0
    if (!seed.active()) seed = ScreenWarp::identity(4, 4);
    seed.normalize();

    auto* form_box = new QGroupBox("变形模式", this);
    auto* form     = new QFormLayout(form_box);

    mode_ = new QComboBox(form_box);
    mode_->setObjectName("warp_mode");
    mode_->addItem("无变形（直出）");
    mode_->addItem("手动网格（拖拽控制点）");
    mode_->addItem("MPCDI（预留）");
    // MPCDI 禁用占位（本轮不实装；Qt 禁用项 role = Qt::UserRole-1）
    mode_->setItemData(2, 0, Qt::UserRole - 1);
    mode_->setCurrentIndex(seed_mode);

    cols_ = new QSpinBox(form_box);
    cols_->setObjectName("warp_cols");
    cols_->setRange(2, 16);
    cols_->setValue(seed.cols);
    cols_->setToolTip("网格列数（控制点列数）；变更时按曲面保形重采样");

    rows_ = new QSpinBox(form_box);
    rows_->setObjectName("warp_rows");
    rows_->setRange(2, 16);
    rows_->setValue(seed.rows);
    rows_->setToolTip("网格行数（控制点行数）；变更时按曲面保形重采样");

    reset_btn_ = new QPushButton("重置为恒等网格", form_box);
    reset_btn_->setObjectName("warp_reset");
    reset_btn_->setToolTip("全部控制点回位（取消变形，网格形状不变）");

    auto* dens_row = new QHBoxLayout;
    dens_row->addWidget(cols_);
    dens_row->addWidget(new QLabel("×", form_box));
    dens_row->addWidget(rows_);
    dens_row->addStretch(1);
    dens_row->addWidget(reset_btn_);

    form->addRow("模式：", mode_);
    form->addRow("网格密度：", dens_row);

    // ---- 编辑区 ------------------------------------------------------------
    edit_ = new WarpEditArea(this);
    edit_->setObjectName("warp_edit");
    // 先回填数据再连信号：避免初值 setValue 触发一次无谓重采样
    edit_->setGrid(seed.cols, seed.rows, seed.pts);
    // 拖拽变化 → 面板统一经 emitLive 实时外发（宿主未注入时为空操作）
    edit_->setOnChange([this] { emitLive(); });

    auto* hint = new QLabel(
        tr("拖拽白色控制点实时预览变形效果（输出端即时跟随）；点击选中后"
           "可用方向键微调（Shift 加速）；虚线为未变形参考位置。属性框"
           "取消 = 恢复打开时的变形状态。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: gray; font-size: 11px;");

    // ---- 信号（值回填完成后连接）------------------------------------------
    connect(mode_, &QComboBox::currentIndexChanged,
            this, &ScreenWarpPanel::onModeChanged);
    connect(cols_, &QSpinBox::valueChanged,
            this, &ScreenWarpPanel::onDensityChanged);
    connect(rows_, &QSpinBox::valueChanged,
            this, &ScreenWarpPanel::onDensityChanged);
    connect(reset_btn_, &QPushButton::clicked,
            this, &ScreenWarpPanel::onReset);

    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0); // 内嵌 tab 页：边距由页布局管
    main->addWidget(form_box);
    main->addWidget(edit_, /*stretch=*/1);
    main->addWidget(hint);

    onModeChanged(mode_->currentIndex()); // 同步一次启停状态
}

ScreenWarpPanel::~ScreenWarpPanel() = default;

// ---------------------------------------------------------------------------
// current —— 当前编辑状态 → ScreenWarp（applyTo 与实时回调共用同一构造，
// 保证"实时看到的就是落盘的"）
// ---------------------------------------------------------------------------
ScreenWarp ScreenWarpPanel::current() const {
    if (mode_->currentIndex() != 1)
        return ScreenWarp{}; // 无变形：整体重置（active()==false）
    ScreenWarp w;
    w.mode = 1;
    w.cols = cols_->value();
    w.rows = rows_->value();
    w.pts  = edit_->points();
    w.normalize(); // 坐标钳制 + 尺寸收口（拖拽侧已钳，双保险）
    return w;
}

void ScreenWarpPanel::applyTo(ScreenWarp& w) const { w = current(); }

// ---------------------------------------------------------------------------
// emitLive —— 实时预览外发（拖拽/密度/重置/模式变化共用；未注入 = 空操作）
// ---------------------------------------------------------------------------
void ScreenWarpPanel::emitLive() {
    if (live_apply_) live_apply_(current());
}

// ---------------------------------------------------------------------------
// 模式切换：无变形时灰掉密度与编辑区（保留数值——切回不丢已调形状）
// ---------------------------------------------------------------------------
void ScreenWarpPanel::onModeChanged(int index) {
    const bool manual = (index == 1);
    cols_->setEnabled(manual);
    rows_->setEnabled(manual);
    reset_btn_->setEnabled(manual);
    edit_->setEnabled(manual);
    emitLive(); // 实时：切"无变形"立即恢复直出，切回手动立即上当前网格
}

// ---------------------------------------------------------------------------
// 密度变更：双线性重采样旧节点位置到新网格
//   网格曲面 = 节点位置的分片双线性插值——新节点位置取旧曲面在对应
//   归一化坐标处的值，加密/减密不改变已调形状（保形）。
// ---------------------------------------------------------------------------
void ScreenWarpPanel::onDensityChanged() {
    const int nc = cols_->value();
    const int nr = rows_->value();
    const int oc = edit_->cols();
    const int orw = edit_->rows();
    if (oc == nc && orw == nr) return; // 无实际变化

    const std::vector<double> old_pts = edit_->points();
    std::vector<double> fresh(static_cast<std::size_t>(nc) * nr * 2, 0.0);
    // 旧曲面取值：P(i,j) = old_pts[(j*oc+i)*2+comp]
    const auto P = [&](int i, int j, int comp) -> double {
        return old_pts[(static_cast<std::size_t>(j) * oc + i) * 2 + comp];
    };
    for (int r = 0; r < nr; ++r) {
        for (int c = 0; c < nc; ++c) {
            // 新节点归一化坐标 → 旧网格连续下标
            const double u = nc == 1 ? 0.0
                                     : static_cast<double>(c) / (nc - 1);
            const double v = nr == 1 ? 0.0
                                     : static_cast<double>(r) / (nr - 1);
            const double x = u * (oc - 1);
            const double y = v * (orw - 1);
            const int i0 = static_cast<int>(
                std::min(static_cast<double>(oc - 2), std::floor(x)));
            const int j0 = static_cast<int>(
                std::min(static_cast<double>(orw - 2), std::floor(y)));
            const double f = x - i0;
            const double g = y - j0;
            const std::size_t o =
                (static_cast<std::size_t>(r) * nc + c) * 2;
            for (int comp = 0; comp < 2; ++comp) {
                fresh[o + comp] =
                    (1 - f) * (1 - g) * P(i0, j0, comp) +
                    f * (1 - g) * P(i0 + 1, j0, comp) +
                    (1 - f) * g * P(i0, j0 + 1, comp) +
                    f * g * P(i0 + 1, j0 + 1, comp);
            }
        }
    }
    edit_->setGrid(nc, nr, std::move(fresh));
    emitLive(); // 实时：密度变化立即按新网格下发预览
}

// ---------------------------------------------------------------------------
// 重置：当前密度下的恒等网格
// ---------------------------------------------------------------------------
void ScreenWarpPanel::onReset() {
    ScreenWarp id = ScreenWarp::identity(cols_->value(), rows_->value());
    edit_->setGrid(id.cols, id.rows, std::move(id.pts));
    emitLive(); // 实时：重置立即按恒等网格下发预览
}

} // namespace ms
