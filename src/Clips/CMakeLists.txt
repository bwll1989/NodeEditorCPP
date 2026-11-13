# ============================================================================
#  src/Clips —— 媒体片段磁盘插件（.plugin 动态库；每片段一目录）
#
#  【2026-11-12 自 Widget/Timeline/plugins/ 迁入】
#  【2026-11-13 命名统一：clip 基类层回归 BaseClass.dll（ms_clipbase 撤销）】
#  【2026-11-13 片段插件化（用户定案「用户只关注片段的实现」）】：
#    * 目录化：一种媒体类型 = 一个子目录（Video/Image/Text/Audio/Live），
#      片段实现（模型 + 工厂 + 类型专属编辑区）全部在各自目录内——
#      后续变更只改本目录 → 重编该插件 → 替换 bin/plugins/TimeLine/*.plugin
#      即可，无需重编 Flow 主程序；
#    * Live 磁盘化：live.plugin 与其他四类统一（Q_PLUGIN_METADATA 单份
#      约束下内置兜底同步删除——QtTimelineModelAdapter 的 Builtin 表已
#      移除 Live；删 live.plugin 后 Live 类型不可建，与其他类型一致）；
#    * UI 扩展点：Text/Live 插件经 ClipPlugInterface::createPropertySection
#      供给属性面板专属编辑区（TextClipSection/LiveClipSection 随插件分
#      发，字段写回内化在区块内）；宿主经 ClipSectionInterface 广播灌值。
#
#  插件结构约定（一目录 = XxxClipModel + XxxClipPlugin [+ XxxClipSection]）：
#    * XxxClipModel（类型专属模型：默认色 + 类型标志 + 业务镜像，
#      继承 BaseClass.dll 的 SceneClipModel）
#    * XxxClipPlugin（ClipPlugInterface 工厂：clipType/createModel/
#      createPropertySection；Q_PLUGIN_METADATA 磁盘插件元数据）
#    * XxxClipSection（可选：类型专属属性编辑区；仅 Text/Live 现有）
#
#  每个插件 target：
#    * MODULE 库 + PREFIX "" + SUFFIX ".plugin"（QPluginLoader 按
#      "*.plugin" 过滤，本质仍是 Windows DLL）；
#    * Q_PLUGIN_METADATA(IID ClipPlugInterface_iid) 已在插件头声明
#      （2026-11-13 片段与内核彻底解耦：插件头不再编进主程序，
#      MS_DISK_PLUGIN 条件宏已删——本 target 是插件头唯一编译形态）；
#    * 链 BaseClass（基类 dllimport——SceneClipModel/AbstractClipModel
#      全工程唯一导出，跨模块 qobject_cast/dynamic_cast 均可用）+ Qt6；
#    * Text/Live 另链 MediaCore（Asset 模型/TextRasterizer 测量/LiveIngest
#      注册表——专属区直写绑定素材）+ GUIElements（DragValue 控件，经
#      BaseClass PUBLIC 传播已可达，显式列出表意图）；
#    * 【勿把 BaseClass 库头列入 SOURCES】——AUTOMOC 只扫 target
#      sources，列入便在插件内重复生成 moc，与 dllimport 的
#      staticMetaObject 冲突（C2491/LNK2019）。库头经 include 路径解析；
#      插件自有头必须列入（moc 生成）。
# ============================================================================

# 每种媒体类型一个插件：类型串 → 目录（与 clipType() 一致）
# Control（2026-11-13）：指令片段（合并前 Trigger 语义后继）——无媒体文件、
# 无画布呈现，仅时间轴存在；消息列表存 Clip.control_messages（片段域）。
set(MS_PLUGIN_TYPES Video Image Text Audio Live Control)

foreach(_t IN LISTS MS_PLUGIN_TYPES)
    string(TOLOWER "${_t}" _lower)
    set(_tgt "plugin_${_t}")
    set(_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_t}")

    # 插件源 = 模型+工厂头（必含）+ 专属编辑区（Text/Live 现有）
    set(_srcs "${_dir}/${_t}ClipPlugin.hpp")
    if(EXISTS "${_dir}/${_t}ClipSection.cpp")
        list(APPEND _srcs
             "${_dir}/${_t}ClipSection.hpp"
             "${_dir}/${_t}ClipSection.cpp")
    endif()

    add_library(${_tgt} MODULE ${_srcs})

    set_target_properties(${_tgt} PROPERTIES
        PREFIX ""              # 去掉 lib/dll 前缀
        OUTPUT_NAME "${_lower}" # 输出 video.plugin / image.plugin / ...
        SUFFIX ".plugin"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        # 产物直接落 bin/plugins/TimeLine/（2026-11-13 用户定案；对齐节点插件
        # bin/plugins/DataFlow 先例——RUNTIME/LIBRARY/ARCHIVE 三件套照抄）；
        # 该目录同时是 WorkScenePluginLoader 的默认扫描目录（exe 相对定位），
        # 无需 POST_BUILD 拷贝。
        RUNTIME_OUTPUT_DIRECTORY "${TIMELINE_PLUGIN_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${TIMELINE_PLUGIN_OUTPUT_DIRECTORY}"
        # ARCHIVE（Windows DLL 的 import .lib + .exp）是链接副产物——插件由
        # QPluginLoader 运行时加载，无人链接其 import lib；落到插件目录会
        # 污染可整目录分发的 bin/plugins/TimeLine/（2026-11-13 路径审查），
        # 重定向到 build 内部目录。
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/archive/TimeLine")

    target_link_libraries(${_tgt}
        PRIVATE
            BaseClass          # AbstractClipModel/SceneClipModel 基类（dllimport）
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Gui
            Qt${QT_VERSION_MAJOR}::Widgets)
    # Text/Live 的专属编辑区依赖 MediaCore（Asset/TextRasterizer 测量/LiveIngest）
    # + GUIElements（DragValue 控件，经 BaseClass PUBLIC 传播已可达，显式列出表意图）
    # ——按目录文件存在性追加；Control 另链 OscListWidget（OSC 消息列表控件，
    # include 根 src/Common/Gui 由 GUIElements PUBLIC 传播可达）+ StatusContainer
    #（parseOSC 发送入口，PUBLIC include 含 src 根）
    if(EXISTS "${_dir}/${_t}ClipSection.cpp")
        target_link_libraries(${_tgt} PRIVATE MediaCore GUIElements)
        if(_t STREQUAL "Control")
            # OscListWidget 的头引 QJSEngine（表达式求值）——其 Qml 依赖是
            # PRIVATE 链不传播，插件 TU 需要头路径自链（对齐 OscListWidget）
            target_link_libraries(${_tgt} PRIVATE
                OscListWidget StatusContainer
                Qt${QT_VERSION_MAJOR}::Qml)
        endif()
    endif()

    unset(_srcs)
    unset(_dir)
endforeach()
