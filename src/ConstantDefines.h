//
// Created by WuBin on 2025/10/28.
//
#pragma once
#include <QDesktopServices>
#include <QStandardPaths>
// 版本宏的默认值；若构建系统已注入，则使用注入值
#ifndef PRODUCT_VERSION
#define PRODUCT_VERSION "dev"  // 构建系统注入时会覆盖
#endif
// 应用程序常量定义
namespace AppConstants {
    // OSC 外部反馈主机，即OSC向外发送主机地址，默认值为本地回环地址
    constexpr const char* EXTRA_FEEDBACK_HOST = "127.0.0.1";
    // 软件内OSC控制主机，默认值为本地回环地址
    constexpr const char* OSC_INTERNAL_CONTROL_HOST = "127.0.0.1";
    // OSC外部反馈端口号
    constexpr int EXTRA_FEEDBACK_PORT = 8990;
    // OSC外部控制端口号
    constexpr int EXTRA_CONTROL_PORT = 8991;
    // OSC Web 端口号
    constexpr int OSC_WEB_PORT = 8992;

    // 产品信息

    const QString COMPANY_NAME = "WuBin";
    const QString PRODUCT_NAME = "NodeStudio";
    const QString LUA_PATH = "./lua;";
    const QString FILE_DESCRIPTION = "NodeStudio based on Qt 6.10.1 (MSVC, 64 bit)";
    const QString LEGAL_COPYRIGHT = "Copyright 2008-2025 The Qt Company Ltd. All rights reserved.";
    // 日志存储目录
    const QString LOGS_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NodeStudio/Logs";
    // 日志最多保留数量
    constexpr int MaxLogEntries = 200;
    // 媒体库存储目录
    const QString MEDIA_LIBRARY_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NodeStudio/Medias";
    // 保存最近打开文件最大数量
    constexpr int MaxRecentFiles = 5 ;
    // 最近打开文件存储路径
    const QString RECENT_FILES_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NodeStudio/Cfg";

    // Art-Net 相关常量
    constexpr int ARTNET_PORT = 6454;
    // Art-Net 输出FPS
    constexpr double ARTNET_OUTPUT_FPS = 50.0; // 25ms

}