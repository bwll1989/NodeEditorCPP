//
// Created by WuBin on 2025/10/28.
//
#pragma once
// 应用程序常量定义
namespace AppConstants {
    // 外部控制端口号
    constexpr int EXTRA_CONTROL_PORT = 8991;
    constexpr int OSC_WEB_PORT = 8992;
    // 产品信息
    const QString PRODUCT_VERSION = "1.4";
    const QString COMPANY_NAME = "Bwll1989";
    const QString PRODUCT_NAME = "NodeStudio";
    const QString LUA_PATH = "./lua;";
    const QString FILE_DESCRIPTION = "Qt Creator based on Qt 6.5 (MinGw, 64 bit)";
    const QString LEGAL_COPYRIGHT = "Copyright 2008-2025 The Qt Company Ltd. All rights reserved.";
    // 初始化完成的状态消息常量，避免魔法字符串
    const QString INIT_SUCCESS_MESSAGE = "Initialization nodes library success";

}
