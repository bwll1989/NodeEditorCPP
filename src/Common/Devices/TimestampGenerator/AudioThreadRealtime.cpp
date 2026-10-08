#include "AudioThreadRealtime.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <avrt.h>
#endif

AudioThreadRealtimeGuard::AudioThreadRealtimeGuard(const wchar_t *taskName)
{
#ifdef _WIN32
    if (!taskName) {
        taskName = L"Pro Audio";
    }
    DWORD taskIndex = 0;
    HANDLE h = AvSetMmThreadCharacteristicsW(taskName, &taskIndex);
    if (h && h != INVALID_HANDLE_VALUE) {
        // 在 MMCSS 任务内再抬一档，有利于音频截止期
        AvSetMmThreadPriority(h, AVRT_PRIORITY_HIGH);
        handle_ = h;
    }
#else
    (void)taskName;
#endif
}

AudioThreadRealtimeGuard::~AudioThreadRealtimeGuard()
{
#ifdef _WIN32
    if (handle_) {
        AvRevertMmThreadCharacteristics(static_cast<HANDLE>(handle_));
        handle_ = nullptr;
    }
#endif
}
