# ============================================================================
#  MediaCore —— MediaServer 收编 DLL 版（2026-11-12 从 src/Widget/MediaServer/
#  src/core 整体迁入 src/Common/MediaCore，全量 12 模块编译为 SHARED）
#
#  组件构成（对照原 MediaServer 设计文档 §2.2 组件表）：
#    model/    —— 运行时内存模型（header-only，对序列化零感知）
#    engine/   —— TimelineEngine（evaluate 纯函数）+ MediaFactory 扩展点 +
#                 MediaScheduler（源实例缓存）+ ScreenMapper（画布映射/绘制列表）
#    loader/   —— SceneLoader（pb ↔ 模型 + 引用完整性校验）
#    clock/    —— ClockProvider（复合对齐时钟）
#    config/   —— IniConfig（启动配置：config.ini，两端同源）
#    sync/     —— P2 同步：SyncFrame(UDP) / AnchorScheduler / DriftController
#    net/      —— P2 网络传输：FrameCodec / ControlServer/Client(TCP) / SyncUdp
#    runtime/  —— P2 运行时装配：EditorRuntime / PlayerRuntime（时间注入）
#    media/    —— P1 渲染管线：FfmpegMediaSource（IMediaSource 视频解码实现）
#    protocol/ —— scene.proto + sync.proto（protoc 构建期生成）
#    audio/    —— 音频混音引擎 + PortAudio 输出 + 素材音频解码源
#    mpcdi/    —— MPCDI 包解析/烘焙（minizip/zlib/tinyxml2/stb_image）
#    live/     —— 实时源接入（NDI/Spout/WGC/Camera Ingest + LiveHub）
#    standalone/ —— display 独立模式装配（TimelineRegistry/SceneAssembler/NodeController）
#    platform/ —— 控制台 UTF-8 / 崩溃现场捕获
#
#  依赖原则：Core 不依赖 Qt（保证与 Flow 节点体系解耦）；第三方依赖全部
#  以 NodeEditorCPP/3rdParty 为准（FFmpeg/protobuf/POCO/spdlog/Eigen/
#  portaudio/Gist/NDI/Spout/stb/mpcdi-libs）。
#
#  DLL 化（2026-11-12）：CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 自动导出全部
#  符号，源码零标注（原工程为 STATIC 无导出宏；手写 dllexport 不现实）。
# ============================================================================
cmake_minimum_required(VERSION 3.24)

# ---------------------------------------------------------------------------
# 1. protoc 代码生成：scene.proto + sync.proto → .pb.{h,cc}
#    protoc 用 Flow 侧 prebuilt 产物（install_msvc_prebuilt，Release /MD ABI）
# ---------------------------------------------------------------------------
set(MC_PROTO_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/protocol")
set(MC_PROTO_GEN   "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(MS_PROTO_OUT
    "${MC_PROTO_GEN}/scene.pb.cc" "${MC_PROTO_GEN}/scene.pb.h"
    "${MC_PROTO_GEN}/sync.pb.cc"  "${MC_PROTO_GEN}/sync.pb.h")

file(MAKE_DIRECTORY "${MC_PROTO_GEN}")

set(MC_PROTOC_EXECUTABLE "${DEPENDS_DIR}/protobuf-35.1/install_msvc_prebuilt/bin/protoc.exe")

add_custom_command(
    OUTPUT  ${MS_PROTO_OUT}
    COMMAND "${MC_PROTOC_EXECUTABLE}"
            "--cpp_out=${MC_PROTO_GEN}"
            "-I" "${CMAKE_CURRENT_SOURCE_DIR}/protocol"
            "${CMAKE_CURRENT_SOURCE_DIR}/protocol/scene.proto"
            "${CMAKE_CURRENT_SOURCE_DIR}/protocol/sync.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protocol/scene.proto"
            "${CMAKE_CURRENT_SOURCE_DIR}/protocol/sync.proto"
    COMMENT "protoc 生成 scene.pb.* / sync.pb.*"
    VERBATIM)

# ---------------------------------------------------------------------------
# 2. MediaCore 动态库（全量模块）
# ---------------------------------------------------------------------------
add_library(MediaCore SHARED
    # ---- engine：时间轴求值/缓动/画布映射/软渲染 ----
    engine/TimelineEngine.cpp
    engine/TweenEngine.cpp
    engine/ScreenMapper.cpp
    engine/SoftRenderer.cpp
    # ---- audio：混音引擎 + PortAudio 输出 + 素材音频解码源 ----
    audio/AudioEngine.cpp
    audio/AudioOut.cpp
    audio/AudioMediaSource.cpp
    # ---- gist 时域特征显式实例化 TU（float/double；GPLv3 vendor——
    #      RMS 电平检测实现体，漏编即 LNK2019；源码取 Flow Gist-1.0.7）----
    "${DEPENDS_DIR}/Gist-1.0.7/src/CoreTimeDomainFeatures.cpp"
    # ---- clock / config / loader ----
    clock/ClockProvider.cpp
    config/IniConfig.cpp
    loader/SceneLoader.cpp
    loader/ShowFileIO.cpp
    # ---- sync（P2 同步）----
    sync/SyncFrame.cpp
    # ---- net（P2 网络传输）----
    net/FrameCodec.cpp
    net/ControlServer.cpp
    net/ControlClient.cpp
    net/DisplayConnector.cpp
    net/SyncUdp.cpp
    net/Discovery.cpp
    net/AssetHttpServer.cpp
    net/AssetReceiver.cpp
    # ---- runtime（P2 运行时装配）----
    runtime/EditorRuntime.cpp
    runtime/PlayerRuntime.cpp
    runtime/RenderThread.cpp
    # ---- standalone（display 独立模式装配）----
    standalone/TimelineRegistry.cpp
    standalone/SceneAssembler.cpp
    standalone/NodeController.cpp
    # ---- mpcdi（包解析/烘焙）----
    mpcdi/mpcdi_zip.cpp
    mpcdi/mpcdi_xml.cpp
    mpcdi/mpcdi_pfm.cpp
    mpcdi/mpcdi_alpha.cpp
    mpcdi/mpcdi_build.cpp
    mpcdi/mpcdi_to_warp.cpp
    # ---- media（渲染管线）----
    media/FfmpegMediaSource.cpp
    media/HwAccel.cpp
    media/TextRasterizer.cpp
    media/TextMediaSource.cpp
    media/LiveMediaSource.cpp
    # ---- live（实时源接入）----
    live/LiveIngest.cpp
    live/IngestBase.cpp
    live/NdiIngest.cpp
    live/SpoutIngest.cpp
    live/WgcIngest.cpp
    live/CameraIngest.cpp
    live/LiveHub.cpp
    # ---- platform ----
    platform/CrashDump.cpp
    # ---- protoc 生成的 pb 源 ----
    ${MS_PROTO_OUT}
)

set_target_properties(MediaCore PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    # 导出符号：不用 CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS——其 def 生成会把
    # guiddef.h（extern "C" 内 __inline GUID operator==/!=）的字面名符号
    # `==`/`!=` 写进 EXPORTS → link LNK2001（静态库时代不生成 def 从未暴露）。
    # 改用 gen_exports.py 自建 def（过滤非法符号名），PRE_LINK 生成。
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
    COMPILE_PDB_NAME MediaCore)

# ---- 自建 def：dumpbin 扫 obj → 过滤 → EXPORTS（PRE_LINK 时 obj 已全在）----
get_filename_component(MC_TOOLCHAIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
set(MC_DUMPBIN "${MC_TOOLCHAIN_DIR}/dumpbin.exe")
find_program(MC_PYTHON python)
set(MC_AUTO_DEF "${CMAKE_CURRENT_BINARY_DIR}/MediaCore_auto.def")
add_custom_command(TARGET MediaCore PRE_LINK
    COMMAND ${MC_PYTHON}
            "${CMAKE_CURRENT_SOURCE_DIR}/gen_exports.py"
            "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/MediaCore.dir"
            "${MC_DUMPBIN}"
            "${MC_AUTO_DEF}"
    VERBATIM)
target_link_options(MediaCore PRIVATE "/DEF:${MC_AUTO_DEF}")

target_include_directories(MediaCore
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"   # 模块相对路径：model/ engine/ ...
        "${MC_PROTO_GEN}")              # scene.pb.h / sync.pb.h 所在目录
# stb_truetype/stb_image（单头库）：TextRasterizer.cpp / mpcdi_alpha.cpp
# gist 头（GPLv3）：AudioEngine.cpp include "CoreTimeDomainFeatures.h"
target_include_directories(MediaCore PRIVATE
    "${DEPENDS_DIR}/stb"
    "${DEPENDS_DIR}/Gist-1.0.7/src")

# ---------------------------------------------------------------------------
#  protobuf（prebuilt install 产物，Release /MD ABI；PROTOBUF_USE_DLLS /
#  ABSL_CONSUME_DLL 缺一即 LNK2001 __imp_ 前缀数据符号）
# ---------------------------------------------------------------------------
set(MC_PROTOBUF_ROOT "${DEPENDS_DIR}/protobuf-35.1/install_msvc_prebuilt")
add_library(mc_protobuf SHARED IMPORTED)
set_target_properties(mc_protobuf PROPERTIES
    IMPORTED_IMPLIB "${MC_PROTOBUF_ROOT}/lib/libprotobuf.lib"
    IMPORTED_LOCATION "${MC_PROTOBUF_ROOT}/bin/libprotobuf.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${MC_PROTOBUF_ROOT}/include")
add_library(mc_abseil SHARED IMPORTED)
set_target_properties(mc_abseil PROPERTIES
    IMPORTED_LOCATION "${MC_PROTOBUF_ROOT}/bin/abseil_dll.dll"
    IMPORTED_IMPLIB   "${MC_PROTOBUF_ROOT}/lib/abseil_dll.lib")
target_compile_definitions(MediaCore PUBLIC PROTOBUF_USE_DLLS ABSL_CONSUME_DLL)

# ---------------------------------------------------------------------------
# 依赖总表（全部指向 NodeEditorCPP/3rdParty）：
#   PUBLIC  —— 消费方（ms_editor_ui/主程序）头文件里会碰到：pb.h、spdlog、
#              POCO net/foundation、FFmpeg include
#   PRIVATE —— DLL 内部实现消费：Eigen 几何、NDI/Spout 接收、mpcdi 底层件、
#              PortAudio、MF 采集、WGC 系统库
# ---------------------------------------------------------------------------
target_link_libraries(MediaCore
    PUBLIC  mc_protobuf mc_abseil
            ${Spdlog_LIBRARIES}
            Poco::Net Poco::Foundation
            "${FFMPEG_LIB_DIRS}/avcodec.lib"
            "${FFMPEG_LIB_DIRS}/avformat.lib"
            "${FFMPEG_LIB_DIRS}/avutil.lib"
            "${FFMPEG_LIB_DIRS}/swscale.lib"
            "${FFMPEG_LIB_DIRS}/swresample.lib"
    PRIVATE Eigen3::Eigen
            ms_ndi
            SpoutBase
            mc_mpcdi_libs
            ${portaudio_LIBRARIES}
            windowsapp dwmapi
            mfplat mf mfuuid)
# FFmpeg include（FfmpegMediaSource/HwAccel 引 av_*.h；FFMPEG_INCLUDE_DIRS
# 已由主工程全局 include_directories 覆盖，无需重复设置）

# portaudio include（Flow 的 portaudio_LIBRARIES 只有 lib 路径）
target_include_directories(MediaCore PRIVATE "${portaudio_DIR}")

# ---------------------------------------------------------------------------
#  mpcdi-libs（MPCDI 包解析底层件）：minizip/zlib 精简集/tinyxml2 源码静态编译
#  （2026-11-12 从 MediaServer 3rdparty 收编入 Flow 3rdParty）
# ---------------------------------------------------------------------------
add_library(mc_mpcdi_libs STATIC
    "${DEPENDS_DIR}/mpcdi-libs/minizip/ioapi.c"
    "${DEPENDS_DIR}/mpcdi-libs/minizip/iowin32.c"
    "${DEPENDS_DIR}/mpcdi-libs/minizip/unzip.c"
    "${DEPENDS_DIR}/mpcdi-libs/tinyxml2/tinyxml2.cpp"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/adler32.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/crc32.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/deflate.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/infback.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/inffast.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/inflate.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/inftrees.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/trees.c"
    "${DEPENDS_DIR}/mpcdi-libs/zlib/zutil.c")
target_include_directories(mc_mpcdi_libs PUBLIC
    "${DEPENDS_DIR}/mpcdi-libs/minizip"
    "${DEPENDS_DIR}/mpcdi-libs/tinyxml2"
    "${DEPENDS_DIR}/mpcdi-libs/zlib")
target_compile_options(mc_mpcdi_libs PRIVATE /w)

# ---------------------------------------------------------------------------
#  运行时 DLL 拷贝：protobuf/abseil（prebuilt DLL 模式必需）
#  （NDI DLL 与 FFmpeg DLL 由 Flow 主程序部署链负责，见主 CMakeLists）
#  PB_ROOT 由调用方传入（子目录变量不回传父作用域，不能依赖 MC_PROTOBUF_ROOT）
# ---------------------------------------------------------------------------
function(mc_copy_runtime_dlls TARGET_NAME PB_ROOT)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PB_ROOT}/bin/libprotobuf.dll"
            "${PB_ROOT}/bin/abseil_dll.dll"
            "${PB_ROOT}/bin/libutf8_range.dll"
            "${PB_ROOT}/bin/libutf8_validity.dll"
            $<TARGET_FILE_DIR:${TARGET_NAME}>)
endfunction()
