cmake_minimum_required(VERSION 3.10)
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/DataFlow)

# 添加构建选项
option(BUILD_ALL_NODES "Build all node plugins at once" ON)
option(BUILD_NODES_INDIVIDUALLY "Build nodes individually (default behavior)" ON)

# 定义所有节点列表
set(ALL_NODES
    VST3PluginNode
    DataInfoNode                              #数据显示
    TCPServerNode                             #TCP服务器模块
    TCPClientNode                             #TCP客户端模块
    TimeLineNode                              #时间轴模块
    UDPSocketNode                             #UDP模块
    SerialPortNode                            #串口模块
    CanBusNode                                #CAN (candleLight/gs_usb)
    RangerMiniNode                            #AgileX Ranger Mini 3.0 (+ Nav Controller)
    MissionNode                               #导航任务 Nav Missions
    DelayNode                                 #延迟模块
    LogicOperationNode                        #逻辑操作模块
    MathOperationNode                         #数学操作模块
    LFONode                                   #LFO模块
#    CurveNode                               #曲线模块
    CurvesNode                                #曲线家族（Curve1D/2D/3D 三子节点合一插件）
    AudioDecoderNode                          #音频解码模块
    AudioListDecoderNode                      #音频播放列表解码
    AudioDeviceOutNode                        #音频输出模块
    HotKeyNode                                #热键模块
    ExtractNode                               #数据分离模块
    CameraNode                                #摄像头输入
    ScreenCaptureNode                         #显示器/窗口捕获
    WebRenderNode                             #网页离屏渲染（TD Web Render TOP 模式）
    LibretroNode                              #Libretro 模拟器节点（mGBA core：图像/音频/手柄）
    CaptureNode                               #图像捕获
    ROINode                                   #交互式 ROI 裁剪
    ImageConstNode                            #图像常量
    CountNode                                 #条件计数
    MergeNode                                 #数据合并
    OscInNode                                 #OSC输入
    OscOutNode                                #OSC输出
    SyncNode                                  #跨主机 Variable 同步 Sync Out/In
    ArtnetInNode                              #Arnet输入
    MpvControllerNode                         #mpv控制
    PingNode                                  #ICMP ping 连通性
    VLC_ControllerNode                            #VLC远程控制
    SPlayControllerNode                       #ENTTEC S-Play 多 playlist 控制
    QSysQrcNode                               #Q-SYS ECP（Extract Named Controls XML）
    SwitchNode                                #输入切换
    ConditionNode                             #条件判断
    WebSocketServerNode                       #WebSocket服务器
    WebSocketClientNode                       #WebSocket客户端
    HttpClientNode                            #HTTP客户端
    SlideShowNode                             #SlideShow 数字标牌控制
    JavaScriptNode                            #JavaScript脚本
    ImageOperates                             #图像操作集合
    ShowStoreGBxNode                         #ELC showStoreGBx 控制器
    USR_IO808Node                           #基于Modbus Master实现与USR-IO808 控制器对接
    TSETLNode                                 #对接TSETL
    AuroraSNode                               #Aurora S 设备状态
    HoldNode                                  #信号保持
    SpoutInNode                               #Spout输入
    RTSPNode                                #RTSP拉流输入
    SpoutOutNode                              #Spout输出
    NDVPlayerNode                             #NDV player
    DMXDeviceNode                             #DMX 设备
    DMXUniverseNode                           #DMX Universe
    ArtnetOutNode                             #Artnet输出
    UniversePlaybackNode                      #DMX Universe 数据回放
    CustomScriptNode                          #自定义js节点
    AudioDeviceInNode                         #音频输入节点
    NoiseGeneratorNode                        #噪声生成器节点
    NDIInNode                                 #NDI输入节点
    NDIOutNode                                #NDI输出节点
    PJLinkNode                                #PJLink输出节点
    MqttClientNode                              #Mqtt客户端节点
    LTCDecoderNode                              #LTC时间码解析节点
    LTCGeneratorNode                            #LTC时间码生成节点
    MTCGeneratorNode                            #MTC时间码生成节点
    MTCDecoderNode                              #MTC时间码解码节点
    AudioMatrixNode                             #音频矩阵节点
    AudioAnalysisNode                           #音频分析节点
    DAWController                          #DAW控制器节点
    StyleTransferONNXNode                       #神经网络风格迁移
    DataVisualNode                              #数据可视化
    FileLoadNode                                #文件导入
    ToFiles                                     #文件导出（图像/视频/音频）
    ColorNode                                   #颜色节点
    FmodNode                                    #Fmod 节点
    AudioDuckingNode                            #音频闪避
    AudioCrossFaderNode                         #音频交叉淡化过渡
    AudioPriorityNode                           #音频优先级
    WebMicNode                                  #网页对讲麦克风
    AudioGate                                   #噪声门
    AudioDelayNode                              #音频延时
    PoseDetectionNode                            #Yolo v11n 姿态
    ObjectDetectionNode                         #Yolo v11n 检测
    ObjectTrackNode                             #YOLOE 参考图跟踪
    FaceDetectionNode                              #人脸检测
    MediaPipe/HandPoseEstimationMediaPipeNode      #MediaPipe 手部关节点
    MediaPipe/PoseEstimationMediaPipeNode          #MediaPipe 人体姿态
    MediaPipe/HumanSegmentationPPHumanSegNode      #MediaPipe 人体分割
    MediaPipe/GestureNode                          #手势动作库模板匹配（.gesture）
    VideoDecoderNode                              #视频解码器节点
    PLC-ModBus                                  #modbus控制PLC
    ModBusSlave                                 #Modbus从机节点
    ModBusMaster                                #Modbus主机节点
    InternalCommandsNode                        #内部控制节点
    InjectNode                                  #数据注入
    ValueLookupNode                          #值查表 Value Lookup
    FT_C040ANode                            #触摸屏
    FT_AJTNode                              #AJT Node + AJT Gateway 六路调光
    ViosoNode                               #Vioso节点
    MpcdiNode                               #MPCDI 变形融合
    PosiStageNetClient                      #PosiStageNet数据接收
    ISFNode                                 #ISF数据加载
    GDTFNode                                #GDTF灯具文件解析

)

# 根据选项决定构建方式（2026-11-13 CMake 重整：两种模式共用 ALL_NODES 单一清单，
# EXISTS 检查兜底目录缺失——清单与磁盘目录永远一致，只维护一处）。
# 旧 BUILD_NODES_INDIVIDUALLY 分支的手写 add_subdirectory 列表已删——其中含
# 已不存在目录的死引用（CurveNode/VLC_RemoteNode）、showStoreGBxNode 重复
# 条目，且长期缺新节点（AudioAnalysisNode、MediaPipe/GestureNode 等）。
if(BUILD_ALL_NODES OR BUILD_NODES_INDIVIDUALLY)
    if(BUILD_ALL_NODES)
        set(NODE_BUILD_MODE "All Nodes")
    else()
        set(NODE_BUILD_MODE "Individually (shared ALL_NODES list)")
    endif()
    foreach(NODE_NAME ${ALL_NODES})
        # 检查节点目录是否存在（缺失只告警，不中断 configure）
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${NODE_NAME}")
            add_subdirectory(${NODE_NAME})
        else()
            message(WARNING "Node directory not found: ${NODE_NAME}")
        endif()
    endforeach()
else()
    message(STATUS "No nodes will be built. Enable BUILD_ALL_NODES or BUILD_NODES_INDIVIDUALLY.")
endif()

# 可单独构建全部节点的聚合目标（不依赖选项）
add_custom_target(BuildAllNodes)
foreach(NODE_NAME ${ALL_NODES})
    get_filename_component(NODE_TARGET ${NODE_NAME} NAME)
    if (TARGET ${NODE_TARGET})
        add_dependencies(BuildAllNodes ${NODE_TARGET})
    endif()
endforeach()

# 打印构建信息
if(BUILD_ALL_NODES OR BUILD_NODES_INDIVIDUALLY)
    list(LENGTH ALL_NODES NODE_COUNT)
    message(STATUS "=== Node Build Configuration ===")
    message(STATUS "Build Mode: ${NODE_BUILD_MODE}")
    message(STATUS "Total Nodes: ${NODE_COUNT}")
    message(STATUS "Output Directory: ${PLUGIN_OUTPUT_DIRECTORY}")
    message(STATUS "================================")
endif()
