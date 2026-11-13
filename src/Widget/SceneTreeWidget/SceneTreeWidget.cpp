// ============================================================================
//  SceneTreeWidget.cpp —— 场景树 dock 控件实现
//  （第十迭代快速选中 + 2026-11-04 升级：显示端连接管理
//    + 2026-11-04 场景树增强：双列信息 / 展开保持 / 播放联动 / 右键扩展
//    + 2026-11-05 移除：素材分组展示与其右键菜单——素材管理走导入/
//      上传链路，树只承担画布结构导航）
// ============================================================================

#include "SceneTreeWidget.hpp"

#include <QBrush>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QHash>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>

#include <algorithm>

namespace ms {

namespace {
/// 节点数据角色：kind / 轨道或屏幕下标 / 片段下标（与画布 Selection 同码）
constexpr int kRoleKind = Qt::UserRole;
constexpr int kRoleA    = Qt::UserRole + 1;
constexpr int kRoleB    = Qt::UserRole + 2;
/// 节点通用角色：id 字符串（屏幕 = player_id、片段定位等；下标会因重建
/// 漂移，id 稳定；与时间轴回写定位"noteName 锚点"同思路）
constexpr int kRoleId   = Qt::UserRole + 3;
/// 【展开保持 2026-11-04】轨道/分组行的稳定标识角色（轨道 id）——展开态
/// 表的 key。轨道下标会因增删轨道漂移，id 稳定（与素材 id 锚点同思路）。
constexpr int kRoleKey  = Qt::UserRole + 4;

/// 节点 kind 常量（与画布 Selection 同码；2=素材节点已退役（2026-11-05
/// 场景树移除素材分组），编号保留防与画布选中码冲突）
constexpr int kKindGroup  = 0;
constexpr int kKindScreen = 1;
constexpr int kKindClip   = 4;
/// 【场景根 2026-11-05】第一级 scene 节点（树 = 整个场景的格式化展示）：
/// 右键 = 场景级动作（同步媒体/下发场景/适应窗口/放弃本地编辑）。
/// 编号取 5（画布选中码 1=screen 4=clip，scene 根不可选中画布对象不冲突）
constexpr int kKindScene  = 5;

/// 轨道类型中文名（与时间轴插件类型一致）
const char* trackKindName(AssetKind k) {
    switch (k) {
        case AssetKind::Video: return "视频";
        case AssetKind::Image: return "图片";
        case AssetKind::Text:  return "文本";
        case AssetKind::Audio: return "音频";
        case AssetKind::Live:  return "实况";
        case AssetKind::Control: return "指令";
        default:               return "未知";
    }
}

/// 轨道类型色（与画布片段边框同组色值——视觉分组统一，改动两处同步）
QColor trackKindColor(AssetKind k) {
    switch (k) {
        case AssetKind::Video: return QColor(0xE6, 0x8A, 0x33); // 橙
        case AssetKind::Image: return QColor(0x33, 0xB2, 0xA6); // 青
        case AssetKind::Text:  return QColor(0xD9, 0xC2, 0x3C); // 黄
        case AssetKind::Audio: return QColor(0x9B, 0x6F, 0xD1); // 紫
        case AssetKind::Live:  return QColor(0x3C, 0xC4, 0xD9); // 亮青（实时源）
        case AssetKind::Control: return QColor(0xF0, 0x8A, 0x5D); // 橙（指令，同插件色）
        default:               return QColor(0x88, 0x88, 0x88);
    }
}

/// ---------------------------------------------------------------------------
/// 【显示优化 2026-11-13】代码绘制节点小图标（无资源文件依赖，深浅主题
/// 均适口；rebuild 由编辑动作 50ms 防抖驱动，按 (色值, 形状) 缓存 QPixmap）：
///   * kindDotIcon  —— 实心圆点（轨道分组/片段行，色 = 轨道类型色）
///   * screenIcon   —— 横放圆角矩形（屏幕行/屏幕分组，色 = 运行状态色，
///                     在线绿/离线红/未登记灰——图标颜色随状态即读）
///   * sceneIcon    —— 空心圆角矩形（场景根，中性描边）
/// ---------------------------------------------------------------------------
QPixmap kindDotIcon(const QColor& color) {
    static QHash<quint32, QPixmap> cache;
    const quint32 key = color.rgba();
    const auto it = cache.find(key);
    if (it != cache.end()) return *it;
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawEllipse(4, 4, 8, 8);
    p.end();
    cache.insert(key, pm);
    return pm;
}

QPixmap screenIcon(const QColor& color) {
    static QHash<quint32, QPixmap> cache;
    const quint32 key = color.rgba();
    const auto it = cache.find(key);
    if (it != cache.end()) return *it;
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawRoundedRect(2, 4, 12, 8, 2, 2); // 横放"显示器"轮廓
    p.end();
    cache.insert(key, pm);
    return pm;
}

QPixmap sceneIcon() {
    static const QPixmap pm = [] {
        QPixmap pm(16, 16);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor(0x9A, 0x9A, 0x9A));
        pen.setWidthF(1.4);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(2, 3, 12, 10, 2, 2); // 舞台轮廓（空心）
        p.drawLine(5, 15, 11, 15);             // 台口基线
        p.end();
        return pm;
    }();
    return pm;
}

/// 分组行加粗字体（场景根/屏幕组/轨道组——层次一眼分明）
QFont boldItemFont(const QFont& base) {
    QFont f = base;
    f.setBold(true);
    return f;
}

/// 【信息列 2026-11-04】us → "m:ss.t"（树列宽友好；毫秒一位四舍五入）
QString fmtTime(std::int64_t us) {
    const qint64 total_ms = (us + 50) / 100; // 0.1s 粒度
    return QStringLiteral("%1:%2.%3")
        .arg(total_ms / 60000)                       // 分
        .arg((total_ms / 1000) % 60, 2, 10, QChar('0'))   // 秒（补零）
        .arg((total_ms % 1000) / 100);               // 0.1s 位
}

/// 屏幕行运行状态后缀与着色（rebuild 与 refreshDisplayStatuses 共用——
/// 两处取值规则必须一致，否则 200ms 拍轻量刷新会把 rebuild 的着色改掉）。
///   * 未绑定（player_id 空）：无后缀、灰（调用方自行显示"(未绑定)"）
///   * 已绑定但 connector 无此槽位：灰"（未登记）"
///   * 槽位被停用：灰"（已停用）"；在线：绿"●在线"；
///     TCP 已连但未注册：灰"（已连接）"；其余：红"（离线）"
void screenStatusSuffix(const SceneTreeWidget::DisplayStates& states,
                        const std::string& player_id,
                        QString& status, QColor& color) {
    status.clear();
    color = QColor(0x88, 0x88, 0x88);
    if (player_id.empty()) return;
    const auto st = states.find(player_id);
    if (st == states.end()) {
        status = QStringLiteral("（未登记）");
    } else if (!st->second.enabled) {              // enabled=false
        status = QStringLiteral("（已停用）");
    } else if (st->second.online) {                // 在线且已注册
        status = QStringLiteral("  ●在线");
        color  = QColor(0x3C, 0xB4, 0x4F);
    } else if (st->second.connected) {             // TCP 在线但尚未注册
        status = QStringLiteral("（已连接）");       // 等 注册帧+指派 往返
        color  = QColor(0x88, 0x88, 0x88);
    } else {
        status = QStringLiteral("（离线）");
        color  = QColor(0xCC, 0x3C, 0x3C);
    }
}
} // namespace

SceneTreeWidget::SceneTreeWidget(QWidget* parent) : QTreeWidget(parent) {
    // 【双列 2026-11-04】列 0=名称，列 1=信息（片段时间区间/素材分辨率
    // 与引用数/屏幕几何）——原先全塞 tooltip，扫一眼树看不到关键参数。
    setColumnCount(2);
    setHeaderHidden(true);          // 列语义随节点类型变化，不设表头
    setRootIsDecorated(true);
    setUniformRowHeights(true);
    // 【显示优化 2026-11-13】交替行色提升多行可读性（颜色随宿主调色板/
    // QSS 自适应）；信息列右对齐——数字区间贴边，扫读时间更快
    setAlternatingRowColors(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setExpandsOnDoubleClick(false); // 双击 = 属性（展开交给单击箭头/双击分组）
    setFrameShape(QFrame::NoFrame);
    // 信息列只读展示：用户可拖列宽（默认 3:2），点击列头不排序（树序=
    // 场景序，排序会破坏"下标=场景下标"的选中约定）
    header()->setSectionResizeMode(0, QHeaderView::Interactive);
    header()->setSectionResizeMode(1, QHeaderView::Interactive);
    header()->setStretchLastSection(true);
    // 【列宽 2026-11-05】两级结构（场景根→分组→子项）缩进加深后，默认
    // 窄列 0 会把名称截成"屏幕…/视频…"——给足初始宽（用户仍可拖）。
    // 名字是树的主信息（片段行=clip-id · 素材文件名），窄列等于没有。
    setColumnWidth(0, 210);
    // 树序 = 场景序（"下标 = 场景下标"选中约定的基础）；Qt 树默认不排序，
    // 无需额外关闭

    // 单击节点 → 上报选中请求（分组/素材行不可选中画布对象）
    connect(this, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* cur, QTreeWidgetItem*) {
        if (!cur) return;
        const int kind = cur->data(0, kRoleKind).toInt();
        if (kind != kKindScreen && kind != kKindClip) return;
        emit selectionRequested(kind, cur->data(0, kRoleA).toInt(),
                                cur->data(0, kRoleB).toInt());
    });
    // 双击节点 → 属性对话框（分组/素材行忽略；展开语义已由
    // setExpandsOnDoubleClick(false) 关闭，双击全部让位给属性编辑）
    connect(this, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item, int) {
        if (!item) return;
        const int kind = item->data(0, kRoleKind).toInt();
        if (kind != kKindScreen && kind != kKindClip) return;
        emit propertiesRequested(kind, item->data(0, kRoleA).toInt(),
                                 item->data(0, kRoleB).toInt());
    });
}

void SceneTreeWidget::rebuild(const Scene* scene,
                              const DisplayStates* states) {
    // 显示端状态快照（右键菜单文案 / 节点后缀取值；值拷贝自持）
    display_states_.clear();
    if (states) display_states_ = *states;

    // ---- 【展开保持】重建前收集各分组展开态（key = 分组稳定标识；
    //      首次建树时 expanded_ 为空，新分组走 setExpanded(key,true) 默认
    //      全展开——与旧行为一致）+ 滚动位置 ----
    // 【2026-11-05 层级升级】顶层现在只有 scene 根（":scene"），原顶层
    // 分组（屏幕组/轨道组）降为 scene 根的 children——收集/恢复都要下探
    // 一层（根 + 分组两层都保展开态）。
    expanded_.clear();
    {
        // scene 根自身的展开态（首建无记录 = 默认展开）
        if (topLevelItemCount() > 0) {
            const QTreeWidgetItem* root = topLevelItem(0);
            const QString rkey = root->data(0, kRoleKey).toString();
            if (!rkey.isEmpty()) expanded_[rkey] = root->isExpanded();
            // 分组层展开态（屏幕组/轨道组）
            for (int i = 0; i < root->childCount(); ++i) {
                const QTreeWidgetItem* grp = root->child(i);
                const QString key = grp->data(0, kRoleKey).toString();
                if (!key.isEmpty()) expanded_[key] = grp->isExpanded();
            }
        }
    }
    const int scroll_y = verticalScrollBar()->value();

    setUpdatesEnabled(false); // 批量重建防闪烁
    clear();
    clip_nodes_.clear();      // 片段节点缓存（播放联动用）随树重建
    screen_nodes_.clear();    // 屏幕节点缓存（同步进度标注用）随树重建
    if (scene) {
        // ---- 场景根节点（第一级；树 = 整个场景的格式化展示）------------
        // 文本 = 场景 id；信息列 = 组成摘要（屏数/轨数/时长）。
        // 【语义澄清 2026-11-05】Scene 本身没有"大小"概念（WATCHOUT 语义：
        // 演出 = 屏幕集合 + 时间轴；分辨率是各屏幕自己的属性）——树根不
        // 展示 canvas_w/h（那只是编辑器预览的虚拟画布/布局工作台尺寸，
        // 属于编辑期工具参数而非场景内容属性）。
        auto* scene_root = new QTreeWidgetItem(this);
        scene_root->setText(0, QString("场景 · %1")
                                   .arg(QString::fromStdString(scene->id)));
        scene_root->setText(1, QStringLiteral("%1屏 · %2轨 · %3")
                                   .arg((int)scene->screens.size())
                                   .arg((int)scene->timeline.tracks.size())
                                   .arg(fmtTime(scene->timeline.duration_us)));
        scene_root->setIcon(0, sceneIcon());
        scene_root->setFont(0, boldItemFont(font()));
        scene_root->setFont(1, boldItemFont(font()));
        scene_root->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        scene_root->setForeground(1, QColor(0x88, 0x88, 0x88));
        scene_root->setData(0, kRoleKind, kKindScene);
        scene_root->setData(0, kRoleKey, QStringLiteral(":scene"));
        scene_root->setFlags(scene_root->flags() & ~Qt::ItemIsSelectable);
        scene_root->setToolTip(0, QString("场景 %1 · %2 屏 · %3 轨 · 时长 %4\n"
                                          "右键：同步媒体 / 下发场景 / 适应窗口"
                                          " / 放弃本地编辑")
                                      .arg(QString::fromStdString(scene->id))
                                      .arg((int)scene->screens.size())
                                      .arg((int)scene->timeline.tracks.size())
                                      .arg(fmtTime(scene->timeline.duration_us)));

        // ---- 屏幕分组（display 输出区域；label 带绑定关系 + 运行状态）----
        // 【层级】挂 scene 根下（第二级）——原来在顶层
        auto* scr_root = new QTreeWidgetItem(scene_root);
        scr_root->setText(0, QString("屏幕 (%1)").arg(scene->screens.size()));
        scr_root->setIcon(0, screenIcon(QColor(0x88, 0x88, 0x88)));
        scr_root->setFont(0, boldItemFont(font()));
        scr_root->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
        scr_root->setData(0, kRoleKind, kKindGroup); // 分组行：不可选中
        scr_root->setData(0, kRoleKey, QStringLiteral(":screens"));
        scr_root->setFlags(scr_root->flags() & ~Qt::ItemIsSelectable);
        for (int i = 0; i < (int)scene->screens.size(); ++i) {
            const Screen& s = scene->screens[i];
            // 运行状态后缀（display_states_ 命中才有）——取值规则与
            // refreshDisplayStatuses（200ms 拍轻量刷新）共用同一辅助函数
            QString status;
            QColor  status_color(0x88, 0x88, 0x88);
            screenStatusSuffix(display_states_, s.player_id,
                               status, status_color);
            auto* it = new QTreeWidgetItem(scr_root);
            const QString prefix =
                QString("%1 → %2")
                    .arg(QString::fromStdString(s.id),
                         s.player_id.empty()
                             ? QString("(未绑定)")
                             : QString::fromStdString(s.player_id));
            it->setText(0, prefix + status);
            // 【信息列】几何直读（旋转/Z 一并列出，tooltip 不再是唯一入口）
            it->setText(1, QString("%1×%2 @(%3,%4)")
                               .arg(s.w).arg(s.h).arg(s.x).arg(s.y));
            it->setIcon(0, screenIcon(status_color)); // 图标色 = 运行状态色
            it->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            it->setForeground(1, QColor(0x88, 0x88, 0x88));
            it->setForeground(0, status_color);
            it->setToolTip(0, QString("屏幕 %1 · %2×%3 @(%4,%5) · 旋转 %6° · z=%7\n"
                                      "右键：属性 / 单独连接断开该显示端；双击：属性")
                                  .arg(QString::fromStdString(s.id))
                                  .arg(s.w).arg(s.h).arg(s.x).arg(s.y)
                                  .arg(s.rotation).arg(s.z));
            it->setData(0, kRoleKind, kKindScreen);
            it->setData(0, kRoleA, i);
            it->setData(0, kRoleId, QString::fromStdString(s.player_id));
            // 【素材同步进度】已绑定显示端的屏幕节点进缓存——updateSync
            // Progress 据此追加进度后缀（未绑定屏无可标注对象）
            if (!s.player_id.empty()) {
                ScreenNode sn;
                sn.player_id  = QString::fromStdString(s.player_id);
                sn.prefix     = prefix;      // 不含状态后缀（轻量刷新重组用）
                sn.base_text  = it->text(0); // prefix + 状态后缀
                sn.base_color = status_color;
                sn.item       = it;
                screen_nodes_.push_back(sn);
            }
        }
        // ---- 素材分组（媒体文件清单；右键删除/添加到画布/打开文件夹）----
        // 【2026-11-05 移除】场景树不再展示"素材"分组（素材管理走导入/
        // 上传链路，树只承担画布结构导航）——素材子项的右键菜单、引用
        // 计数预扫随本组一并退役（EditorWindow 的槽已同步删除）。
        // ---- 轨道分组（轨道名 + 类型；子行 = 片段，文字染轨道类型色）----
        // 【层级】挂 scene 根下（第二级）——原来在顶层
        for (int tr = 0; tr < (int)scene->timeline.tracks.size(); ++tr) {
            const Track& t = scene->timeline.tracks[tr];
            auto* tr_root = new QTreeWidgetItem(scene_root);
            tr_root->setText(0, QString("%1轨 · %2 (%3)")
                                    .arg(trackKindName(t.kind),
                                         QString::fromStdString(t.name))
                                    .arg(t.clips.size()));
            tr_root->setData(0, kRoleKind, kKindGroup);
            tr_root->setData(0, kRoleKey,
                             QString::fromStdString(t.id)); // 展开态 key
            tr_root->setForeground(0, trackKindColor(t.kind)); // 分组行染类型色
            tr_root->setIcon(0, kindDotIcon(trackKindColor(t.kind)));
            tr_root->setFont(0, boldItemFont(font()));
            tr_root->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
            tr_root->setFlags(tr_root->flags() & ~Qt::ItemIsSelectable);
            for (int cl = 0; cl < (int)t.clips.size(); ++cl) {
                const Clip& c = t.clips[cl];
                auto* it = new QTreeWidgetItem(tr_root);
                // 【名称列 2026-11-05 用户要求】片段行显示绑定的素材名——
                // "clip-id · 文件名"（素材表命中才追加文件名；未绑定只显
                // 示片段 id）。文件名取 path 末段（相对路径 media/xxx.mp4
                // → xxx.mp4，树列宽友好）——素材才是用户视角的"内容名"。
                const Asset* a = scene->findAsset(c.asset_id);
                it->setText(0, QString::fromStdString(c.id) +
                               (a ? QStringLiteral(" · ") +
                                    QFileInfo(QString::fromStdString(a->path))
                                        .fileName()
                                  : QString()));
                // 【信息列】时间轴区间直读（m:ss.t–m:ss.t）；tooltip 保留
                // 几何详情
                it->setText(1, QStringLiteral("%1–%2")
                                   .arg(fmtTime(c.start_us),
                                        fmtTime(c.start_us + c.duration_us)));
                it->setForeground(1, QColor(0x88, 0x88, 0x88));
                it->setForeground(0, trackKindColor(t.kind));
                it->setIcon(0, kindDotIcon(trackKindColor(t.kind)));
                it->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
                it->setToolTip(0, QString("片段 %1 · 素材 %2 · %3×%4 @(%5,%6)"
                                          "\n区间 %7–%8（时长 %9）\n"
                                          "右键：定位 / 复制 / 删除 / 属性")
                                      .arg(QString::fromStdString(c.id))
                                      .arg(a ? QString::fromStdString(a->id)
                                             : QStringLiteral("(未绑定)"))
                                      .arg(c.transform.w > 0 ? c.transform.w : 0)
                                      .arg(c.transform.h > 0 ? c.transform.h : 0)
                                      .arg(c.transform.x)
                                      .arg(c.transform.y)
                                      .arg(fmtTime(c.start_us))
                                      .arg(fmtTime(c.start_us + c.duration_us))
                                      .arg(fmtTime(c.duration_us)));
                it->setData(0, kRoleKind, kKindClip);
                it->setData(0, kRoleA, tr);
                it->setData(0, kRoleB, cl);
                // 【播放联动】缓存片段节点（时间参数 + 指针；rebuild 后
                // 下标即场景下标，选中/删除请求都以此为准）
                ClipNode cn;
                cn.start_us  = c.start_us;
                cn.length_us = c.duration_us;
                cn.item      = it;
                clip_nodes_.push_back(cn);
            }
        }
        // ---- 【展开保持】恢复展开态（scene 根 + 分组两层；无记录的
        //      分组默认展开 = 旧 expandAll 行为；用户手动折叠的状态此后
        //      不再被顶开）----
        if (topLevelItemCount() > 0) {
            QTreeWidgetItem* root = topLevelItem(0);
            const QString rkey = root->data(0, kRoleKey).toString();
            bool root_open = true;
            if (!rkey.isEmpty()) {
                const auto found = expanded_.find(rkey);
                if (found != expanded_.end()) root_open = found->second;
            }
            root->setExpanded(root_open);
            for (int i = 0; i < root->childCount(); ++i) {
                QTreeWidgetItem* grp = root->child(i);
                const QString key = grp->data(0, kRoleKey).toString();
                bool open = true;
                if (!key.isEmpty()) {
                    const auto found = expanded_.find(key);
                    if (found != expanded_.end()) open = found->second;
                }
                grp->setExpanded(open);
            }
        }
        verticalScrollBar()->setValue(scroll_y); // 滚动位置一并保留
    }
    setUpdatesEnabled(true);
}

void SceneTreeWidget::syncSelection(int kind, int a, int b) {
    // 画布 → 树：blockSignals 防止 currentItemChanged 再把请求发回画布
    // （递归震荡）；无匹配（kind=0 或下标失效）清除当前选择
    // 【层级 2026-11-05】子项在 scene 根的分组下——遍历三层（根→组→项）
    QTreeWidgetItem* target = nullptr;
    if (kind != 0 && topLevelItemCount() > 0) {
        const QTreeWidgetItem* root = topLevelItem(0);
        for (int i = 0; i < root->childCount() && !target; ++i) {
            const QTreeWidgetItem* grp = root->child(i);
            for (int j = 0; j < grp->childCount(); ++j) {
                QTreeWidgetItem* ch = grp->child(j);
                if (ch->data(0, kRoleKind).toInt() == kind &&
                    ch->data(0, kRoleA).toInt() == a &&
                    (kind == kKindScreen ||
                     ch->data(0, kRoleB).toInt() == b)) {
                    target = ch;
                    break;
                }
            }
        }
    }
    blockSignals(true);
    setCurrentItem(target); // nullptr = 清除（setCurrentItem(nullptr) 合法）
    blockSignals(false);
}

// ---------------------------------------------------------------------------
// 播放联动（2026-11-04）：活跃片段（start ≤ t < start+duration）加 ▶ 前缀。
// 幂等：状态不变的节点不重写文本（200ms 低频拍下也无谓重绘为零）。
// 时长 ≤0 的片段视为永不活跃（防御非法数据）。
// ---------------------------------------------------------------------------
void SceneTreeWidget::markPlayhead(std::int64_t t_us) {
    for (auto& cn : clip_nodes_) {
        if (!cn.item) continue; // 节点已随树销毁（rebuild 中途）——防御
        const bool now_active =
            cn.length_us > 0 && t_us >= cn.start_us &&
            t_us < cn.start_us + cn.length_us;
        if (now_active == cn.active) continue; // 状态未变：跳过
        cn.active = now_active;
        // 前缀切换（文本本体保留——从原始文本缓存拼？直接去/加前缀即可：
        // ▶ 与名称间一个空格，格式固定无歧义）
        QString text = cn.item->text(0);
        if (now_active) {
            if (!text.startsWith(QStringLiteral("▶ ")))
                cn.item->setText(0, QStringLiteral("▶ ") + text);
        } else {
            if (text.startsWith(QStringLiteral("▶ ")))
                cn.item->setText(0, text.mid(2));
        }
        // 【显示优化 2026-11-13】活跃片段整行淡高亮——▶ 前缀之外的第二个
        // 视觉信号（宿主 Highlight 色 + 透明度，深浅主题自适应；选中行
        // 背景由 QSS 管理不受影响）。alpha 0 = 恢复透明。
        QColor bg = palette().color(QPalette::Highlight);
        bg.setAlpha(now_active ? 60 : 0);
        cn.item->setBackground(0, QBrush(bg));
        cn.item->setBackground(1, QBrush(bg));
    }
}

// ---------------------------------------------------------------------------
// 素材同步进度标注（2026-11-04）：display_id → 进度文案追加到屏幕节点
// 基础文本之后（"scr-1 → player-1 ●在线  ⤓ 同步 2/5 · 34%"）。
// 幂等：文案与着色均不变时零重写；空文案 = 清除标注（恢复 rebuild 时的
// 基础文本与状态着色——离线/停滞/无同步任务）。
// 着色：进行中橙（与轨道类型色区分度足够）；完成绿（✓ 与 ●在线同色系）。
// ---------------------------------------------------------------------------
void SceneTreeWidget::updateSyncProgress(
        const std::map<QString, QString>& sync_texts) {
    for (auto& sn : screen_nodes_) {
        if (!sn.item) continue; // 节点已随树销毁（rebuild 中途）——防御
        QString suffix;
        QColor  color = sn.base_color;
        const auto found = sync_texts.find(sn.player_id);
        if (found != sync_texts.end() && !found->second.isEmpty()) {
            suffix = QStringLiteral("  ") + found->second;
            // 【2026-11-13 修复】原 QLatin1Char('✓') 把 U+2713 截断成
            // 单字节（C4309），比较恒不命中 → "✓ 已同步"绿色标注失效
            // （恒走橙色分支）。改用完整 QChar 比对。
            color  = found->second.startsWith(QChar(0x2713))
                         ? QColor(0x3C, 0xB4, 0x4F)   // ✓ 完成：绿
                         : QColor(0xE6, 0x8A, 0x33);  // ⤓ 进行中：橙
        }
        sn.sync_suffix = suffix; // 记录当前标注（refreshDisplayStatuses
                                 // 重组文本时要拼上，否则会抹掉进度标注）
        const QString want = sn.base_text + suffix;
        if (want == sn.item->text(0)) continue; // 无变化：零重写
        sn.item->setText(0, want);
        sn.item->setForeground(0, color);
    }
}

// ---------------------------------------------------------------------------
// 显示端连接状态轻量刷新（2026-11-06）：200ms UI 拍驱动，只更新屏幕行的
// 状态后缀与颜色，不重建整树。
// 【修复"display 已连接但树恒红"】原先连接状态只在 rebuild（编辑触发）
// 时刷新——display 在编辑间隙连上/断开，树里那行永远停在旧状态；偶有一
// 次编辑赶上连接刚建立便"绿一闪而过"。现与状态栏同拍（≤200ms 延迟），
// 编辑与否都能如实反映在线状态。
// 幂等：后缀与着色均不变时零重写（200ms 拍无谓重绘会拖累树的渲染）。
// ---------------------------------------------------------------------------
void SceneTreeWidget::refreshDisplayStatuses(const DisplayStates& states) {
    display_states_ = states; // 右键菜单"连接/断开"文案同源取最新
    // 联测探针：状态变化留痕（MS_TREE_DBG 门控；幂等设计下仅在变化时打）
    static const bool s_dbg = std::getenv("MS_TREE_DBG") != nullptr;
    for (auto& sn : screen_nodes_) {
        if (!sn.item) continue; // 节点已随树销毁——防御
        QString status;
        QColor  color(0x88, 0x88, 0x88);
        screenStatusSuffix(display_states_, sn.player_id.toStdString(),
                           status, color);
        sn.base_text  = sn.prefix + status;  // 供 updateSyncProgress 组合
        sn.base_color = color;
        const QString want = sn.base_text + sn.sync_suffix;
        const bool text_changed  = (want != sn.item->text(0));
        const bool color_changed = (color != sn.item->foreground(0).color());
        if (!text_changed && !color_changed) continue; // 无变化：零重写
        if (text_changed) sn.item->setText(0, want);
        sn.item->setForeground(0, color);
        // 【显示优化 2026-11-13】屏幕图标色随运行状态走（在线绿/离线红/
        // 未登记灰）——200ms 拍轻量刷新与 rebuild 同一套取色
        sn.item->setIcon(0, screenIcon(color));
        if (s_dbg) {
            std::fprintf(stderr, "[tree] %s 状态刷新: %s\n",
                         sn.player_id.toUtf8().constData(),
                         want.toUtf8().constData());
            std::fflush(stderr);
        }
    }
}

// ---------------------------------------------------------------------------
// 右键菜单（2026-11-04 升级核心 + 2026-11-05 两级结构重设计）：
//   场景级菜单（scene 根 / 分组行 / 空白区右键）= 同步媒体 · 下发场景 ·
//     适应窗口 · 放弃本地编辑——整树是"当前场景的格式化展示"，场景级
//     动作挂在场景根与其容器行上；
//   屏幕节点 = 连接/断开（单独控制）+ 属性；
//   片段节点 = 复制 / 删除（2026-11-05 用户定案收敛：定位播放头与属性
//   两项移除——定位走时间轴拖播放头，属性面板唯一入口 = timeline 双击）。
//   （屏幕/片段节点不再混入场景级动作——语义分层：对象操作 vs 场景操作）
// ---------------------------------------------------------------------------
void SceneTreeWidget::contextMenuEvent(QContextMenuEvent* e) {
    QTreeWidgetItem* it = itemAt(e->pos());
    const int kind = it ? it->data(0, kRoleKind).toInt() : 0;

    QMenu menu(this);
    bool scene_level = true; // 本右键是否场景级（屏幕/片段节点 = 对象级）

    // ---- 屏幕节点：单独连接/断开 + 属性（对象级）------------------------
    if (kind == kKindScreen && it) {
        scene_level = false;
        // display_id 直接取节点数据（rebuild 时存入 kRoleId——不依赖
        // 显示文本格式，状态后缀变更不影响此解析）
        const QString player = it->data(0, kRoleId).toString();
        const int scr_idx = it->data(0, kRoleA).toInt();

        if (!player.isEmpty()) {
            // 当前启用状态（display_states_ 未登记视为启用——纯防御）
            bool enabled = true;
            const auto st = display_states_.find(player.toStdString());
            if (st != display_states_.end()) enabled = st->second.enabled;

            QAction* act = menu.addAction(
                enabled ? QStringLiteral("断开此显示端")
                        : QStringLiteral("连接此显示端"));
            act->setToolTip(enabled
                ? QStringLiteral("断开 %1 并停止自动重连（其他显示端不受影响）")
                      .arg(player)
                : QStringLiteral("恢复自动连接 %1（看门狗下一拍发起）")
                      .arg(player));
            connect(act, &QAction::triggered, this, [this, player, enabled] {
                emit displayConnectRequested(player, !enabled);
            });
        }
        // 属性（与双击同路径；未绑定屏也可编辑几何）
        QAction* prop = menu.addAction(QStringLiteral("属性…"));
        connect(prop, &QAction::triggered, this, [this, scr_idx] {
            emit propertiesRequested(kKindScreen, scr_idx, 0);
        });
    }
    // 【2026-11-05 移除】素材节点右键菜单（添加到画布/打开文件夹/删除）
    // ——场景树不再展示素材分组，此分支随之退役。
    // ---- 片段节点：复制/删除（对象级）------------------------------------
    // 【2026-11-05 移除】"定位播放头到片段起点"与"属性…"两项（用户定案
    // 收敛右键）——定位走时间轴拖播放头，属性面板唯一入口 = timeline 双击；
    // start_us 反查循环随定位项一并退役（clipSeekRequested 信号同删）。
    else if (kind == kKindClip && it) {
        scene_level = false;
        const int tr = it->data(0, kRoleA).toInt();
        const int cl = it->data(0, kRoleB).toInt();
        QAction* dup =
            menu.addAction(QStringLiteral("复制片段（起点顺延）"));
        dup->setToolTip(QStringLiteral("在同轨克隆此片段：新片段起点 = 原片段"
                                       "结束点，自动命名并置脏下发"));
        connect(dup, &QAction::triggered, this, [this, tr, cl] {
            emit clipDuplicateRequested(tr, cl);
        });
        QAction* del = menu.addAction(QStringLiteral("删除片段"));
        del->setToolTip(QStringLiteral("从轨道移除片段 %1（时间轴同步删除，"
                                       "稍后自动下发）")
                            .arg(it->text(0)));
        connect(del, &QAction::triggered, this, [this, tr, cl] {
            emit clipDeleteRequested(tr, cl);
        });
    }

    // ---- 场景级菜单（scene 根 / 分组行 / 空白区右键）---------------------
    // 顺序按用户约定：同步媒体 → 下发场景 → 适应窗口；"放弃本地编辑"是
    // 既有场景级功能，列在最后（不需要可在场景根菜单再裁）。
    if (scene_level) {
        // 【2026-11-13 移除】"添加显示端…"入口（用户要求场景树不提供添加
        // display 功能；对齐合并前单入口约定）——屏幕与显示端一一对应，
        // 画布右键"添加屏幕"自动连带登记本机显示端；改绑走屏幕属性对话框
        // "绑定"页（UDP 扫描选设备 + 手填兜底）。

        QAction* act_sync = menu.addAction(QStringLiteral("同步媒体"));
        act_sync->setToolTip(QStringLiteral(
            "把场景引用的媒体文件注册进分发服务并推送清单给在线显示端"
            "（幂等；已同步的文件显示端自动跳过，缺失/变化的才拉取）"));
        connect(act_sync, &QAction::triggered, this,
                [this] { emit syncMediaRequested(); });

        QAction* act_pub = menu.addAction(QStringLiteral("下发场景"));
        act_pub->setToolTip(QStringLiteral(
            "立即把工作副本全量广播给所有显示端（编辑停止后也会自动下发；"
            "含未绑定素材片段时弹窗指引绑定）"));
        connect(act_pub, &QAction::triggered, this,
                [this] { emit publishSceneRequested(); });

        QAction* act_fit = menu.addAction(QStringLiteral("适应窗口"));
        act_fit->setToolTip(QStringLiteral(
            "画布视图复位：整个 scene 铺满预览区（Ctrl+滚轮缩放/中键平移"
            "浏览后一键回家）"));
        connect(act_fit, &QAction::triggered, this,
                [this] { emit fitViewRequested(); });

        QAction* act_reload = menu.addAction(QStringLiteral("放弃本地编辑"));
        act_reload->setToolTip(QStringLiteral(
            "丢弃本地未下发的修改，用权威场景覆盖工作副本"));
        connect(act_reload, &QAction::triggered, this,
                [this] { emit reloadSceneRequested(); });
    }

    menu.exec(e->globalPos());
}

} // namespace ms
