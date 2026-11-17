cmake_minimum_required(VERSION 3.10)

project(ClipPanelBase LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# ============================================================================
#  ClipPanelBase —— 通用片段 UI 框架 DLL（片段属性面板 + Tween 合并区）
#
#  【2026-11-13 片段插件化，用户定案「用户只关注片段的实现」】自主程序
#  迁入两对文件：
#    * ClipPropertyPanel —— 片段属性面板框架（媒体绑定区 + appendSection
#      追加区宿主；写回经注入的 applier 回调，面板不持模型指针）
#    * TweenSection      —— 变换与关键帧合并编辑区（全类型片段追加；
#      transform 静态基准 + tweens 通道双模直写，v4 姿态合并版）
#
#  【P3 面板迁库 2026-11-17】库通用面板自 BaseClass 迁入（BaseClass 回归
#  零 UI 纯模型/接口库）：
#    * ClipTimePanel        —— 页 1 时间属性区块（构造尾自治登记 OSC 外控
#      6 控件 + pushOscInitialState，原 AbstractClipModel::initPropertyWidget
#      尾部注册块收编入构造）；
#    * ClipAppearancePanel  —— 页 1 颜色/备注区块（QSS qproperty-* 可配）。
#  ClipPropertyPanel 构造参数 owner_model 非空即在页 1 顶部自治装配两区块
#（顺序 = 时间 → 片段 → 媒体）——装配中心收拢完成。
#
#  片段专属编辑区（TextClipSection/LiveClipSection）不在本库——随各自
#  片段插件走（src/Clips/Text|Live/，经 ClipPlugInterface::
#  createPropertySection 工厂加载，宿主经 ClipSectionInterface 广播灌值）。
#
#  命名序列（用户定案）："XxxBase" = BaseClass / TimeLineBase / SpoutBase /
#  StandaloneBase / ClipPanelBase。导出宏 CLIPPANELBASE_EXPORT（专有名——
#  勿用 DLL_EXPORT 通用名，防 PluginDefinition.hpp #ifndef 抢注）。
#
#  依赖（DLL 化铁律：符号自足）：
#    * MediaCore   —— ms::Clip/Scene 模型 + TweenEngine 关键帧求值
#    * MediaManger —— MediaLibrary（媒体库选择器候选源）
#    * GUIElements —— SelectorComboBox / FloatDragValueWidget /
#                     IntDragValueWidget（ClipTimePanel 帧数微调框）
#    * BaseClass   —— TweenPalette（tween 行指示色与曲线同源）+
#                     AbstractClipModel（通用区 owner 模型信号连接）
#  include 解析：model//engine/（MediaCore PUBLIC 根）、Elements/
#  （全局 src/Common/Gui）、Common/Devices/（全局 src/Common/Devices）、
#  "Common/ClipPanelBase/Xxx.hpp" 消费形式（全局 src 根 + SDK 安装树）。
# ============================================================================

set(ClipPanelBase_sources
        ClipPanelBaseExport.h
        ClipPropertyPanel.hpp
        ClipPropertyPanel.cpp
        TweenSection.hpp
        TweenSection.cpp
        # ---- 库通用面板（P3 自 BaseClass 迁入 2026-11-17）----
        ClipTimePanel.hpp
        ClipTimePanel.cpp
        ClipAppearancePanel.hpp
        ClipAppearancePanel.cpp
)

add_library(ClipPanelBase SHARED ${ClipPanelBase_sources})

target_include_directories(ClipPanelBase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>   # 库内直接名
        $<INSTALL_INTERFACE:include/Common/ClipPanelBase>
)

target_link_libraries(ClipPanelBase
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        MediaCore          # ms::Clip/Scene + TweenEngine（符号自足）
        MediaManger        # MediaLibrary（媒体绑定区候选源）
        GUIElements        # SelectorComboBox/FloatDragValueWidget
        BaseClass          # TweenPalette（行指示色同源）
)

target_compile_definitions(ClipPanelBase
    PRIVATE
        CLIPPANELBASE_LIBRARY
)
