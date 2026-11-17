// ============================================================================
//  ScreenPropertyDialog.hpp —— Screen 属性编辑对话框（第五迭代；
//  2026-11-17 绑定简化：设备发现整体退役——扫描列表/一键绑定/接入状态
//  徽标全部移除，绑定页回归纯手输表单（id/host/port/配对码四字段）。
//  连接后 Editor 经 RegisterAck 把绑定 id 指派给 display（指派模型
//  2026-11-06），无需 display 侧预配置一致。
//
//  【2026-11-13 收编恢复】MediaServer → Flow 收编时本类未随迁，首版收编
//  重建件（非模态平铺）与合并前差异过大（用户要求以合并前为准）——本文件
//  即 MediaServer/src/editor 原版原文，仅路径随迁（include 经 MediaCore
//  根：model/SceneModel.hpp）。
//
//  双击 Scene 预览中的 screen 矩形弹出，编辑：
//    * 画布位置与显示大小（x/y/w/h，虚拟画布像素）
//    * 旋转角度（度，顺时针绕中心；竖屏/异形拼接的取景窗语义）
//    * 叠放层级 z（多屏重叠时的绘制先后）
//    * 输出变形（几何结构页内嵌网格编辑器——拖拽控制点实时预览）
//    * 绑定的显示端（player_id + host + port + 配对码）：
//        - 【纯手输（2026-11-17 定案）】UDP 广播发现的可靠性在防火墙/
//          多网卡/跨网段环境下天然劣于手输（广播静默失败的环节太多），
//          交互复杂度又远超便利收益——整体退役。display 以 --id/--port
//          启动监听，editor 按 ip:port 登记即看门狗直连；
//        - 连接成功后 display 自动采用指派 id，无需与本地标签一致。
//  OK 写回工作副本 ms::Screen（player_id）+ 经 displayHost/displayPort
//  交调用方同步显示端登记（EditorRuntime::removeDisplay+addDisplay）。
// ============================================================================

#pragma once

#include "FramelessDialog/FramelessDialog.hpp" // 统一无边框弹窗基类（2026-11-17）

#include <cstdint>
#include <functional>

#include "model/SceneModel.hpp"

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;

namespace ms {

class ScreenWarpPanel;

class ScreenPropertyDialog : public FramelessDialog {
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

private:
    QDoubleSpinBox* x_ = nullptr; ///< 画布坐标 X
    QDoubleSpinBox* y_ = nullptr; ///< 画布坐标 Y
    QDoubleSpinBox* w_ = nullptr; ///< 显示宽度
    QDoubleSpinBox* h_ = nullptr; ///< 显示高度
    QDoubleSpinBox* rotation_ = nullptr; ///< 旋转角度（度，顺时针绕中心）
    QSpinBox*       z_ = nullptr;        ///< 叠放层级（多屏重叠绘制次序）
    QLineEdit*      player_id_ = nullptr; ///< 绑定的显示端（player id；
                                          ///  连接成功后 display 自动采用）
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
};

} // namespace ms
