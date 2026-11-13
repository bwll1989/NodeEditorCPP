// ============================================================================
//  AudioCasting.hpp —— 音源选配（编辑器预览拍与 display 渲染拍的共用出口）
//
//  设计依据（2026-11-10 收口）：两端此前各持一份 ~40 行同构的"活跃片段
//  筛选 → 音源池化 → AudioVoice 组装"逻辑——语义漂移温床（A/V 同步的
//  锚点逻辑必须单源），收敛为本头单一实现。
//
//  选源规则（2026-11-11 用户定案：无音轨 = 无声）：
//    * 真实素材音频优先（AudioMediaSource，池化跨拍复用——解码器开启/
//      线程启动是重操作，每片段仅构建一次）；
//    * 素材无音轨/打开失败（文件未到达）→ 不建 voice（无声）；文件落盘
//      后下拍自动接管（与视频"prepare 每帧重试"自愈语义同源）；
//    * MS_AUDIO_SINE=1 强制正弦（输出链路诊断用，频率 = 220 + hash%661）。
//    【退役记录】原"无音轨 → 正弦测试音回退"是 2026-11-10 音频验证期
//    的临时设计——真实使用中被用户判为缺陷（无音轨素材响测试音）。
//
//  【池条目指纹铁律】池键 = clip id，但源内容取决于素材路径——片段换绑
//  素材（同 clip id 指向新 asset）时旧源不得命中：条目（AudioPoolEntry，
//  定义在 AudioTypes.hpp）记录构建时的素材路径，不匹配即重建（否则
//  声音错素材）。
// ============================================================================

#pragma once

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "audio/AudioTypes.hpp"       // IAudioSource/AudioPoolEntry
#include "audio/AudioMediaSource.hpp"
#include "audio/SineAudioSource.hpp"
#include "engine/TimelineEngine.hpp" // isActive（与视频取帧同一活跃判定）
#include "engine/TweenEngine.hpp"    // resolveGain（增益解算一个出口）
#include "model/SceneModel.hpp"      // Scene/Clip/Asset

namespace ms::audio {

// 池超限兜底阈值：异常场景（clip id 爆量）下丢弃重建，防无界增长
inline constexpr std::size_t kAudioPoolLimit = 64;

// ---------------------------------------------------------------------------
// buildVoices：收集活跃音频片段 → 池化选源 → 组装混音输入清单。
//   \param t_us     播放头（会话时间；非 Playing 时调用方直接清空 out）
//   \param assets_root 素材根目录（透传 AudioMediaSource）
//   \param pool     跨拍音源池（clip id → 条目；调用方持有）
//   \param out      本拍混音输入（调用方复用缓冲；先清空）
// 【池清空铁律】池超限清空必须在取声源指针之前——unique_ptr 清空会使
// 早先 voices 里的裸指针悬垂（本拍内只允许一次、且在循环外）。
//
// 【无音轨 = 无声（2026-11-11 用户定案）】素材无音轨/打开失败（文件未
//   到达/坏文件）→ 不建 voice（片段无声），正弦测试音回退退役——用户
//   明确"素材无音频时应该是无音频输出"，测试音在真实使用中是噪音。
//   自愈语义与视频一致：文件落盘后下一拍打开成功入池，声音自动出现。
//   MS_AUDIO_SINE=1 诊断开关保留（强制全部正弦，输出链路验证用）。
// ---------------------------------------------------------------------------
inline void buildVoices(const Scene& scene, std::int64_t t_us,
                        const std::string& assets_root,
                        std::map<std::string, AudioPoolEntry>& pool,
                        std::vector<AudioVoice>& out) {
    if (pool.size() > kAudioPoolLimit) pool.clear();
    out.clear();
    static const bool kForceSine = [] {
        const char* e = std::getenv("MS_AUDIO_SINE");
        return e && std::string_view(e) == "1";
    }();
    for (const auto& track : scene.timeline.tracks)
        for (const auto& c : track.clips) {
            if (!TimelineEngine::isActive(c, t_us)) continue;
            // 素材路径指纹（空 = 未绑定素材/强制正弦诊断）——池命中还须
            // 路径一致（换绑素材旧源不得命中）
            std::string asset_path;
            if (!kForceSine) {
                const Asset* a = scene.findAsset(c.asset_id);
                if (a && !a->path.empty()) {
                    asset_path = a->path;
                } else if (!a && !c.asset_id.empty()) {
                    // 【无声定位器 2026-11-13】clip 活跃但引用素材在场景
                    // 素材表里不存在——"视频有画面（transform.w 兜底）而
                    // 无声"的头号嫌疑分支，首次命中打日志（按 asset_id 节流）。
                    // （素材存在但 path 空 = 文字等无文件素材，正常无声不报）
                    static std::map<std::string, bool> s_missing_logged;
                    if (!s_missing_logged.count(c.asset_id)) {
                        s_missing_logged[c.asset_id] = true;
                        std::fprintf(stderr,
                                     "[audio] clip=%s 引用素材缺失: "
                                     "asset_id='%s' t=%.2fs → 无声（查场景"
                                     "素材表/asset_id 链）\n",
                                     c.id.c_str(), c.asset_id.c_str(),
                                     static_cast<double>(t_us) / 1e6);
                        std::fflush(stderr);
                    }
                }
            }
            auto it = pool.find(c.id);
            if (it != pool.end() && it->second.asset_path != asset_path) {
                pool.erase(it); // 换绑素材：旧条目作废（下拍按新路径重建）
                it = pool.end();
            }
            if (it == pool.end()) {
                AudioPoolEntry e;
                e.asset_path = asset_path;
                if (!asset_path.empty()) {
                    auto ams = std::make_unique<AudioMediaSource>(
                        assets_root, asset_path);
                    if (ams->opened()) {
                        e.source = std::move(ams);
                    } else {
                        // 【无声定位器 2026-11-13】打开失败原为静默（每拍
                        // 重试）——按路径首次打日志，定位"文件不存在/无
                        // 音轨/解码器拒绝"三类无声根因
                        static std::map<std::string, bool> s_openfail_logged;
                        if (!s_openfail_logged.count(asset_path)) {
                            s_openfail_logged[asset_path] = true;
                            std::fprintf(stderr,
                                         "[audio] 音频源打开失败: %s"
                                         "（clip=%s，本拍无声，落盘/修复后"
                                         "下拍自动接管）\n",
                                         asset_path.c_str(), c.id.c_str());
                            std::fflush(stderr);
                        }
                    }
                    // opened()=false（无音轨/文件未到达）：不建条目 →
                    // 本拍无声，下拍重试（文件到达自动接管，同视频
                    // "prepare 每帧重试"语义）
                }
                if (!e.source) {
                    if (!kForceSine) continue; // 无音轨 = 无声（不再回退正弦）
                    e.source = std::make_unique<SineAudioSource>(
                        220.0 + static_cast<double>(
                                    std::hash<std::string>{}(c.id) % 661));
                }
                // 【声道契约告警（[S3] 2026-11-11）】IAudioSource 契约：
                // channelCount() 不得超 kMaxSourceChannels(8)。现有实现
                // 构造期已自钳（AudioMediaSource min(8)/Sine 恒 1）不会
                // 触达；此告警是未来新音源（Live NDI 音频等）的启动期
                // 哨兵——超限声明 = 契约违约，引擎按上限截断配读取缓冲
                //（防堆越界），而非运行期静默越界。
                if (e.source &&
                    e.source->channelCount() > kMaxSourceChannels)
                    std::fprintf(stderr,
                                 "[audio] 契约违约：源声道数 %u > "
                                 "kMaxSourceChannels=%u（按上限截断）"
                                 "clip=%s\n",
                                 e.source->channelCount(),
                                 kMaxSourceChannels, c.id.c_str());
                it = pool.emplace(c.id, std::move(e)).first;
            }
            out.push_back(
                {it->second.source.get(),
                 static_cast<float>(TweenEngine::resolveGain(c, t_us)),
                 // 素材内时间与视频取帧同口径：in_us + (t - start)——
                 // 裁剪片段音画同点位；read 对前后跳自消化 drop/seek
                 c.in_us + (t_us - c.start_us)});
        }
}

} // namespace ms::audio
