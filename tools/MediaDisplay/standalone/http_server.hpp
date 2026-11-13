// ============================================================================
//  http_server —— standalone HTTP 控制面骨架（收编自 MMC MPV_Media_Console_PC，
//  2026-11-11 display×MMC 合并 M2；原文件 Poco 生命周期/静态托管/CORS 语义
//  原样保留，AppContext 依赖替换为 StandaloneApi，路由组收拢为单入口）。
//
//  职责：OPTIONS 预检、/api/v1/* 分发给 StandaloneApi（全落空回 404）、
//  web 目录静态文件托管、服务器起停。业务路由见 standalone_api.cpp。
// ============================================================================

#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace media_api {

class StandaloneApi;

class HttpServer {
public:
    explicit HttpServer(StandaloneApi& api);
    ~HttpServer();

    bool start(const std::string& bind, int port, std::string& err);
    void stop();

    /// 实际绑定端口（start 后有效；0 = 未启动——端口 0 时系统自动分配，
    /// 测试经此拿真实端口）
    int boundPort() const;

private:
    class Impl;
    StandaloneApi& api_;
    std::unique_ptr<Impl> impl_;
    std::atomic<bool> running_{false};
};

}  // namespace media_api
