# ============================================================================
#  MediaDisplay/CMakeLists.txt —— 显示端独立程序 media_display.exe
#
#  收编适配（2026-11-12 从 MediaServer 迁入 Flow）：
#    * engine_core → MediaCore（收编 SHARED DLL，src/Common/MediaCore；
#      PUBLIC 传播 Poco/protobuf/FFmpeg 的头与链接，display 只需链它）；
#    * ms_poco_JSON → Poco::JSON（Flow 主工程 find_package 全套 Poco）；
#    * 输出独立布局 bin/display/（2026-11-13 恢复原 MS_OUT_DISPLAY 式隔离——
#      曾并入 bin/ 与主程序同址，显示端的 web/timelines/config/媒体库与
#      编辑端产物混居不便管理）。exe 移入 display/ 后 Windows 加载器不再
#      搜 bin/ 父目录，故 POST_BUILD 显式部署依赖 DLL（全部取 3rdParty
#      权威源，与构建序无关）：MediaCore/Poco/protobuf+abseil/FFmpeg/
#      portaudio/NDI。display 自含可整目录分发（显示节点部署形态）；
#    * config.display.example.ini 模板与 icons/display.ico 未随迁——图标改用
#      Flow 主程序 res/res.ico，config 模板段删除（IniConfig 内置默认兜底）。
#      运行时数据（config.ini/timelines//web//素材/媒体库）一律相对 exe
#      目录（ms::config::exeDirectory）——自动落 display/ 内，零代码改动。
#
#  角色对应（设计文档 §2.1）：
#    Player = 引擎 SLAVE：监听端口等待 Editor 接入（默认 17100）→ 收 scene →
#    锚点对齐 → 渲染自己的 screen → UDP 上报。零 Qt 依赖。
# ============================================================================

# ---- standalone 控制面（M2，收编自 MMC；Poco HTTP + JSON）-------------------
#  静态库：REST 控制面（时间轴文档/回放/素材库/MPCDI）+ 媒体库扫描上传。
#  C++20：include media/FfmpegMediaSource.hpp（atomic<shared_ptr> 特性）。
#  2026-11-13 更名 StandaloneBase（原 ms_standalone，与 BaseClass/TimeLineBase
#  命名逻辑统一；本地 standalone/ 目录名不变——include "standalone/xxx" 前缀
#  与 MediaCore 同名目录的消歧靠本地 include 置前，见文件尾注释）。
add_library(StandaloneBase STATIC
    standalone/http_server.cpp            # 骨架：分发/静态托管/CORS/起停
    standalone/standalone_api.cpp         # REST API：timelines/playback/status/library
    standalone/http_util.cpp              # 响应发送/MIME/请求体
    standalone/media_library.cpp          # 媒体库扫描/上传三段式
    standalone/mpcdi_service.cpp          # MPCDI 校准包服务（M4 收编自 MMC）
    standalone/mpcdi_service_session.cpp  # MPCDI 会话持久化/启动恢复
    standalone/path_utf8.cpp              # UTF-8 路径往返
)
set_target_properties(StandaloneBase PROPERTIES
    CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_link_libraries(StandaloneBase
    PUBLIC  MediaCore    # PlayerRuntime/TimelineRegistry/NodeController/装配器
            Poco::JSON   # 时间轴文档 DOM 解析（DLL 已在 bin/）
)
target_include_directories(StandaloneBase PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(media_display
    main.cpp
    PreviewWindowWin32.cpp  # Win32 GDI 输出预览窗口（soft 后端呈现路径；
                            #  GL 后端有自己的窗口——两窗互斥见 main.cpp）
    GlLoader.cpp            # 手写 WGL/GL 3.3 函数表（零第三方依赖）
    GlContextWGL.cpp        # GL 窗口 + 3.3 core 上下文（渲染线程创建）
    GlRenderer.cpp          # GL 合成后端（IComposeRenderer 实现）
    GlCudaInterop.cpp       # CUDA-GL 互操作（0 拷贝；无工具链=桩）
    display.rc              # 应用图标（同目录 display.ico——源图 display.png
                            # 经 Pillow 多尺寸导出；2026-11-13 起 rc 指同目录，
                            # 旧 icons/display.ico 引用指向不存在文件已废弃）
)
set_target_properties(media_display PROPERTIES
    CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/display"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/display")
target_link_libraries(media_display
    PRIVATE
        StandaloneBase  # standalone 控制面（引擎链经其 PUBLIC 传播）
        MediaCore       # 模型/时钟/网络/运行时（与编辑端同源）
        opengl32        # GL 1.1 经典函数直链（1.2+ 经 wglGetProcAddress）
)
# include 歧义保险：本地 standalone/（REST 控制面）必须先于 MediaCore 的
# standalone/（PlayerRuntime 侧装配）——"standalone/xxx" 前缀两目录同名，
# 本地目录置前保证 standalone_api/mpcdi_service 解析到本目录。
target_include_directories(media_display PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}")

# ---- CUDA Toolkit 可选探测（③阶段 0 拷贝；QUIET=缺失不致命）--------------
# 找到 → MS_HAS_CUDA 编译启用完整 interop 实现 + cudart_static 静态链接
#（运行机只需显卡驱动，无 cudart DLL 部署负担）；找不到 → 桩实现，
# 协商旗标自动回退 av_hwframe_transfer_data 下载路径（功能无损）。
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
    target_compile_definitions(media_display PRIVATE MS_HAS_CUDA=1)
    target_link_libraries(media_display PRIVATE CUDA::cudart_static)
    message(STATUS "media_display: CUDA ${CUDAToolkit_VERSION} interop 已启用（MS_HAS_CUDA）")
else()
    message(STATUS "media_display: 未找到 CUDA Toolkit——0 拷贝走桩实现（回退下载路径）")
endif()

# ---- 运行 DLL：display/ 自含部署（2026-11-13 独立布局）----------------------
# exe 移出 bin/ 后加载器不再搜父目录——POST_BUILD 显式拷贝依赖 DLL（清单 =
# dumpbin /DEPENDENTS media_display.exe + MediaCore.dll 实测，剔系统 DLL）。
# 全部取 3rdParty 权威源（源文件恒存在，与构建序无关；MediaCore 用生成器
# 表达式保证拷贝发生在其链接后）。display/ 目录因此可整目录独立分发。
set(MS_DISPLAY_RUNTIME_DIR "${CMAKE_BINARY_DIR}/bin/display")
add_custom_command(TARGET media_display POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        # MediaCore（引擎全链；生成器表达式绑定构建序）
        "$<TARGET_FILE:MediaCore>"
        # Poco（Foundation=基础 JSON=时间轴文档 DOM Net=HTTP 控制面）
        "${DEPENDS_DIR}/poco-1.15.3-all/install/bin/PocoFoundation.dll"
        "${DEPENDS_DIR}/poco-1.15.3-all/install/bin/PocoJSON.dll"
        "${DEPENDS_DIR}/poco-1.15.3-all/install/bin/PocoNet.dll"
        # protobuf/abseil（prebuilt DLL 模式；utf8 校验 DLL 随 protobuf 35.x）
        "${DEPENDS_DIR}/protobuf-35.1/install_msvc_prebuilt/bin/libprotobuf.dll"
        "${DEPENDS_DIR}/protobuf-35.1/install_msvc_prebuilt/bin/abseil_dll.dll"
        "${DEPENDS_DIR}/protobuf-35.1/install_msvc_prebuilt/bin/libutf8_range.dll"
        "${DEPENDS_DIR}/protobuf-35.1/install_msvc_prebuilt/bin/libutf8_validity.dll"
        # FFmpeg shared（解码/混音/缩放）
        "${DEPENDS_DIR}/ffmpeg-8.1.1-full_build-shared/bin/avcodec-62.dll"
        "${DEPENDS_DIR}/ffmpeg-8.1.1-full_build-shared/bin/avformat-62.dll"
        "${DEPENDS_DIR}/ffmpeg-8.1.1-full_build-shared/bin/avutil-60.dll"
        "${DEPENDS_DIR}/ffmpeg-8.1.1-full_build-shared/bin/swresample-6.dll"
        "${DEPENDS_DIR}/ffmpeg-8.1.1-full_build-shared/bin/swscale-9.dll"
        # 音频输出（PortAudio MME）
        "${DEPENDS_DIR}/portaudio/build/MSVC/lib/portaudio.dll"
        # NDI 6（网络接收）
        "${DEPENDS_DIR}/NDI 6 SDK/Bin/x64/Processing.NDI.Lib.x64.dll"
        "${MS_DISPLAY_RUNTIME_DIR}"
    COMMENT "deploy media_display runtime DLLs -> bin/display/"
    VERBATIM)

# ---- standalone 网页（缩水版 Editor：编排/媒体库/播放/设置）→ bin/display/web/
# StandaloneApi 以 exe 同目录 web/ 为静态根（web_dir=exe_dir+"/web"）——
# 目标目录跟 RUNTIME_OUTPUT_DIRECTORY 自动落到 display/web/。
add_custom_command(TARGET media_display POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/standalone/web"
        "$<TARGET_FILE_DIR:media_display>/web"
    COMMENT "deploy standalone web -> bin/display/web/"
    VERBATIM)
