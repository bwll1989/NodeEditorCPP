#pragma once

#include "TimestampGenerator.hpp"

/**
 * @brief 当前线程注册 Windows MMCSS（Pro Audio），析构时撤销。
 * 非 Windows 为空操作。供时钟线程与音频处理线程在入口处使用。
 */
class TIMESTAMPGENERATOR AudioThreadRealtimeGuard
{
public:
    explicit AudioThreadRealtimeGuard(const wchar_t *taskName = L"Pro Audio");
    ~AudioThreadRealtimeGuard();

    AudioThreadRealtimeGuard(const AudioThreadRealtimeGuard &) = delete;
    AudioThreadRealtimeGuard &operator=(const AudioThreadRealtimeGuard &) = delete;

    bool isActive() const { return handle_ != nullptr; }

private:
    void *handle_ = nullptr; // HANDLE，避免头文件依赖 windows.h
};
