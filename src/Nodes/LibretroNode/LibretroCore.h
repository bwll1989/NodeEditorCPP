#pragma once

/**
 * @file LibretroCore.h
 * @brief libretro 前端最小封装 — 纯 C++/Win32，不依赖 Qt
 *
 * 职责：LoadLibrary 加载 core DLL → 解析 retro_* 函数指针 →
 *       实现前端侧四个回调（environment / video_refresh / audio_sample_batch /
 *       input_state）→ 暴露 loadGame / runFrame / serialize 等驱动接口。
 *
 * 线程约定：load/loadGame/runFrame/serialize/unserialize/unload 必须都在
 * 同一个线程（emulation 线程）调用；回调也在该线程内同步触发。
 * 输入采用帧级位图（joypadMask），由调用方每帧传入，天然线程安全。
 *
 * 许可说明：libretro API（libretro.h）为 MIT；core DLL 许可各不相同
 * （mGBA = MPL-2.0，可商用），分发时需核对所带 core 的许可证。
 */

#include "libretro.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Nodes
{

class LibretroCore
{
public:
    LibretroCore() = default;
    ~LibretroCore() { unload(); }

    LibretroCore(const LibretroCore&) = delete;
    LibretroCore& operator=(const LibretroCore&) = delete;

    /** @brief 加载 core DLL 并解析全部需要的符号。失败时返回 false 并填充 error。 */
    bool load(const std::wstring& dllPath, std::string& error);

    /**
     * @brief 初始化 core 并加载 ROM（内存数据 + 路径）。
     * @param romPathUtf8 ROM 路径（供 core 记录/派生其它路径）
     * @param data        ROM 内存镜像（可为 nullptr，此时 core 走 need_fullpath）
     * @note  成功后可读取 fps()/sampleRate()/baseWidth()/baseHeight()。
     */
    bool loadGame(const std::string& romPathUtf8, const void* data, size_t size,
                  std::string& error);

    /** @brief 卸载 ROM + 反初始化 + 释放 DLL。可重复调用。 */
    void unload();

    bool isGameLoaded() const { return m_gameLoaded; }
    bool isDllLoaded() const { return m_lib != nullptr; }

    /** @brief 驱动一帧。mask 位 i = RETRO_DEVICE_ID_JOYPAD_x 按下。 */
    void runFrame(uint16_t joypadMask);

    // ---- AV 信息（loadGame 成功后有效）----
    double fps() const { return m_avInfo.timing.fps; }
    double sampleRate() const { return m_avInfo.timing.sample_rate; }
    unsigned baseWidth() const { return m_avInfo.geometry.base_width; }
    unsigned baseHeight() const { return m_avInfo.geometry.base_height; }
    const std::string& libraryName() const { return m_libName; }
    const std::string& libraryVersion() const { return m_libVersion; }
    int pixelFormat() const { return m_pixelFormat; }

    // ---- 存档点 ----
    size_t serializeSize();
    bool serialize(std::vector<uint8_t>& out);
    bool unserialize(const std::vector<uint8_t>& in);
    void resetGame();

    /** @brief system/save 目录（loadGame 前设置；SRAM/存档落在该目录） */
    void setSystemDirectory(const std::string& utf8Path) { m_sysDir = utf8Path; }

    // ---- 回调槽（emulation 线程内同步调用）----
    /// pixels: RGB565 或 XRGB8888（见 pixelFormat()），按 pitch 步进
    std::function<void(const void* pixels, unsigned w, unsigned h, unsigned pitch, int pixFmt)> onVideo;
    /// 有符号 16bit 交织立体声，frames = 每声道采样帧数
    std::function<void(const int16_t* samples, size_t frames)> onAudio;
    /// 捕获 core 的 error 级日志（log interface）
    std::function<void(const std::string&)> onErrorLog;

private:
    // ---- libretro 静态回调：转发到当前活动实例 ----
    static bool staticEnvironment(unsigned cmd, void* data);
    static void staticVideoRefresh(const void* data, unsigned w, unsigned h, size_t pitch);
    static size_t staticAudioBatch(const int16_t* data, size_t frames);
    static void staticAudioSample(int16_t left, int16_t right);
    static void staticInputPoll();
    static int16_t staticInputState(unsigned port, unsigned device, unsigned index, unsigned id);
    static void staticLog(enum retro_log_level level, const char* fmt, ...);

    bool handleEnvironment(unsigned cmd, void* data);
    int16_t handleInputState(unsigned port, unsigned device, unsigned index, unsigned id);

    template <typename T>
    T sym(const char* name, std::string& error, bool required = true);

private:
    struct VoidDeleter { void operator()(void* p) const; };
    void* m_lib = nullptr;                 // HMODULE
    bool m_symbolsResolved = false;
    bool m_coreInited = false;
    bool m_gameLoaded = false;

    std::string m_libName = "unknown";
    std::string m_libVersion;
    std::string m_sysDir;
    int m_pixelFormat = RETRO_PIXEL_FORMAT_0RGB1555;

    retro_system_av_info m_avInfo{};
    uint16_t m_inputMask = 0;              // runFrame 每帧更新，回调同线程读取

    static LibretroCore* s_active;         // 回调无用户数据，用单活动实例转发

// ---- retro_* 函数指针 ----
#define LR_FOREACH_SYMF(F)                                                        \
    F(void, retro_init, (void))                                                   \
    F(void, retro_deinit, (void))                                                 \
    F(unsigned, retro_api_version, (void))                                        \
    F(void, retro_set_environment, (retro_environment_t))                         \
    F(void, retro_set_video_refresh, (retro_video_refresh_t))                     \
    F(void, retro_set_audio_sample, (retro_audio_sample_t))                       \
    F(void, retro_set_audio_sample_batch, (retro_audio_sample_batch_t))           \
    F(void, retro_set_input_poll, (retro_input_poll_t))                           \
    F(void, retro_set_input_state, (retro_input_state_t))                         \
    F(void, retro_get_system_info, (struct retro_system_info*))                   \
    F(void, retro_get_system_av_info, (struct retro_system_av_info*))             \
    F(void, retro_set_controller_port_device, (unsigned, unsigned))               \
    F(void, retro_reset, (void))                                                  \
    F(void, retro_run, (void))                                                    \
    F(size_t, retro_serialize_size, (void))                                       \
    F(bool, retro_serialize, (void*, size_t))                                     \
    F(bool, retro_unserialize, (const void*, size_t))                             \
    F(bool, retro_load_game, (const struct retro_game_info*))                     \
    F(void, retro_unload_game, (void))                                            \
    F(unsigned, retro_get_region, (void))

#define LR_DECL(ret, name, args) ret(*m_##name) args = nullptr;
    LR_FOREACH_SYMF(LR_DECL)
#undef LR_DECL
};

} // namespace Nodes
