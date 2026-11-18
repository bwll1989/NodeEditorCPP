# =============================================================================
# CefBox — CEF OSR 运行时底座（Qt-free）
#
# 版本锁：CEF 156.0.31+ge603d97+chromium-156.0.8078.100 (windows64_minimal)
#         sha1 = d179fb251141d6c30992a1775db80a6a879852ba
#         位置 = 3rdParty/cef/cef_binary_156.0.31+ge603d97+chromium-156.0.8078.100_windows64_minimal
# 升级 CEF 版本时：替换 3rdParty/cef/ 目录并同步改 CEF_ROOT 与上面注释。
#
# 目标：
#   cef_dll_wrapper  STATIC  官方 C++ 包装层（Windows 全量 .cc）
#   CefBox           SHARED  运行时底座（CefRuntime 泵 + CefOsrBrowser）
#   FlowCefHelper    EXE     CEF 子进程宿主（browser/renderer/gpu 统一入口）
#   cefbox_test      EXE     P0 冒烟测试
# =============================================================================
set(CEF_ROOT "${CMAKE_SOURCE_DIR}/3rdParty/cef/cef_binary_156.0.31+ge603d97+chromium-156.0.8078.100_windows64_minimal")
set(CEF_LIB  "${CEF_ROOT}/Release/libcef.lib")

# ---------- CEF 公共编译定义（对齐官方 cmake/cef_variables.cmake Windows 段） ----------
set(CEF_COMMON_DEFINES
    __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS
    NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00 NTDDI_VERSION=NTDDI_WIN10_FE
    _HAS_EXCEPTIONS=0)
set(CEF_COMMON_FLAGS /GR- /EHs-c- /MP /utf-8 /W3 /wd4251 /wd4275)

# ---------- libcef_dll_wrapper（STATIC） ----------
# 213 个 .cc 全量参与（平台差异仅 .mm/_win.cc，无 Linux-only 源）
file(GLOB_RECURSE CEF_WRAPPER_SRCS "${CEF_ROOT}/libcef_dll/*.cc")
add_library(cef_dll_wrapper STATIC ${CEF_WRAPPER_SRCS})
target_include_directories(cef_dll_wrapper PUBLIC "${CEF_ROOT}")
target_compile_definitions(cef_dll_wrapper PRIVATE WRAPPING_CEF_SHARED ${CEF_COMMON_DEFINES})
target_compile_options(cef_dll_wrapper PRIVATE ${CEF_COMMON_FLAGS})
# 运行时库沿用全局 CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL（/MD），与 CefBox 一致

# ---------- CefBox（SHARED） ----------
add_library(CefBox SHARED
    CefBoxExport.h
    CefRuntime.h    CefRuntime.cpp
    CefOsrBrowser.h CefOsrBrowser.cpp)
target_compile_definitions(CefBox PRIVATE CEFBOX_LIBRARY ${CEF_COMMON_DEFINES})
target_compile_options(CefBox PRIVATE ${CEF_COMMON_FLAGS})
target_include_directories(CefBox PUBLIC "${CEF_ROOT}")  # 消费者需编译 CefOsrBrowser.h 内的 CEF 头
target_link_libraries(CefBox PUBLIC cef_dll_wrapper "${CEF_LIB}" PRIVATE user32 gdi32)

# ---------- FlowCefHelper（WIN32 子系统，无窗口无 Qt） ----------
add_executable(FlowCefHelper WIN32 subprocess/main.cpp)
target_compile_definitions(FlowCefHelper PRIVATE ${CEF_COMMON_DEFINES})
target_compile_options(FlowCefHelper PRIVATE ${CEF_COMMON_FLAGS})
target_link_libraries(FlowCefHelper PRIVATE cef_dll_wrapper "${CEF_LIB}")
# CEF 主线程需大栈；64 位官方推荐 8MiB
target_link_options(FlowCefHelper PRIVATE /STACK:0x800000)

# ---------- cefbox_test（P0 冒烟测试，控制台） ----------
add_executable(cefbox_test cefbox_test.cpp)
target_compile_definitions(cefbox_test PRIVATE ${CEF_COMMON_DEFINES})
target_compile_options(cefbox_test PRIVATE ${CEF_COMMON_FLAGS})
target_link_libraries(cefbox_test PRIVATE CefBox user32)

# ---------- CEF 运行时文件平铺到 bin/（Flow.exe / FlowCefHelper.exe 旁） ----------
add_custom_command(TARGET CefBox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_ROOT}/Release/"   "$<TARGET_FILE_DIR:CefBox>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_ROOT}/Resources/" "$<TARGET_FILE_DIR:CefBox>"
    COMMENT "CefBox: copy CEF runtime (Release + Resources) -> bin/")
