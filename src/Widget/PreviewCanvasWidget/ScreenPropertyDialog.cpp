// ============================================================================
//  ScreenPropertyDialog.cpp —— Screen 属性编辑对话框实现（第五迭代；
//  2026-11-17 绑定简化：设备发现整体退役，绑定页回归纯手输表单）
//
//  与 ClipPropertyDialog 同一套交互约定：预填 → exec → applyTo 写回；
//  取消 = 丢弃全部改动（几何/变形有实时链，取消时由宿主还原备份）。
//  范围与场景画布（默认 1920×1080 上限）一致。
//
//  交互设计：
//    * 「通用」页几何六字段实时预览（spinbox 变化即经 geometry_live_
//      转发宿主写场景+自动下发，取消还原）；
//    * 「几何结构」页 = ScreenWarpPanel 内嵌（编辑实时转发，取消还原）；
//    * 「绑定」页 = 纯手输四字段表单（id/host/port/配对码）——设备发现
//      （UDP 扫描）2026-11-17 整体退役：广播类发现在防火墙/多网卡/跨
//      网段环境下"经常搜不到"是固有属性（广播静默失败环节太多，多接口
//      广播补丁也救不全），交互复杂度远超便利收益。绑定 = 手输一次随
//      工程保存；连接成功后 display 经 RegisterAck 自动采用指派 id。
//
//  【2026-11-13 收编恢复】MediaServer/src/editor 原版原文（用户要求属性
//  面板以合并前为准）；宿主接线见 SceneCanvasWidget.cpp openScreenProperty。
// ============================================================================

#include "ScreenPropertyDialog.hpp"

#include "ScreenWarpPanel.hpp" // 输出变形编辑面板（「几何结构」页内嵌）

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace ms {

ScreenPropertyDialog::ScreenPropertyDialog(Screen& screen,
                                           const std::string& cur_host,
                                           std::uint16_t cur_port,
                                           const std::string& cur_pairing_code,
                                           QWidget* parent)
    : FramelessDialog(parent) { // 预填直接读入参引用
    setDialogTitle(tr("Display 属性"));
    setCloseButtonVisible(true); // 标题栏 × = 取消（reject）

    auto* form_box = new QGroupBox("画布位置与显示大小（像素）", this);
    auto* form     = new QFormLayout(form_box);

    // ---- 投影模式行+字段退役（2026-11-20 用户定案，不考虑兼容）：纯 2D
    // 取景范式下 Display 恒为取景窗（Framed），模式选择无意义；模型/
    // proto 字段 projectionMode 与渲染分支一并删除（渲染端本就未消费）。

    // ---- 位置/大小：允许负坐标（画布外扩屏）与 1~8192 尺寸 --------------
    // 【2026-11-18 控件风格统一】全部 spinbox → FloatDragValueWidget/
    // IntDragValueWidget（与节点属性面板同风格：横向拖拽调值/双击键入/
    // 修饰键步进 Shift×0.1 Ctrl×10）。valueChanged 信号同型，实时链
    // connect 写法不变；FloatDrag 无环绕——rotation 到 ±3600° 边界停住
    //（原 QDoubleSpinBox wrapping 退役，影响可忽略）
    x_ = new FloatDragValueWidget(form_box);
    x_->setRange(-8192.0, 8192.0);
    x_->setDecimals(1);
    x_->setSingleStep(10.0);
    x_->setValue(screen.x);

    y_ = new FloatDragValueWidget(form_box);
    y_->setRange(-8192.0, 8192.0);
    y_->setDecimals(1);
    y_->setSingleStep(10.0);
    y_->setValue(screen.y);

    w_ = new FloatDragValueWidget(form_box);
    w_->setRange(1.0, 8192.0);
    w_->setDecimals(1);
    w_->setSingleStep(10.0);
    w_->setValue(screen.w > 0 ? screen.w : 960.0);

    h_ = new FloatDragValueWidget(form_box);
    h_->setRange(1.0, 8192.0);
    h_->setDecimals(1);
    h_->setSingleStep(10.0);
    h_->setValue(screen.h > 0 ? screen.h : 540.0);

    form->addRow("位置 X：", x_);
    form->addRow("位置 Y：", y_);
    form->addRow("宽度：", w_);
    form->addRow("高度：", h_);

    // ---- 三轴旋转（2026-11-20 用户定案：标题按轴命名 X/Y/Z，全 float）--
    // 轴语义与片段 Tween 旋转三维打包一致（ClipSchema v4 / TweenSection）：
    // X = 俯仰（绕画布 X 轴，正=顶边后仰）、Y = 偏航（绕画布 Y 轴，
    // 正=右缘远离）、Z = 滚转（绕画布 Z 轴面内旋转，顺时针）。
    // 步进统一 5°（原 Z 滚转 90° 步进退役——用户反馈步进幅度太大）。
    // X/Y 钳 ±85°：89° 时单应已近奇异（用户实证画面异常拉扯——数学未
    // 翻面但透视极端），85° 留安全边距；ScreenMapper 端另有
    // solveHomography 失败→恒等仿射防御兜底。Z 滚转 ±3600°（10 圈）。
    rot_x_ = new FloatDragValueWidget(form_box);
    rot_x_->setRange(-85.0, 85.0);
    rot_x_->setDecimals(1);
    rot_x_->setSingleStep(5.0);
    rot_x_->setValue(screen.rot_x);
    rot_x_->setToolTip(tr("取景窗姿态：旋转 X = 俯仰（度，绕画布 X 轴）。\n"
                          "服务地面屏/斜幕等非正交安装；0 = 无姿态（仿射映射）。"));

    rot_y_ = new FloatDragValueWidget(form_box);
    rot_y_->setRange(-85.0, 85.0);
    rot_y_->setDecimals(1);
    rot_y_->setSingleStep(5.0);
    rot_y_->setValue(screen.rot_y);
    rot_y_->setToolTip(tr("取景窗姿态：旋转 Y = 偏航（度，绕画布 Y 轴）。\n"
                          "非 0 时取景走透视单应（内容不缩放语义不变）。"));

    rotation_ = new FloatDragValueWidget(form_box);
    rotation_->setRange(-3600.0, 3600.0);
    rotation_->setDecimals(1);
    rotation_->setSingleStep(5.0);
    rotation_->setValue(screen.rotation);
    rotation_->setToolTip(tr("取景窗姿态：旋转 Z = 滚转（度，面内顺时针绕中心）\n"
                             "——竖屏/异形拼接取景窗。"));

    form->addRow("旋转X：", rot_x_);
    form->addRow("旋转Y：", rot_y_);
    form->addRow("旋转Z：", rotation_);

    // 【2026-11-18 定案】层级 z 行移除：Screen = 取景窗，无叠放语义
    //（模型字段已随 2026-11-20 清理删除；多屏重叠命中序 = 场景追加序）。

    // ---- 输出校色（2026-11-20 Display 级 RGB 增益 + Gamma）--------------
    // 本屏输出环节的最终校色：素材采样后逐通道乘增益，gamma≠1 再
    // pow(c,1/γ)。与片段级（片段属性面板 rgb_gain/gamma 行）在解算层
    // 合成（增益相乘、gamma 相乘）。1=默认（逐位历史行为）；gain=0 =
    // 该通道压黑；>1 提亮。实时链与几何字段同构（变化即下发）。
    auto* grade_box = new QGroupBox("输出校色", this);
    auto* grade_form = new QFormLayout(grade_box);

    gain_r_ = new FloatDragValueWidget(grade_box);
    gain_r_->setRange(0.0, 2.0);
    gain_r_->setDecimals(2);
    gain_r_->setSingleStep(0.05);
    gain_r_->setValue(screen.rgb_gain[0]);
    gain_r_->setToolTip(tr("R 通道线性增益：1=不变，0=压黑，>1 提升。"));

    gain_g_ = new FloatDragValueWidget(grade_box);
    gain_g_->setRange(0.0, 2.0);
    gain_g_->setDecimals(2);
    gain_g_->setSingleStep(0.05);
    gain_g_->setValue(screen.rgb_gain[1]);
    gain_g_->setToolTip(tr("G 通道线性增益：1=不变，0=压黑，>1 提升。"));

    gain_b_ = new FloatDragValueWidget(grade_box);
    gain_b_->setRange(0.0, 2.0);
    gain_b_->setDecimals(2);
    gain_b_->setSingleStep(0.05);
    gain_b_->setValue(screen.rgb_gain[2]);
    gain_b_->setToolTip(tr("B 通道线性增益：1=不变，0=压黑，>1 提升。"));

    gamma_ = new FloatDragValueWidget(grade_box);
    gamma_->setRange(0.1, 4.0);
    gamma_->setDecimals(2);
    gamma_->setSingleStep(0.05);
    gamma_->setValue(screen.gamma);
    gamma_->setToolTip(tr("伽马指数：1.0=不校正；<1 画面变亮，>1 画面变暗\n"
                          "（c' = c^(1/γ)——补正显示端物理屏伽马偏差）。"));

    grade_form->addRow("R 增益：", gain_r_);
    grade_form->addRow("G 增益：", gain_g_);
    grade_form->addRow("B 增益：", gain_b_);
    grade_form->addRow("Gamma：", gamma_);

    // ---- 几何八字段实时预览（2026-11-11 用户需求：调属性即时可见）--------
    // 旧语义只有 OK 才写回——editor 画布/display 输出都要等确认。现与 warp
    // 实时链同构：spinbox 每次值变化（点箭头/步进/键入后提交）即把表单几何
    // 发给宿主回调（写工作副本 + refreshViews → sceneEditedLive → 适配器
    // requestAutoPublish 200ms 节流下发）。回调收到的 Screen 仅几何字段有效。
    // 【2026-11-18 修复】原外层 if(geometry_live_) 恒 false——宿主在构造
    // 之后才经 setGeometryLiveApply 注入回调（本类 setter 只存函数无补连
    // 逻辑），8 个 connect 从未建立，实时链整个死亡（调参必须 OK 才生效，
    // 用户实证）。改无条件 connect：lambda 内已有空回调防御，未注入时
    // 零开销空转；注入时机无关紧要（与 warp 链同构——面板信号构造时接、
    // 运行时查回调）。
    auto emitGeometryLive = [this]() {
        if (!geometry_live_) return;
        Screen g; // 仅几何+校色字段有效（其余字段宿主不取）
        g.x        = x_->value();
        g.y        = y_->value();
        g.w        = w_->value();
        g.h        = h_->value();
        g.rotation = rotation_->value();
        g.rot_x    = rot_x_->value();
        g.rot_y    = rot_y_->value();
        // 输出校色（2026-11-20）：与几何字段同链实时下发
        g.rgb_gain[0] = gain_r_->value();
        g.rgb_gain[1] = gain_g_->value();
        g.rgb_gain[2] = gain_b_->value();
        g.gamma       = gamma_->value();
        // g.z 不再提供——宿主侧不拷贝 z（层级行已退役）
        geometry_live_(g);
    };
    connect(x_,        &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(y_,        &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(w_,        &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(h_,        &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(rotation_, &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(rot_x_,    &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(rot_y_,    &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(gain_r_,   &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(gain_g_,   &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(gain_b_,   &FloatDragValueWidget::valueChanged, this, emitGeometryLive);
    connect(gamma_,    &FloatDragValueWidget::valueChanged, this, emitGeometryLive);

    // ---- 输出变形（2026-11-11 WATCHOUT 式内嵌）----------------------------
    // 编辑面板直接内嵌「几何结构」页（用户需求：不要弹窗）——模式/密度/
    // 重置/网格编辑区与原变形弹窗逐一同构；编辑中实时变化转发宿主
    //（写场景+自动下发，display 拖动即见）；属性框整体取消由宿主还原
    //（打开前备份），面板自身不再持暂存——applyTo 时从面板终态写回。
    warp_panel_ = new ScreenWarpPanel(screen.warp, this);
    warp_panel_->setLiveApply([this](const ScreenWarp& w) {
        if (warp_live_) warp_live_(w);
    });

    // ---- 绑定显示端（2026-11-17 简化：纯手输四字段表单）------------------
    // 设备发现（UDP 扫描）已整体退役：广播类发现在防火墙/多网卡/跨网段
    // 环境下"经常搜不到"是固有属性（可靠性做不到手输的水准），交互复杂
    // 度又远超便利收益。绑定 = id + ip:port 手输一次（随工程保存）；
    // 连接成功后 display 经 RegisterAck 自动采用指派 id（无需预配置）。
    auto* bind_box = new QGroupBox("显示端绑定", this);
    auto* bind_lay = new QVBoxLayout(bind_box);
    auto* bind_form = new QFormLayout;
    player_id_ = new QLineEdit(QString::fromStdString(screen.player_id),
                               bind_box);
    player_id_->setPlaceholderText("显示端标识；连接后 display 自动采用");
    bind_form->addRow("Player ID：", player_id_);
    host_ = new QLineEdit(QString::fromStdString(cur_host), bind_box);
    host_->setPlaceholderText("显示端 IP（display 所在主机）");
    bind_form->addRow("主机：", host_);
    port_ = new IntDragValueWidget(bind_box);
    port_->setRange(1, 65535);
    port_->setValue(cur_port);
    bind_form->addRow("端口：", port_);
    // 配对码（2026-11-03 多 Editor 隔离）：与显示端 config.ini 的
    // [display] pairing_code 一致才接入；留空 = 不认证（默认零配置互通）
    pairing_code_ = new QLineEdit(QString::fromStdString(cur_pairing_code),
                                  bind_box);
    pairing_code_->setPlaceholderText("留空不认证；填码须与显示端配置一致");
    bind_form->addRow("配对码：", pairing_code_);
    bind_lay->addLayout(bind_form);

    auto* hint = new QLabel(
        tr("按 display 所在主机的 IP 与监听端口填写（默认 17100）；"
           "OK 后自动连接，连接成功 display 将采用上方 Player ID。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: gray; font-size: 11px;");
    bind_lay->addWidget(hint);

    // ---- 按钮（FramelessDialog 统一按钮行，右对齐）------------------------
    auto* ok_btn = addButton(tr("确定"), ButtonRole::Accept);
    connect(ok_btn, &QPushButton::clicked, this, &QDialog::accept);
    addButton(tr("取消"), ButtonRole::Reject); // 自动连接 reject()

    // ---- WATCHOUT 式分页（2026-11-11）：通用 / 几何结构 / 绑定 ------------
    // 对齐 WATCHOUT 显示设置对话框的页组织：第一页通用（位置/大小/旋转/
    // 屏姿态），第二页几何结构（内嵌输出变形网格编辑，不弹窗），第三页绑定
    //（显示端手输登记——WATCHOUT 通用页"计算机"字段同型）。
    auto* tabs = new QTabWidget(this);
    {
        auto* general_page = new QWidget(tabs);
        auto* general_lay  = new QVBoxLayout(general_page);
        general_lay->addWidget(form_box);
        general_lay->addWidget(grade_box); // 输出校色（2026-11-20）
        tabs->addTab(general_page, "通用");

        auto* geom_page = new QWidget(tabs);
        auto* geom_lay  = new QVBoxLayout(geom_page);
        geom_lay->addWidget(warp_panel_);
        tabs->addTab(geom_page, "几何结构");

        auto* bind_page = new QWidget(tabs);
        auto* bind_lay  = new QVBoxLayout(bind_page);
        bind_lay->addWidget(bind_box);
        tabs->addTab(bind_page, "绑定");
    }

    bodyLayout()->addWidget(tabs); // 内容进统一弹窗内容区（按钮行基类已建）
}

void ScreenPropertyDialog::applyTo(Screen& s) const {
    s.x         = x_->value();
    s.y         = y_->value();
    s.w         = w_->value();
    s.h         = h_->value();
    s.rotation  = rotation_->value();
    s.rot_x     = rot_x_->value();
    s.rot_y     = rot_y_->value();
    // 输出校色（2026-11-20 Display 级）：与几何字段同语义写回
    s.rgb_gain[0] = gain_r_->value();
    s.rgb_gain[1] = gain_g_->value();
    s.rgb_gain[2] = gain_b_->value();
    s.gamma       = gamma_->value();
    // s.z 已随模型字段删除（层级行 2026-11-18 退役）
    // projectionMode 已退役（2026-11-20 用户定案）——模型字段已删。
    s.player_id = player_id_->text().toStdString();
    // 输出变形 = 「几何结构」页内嵌面板的当前编辑状态（2026-11-11 内嵌化：
    // 面板即唯一真值——无变形时写默认清空值，与旧弹窗 OK 语义一致）
    warp_panel_->applyTo(s.warp);
}

std::string ScreenPropertyDialog::displayHost() const {
    return host_->text().toStdString();
}

std::uint16_t ScreenPropertyDialog::displayPort() const {
    return static_cast<std::uint16_t>(port_->value());
}

std::string ScreenPropertyDialog::displayPairingCode() const {
    return pairing_code_->text().toStdString();
}

} // namespace ms
