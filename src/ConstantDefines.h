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
    // OSC 外部反馈主机
    constexpr const char* EXTRA_FEEDBACK_HOST = "127.0.0.1";
    // OSC外部反馈端口号
    constexpr int EXTRA_FEEDBACK_PORT = 8990;
    // OSC外部控制端口号
    constexpr int EXTRA_CONTROL_PORT = 8991;
    // OSC Web 端口号
    constexpr int OSC_WEB_PORT = 8992;
    // 产品信息

    const QString COMPANY_NAME = "Bwll1989";
    const QString PRODUCT_NAME = "NodeStudio";
    const QString LUA_PATH = "./lua;";
    const QString FILE_DESCRIPTION = "Qt Creator based on Qt 6.5 (MinGw, 64 bit)";
    const QString LEGAL_COPYRIGHT = "Copyright 2008-2025 The Qt Company Ltd. All rights reserved.";
    // 初始化完成的状态消息常量，避免魔法字符串
    const QString INIT_SUCCESS_MESSAGE = "Initialization nodes library success";

    const QString LOGS_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NodeStudio/Logs";
    const QString MEDIA_LIBRARY_STORAGE_DIR = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/NodeStudio/Medias";


}
