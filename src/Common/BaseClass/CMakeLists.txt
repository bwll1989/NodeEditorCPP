cmake_minimum_required(VERSION 3.10)

project(BaseClass LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# ============================================================================
#  BaseClass —— 基类 DLL（节点委托 + 时间轴片段基类层）
#
#  【2026-11-13 定稿：片段基类层回归本库】用户命名统一定案——基类都在
#  BaseClass.dll（"XxxBase" 命名逻辑：BaseClass / TimeLineBase）。此前
#  一度拆出的 ms_clipbase.dll 独立库撤销，文件全部回归本目录：
#
#  组成：
#    * AbstractDelegateModel        —— 节点委托基类（QtNodes 派生；
#      ExternalBinding OSC 树；80+ 节点继承）
#    * AbstractClipModel            —— 时间轴片段基类（μs 原生；OSC 外控）
#    * SceneClipModel               —— 场景片段业务基类（素材镜像 +
#      合并关键帧带；全工程唯一 staticMetaObject）
#    * AbstractClipInterface        —— clip 插件工厂接口（ClipPlugInterface）
#    * ClipTimePanel/ClipAppearancePanel —— 片段属性通用面板
#    * TimeLineDefines/TimeLineStyle —— 时间轴共享定义/全局样式
#      （TimeLineStyle 全局量 extern 化：跨 DLL 单实例，保 [clip-sync]；
#       TimeCodeDefines 不在本库——与 AppConfig 原有定义合并，引用
#       "Common/AppConfig/TimeCodeDefines.h"，防双副本漂移）
#    * TweenDots/TweenPalette       —— Tween 关键帧镜像数据/色表
#    * BasePluginLoader             —— clip 插件加载器（磁盘 .plugin 扫描 +
#      进程内注册；时间轴经 setPluginLoader 注入使用）
#
#  新片段类型扩展方式（src/Clips/）：
#    class XxxClipModel : public SceneClipModel { ... };          // 模型
#    class XxxClipPlugin : public QObject, public ClipPlugInterface { ... }; // 工厂
#    一个媒体类型一个头文件，经 BasePluginLoader 注册（磁盘 .plugin 或进程内）。
#
#  导出：BASECLASS_EXPORT（BaseClassExport.h，PRIVATE BASECLASS_LIBRARY）。
#  时间轴视图库见 ../TimeLineBase/（NODE_TIMELINE_* 宏体系不变）。
# ============================================================================

set(BaseClass_sources
        BaseClassExport.h
        # ---- 节点委托基类 ----
        AbstractDelegateModel.h
        AbstractDelegateModel.cpp
        # ---- 时间轴片段基类层 ----
        AbstractClipModel.hpp
        AbstractClipModel.cpp
        AbstractClipInterface.h
        SceneClipModel.hpp
        TweenDots.h
        TweenPalette.h
        TimeLineDefines.h
        TimeLineStyle.h
        TimeLineStyle.cpp
        ClipTimePanel.hpp
        ClipTimePanel.cpp
        ClipAppearancePanel.hpp
        ClipAppearancePanel.cpp
        # ---- clip 插件加载器 ----
        BasePluginLoader.h
        BasePluginLoader.cpp
)

add_library(BaseClass SHARED ${BaseClass_sources})

target_include_directories(BaseClass
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/Common/Devices>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/Common/Gui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>   # AbstractClipModel.hpp 等
                                                         # 直接 include 形式
        $<INSTALL_INTERFACE:include>
        $<INSTALL_INTERFACE:include/Common/Devices>
        $<INSTALL_INTERFACE:include/Common/Gui>
)

# GUIElements（IntDragValueWidget——AbstractClipModel 外控 qobject_cast +
# ClipTimePanel 微调框）；StatusContainer——外控登记/GlobalEventBus 反馈；
# QtNodes/PropertyTreeWidget——AbstractDelegateModel（节点委托）。
# DLL 化后符号必须自足（静态库时代由最终 exe 兜底解析的隐性依赖显式化）。
target_link_libraries(BaseClass
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        QtNodes::QtNodes
        StatusContainer
        PropertyTreeWidget
        GUIElements
)

target_compile_definitions(BaseClass
    PRIVATE
        BASECLASS_LIBRARY
        NODE_EDITOR_SHARED
)
