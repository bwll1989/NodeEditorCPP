#include "LibretroCore.h"

#include <windows.h>

#include <cstdio>
#include <cstdarg>

namespace Nodes
{

LibretroCore* LibretroCore::s_active = nullptr;

void LibretroCore::VoidDeleter::operator()(void* p) const
{
    if (p) {
        FreeLibrary(static_cast<HMODULE>(p));
    }
}

template <typename T>
T LibretroCore::sym(const char* name, std::string& error, bool required)
{
    FARPROC p = GetProcAddress(static_cast<HMODULE>(m_lib), name);
    if (!p && required && error.empty()) {
        error = std::string("missing export: ") + name;
    }
    return reinterpret_cast<T>(p);
}

bool LibretroCore::load(const std::wstring& dllPath, std::string& error)
{
    if (m_lib) {
        unload();
    }

    // 单活动实例约束：core DLL 的内部状态（retro_init 静态数据）是 per-DLL 的，
    // 两个实例共享同一 DLL 会互相踩状态；且任一实例 FreeLibrary 会牵动另一方
    if (s_active && s_active != this) {
        error = "another Libretro node is running (one core instance at a time)";
        return false;
    }

    HMODULE lib = LoadLibraryW(dllPath.c_str());
    if (!lib) {
        error = "LoadLibrary failed, GetLastError=" + std::to_string(GetLastError());
        return false;
    }
    m_lib = lib;

#define LR_GET(ret, name, args) m_##name = sym<ret(*) args>(#name, error);
    LR_FOREACH_SYMF(LR_GET)
#undef LR_GET

    if (!error.empty()) {
        FreeLibrary(lib);
        m_lib = nullptr;
        // 清掉半初始化的函数指针
#define LR_CLEAR(ret, name, args) m_##name = nullptr;
        LR_FOREACH_SYMF(LR_CLEAR)
#undef LR_CLEAR
        return false;
    }
    m_symbolsResolved = true;

    if (m_retro_api_version() != RETRO_API_VERSION) {
        error = "retro_api_version mismatch";
        FreeLibrary(static_cast<HMODULE>(m_lib));
        m_lib = nullptr;
        return false;
    }

    // 收集 core 元信息（不依赖 game）
    retro_system_info si{};
    m_retro_get_system_info(&si);
    m_libName = si.library_name ? si.library_name : "unknown";
    m_libVersion = si.library_version ? si.library_version : "";

    s_active = this;
    return true;
}

bool LibretroCore::loadGame(const std::string& romPathUtf8, const void* data,
                            size_t size, std::string& error)
{
    if (!m_symbolsResolved) {
        error = "core not loaded";
        return false;
    }

    // 注册回调（set_environment 必须最先）
    m_retro_set_environment(&LibretroCore::staticEnvironment);
    m_retro_set_video_refresh(&LibretroCore::staticVideoRefresh);
    m_retro_set_audio_sample(&LibretroCore::staticAudioSample);
    m_retro_set_audio_sample_batch(&LibretroCore::staticAudioBatch);
    m_retro_set_input_poll(&LibretroCore::staticInputPoll);
    m_retro_set_input_state(&LibretroCore::staticInputState);

    if (!m_coreInited) {
        m_retro_init();
        m_coreInited = true;
    }

    retro_game_info info{};
    info.path = romPathUtf8.c_str();
    info.data = data;
    info.size = size;
    info.meta = nullptr;

    if (!m_retro_load_game(&info)) {
        error = "retro_load_game failed (unsupported content?)";
        return false;
    }
    m_gameLoaded = true;

    m_retro_get_system_av_info(&m_avInfo);
    m_retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    return true;
}

void LibretroCore::unload()
{
    if (!m_lib) {
        s_active = (s_active == this) ? nullptr : s_active;
        return;
    }

    if (m_gameLoaded) {
        m_retro_unload_game();
        m_gameLoaded = false;
    }
    if (m_coreInited) {
        m_retro_deinit();
        m_coreInited = false;
    }

    FreeLibrary(static_cast<HMODULE>(m_lib));
    m_lib = nullptr;
    m_symbolsResolved = false;
#define LR_CLEAR2(ret, name, args) m_##name = nullptr;
    LR_FOREACH_SYMF(LR_CLEAR2)
#undef LR_CLEAR2
    m_avInfo = {};
    s_active = (s_active == this) ? nullptr : s_active;
}

void LibretroCore::runFrame(uint16_t joypadMask)
{
    if (!m_gameLoaded) {
        return;
    }
    m_inputMask = joypadMask;
    m_retro_run();
}

size_t LibretroCore::serializeSize()
{
    return m_gameLoaded ? m_retro_serialize_size() : 0;
}

bool LibretroCore::serialize(std::vector<uint8_t>& out)
{
    if (!m_gameLoaded) {
        return false;
    }
    out.resize(m_retro_serialize_size());
    if (out.empty()) {
        return false;
    }
    return m_retro_serialize(out.data(), out.size());
}

bool LibretroCore::unserialize(const std::vector<uint8_t>& in)
{
    if (!m_gameLoaded || in.empty()) {
        return false;
    }
    return m_retro_unserialize(in.data(), in.size());
}

void LibretroCore::resetGame()
{
    if (m_gameLoaded) {
        m_retro_reset();
    }
}

// ---------------- 静态回调转发 ----------------

bool LibretroCore::staticEnvironment(unsigned cmd, void* data)
{
    return s_active ? s_active->handleEnvironment(cmd, data) : false;
}

void LibretroCore::staticVideoRefresh(const void* data, unsigned w, unsigned h, size_t pitch)
{
    if (!s_active || !s_active->onVideo) {
        return;
    }
    s_active->onVideo(data, w, h, pitch, s_active->m_pixelFormat);
}

size_t LibretroCore::staticAudioBatch(const int16_t* data, size_t frames)
{
    if (s_active && s_active->onAudio) {
        s_active->onAudio(data, frames);
    }
    return frames; // 告知 core 全部消费
}

void LibretroCore::staticAudioSample(int16_t, int16_t)
{
    // batch 模式已注册，单采样回调不会用到
}

void LibretroCore::staticInputPoll()
{
    // 输入是拉模式（input_state 查询），无需实现
}

int16_t LibretroCore::staticInputState(unsigned port, unsigned device, unsigned index, unsigned id)
{
    return s_active ? s_active->handleInputState(port, device, index, id) : 0;
}

void LibretroCore::staticLog(enum retro_log_level level, const char* fmt, ...)
{
    if (!s_active) {
        return;
    }
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (level >= RETRO_LOG_ERROR && s_active->onErrorLog) {
        s_active->onErrorLog(std::string(buf));
    }
}

bool LibretroCore::handleEnvironment(unsigned cmd, void* data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *static_cast<bool*>(data) = true;
        return true;

    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        if (m_sysDir.empty()) {
            return false;
        }
        *static_cast<const char**>(data) = m_sysDir.c_str();
        return true;

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
        const int fmt = *static_cast<const int*>(data);
        // 支持 XRGB8888 / RGB565；0RGB1555 是历史遗留默认值，直接拒绝
        if (fmt == RETRO_PIXEL_FORMAT_XRGB8888 || fmt == RETRO_PIXEL_FORMAT_RGB565) {
            m_pixelFormat = fmt;
            return true;
        }
        return false;
    }

    case RETRO_ENVIRONMENT_GET_VARIABLE:
        return false; // 全部用 core 默认值

    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *static_cast<bool*>(data) = false;
        return true;

    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        // 必须填有效指针，否则 core 调空指针崩溃
        static_cast<struct retro_log_callback*>(data)->log = &LibretroCore::staticLog;
        return true;

    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        return true;

    default:
        return false;
    }
}

int16_t LibretroCore::handleInputState(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port != 0 || device != RETRO_DEVICE_JOYPAD || index != 0) {
        return 0;
    }
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
        return static_cast<int16_t>(m_inputMask);
    }
    if (id > 15) {
        return 0;
    }
    return (m_inputMask >> id) & 1;
}

} // namespace Nodes
