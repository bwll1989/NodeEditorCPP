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
//  Scene 预览中 screen 右键「屏幕属性…」弹出（2026-11-25 用户定案：双击
//  入口取消——与选中交互冲突，属性面板唯一入口 = 右键菜单），编辑：
//    * 画布位置与显示大小（x/y/w/h，画布像素，左上原点 Y 向下）
//    * 旋转角度（度，面内顺时针绕中心）+ 屏姿态 rot_x/rot_y（度，
//      投影字段族——冻结保留不扩）
//    * 输出校色（RGB 线性增益 + Gamma，2026-11-20 Display 级四字段）
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

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

#include <cstdint>
#include <functional>

#include "model/SceneModel.hpp"

class QLineEdit;

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

    /// 把表单值写回 screen（accepted 槽内调用——OK 写回；非模态后无
    /// exec 返回值语义）
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
    /// 注入几何+校色实时回调——通用页控件每次变化（拖拽/步进/键入提交）
    /// 立即转发，宿主只取几何+校色字段写回工作副本走自动下发链路。回调收到
    /// 的 Screen 仅几何+校色字段有效（warp/绑定不在实时链内）。
    /// 空 = 不实时应用（纯确认制，旧语义）。
    void setGeometryLiveApply(std::function<void(const Screen&)> fn) {
        geometry_live_ = std::move(fn);
    }

    /// 【2026-11-25 双向联动·画布→弹窗】画布拖动/缩放/旋转/gizmo 会话更新
    /// 工作副本后由宿主调用：把场景当前几何+校色刷进控件。内部
    /// QSignalBlocker 防回环——setValue 触发 valueChanged 会经
    /// geometry_live_ 再写回场景，与画布会话形成抖动环。
    void syncFrom(const Screen& s);

    /// 【2026-11-25 display 名称行 = 弹窗标题（用户定案）】标题栏显示绑定
    /// id + 在线状态 + 地址（宿主组装富文本；display 注册帧无独立名称——
    /// 名称即绑定 player_id）。打开时注入一次，运行中随宿主 200ms 拍重设
    ///（上/下线标题内实时可见）。空 = 回退静态标题"Display 属性"。
    void setDisplayInfo(const QString& text);

private:
    FloatDragValueWidget* x_ = nullptr; ///< 画布坐标 X
    FloatDragValueWidget* y_ = nullptr; ///< 画布坐标 Y
    FloatDragValueWidget* w_ = nullptr; ///< 显示宽度
    FloatDragValueWidget* h_ = nullptr; ///< 显示高度
    FloatDragValueWidget* rotation_ = nullptr; ///< 旋转 Z（滚转，度，面内顺时针绕中心）
    FloatDragValueWidget* rot_x_ = nullptr; ///< 旋转 X（俯仰，度；屏取景窗 3D 姿态）
    FloatDragValueWidget* rot_y_ = nullptr; ///< 旋转 Y（偏航，度；屏取景窗 3D 姿态）
    // z_ 已删除（2026-11-18 层级行退役；2026-11-20 模型/proto 字段一并删除）
    // ---- 输出校色（2026-11-20 Display 级 RGB 增益 + Gamma）--------------
    FloatDragValueWidget* gain_r_ = nullptr; ///< R 通道线性增益（1=不变）
    FloatDragValueWidget* gain_g_ = nullptr; ///< G 通道线性增益
    FloatDragValueWidget* gain_b_ = nullptr; ///< B 通道线性增益
    FloatDragValueWidget* gamma_  = nullptr; ///< 伽马指数（1.0=不校正）
    // projection_mode_ 已退役（2026-11-20 用户定案）：纯 2D 取景范式下
    // Display 恒为取景窗（Framed）；模型/proto 字段 projectionMode 已删
    QLineEdit*      player_id_ = nullptr; ///< 绑定的显示端（player id；
                                          ///  连接成功后 display 自动采用）
    QLineEdit*      host_ = nullptr;      ///< 显示端主机（登记用）
    IntDragValueWidget* port_ = nullptr; ///< 显示端监听端口（登记用）
    QLineEdit*      pairing_code_ = nullptr; ///< 配对码（空=不认证）

    // ---- 输出变形（2026-11-11 内嵌「几何结构」页，不再弹窗）--------------
    // 编辑中实时变化经 setLiveApply 回调转发 warp_live_（宿主写场景 +
    // 自动下发）；属性框整体取消由宿主还原（打开前备份）。
    ScreenWarpPanel* warp_panel_ = nullptr; ///< 几何结构页内嵌编辑面板

    /// warp 实时应用回调（宿主注入；编辑中转发，未注入则空操作）
    std::function<void(const ScreenWarp&)> warp_live_;

    /// 几何八字段 + 校色四字段实时回调（2026-11-11 起几何，2026-11-20 并入
    /// 校色；控件变化即转发，未注入则不转发）
    std::function<void(const Screen&)> geometry_live_;
};

} // namespace ms
