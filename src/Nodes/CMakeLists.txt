cmake_minimum_required(VERSION 3.10)
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins/DataFlow)

# 添加构建选项
option(BUILD_ALL_NODES "Build all node plugins at once" ON)
option(BUILD_NODES_INDIVIDUALLY "Build nodes individually (default behavior)" ON)

# 定义所有节点列表
set(ALL_NODES
    VST3PluginNode
    DataInfoNode                              #数据显示
    ImageLoadNode                             #图像导入
#    LuaScriptNode                             #Lua脚本模块(弃用）
    TCPServerNode                             #TCP服务器模块
    TCPClientNode                             #TCP客户端模块
    TimeLineNode                              #时间轴模块
    UDPSocketNode                             #UDP模块
    DelayNode                                 #延迟模块
    LogicOperationNode                        #逻辑操作模块
    MathOperationNode                         #数学操作模块
    LFONode                                   #LFO模块
    CurveNode                                 #曲线模块
    AudioDecoderNode                          #音频解码模块
    AudioDeviceOutNode                        #音频输出模块
    HotKeyNode                                #热键模块
    ExtractNode                               #数据分离模块
    ImageInfoNode                             #图像信息
    ImageScaleNode                            #图像缩放
    CameraNode                                #摄像头输入
    CaptureNode                               #图像捕获
    CutImageNode                              #图像切割
    ImageConstNode                            #图像常量
    CountNode                                 #条件计数
    RectNode                                  #矩形模块
    SizeVarNode                               #尺寸模块
    MergeNode                                 #数据合并
    OscInNode                                 #OSC输入
    OscOutNode                                #OSC输出
    ArtnetInNode                              #Arnet输入
    MpvControllerNode                         #mpv控制
    SwitchNode                                #输入切换
    NDVServerNode                             #NDV控制
    ConditionNode                             #条件判断
    WebSocketServerNode                       #WebSocket服务器
    WebSocketClientNode                       #WebSocket客户端
    JavaScriptNode                            #JavaScript脚本
    InternalControlNode                       #内部控制
    ImageThresholdNode                        #图像二值化
    ImageCompareNode                          #图像对比
    ImageSwitchNode                           #图像切换
    showStoreGBxNode                          #ELC showStoreGBx 控制器
    USR-IO808Node                             #基于Modbus Master实现与USR-IO808 控制器对接
    TSETLNode                                 #对接TSETL
    HoldNode                                  #信号保持
    SpoutInNode                               #Spout输入
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
    LTCDecoderNode                              #LTC时间码解析节点
    AudioMatrixNode                             #音频矩阵节点
    AudioAnalysisNode                           #音频分析节点
    PythonScriptNode                            #python脚本节点
    YoloPoseONNXNode                            #Yolo v11n 姿态
    YoloDetectionONNXNode                       #Yolo v11n 检测
    StyleTransferONNXNode                       #神经网络风格迁移
    DataVisualNode                              #数据可视化
#    AudioIntentNode                              #音频意图分析（弃用）
)

# 根据选项决定构建方式
if(BUILD_ALL_NODES)
    message(STATUS "Building all nodes at once...")
    
    # 批量添加所有节点子目录
    foreach(NODE_NAME ${ALL_NODES})
        # 检查节点目录是否存在
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${NODE_NAME}")
            add_subdirectory(${NODE_NAME})
            message(STATUS "Added node: ${NODE_NAME}")
        else()
            message(WARNING "Node directory not found: ${NODE_NAME}")
        endif()
    endforeach()
    
elseif(BUILD_NODES_INDIVIDUALLY)
    message(STATUS "Building nodes individually (default behavior)...")
    
    # 原有的单独添加方式
    add_subdirectory(VST3PluginNode)
    add_subdirectory(DataInfoNode)                              #数据显示
    add_subdirectory(ImageLoadNode)                             #图像导入
#    add_subdirectory(LuaScriptNode)                             #Lua脚本模块（弃用）
    add_subdirectory(TCPServerNode)                             #TCP服务器模块
    add_subdirectory(TCPClientNode)                             #TCP客户端模块
    add_subdirectory(TimeLineNode)                              #时间轴模块
    add_subdirectory(UDPSocketNode)                             #UDP模块
    add_subdirectory(DelayNode)                                 #延迟模块
    add_subdirectory(LogicOperationNode)                        #逻辑操作模块
    add_subdirectory(MathOperationNode)                         #数学操作模块
    add_subdirectory(LFONode)                                   #LFO模块
    add_subdirectory(CurveNode)                                   #QML模块
    add_subdirectory(AudioDecoderNode)                          #音频解码模块
    add_subdirectory(AudioDeviceOutNode)                        #音频输出模块
    add_subdirectory(HotKeyNode)                                #热键模块
    add_subdirectory(ExtractNode)                               #数据分离模块
    add_subdirectory(ImageInfoNode)                             #图像信息
    add_subdirectory(ImageScaleNode)                            #图像缩放
    add_subdirectory(CameraNode)                                #摄像头输入
    add_subdirectory(CaptureNode)                               #图像捕获
    add_subdirectory(CutImageNode)                              #图像切割
    add_subdirectory(ImageConstNode)                            #图像常量
    add_subdirectory(CountNode)                                 #条件计数
    add_subdirectory(RectNode)                                  #矩形模块
    add_subdirectory(SizeVarNode)                               #尺寸模块
    add_subdirectory(MergeNode)                                 #数据合并
    add_subdirectory(OscInNode)                                 #OSC输入
    add_subdirectory(OscOutNode)                                #OSC输出
    add_subdirectory(ArtnetInNode)                              #Arnet输入
    add_subdirectory(MpvControllerNode)                         #mpv控制
    add_subdirectory(SwitchNode)                                #输入切换
    add_subdirectory(NDVServerNode)                            #NDV控制
    add_subdirectory(ConditionNode)                             #条件判断
    add_subdirectory(WebSocketServerNode)                       #WebSocket服务器
    add_subdirectory(WebSocketClientNode)                       #WebSocket客户端
    add_subdirectory(JavaScriptNode)                            #JavaScript脚本
    add_subdirectory(InternalControlNode)                       #内部控制
    add_subdirectory(ImageThresholdNode)                        #图像二值化
    add_subdirectory(ImageCompareNode)                          #图像对比
    add_subdirectory(ImageSwitchNode)                           #图像切换
    add_subdirectory(showStoreGBxNode)                          #ELC showStoreGBx 控制器
    add_subdirectory(USR-IO808Node)                             #基于Modbus Master实现与USR-IO808 控制器对接
    add_subdirectory(TSETLNode)                                 #对接TSETL
    add_subdirectory(HoldNode)                                  #信号保持
    add_subdirectory(SpoutInNode)                               #Spout输入
    add_subdirectory(NDVPlayerNode)                             #NDV player
    add_subdirectory(DMXDeviceNode)                             #DMX 设备
    add_subdirectory(DMXUniverseNode)                           #DMX Universe 数据
    add_subdirectory(ArtnetOutNode)                             #Artnet输出
    add_subdirectory(UniversePlaybackNode)                       #DMX Universe 数据回放
    add_subdirectory(CustomScriptNode)                          #自定义js节点
    add_subdirectory(AudioDeviceInNode)                         #音频输入节点
    add_subdirectory(NoiseGeneratorNode)                        #噪声生成器节点
    add_subdirectory(NDIInNode)                                 #NDI输入节点
    add_subdirectory(NDIOutNode)                                #NDI输出节点
    add_subdirectory(PJLinkNode)                                #PJLink输出节点
    add_subdirectory(LTCDecoderNode)                            #LTC时间码解析节点
    add_subdirectory(AudioMatrixNode)                           #音频矩阵节点
    add_subdirectory(DataVisualNode)                            #数据可视化节点
#    add_subdirectory(AudioIntentNode)                            #音频意图分析（弃用）
else()
    message(STATUS "No nodes will be built. Enable BUILD_ALL_NODES or BUILD_NODES_INDIVIDUALLY.")
endif()

# 打印构建信息
if(BUILD_ALL_NODES)
    message(STATUS "=== Node Build Configuration ===")
    message(STATUS "Build Mode: All Nodes")
    message(STATUS "Total Nodes: ${CMAKE_LIST_LENGTH}")
    message(STATUS "Output Directory: ${PLUGIN_OUTPUT_DIRECTORY}")
    message(STATUS "================================")
endif()