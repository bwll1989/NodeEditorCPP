// ============================================================================
//  ScreenPropertyDialog.hpp —— Screen 属性编辑对话框（第四迭代；
//  2026-11-03 局域网设备发现：扫描选设备替代手填 player_id；
//  2026-11-11 WATCHOUT 式分页：「通用」/「几何结构」（内嵌变形编辑，不弹窗）/
//  「绑定」三页——变形编辑主体（ScreenWarpPanel）直接嵌在几何结构页）
//
//  【2026-11-13 收编恢复】MediaServer → Flow 收编时本类未随迁，首版收编
//  重建件（非模态平铺）与合并前差异过大（用户要求以合并前为准）——本文件
//  即 MediaServer/src/editor 原版原文，仅路径随迁（include 经 MediaCore
//  根：model/SceneModel.hpp、net/Discovery.hpp 均已在收编工程 include path）。
//
//  双击 Scene 预览中的 screen 矩形弹出，编辑：
//    * 画布位置与显示大小（x/y/w/h，虚拟画布像素）
//    * 旋转角度（度，顺时针绕中心；竖屏/异形拼接的取景窗语义）
//    * 叠放层级 z（多屏重叠时的绘制先后）
//    * 输出变形（几何结构页内嵌网格编辑器——拖拽控制点实时预览）
//    * 绑定的显示端（player_id + host + port）：
//        - 【扫描选设备】对话框打开即后台 UDP 广播发现查询，
//          局域网内 display 应答（id/主机名/IP/监听端口/接入状态）→
//          列表点选自动填三项——用户不再需要知道/输入 player_id；
//        - 手动输入保留为兜底（display 未开发现端口/跨网段等场景）。
//  OK 写回工作副本 ms::Screen（player_id）+ 经 displayHost/displayPort
//  交调用方同步显示端登记（EditorRuntime::removeDisplay+addDisplay）。
// ============================================================================

#pragma once

#include <QDialog>

#include <cstdint>
#include <functional>

#include "model/SceneModel.hpp"
#include "net/Discovery.hpp"

class QLineEdit;
class QListWidget;
class QSpinBox;
class QTimer;
class QDoubleSpinBox;
class QLabel;

namespace ms {

class ScreenWarpPanel;

class ScreenPropertyDialog : public QDialog {
    Q_OBJECT
public:
    /// \param screen      工作副本中的 screen 引用（构造时预填，OK 后
    ///                    applyTo 写回）
    /// \param cur_host    当前显示端登记主机（预填绑定区；未登记给默认）
    /// \param cur_port    当前显示端登记端口
    /// \param cur_pairing 当前显示端登记配对码（预填；空 = 不认证）
    /// \param parent      Qt 父控件
    explicit ScreenPropertyDialog(Screen& screen,
                                  const std::string& cur_host = "127.0.0.1",
                                  std::uint16_t cur_port = 17100,
                                  const std::string& cur_pairing_code = {},
                                  QWidget* parent = nullptr);
    ~ScreenPropertyDialog() override;

    /// 把表单值写回 screen（仅在 exec()==Accepted 后调用有意义）
    void applyTo(Screen& s) const;

    /// 绑定的显示端登记地址（显示端 host/port 归 .show 登记不归 Scene——
    /// 单独交回调用方，随 player_id 一起同步 runtime 登记）
    std::string   displayHost() const;
    std::uint16_t displayPort() const;
    /// 配对码（空 = 不认证；随登记一起交回调用方同步 .show 与 runtime）
    std::string   displayPairingCode() const;

    /// 【实时预览（2026-11-10）】注入 warp 实时应用回调——「几何结构」页
    /// 内嵌面板编辑过程中每次变化立即转发回调，由宿主写回场景走自动下发
    /// 链路（display 拖动即见）。属性框整体取消由宿主还原（打开前备份）。
    /// 空 = 不实时应用（纯确认制，旧语义）。
    void setWarpLiveApply(std::function<void(const ScreenWarp&)> fn) {
        warp_live_ = std::move(fn);
    }

    /// 【实时预览（2026-11-11 用户需求：几何调整 editor/display 即时可见）】
    /// 注入几何六字段（x/y/w/h/rotation/z）实时回调——通用页 spinbox 每次变化
    /// （点箭头/步进/键入提交）立即转发，宿主只取六字段写回工作副本走自动
    /// 下发链路。回调收到的 Screen 仅几何字段有效（warp/绑定不在实时链内）。
    /// 空 = 不实时应用（纯确认制，旧语义）。
    void setGeometryLiveApply(std::function<void(const Screen&)> fn) {
        geometry_live_ = std::move(fn);
    }

private slots:
    /// 「重新扫描」：重置扫描轮数并立即广播一轮查询
    void onRescan();
    /// 定时器：拉取发现结果刷新列表（保留当前选中项）+ 前几轮补发查询
    void onPollDiscoveries();
    /// 双击列表项 → 三项绑定字段自动填充（替代手填 id 的主路径）
    void onDeviceActivated(int row);

private:
    /// 用当前发现结果重建列表（记住选中 player_id，重建后恢复）
    void refreshDeviceList();

    QDoubleSpinBox* x_ = nullptr; ///< 画布坐标 X
    QDoubleSpinBox* y_ = nullptr; ///< 画布坐标 Y
    QDoubleSpinBox* w_ = nullptr; ///< 显示宽度
    QDoubleSpinBox* h_ = nullptr; ///< 显示高度
    QDoubleSpinBox* rotation_ = nullptr; ///< 旋转角度（度，顺时针绕中心）
    QSpinBox*       z_ = nullptr;        ///< 叠放层级（多屏重叠绘制次序）
    QLineEdit*      player_id_ = nullptr; ///< 绑定的显示端（player id）
    QLineEdit*      host_ = nullptr;      ///< 显示端主机（登记用）
    QSpinBox*       port_ = nullptr;      ///< 显示端监听端口（登记用）
    QLineEdit*      pairing_code_ = nullptr; ///< 配对码（空=不认证）

    // ---- 输出变形（2026-11-11 内嵌「几何结构」页，不再弹窗）--------------
    // 编辑中实时变化经 setLiveApply 回调转发 warp_live_（宿主写场景 +
    // 自动下发）；属性框整体取消由宿主还原（打开前备份）。
    ScreenWarpPanel* warp_panel_ = nullptr; ///< 几何结构页内嵌编辑面板

    /// warp 实时应用回调（宿主注入；编辑中转发，未注入则空操作）
    std::function<void(const ScreenWarp&)> warp_live_;

    /// 几何六字段实时回调（2026-11-11；spinbox 变化即转发，未注入则不转发）
    std::function<void(const Screen&)> geometry_live_;

    // ---- 局域网设备发现（扫描列表）------------------------------------------
    DiscoveryBrowser browser_;   ///< UDP 发现（后台接收线程；析构自动停止）
    QListWidget*     devices_ = nullptr; ///< 扫描结果列表
    QLabel*          scan_status_ = nullptr; ///< 扫描状态文案
    QPushButton*     rescan_btn_ = nullptr;  ///< 重新扫描
    QTimer*          poll_timer_ = nullptr;  ///< 结果拉取定时器
    int              scan_rounds_left_ = 0;  ///< 剩余补发查询轮数（防丢包）
};

} // namespace ms
