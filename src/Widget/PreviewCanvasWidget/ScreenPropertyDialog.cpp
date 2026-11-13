// ============================================================================
//  ScreenPropertyDialog.cpp —— Screen 属性编辑对话框实现（第四迭代；
//  2026-11-03 绑定区改造：局域网扫描选设备替代手填 player_id）
//
//  与 ClipPropertyDialog 同一套交互约定：预填 → exec → applyTo 写回；
//  取消 = 丢弃全部改动。范围与场景画布（默认 1920×1080 上限）一致。
//
//  扫描交互设计：
//    * 对话框打开即启动 DiscoveryBrowser（UDP 广播查询）+ 500ms 定时器；
//    * 前 5 轮每拍补发一次查询（防 UDP 丢包），之后只拉结果不再广播；
//    * 列表每拍整体重建（设备数通常个位数，开销可忽略），按 player_id
//      恢复选中——用户选中的设备不会被刷新打断；
//    * 双击设备行 = 三项绑定字段自动填充（player_id/host/port）；
//    * 「重新扫描」随时手动触发（display 后开机等场景）。
//
//  2026-11-11 WATCHOUT 式分页（用户需求，参照 WATCHOUT 显示设置截图）：
//  对话框主体改 QTabWidget 三页——
//    * 「通用」页 = 画布位置/大小/旋转/层级；
//    * 「几何结构」页 = 输出变形编辑面板内嵌（ScreenWarpPanel，不弹窗——
//      模式/网格密度/重置/拖拽控制点编辑区）；
//    * 「绑定」页 = 显示端发现/绑定（扫描 + 手填兜底）。
//    页内控件行为不变（原 GroupBox/面板原样嵌入）；MPCDI/融合按同结构
//    加页（M3 预留）。
//
//  【2026-11-13 收编恢复】MediaServer/src/editor 原版原文（用户要求属性
//  面板以合并前为准）；宿主接线见 MainWindow.cpp onEditScreen 语义区。
// ============================================================================

#include "ScreenPropertyDialog.hpp"

#include "ScreenWarpPanel.hpp" // 输出变形编辑面板（「几何结构」页内嵌）

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace ms {

ScreenPropertyDialog::ScreenPropertyDialog(Screen& screen,
                                           const std::string& cur_host,
                                           std::uint16_t cur_port,
                                           const std::string& cur_pairing_code,
                                           QWidget* parent)
    : QDialog(parent) { // 预填直接读入参引用（screen_ 死成员已删）
    setWindowTitle("屏幕属性");

    auto* form_box = new QGroupBox("画布位置与显示大小（像素）", this);
    auto* form     = new QFormLayout(form_box);

    // ---- 位置/大小：允许负坐标（画布外扩屏）与 1~8192 尺寸 --------------
    x_ = new QDoubleSpinBox(form_box);
    x_->setRange(-8192.0, 8192.0);
    x_->setDecimals(1);
    x_->setSingleStep(10.0);
    x_->setValue(screen.x);

    y_ = new QDoubleSpinBox(form_box);
    y_->setRange(-8192.0, 8192.0);
    y_->setDecimals(1);
    y_->setSingleStep(10.0);
    y_->setValue(screen.y);

    w_ = new QDoubleSpinBox(form_box);
    w_->setRange(1.0, 8192.0);
    w_->setDecimals(1);
    w_->setSingleStep(10.0);
    w_->setValue(screen.w > 0 ? screen.w : 960.0);

    h_ = new QDoubleSpinBox(form_box);
    h_->setRange(1.0, 8192.0);
    h_->setDecimals(1);
    h_->setSingleStep(10.0);
    h_->setValue(screen.h > 0 ? screen.h : 540.0);

    form->addRow("位置 X：", x_);
    form->addRow("位置 Y：", y_);
    form->addRow("宽度：", w_);
    form->addRow("高度：", h_);

    // ---- 旋转/层级：与 clip transform 同域（度/整数 z）------------------
    // 旋转：±3600°（10 圈）覆盖竖屏 90/270 与异形拼接任意角；步进 90 便于
    // 常见竖屏场景一键到达，细角度仍可直接键入
    rotation_ = new QDoubleSpinBox(form_box);
    rotation_->setRange(-3600.0, 3600.0);
    rotation_->setDecimals(1);
    rotation_->setSingleStep(90.0);
    rotation_->setWrapping(true);
    rotation_->setValue(screen.rotation);

    z_ = new QSpinBox(form_box);
    z_->setRange(-1000, 1000);
    z_->setValue(screen.z);

    form->addRow("旋转°：", rotation_);
    form->addRow("层级 z：", z_);

    // ---- 几何六字段实时预览（2026-11-11 用户需求：调属性即时可见）--------
    // 旧语义只有 OK 才写回——editor 画布/display 输出都要等确认。现与 warp
    // 实时链同构：spinbox 每次值变化（点箭头/步进/键入后提交）即把表单几何
    // 发给宿主回调（写工作副本 + refreshSceneViews + scheduleAutoPublish
    // 20ms 节流下发）。回调收到的 Screen 仅几何字段有效。
    if (geometry_live_) {
        auto emitGeometryLive = [this]() {
            if (!geometry_live_) return;
            Screen g; // 仅几何字段有效（其余字段宿主不取）
            g.x        = x_->value();
            g.y        = y_->value();
            g.w        = w_->value();
            g.h        = h_->value();
            g.rotation = rotation_->value();
            g.z        = z_->value();
            geometry_live_(g);
        };
        connect(x_,        &QDoubleSpinBox::valueChanged, this, emitGeometryLive);
        connect(y_,        &QDoubleSpinBox::valueChanged, this, emitGeometryLive);
        connect(w_,        &QDoubleSpinBox::valueChanged, this, emitGeometryLive);
        connect(h_,        &QDoubleSpinBox::valueChanged, this, emitGeometryLive);
        connect(rotation_, &QDoubleSpinBox::valueChanged, this, emitGeometryLive);
        connect(z_,        &QSpinBox::valueChanged,       this, emitGeometryLive);
    }

    // ---- 输出变形（2026-11-11 WATCHOUT 式内嵌）----------------------------
    // 编辑面板直接内嵌「几何结构」页（用户需求：不要弹窗）——模式/密度/
    // 重置/网格编辑区与原变形弹窗逐一同构；编辑中实时变化转发宿主
    //（写场景+自动下发，display 拖动即见）；属性框整体取消由宿主还原
    //（打开前备份），面板自身不再持暂存——applyTo 时从面板终态写回。
    warp_panel_ = new ScreenWarpPanel(screen.warp, this);
    warp_panel_->setLiveApply([this](const ScreenWarp& w) {
        if (warp_live_) warp_live_(w);
    });

    // ---- 绑定显示端（player_id + 登记地址；扫描选设备替代手填 id）--------
    auto* bind_box = new QGroupBox("显示端绑定", this);
    auto* bind_lay = new QVBoxLayout(bind_box);

    // ---- 扫描区：结果列表 + 重新扫描（对话框打开即自动扫一轮）------------
    devices_ = new QListWidget(bind_box);
    devices_->setToolTip("局域网在线显示端（UDP 自动发现）——双击采用。\n"
                         "看不到目标设备：确认 display 已启动且未被防火墙拦截");
    devices_->setMaximumHeight(120);
    connect(devices_, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem*) {
                onDeviceActivated(devices_->currentRow());
            });
    auto* scan_row = new QHBoxLayout;
    scan_status_ = new QLabel("正在扫描局域网显示端…", bind_box);
    // 状态行不撑宽属性框（2026-11-09 用户反馈：长文案把对话框顶宽）——
    // 最小宽 0 允许布局压缩，文案必须保持短句（诊断细节走列表 tooltip）
    scan_status_->setMinimumWidth(0);
    rescan_btn_ = new QPushButton("重新扫描", bind_box);
    rescan_btn_->setToolTip("重新广播发现查询（display 后启动时使用）");
    scan_row->addWidget(scan_status_, /*stretch=*/1);
    scan_row->addWidget(rescan_btn_);
    connect(rescan_btn_, &QPushButton::clicked,
            this, &ScreenPropertyDialog::onRescan);
    bind_lay->addWidget(devices_);
    bind_lay->addLayout(scan_row);

    // ---- 手填区（兜底：未开发现端口/跨网段；扫描选中后自动填充）----------
    auto* bind_form = new QFormLayout;
    player_id_ = new QLineEdit(QString::fromStdString(screen.player_id),
                               bind_box);
    player_id_->setPlaceholderText("扫描选择自动填充，或手填 media_display --id");
    bind_form->addRow("Player ID：", player_id_);
    host_ = new QLineEdit(QString::fromStdString(cur_host), bind_box);
    host_->setPlaceholderText("显示端 IP（登记用，扫描自动填充）");
    bind_form->addRow("主机：", host_);
    port_ = new QSpinBox(bind_box);
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
        tr("显示端程序启动后在本列表自动出现；双击即完成绑定三项。"
           "若设备被其他编辑器接入会标注「已接入」。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: gray; font-size: 11px;");
    bind_lay->addWidget(hint);

    // ---- 发现浏览器：启动 + 定时拉取 -------------------------------------
    // 打开即扫（5 轮补发防丢包）；浏览器析构自动停止（接收线程 join）
    if (browser_.start()) {
        scan_rounds_left_ = 5;
        browser_.scan(); // 首轮立即发（不等待第一个定时拍）
    } else {
        scan_status_->setText("发现服务启动失败（无网络？可手填绑定）");
        scan_rounds_left_ = 0;
    }
    poll_timer_ = new QTimer(this);
    poll_timer_->setInterval(500);
    connect(poll_timer_, &QTimer::timeout,
            this, &ScreenPropertyDialog::onPollDiscoveries);
    poll_timer_->start();

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // ---- WATCHOUT 式分页（2026-11-11）：通用 / 几何结构 / 绑定 ------------
    // 对齐 WATCHOUT 显示设置对话框的页组织：第一页通用（位置/大小/旋转/
    // 层级），第二页几何结构（内嵌输出变形网格编辑，不弹窗），第三页绑定
    //（显示端发现/手填——WATCHOUT 通用页"计算机"字段的本地化扩展）。
    auto* tabs = new QTabWidget(this);
    {
        auto* general_page = new QWidget(tabs);
        auto* general_lay  = new QVBoxLayout(general_page);
        general_lay->addWidget(form_box);
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

    auto* main = new QVBoxLayout(this);
    main->addWidget(tabs);
    main->addWidget(buttons);
}

ScreenPropertyDialog::~ScreenPropertyDialog() {
    // 定时器随 QObject 树销毁；浏览器接收线程显式停（析构兜底也有 stop，
    // 这里显式表达生命周期意图：对话框关闭即不再参与发现）
    if (poll_timer_) poll_timer_->stop();
    browser_.stop();
}

void ScreenPropertyDialog::applyTo(Screen& s) const {
    s.x         = x_->value();
    s.y         = y_->value();
    s.w         = w_->value();
    s.h         = h_->value();
    s.rotation  = rotation_->value();
    s.z         = z_->value();
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

// ---------------------------------------------------------------------------
// 「重新扫描」：重置补发轮数 + 立即广播（后续拍由定时器继续补发）
// ---------------------------------------------------------------------------
void ScreenPropertyDialog::onRescan() {
    if (!browser_.start()) { // start 幂等：已在跑则直接返回 true
        scan_status_->setText("发现服务启动失败（无网络？可手填绑定）");
        return;
    }
    scan_rounds_left_ = 5;
    browser_.scan();
    scan_status_->setText("正在扫描局域网显示端…");
}

// ---------------------------------------------------------------------------
// 定时拍：剩余轮数内补发查询（防丢包）+ 拉取结果刷新列表
// ---------------------------------------------------------------------------
void ScreenPropertyDialog::onPollDiscoveries() {
    if (scan_rounds_left_ > 0) {
        --scan_rounds_left_;
        browser_.scan();
    }
    refreshDeviceList();
    const int n = devices_->count();
    if (n > 0) {
        scan_status_->setText(QString("发现 %1 台设备（双击采用）").arg(n));
    } else if (scan_rounds_left_ > 0) {
        scan_status_->setText("正在扫描局域网显示端…");
    } else {
        // 短文本（2026-11-09 用户反馈"属性框突然变宽"）：单行 QLabel 的
        // sizeHint=全文宽，长文案会撑宽对话框——诊断细节（display 未启动/
        // 防火墙拦截）在设备列表 tooltip 里，状态行只留动作指引
        scan_status_->setText("未发现设备——可「重新扫描」或手填绑定");
    }
}

// ---------------------------------------------------------------------------
// 发现结果 → 列表（整体重建 + 选中恢复；设备数个位数，开销可忽略）
//   行文本：id · 主机名 · ip:port · [模式] 接入状态
//   模式徽标（M5 编辑器互通）：standalone=独立 / sync=同步 / Unknown（旧版
//   display 未上报）不加前缀——提示该端可否导入/导出 .show 工程（仅独立）
// ---------------------------------------------------------------------------
void ScreenPropertyDialog::refreshDeviceList() {
    const auto devices = browser_.results();

    // 记住当前选中（重建后恢复，防刷新打断用户选择）
    QString selected_id;
    if (auto* cur = devices_->currentItem())
        selected_id = cur->data(Qt::UserRole).toString();

    devices_->blockSignals(true);
    devices_->clear();
    for (const auto& d : devices) {
        QString mode_tag;
        if (d.mode == ms::DiscoveryFrame::PlayerMode::Standalone)
            mode_tag = "[独立] ";
        else if (d.mode == ms::DiscoveryFrame::PlayerMode::Sync)
            mode_tag = "[同步] ";
        const QString state =
            mode_tag +
            (d.listen_port == 0 ? QString("直连模式（自动连接编辑器）")
             : d.bound          ? QString("已被其他编辑器接入")
                                : QString("待接入"));
        auto* item = new QListWidgetItem(QString("%1 · %2 · %3:%4 · %5")
                                             .arg(QString::fromStdString(d.player_id),
                                                  QString::fromStdString(d.hostname),
                                                  QString::fromStdString(d.host))
                                             .arg(d.listen_port)
                                             .arg(state),
                                         devices_);
        // UserRole 挂 player_id：双击恢复选中时用它定位
        item->setData(Qt::UserRole, QString::fromStdString(d.player_id));
        if (selected_id == QString::fromStdString(d.player_id))
            devices_->setCurrentItem(item);
    }
    devices_->blockSignals(false);
}

// ---------------------------------------------------------------------------
// 双击设备行 → 三项绑定字段自动填充（替代手填 id 的主路径）
//   * listen_port=0（直连模式）：端口回落 17100（该模式无登记地址，
//     id 用于诊断/人工核对——display 主动连 editor，无需登记）
// ---------------------------------------------------------------------------
void ScreenPropertyDialog::onDeviceActivated(int row) {
    if (row < 0 || row >= devices_->count()) return;
    const auto* item = devices_->item(row);
    if (!item) return;
    player_id_->setText(item->data(Qt::UserRole).toString());

    // 从发现结果取地址（按 player_id 匹配——行文本可读但不作解析源）
    const std::string pid = item->data(Qt::UserRole).toString().toStdString();
    for (const auto& d : browser_.results()) {
        if (d.player_id != pid) continue;
        host_->setText(QString::fromStdString(d.host));
        port_->setValue(d.listen_port > 0 ? d.listen_port : 17100);
        break;
    }
}

} // namespace ms
