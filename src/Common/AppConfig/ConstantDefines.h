//
// Created by WuBin on 2025/10/28.
//
#pragma once
// #include <QDesktopServices>
#include <QStandardPaths>
// 版本宏的默认值；若构建系统已注入，则使用注入值
#ifndef PRODUCT_VERSION
#define PRODUCT_VERSION "dev"  // 构建系统注入时会覆盖
#endif
// 应用程序常量定义
namespace AppConstants {
    // 产品信息
    const QString COMPANY_NAME = "WuBin";
    const QString PRODUCT_NAME = "Flow";
    const QString PRODUCT_RUNTIME_NAME = "FlowRuntime";
    const QString FILE_DESCRIPTION = "Flow based on Qt 6.10.1 (MSVC, 64 bit)";
    const QString LEGAL_COPYRIGHT = "Copyright 2008-2025 WuBin. All rights reserved.";
    // 日志存储目录
    const QString LOGS_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/Flow/Logs";
    // 媒体库存储目录
    const QString MEDIA_LIBRARY_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/Flow/Medias";
    // FLOW文件存储目录
    const QString MEDIA_LIBRARY_FLOW_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/Flow/Flows";
    // 最近打开文件存储路径
    const QString RECENT_FILES_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/Flow/Cfg";
    // Art-Net 相关常量
    constexpr int ARTNET_PORT = 6454;
    // Art-Net 输出FPS
    constexpr double ARTNET_OUTPUT_FPS = 50.0; // 20ms
    // 深色主题样式表
    const QString DARK_STYLESHEET = ":/styles/styles/DefaultDark.qss";
    // 浅色主题样式表
    const QString LIGHT_STYLESHEET = ":/styles/styles/DefaultLight.qss";

}
namespace AppConfigs {
    // 日志显示最多条目数
    constexpr int MAX_LOG_ENTRIES = 200;
    // 保存最近打开文件最大数量
    constexpr int MAX_RECENT_FILES = 10 ;
    // OSC 外部反馈主机，即OSC向外发送主机地址，默认值为广播地址
    constexpr const char* EXTRA_FEEDBACK_HOST = "255.255.255.255";
    // 软件内OSC控制主机，默认值为本地回环地址
    constexpr const char* OSC_INTERNAL_CONTROL_HOST = "127.0.0.1";
    // OSC外部反馈端口号,即使用这个端口向外发送数据
    constexpr int EXTRA_FEEDBACK_PORT = 8990;
    // OSC外部控制端口号,即使用这个端口向内接收数据
    constexpr int EXTRA_CONTROL_PORT = 8991;
    // HTTP&WebSocket 服务器端口号
    constexpr int HTTP_SERVER_PORT = 8992;
    // 使用暗色主题
    constexpr bool DEFAULT_DARK_THEME = true;
    // 网页访问密码
    constexpr const char* WEB_ACCESS_PASSWORD = "wubin@niubility";
    // 是否启用OSC外部反馈/控制
    constexpr bool OSC_ENABLED = false;
    // 是否启用MQTT外部反馈/控制
    constexpr bool MQTT_ENABLED = false;
    // MQTT Broker 地址
    constexpr const char* MQTT_HOST = "127.0.0.1";
    // MQTT Broker 端口
    constexpr int MQTT_PORT = 1883;
    // MQTT 控制主题（订阅，接收外部控制指令）
    constexpr const char* MQTT_CONTROL_TOPIC = "flow/control";
    // MQTT 反馈主题（发布，向外推送状态）
    constexpr const char* MQTT_FEEDBACK_TOPIC = "flow/feedback";

}