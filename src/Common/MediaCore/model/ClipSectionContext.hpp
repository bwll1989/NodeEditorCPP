// ============================================================================
//  ClipSectionContext.hpp —— 片段专属编辑区上下文（宿主 → 插件 Section 载荷）
//
//  定位（2026-11-13 片段插件化：Text/Live 专属编辑区随各自片段插件走）：
//    * 片段属性面板的"类型专属区"由片段插件供给（ClipPlugInterface::
//      createPropertySection UI 扩展点）——插件与宿主分属不同二进制，
//      业务上下文经一个纯 C++ 结构体穿越 void* 边界；
//    * 本结构即该载荷：宿主（QtTimelineModelAdapter）构造填好，插件侧
//      static_cast<ms::ClipSectionContext*> 还原后使用；
//    * 纯 C++（无 Qt）——与本目录 SceneModel 同约定，MediaCore 不链 Qt。
//
//  字段约定：
//    * clip   —— 工作副本片段（可写指针；Section 据此定位绑定素材并
//                保持刷新同步；指向宿主 Scene 工作副本，生命周期由
//                宿主保证覆盖 Section 存活期）；
//    * scene  —— 场景上下文（assets 素材表查找；findAsset 按
//                clip->asset_id 定位绑定素材——换绑后现查，不缓存）；
//    * on_edited —— 编辑落定回调：Section 把控件值写进素材后调用，
//                宿主负责模型镜像 + sceneEdited + 面板广播灌值。
//                （原适配器 applyClipTextEdit/applyClipLiveEdit 的编排
//                职责收敛于此回调；字段级写回归 Section 自身。）
// ============================================================================

#pragma once

#include <functional>

#include "model/SceneModel.hpp" // ms::Clip / ms::Scene（同库 model 层）

namespace ms {

struct ClipSectionContext {
    Clip* clip = nullptr;            ///< 工作副本片段（可写；定位素材锚点）
    Scene* scene = nullptr;          ///< 场景上下文（素材表 findAsset）
    std::function<void()> on_edited; ///< 编辑落定回调（宿主镜像+广播）
};

} // namespace ms
