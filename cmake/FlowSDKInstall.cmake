# FlowSDK 安装与导出
# 由根 CMakeLists 在目标定义完成后 include。
# 用法：
#   cmake --build <build> --target install-FlowSDK
#   或: cmake --install <build> --prefix <repo>/sdk --component FlowSDK

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(FLOW_SDK_EXPORT_NAME FlowSDKTargets)
set(FLOW_SDK_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/FlowSDK")

# ---------------------------------------------------------------------------
# 需要进入 EXPORT 的库（含 SHARED 的 PRIVATE 依赖，CMake install(EXPORT) 要求）
# ---------------------------------------------------------------------------
set(_FLOW_SDK_EXPORT_TARGETS
    BaseClass
    DataTypes
    StatusContainer
    PropertyTreeWidget
    GUIElements
    AppConfig
    OSCTransmitter
    MediaManger
    ModelDataBridge
    TimestampGenerator
)

set(_FLOW_SDK_RUNTIME_TARGETS)

foreach(_t ${_FLOW_SDK_EXPORT_TARGETS})
    if(NOT TARGET ${_t})
        message(FATAL_ERROR "FlowSDK: missing target ${_t}")
    endif()
endforeach()

install(TARGETS ${_FLOW_SDK_EXPORT_TARGETS}
    EXPORT ${FLOW_SDK_EXPORT_NAME}
    COMPONENT FlowSDK
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

foreach(_t ${_FLOW_SDK_RUNTIME_TARGETS})
    if(TARGET ${_t})
        install(TARGETS ${_t}
            COMPONENT FlowSDK
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
endforeach()

# ---------------------------------------------------------------------------
# 头文件（保持与源码侧相近的 include 路径）
# ---------------------------------------------------------------------------
install(FILES
    "${CMAKE_SOURCE_DIR}/src/Common/BaseClass/AbstractDelegateModel.h"
    "${CMAKE_SOURCE_DIR}/src/Common/BaseClass/AbstractClipDelegateModel.h"
    "${CMAKE_SOURCE_DIR}/src/Common/BaseClass/BaseClassExport.h"
    DESTINATION include/Common/BaseClass
    COMPONENT FlowSDK
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/Common/DataTypes/"
    DESTINATION include/DataTypes
    COMPONENT FlowSDK
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "CMakeLists.txt" EXCLUDE
)

# NodeDataList / 音视频环形缓冲头依赖 TimestampGenerator
install(FILES
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/TimestampGenerator/TimestampGenerator.hpp"
    DESTINATION include/TimestampGenerator
    COMPONENT FlowSDK
)
install(FILES
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/TimestampGenerator/TimestampGenerator.hpp"
    DESTINATION include/Common/Devices/TimestampGenerator
    COMPONENT FlowSDK
)

install(FILES
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/StatusContainer/StatusContainer.h"
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/StatusContainer/StatusItem.h"
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/StatusContainer/GlobalEventBus.hpp"
    DESTINATION include/Common/Devices/StatusContainer
    COMPONENT FlowSDK
)
# 兼容 #include "StatusContainer/..."
install(FILES
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/StatusContainer/StatusContainer.h"
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/StatusContainer/StatusItem.h"
    "${CMAKE_SOURCE_DIR}/src/Common/Devices/StatusContainer/GlobalEventBus.hpp"
    DESTINATION include/StatusContainer
    COMPONENT FlowSDK
)

# 常用 GUI 控件头（IntDrag / FloatDrag / Trigger / Color 等）
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/Common/GUI/Elements/"
    DESTINATION include/Common/Gui/Elements
    COMPONENT FlowSDK
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "CMakeLists.txt" EXCLUDE
)
# 兼容 #include "Elements/..."
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/Common/GUI/Elements/"
    DESTINATION include/Elements
    COMPONENT FlowSDK
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "CMakeLists.txt" EXCLUDE
)

install(FILES
    "${CMAKE_SOURCE_DIR}/src/Common/GUI/PropertyTreeWidget/PropertyTreeWidget.h"
    DESTINATION include/Common/GUI/PropertyTreeWidget
    COMPONENT FlowSDK
)

# StatusContainer 依赖的 OSCMessage（来自 QtTimeLine）
if(EXISTS "${QtTimeLine_DIR}/install/include/OSCMessage.h")
    install(FILES "${QtTimeLine_DIR}/install/include/OSCMessage.h"
        DESTINATION include
        COMPONENT FlowSDK
    )
elseif(EXISTS "${CMAKE_SOURCE_DIR}/3rdParty/QtTimeLine/OSCMessage.h")
    install(FILES "${CMAKE_SOURCE_DIR}/3rdParty/QtTimeLine/OSCMessage.h"
        DESTINATION include
        COMPONENT FlowSDK
    )
endif()

# QtNodes 公共头 + 必要 internal（AbstractDelegateModel 仍引用 internal/NodeDelegateModel.hpp）
if(EXISTS "${QtNodes_DIR}/QtNodes")
    install(DIRECTORY "${QtNodes_DIR}/QtNodes/"
        DESTINATION include/QtNodes
        COMPONENT FlowSDK
    )
endif()
if(EXISTS "${DEPENDS_DIR}/nodeeditor/MSVC/lib/QtNodes.lib")
    install(FILES "${DEPENDS_DIR}/nodeeditor/MSVC/lib/QtNodes.lib"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT FlowSDK
    )
endif()
if(EXISTS "${DEPENDS_DIR}/nodeeditor/MSVC/bin/QtNodes.dll")
    install(FILES "${DEPENDS_DIR}/nodeeditor/MSVC/bin/QtNodes.dll"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT FlowSDK
    )
endif()

# QtTimeLine（BaseClass 含 AbstractClipDelegateModel，Node/Clip 均需要）
if(EXISTS "${QtTimeLine_DIR}/install/include")
    install(DIRECTORY "${QtTimeLine_DIR}/install/include/"
        DESTINATION include
        COMPONENT FlowSDK
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )
endif()
if(EXISTS "${QtTimeLine_DIR}/install/lib/QtTimeLine.lib")
    install(FILES "${QtTimeLine_DIR}/install/lib/QtTimeLine.lib"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT FlowSDK
    )
endif()
if(EXISTS "${QtTimeLine_DIR}/install/bin/QtTimeLine.dll")
    install(FILES "${QtTimeLine_DIR}/install/bin/QtTimeLine.dll"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT FlowSDK
    )
elseif(EXISTS "${QtTimeLine_DIR}/install/QtTimeLine.dll")
    install(FILES "${QtTimeLine_DIR}/install/QtTimeLine.dll"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT FlowSDK
    )
endif()

# ---------------------------------------------------------------------------
# CMake 包配置
# ---------------------------------------------------------------------------
install(EXPORT ${FLOW_SDK_EXPORT_NAME}
    FILE FlowSDKTargets.cmake
    NAMESPACE Flow::
    DESTINATION ${FLOW_SDK_CMAKE_DIR}
    COMPONENT FlowSDK
)

# ---------------------------------------------------------------------------
# 版本：与 Flow 产品版本一致（ConstantDefines.h → PRODUCT_VERSION）
# ---------------------------------------------------------------------------
set(FLOW_SDK_VERSION "${PROJECT_VERSION}")
set(FLOW_SDK_VERSION_MAJOR 0)
set(FLOW_SDK_VERSION_MINOR 0)
set(FLOW_SDK_VERSION_PATCH 0)
if(EXISTS "${CMAKE_SOURCE_DIR}/src/Common/AppConfig/ConstantDefines.h")
    file(STRINGS "${CMAKE_SOURCE_DIR}/src/Common/AppConfig/ConstantDefines.h" _ver_line
        REGEX "^#define PRODUCT_VERSION ")
    if(_ver_line)
        string(REGEX REPLACE "^#define PRODUCT_VERSION \"([^\"]+)\".*" "\\1" FLOW_SDK_VERSION "${_ver_line}")
    endif()
endif()
# 解析 major.minor.patch（多余段忽略）
if(FLOW_SDK_VERSION MATCHES "^([0-9]+)(\\.([0-9]+))?(\\.([0-9]+))?")
    set(FLOW_SDK_VERSION_MAJOR "${CMAKE_MATCH_1}")
    if(CMAKE_MATCH_3 STREQUAL "")
        set(FLOW_SDK_VERSION_MINOR 0)
    else()
        set(FLOW_SDK_VERSION_MINOR "${CMAKE_MATCH_3}")
    endif()
    if(CMAKE_MATCH_5 STREQUAL "")
        set(FLOW_SDK_VERSION_PATCH 0)
    else()
        set(FLOW_SDK_VERSION_PATCH "${CMAKE_MATCH_5}")
    endif()
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/FlowSDKVersion.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FlowSDKVersion.h"
    @ONLY
)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/FlowSDK_VERSION" "${FLOW_SDK_VERSION}\n")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FlowSDKVersion.h"
    DESTINATION include/Flow
    COMPONENT FlowSDK
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FlowSDK_VERSION"
    DESTINATION .
    RENAME VERSION
    COMPONENT FlowSDK
)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/FlowSDKConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FlowSDKConfig.cmake"
    INSTALL_DESTINATION ${FLOW_SDK_CMAKE_DIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FlowSDKConfigVersion.cmake"
    VERSION "${FLOW_SDK_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FlowSDKConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FlowSDKConfigVersion.cmake"
    DESTINATION ${FLOW_SDK_CMAKE_DIR}
    COMPONENT FlowSDK
)

# 模板与文档
install(DIRECTORY "${CMAKE_SOURCE_DIR}/sdk/templates/"
    DESTINATION templates
    COMPONENT FlowSDK
)
install(FILES "${CMAKE_SOURCE_DIR}/sdk/README.md"
    DESTINATION .
    COMPONENT FlowSDK
)

# CLion / 命令行可构建的安装目标：默认安装到仓库 sdk/ 目录
set(_FLOW_SDK_DEFAULT_PREFIX "${CMAKE_SOURCE_DIR}/sdk")
if(NOT DEFINED CACHE{FLOW_SDK_PREFIX}
   OR FLOW_SDK_PREFIX STREQUAL "${CMAKE_BINARY_DIR}/FlowSDK")
    set(FLOW_SDK_PREFIX "${_FLOW_SDK_DEFAULT_PREFIX}" CACHE PATH "FlowSDK install prefix" FORCE)
endif()
add_custom_target(install-FlowSDK
    COMMAND "${CMAKE_COMMAND}"
            --install "${CMAKE_BINARY_DIR}"
            --prefix "${FLOW_SDK_PREFIX}"
            --component FlowSDK
    COMMENT "Installing FlowSDK to ${FLOW_SDK_PREFIX}"
    VERBATIM
)
# 依赖核心库，避免未编译就安装
add_dependencies(install-FlowSDK
    BaseClass
    DataTypes
    StatusContainer
    PropertyTreeWidget
    GUIElements
    AppConfig
    OSCTransmitter
    MediaManger
    ModelDataBridge
    TimestampGenerator
)

message(STATUS "FlowSDK install rules enabled (component FlowSDK, version ${FLOW_SDK_VERSION})")
message(STATUS "  Build target: install-FlowSDK -> ${FLOW_SDK_PREFIX}")
