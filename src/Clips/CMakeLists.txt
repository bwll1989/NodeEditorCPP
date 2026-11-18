# ============================================================================
#  src/Clips —— 媒体片段磁盘插件（.plugin 动态库；每片段一目录）
#
#  【2026-11-12 自 Widget/Timeline/plugins/ 迁入】
#  【2026-11-13 命名统一：clip 基类层回归 BaseClass.dll（ms_clipbase 撤销）】
#  【2026-11-13 片段插件化（用户定案「用户只关注片段的实现」）】：
#    * 目录化：一种媒体类型 = 一个子目录（Video/Image/Text/Audio/Live/
#      Control），片段实现全部在各自目录内——后续变更只改本目录 → 重编
#      该插件 → 替换 bin/plugins/TimeLine/*.plugin 即可，无需重编主程序；
#    * Live 磁盘化：live.plugin 与其他五类统一（Q_PLUGIN_METADATA 单份
#      约束下内置兜底同步删除——QtTimelineModelAdapter 的 Builtin 表已
#      移除 Live；删 live.plugin 后 Live 类型不可建，与其他类型一致）；
#    * UI 扩展点：Text/Live/Control 插件经
#      ClipPlugInterface::createPropertySection 供给属性面板专属编辑区
#      （XxxClipSection 随插件分发，字段写回内化在区块内）；宿主经
#      ClipSectionInterface 广播灌值。
#  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】：文件结构
#    对齐节点插件基准（src/Nodes/<Name>/：PluginDefinition.hpp/.cpp +
#    类型模型独立头 + 独立 CMakeLists.txt + Doc.md，插件类名统一 Plugin）；
#    本文件相应从 foreach 直构收敛为 foreach(add_subdirectory)——类型
#    清单（对齐节点 ALL_NODES 模式）+ 逐目录转发，构建细节下放各目录。
#
#  每目录结构约定（详见各目录 CMakeLists.txt 与 Doc.md）：
#    * <Type>ClipModel.hpp（类型专属模型：默认色 + 类型标志 + 业务镜像，
#      继承 BaseClass.dll 的 SceneClipModel；类名=文件名，对齐节点
#      <Name>DataModel.hpp 模式）
#    * PluginDefinition.hpp/.cpp（插件定义：class Plugin :
#      ClipPlugInterface——clipType/createModel/createPropertySection，
#      Q_PLUGIN_METADATA 磁盘插件元数据；对齐节点命名，类名统一 Plugin）
#    * <Type>ClipSection.hpp/.cpp（可选：类型专属属性编辑区；
#      Text/Live/Control 现有）
#    * 每插件 target：MODULE 库 + PREFIX "" + SUFFIX ".plugin"
#     （QPluginLoader 按 "*.plugin" 过滤）；产物直落
#      bin/plugins/TimeLine/（TIMELINE_PLUGIN_OUTPUT_DIRECTORY，根
#      CMakeLists 定义）；ARCHIVE（import .lib/.exp）重定向
#      build/archive/TimeLine（2026-11-13 路径审查）；
#    * 【勿把 BaseClass 库头列入插件 SOURCES】AUTOMOC 只扫 target
#      sources，列入便在插件内重复生成 moc，与 dllimport 的
#      staticMetaObject 冲突（C2491/LNK2019）。库头经 include 路径解析；
#      插件自有头必须列入（moc 生成）。
# ============================================================================

# 每种媒体类型一个插件：目录名（与 clipType() 一致，对齐节点 ALL_NODES
# 清单模式）。Control（2026-11-13）：指令片段（合并前 Trigger 语义后继）
# ——无媒体文件、无画布呈现，仅时间轴存在；消息列表存 Clip.control_messages
#（片段域）。Artnet/Mapping（2026-11-15）：自节点体系移植的灯光/映射
# 片段（业务数据存 Clip.clip_data 片段域，field 13）。
# Mesh（2026-11-18 阶段 3A）：3D 模型片段——模型素材的时间轴载体
#（拖入入库+asset_id 绑定），画布隐形，银幕 3D mapping 页消费。
set(MS_PLUGIN_TYPES Video Image Text Audio Live Control Artnet Mapping Mesh)

foreach(_t IN LISTS MS_PLUGIN_TYPES)
    add_subdirectory(${_t})
endforeach()
