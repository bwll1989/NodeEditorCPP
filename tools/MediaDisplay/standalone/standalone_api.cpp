// ============================================================================
//  standalone_api —— REST API 实现（display×MMC 合并 M2）
//
//  收编说明：路由分发/上传协议骨架承自 MMC http_routes_{playback,library}.cpp，
//  业务后端从"单文件播放器"整体替换为"多时间轴编排"（v2.2：timelines CRUD +
//  唯一活跃 + 本机播放头控制 + assets_root 媒体库）。
//
//  错误码约定（承 MMC ok_json/err_json 信封）：400 BAD_REQUEST / 404
//  NOT_FOUND / 409 CONFLICT（id 冲突）/ 413 上传超限 / 500 INTERNAL。
// ============================================================================

#include "standalone_api.hpp"

#include "http_util.hpp"
#include "json_util.hpp"
#include "mpcdi_service.hpp"
#include "path_utf8.hpp"
#include "config/IniConfig.hpp"
#include "loader/ShowFileIO.hpp"   // M5 工程互通（.show 导入/导出）
// 前置声明类型的完整定义（构造/方法调用必需——hpp 只持引用）
#include "runtime/PlayerRuntime.hpp"
#include "standalone/NodeController.hpp"
#include "standalone/TimelineRegistry.hpp"

#include "Poco/JSON/Array.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/MessageHeader.h"
#include "Poco/Net/PartHandler.h"
#include "Poco/StreamCopier.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>  // [S2] CompareStringW——NTFS 大小写折叠语义查重
#endif

namespace fs = std::filesystem;

namespace media_api {

using ms::standalone::ClipSpec;
using ms::standalone::SceneSpec;
using ms::standalone::TrackSpec;

namespace {

// ---------------------------------------------------------------------------
// 时间轴文档大小上限（round7 S3-1）：编排意图文档 KB 级（64 时间轴满载
// 实测单文档 ~10KB），5MB 已是数百倍余量；媒体字节走 /library 上传
//（max_upload_mb 门禁）——超限 body 只可能是误用/攻击，413 拒收。
// ---------------------------------------------------------------------------
constexpr std::size_t kMaxTimelineDocBytes = 5ull * 1024ull * 1024ull;

// ---------------------------------------------------------------------------
// Poco Dynamic::Var 安全取值（键缺失/类型不符回落默认——报文字段宽松解析）
// ---------------------------------------------------------------------------
std::string varStr(const Poco::JSON::Object::Ptr& o, const std::string& k,
                   const std::string& fb = {}) {
    const auto v = o->get(k);
    if (v.isEmpty()) return fb;
    try { return v.convert<std::string>(); } catch (...) { return fb; }
}

double varNum(const Poco::JSON::Object::Ptr& o, const std::string& k, double fb = 0.0) {
    const auto v = o->get(k);
    if (v.isEmpty()) return fb;
    try { return v.convert<double>(); } catch (...) { return fb; }
}

std::int64_t varI64(const Poco::JSON::Object::Ptr& o, const std::string& k,
                    std::int64_t fb = 0) {
    const auto v = o->get(k);
    if (v.isEmpty()) return fb;
    try { return v.convert<std::int64_t>(); } catch (...) {
        try { return static_cast<std::int64_t>(v.convert<double>()); } catch (...) { return fb; }
    }
}

bool varBool(const Poco::JSON::Object::Ptr& o, const std::string& k, bool fb = false) {
    const auto v = o->get(k);
    if (v.isEmpty()) return fb;
    try { return v.convert<bool>(); } catch (...) { return fb; }
}

const char* stateName(int s) {
    switch (s) {
        case 1: return "Playing";
        case 2: return "Paused";
        default: return "Idle";
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// 构造
// ---------------------------------------------------------------------------
StandaloneApi::StandaloneApi(ms::PlayerRuntime& player,
                             ms::standalone::TimelineRegistry& registry,
                             ms::standalone::NodeController& node,
                             ms::standalone::SceneAssembler& assembler,
                             std::string player_id, Config cfg)
    : player_(player),
      registry_(registry),
      node_(node),
      assembler_(assembler),
      player_id_(std::move(player_id)),
      cfg_(std::move(cfg)) {
    library_.set_root(cfg_.assets_root);
    library_.refresh();
}

// ---------------------------------------------------------------------------
// 文档解析（body JSON → TimelineDoc）
// ---------------------------------------------------------------------------
namespace {
/// [R8-3] UTF-8 安全的 ASCII 空白 trim（纯空格名落盘=列表"无名"条目根因）
std::string trimAscii(const std::string& s) {
    const char* ws = " \t\r\n\v\f";
    const std::size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    const std::size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}
}  // namespace

bool StandaloneApi::parseDoc(const std::string& body, TimelineDoc& doc, std::string& err) {
    try {
        Poco::JSON::Parser parser;
        const auto root = parser.parse(body);
        if (root.isEmpty()) { err = "empty body"; return false; }
        const auto obj = root.extract<Poco::JSON::Object::Ptr>();

        doc.name = trimAscii(varStr(obj, "name"));   // [R8-3] 名字段 trim
        doc.loop = varBool(obj, "loop", false);
        doc.stop_position_us = varI64(obj, "stop_position_us", 0);
        if (doc.stop_position_us < 0) doc.stop_position_us = 0;
        // [W1] 乐观锁版本：请求体缺省（旧客户端）= 0——与磁盘遗留文档
        //（rev=0）相等放行；磁盘已 bump 过的文档会被旧客户端 409（保护性）
        doc.rev = varI64(obj, "rev", 0);
        if (doc.rev < 0) doc.rev = 0;

        const auto sceneVar = obj->get("scene");
        if (sceneVar.isEmpty()) { err = "missing scene"; return false; }
        const auto scene = sceneVar.extract<Poco::JSON::Object::Ptr>();
        doc.scene.canvas_w = varNum(scene, "canvas_w", 1920.0);
        doc.scene.canvas_h = varNum(scene, "canvas_h", 1080.0);
        if (doc.scene.canvas_w <= 0) doc.scene.canvas_w = 1920.0;
        if (doc.scene.canvas_h <= 0) doc.scene.canvas_h = 1080.0;

        const auto tracksVar = scene->get("tracks");
        if (!tracksVar.isEmpty()) {
            const auto tracks = tracksVar.extract<Poco::JSON::Array::Ptr>();
            for (std::size_t ti = 0; ti < tracks->size(); ++ti) {
                const auto trVar = tracks->get(ti);
                if (trVar.isEmpty()) continue;
                const auto tr = trVar.extract<Poco::JSON::Object::Ptr>();
                TrackSpec t;
                t.name = varStr(tr, "name", "轨 " + std::to_string(ti + 1));
                const auto clipsVar = tr->get("clips");
                if (!clipsVar.isEmpty()) {
                    const auto clips = clipsVar.extract<Poco::JSON::Array::Ptr>();
                    for (std::size_t ci = 0; ci < clips->size(); ++ci) {
                        const auto clVar = clips->get(ci);
                        if (clVar.isEmpty()) continue;
                        const auto cl = clVar.extract<Poco::JSON::Object::Ptr>();
                        ClipSpec c;
                        c.file         = varStr(cl, "file");
                        c.text         = varStr(cl, "text");
                        c.start_us     = varI64(cl, "start_us", 0);
                        c.duration_us  = varI64(cl, "duration_us", 0);
                        c.in_us        = varI64(cl, "in_us", 0);
                        c.x            = varNum(cl, "x", 0.0);
                        c.y            = varNum(cl, "y", 0.0);
                        c.w            = varNum(cl, "w", 0.0);
                        c.h            = varNum(cl, "h", 0.0);
                        c.opacity      = varNum(cl, "opacity", 1.0);
                        c.z            = static_cast<int>(varI64(cl, "z", 0));
                        c.fade_in_us   = varNum(cl, "fade_in_us", 0.0);
                        c.fade_out_us  = varNum(cl, "fade_out_us", 0.0);
                        // 旋转/裁剪（2026-11-11 补齐；坐标/尺寸=画布 px 绝对值，
                        // 裁剪相对绘制矩形边缘向内收 px）
                        c.rotation     = varNum(cl, "rotation", 0.0);
                        c.crop_l       = varNum(cl, "crop_l", 0.0);
                        c.crop_r       = varNum(cl, "crop_r", 0.0);
                        c.crop_t       = varNum(cl, "crop_t", 0.0);
                        c.crop_b       = varNum(cl, "crop_b", 0.0);
                        if (c.start_us < 0) c.start_us = 0;
                        if (c.duration_us < 0) c.duration_us = 0;
                        if (c.opacity < 0.0) c.opacity = 0.0;
                        if (c.opacity > 1.0) c.opacity = 1.0;
                        if (c.fade_in_us < 0) c.fade_in_us = 0;
                        if (c.fade_out_us < 0) c.fade_out_us = 0;
                        if (c.crop_l < 0) c.crop_l = 0;   // 裁剪只向内收，负值钳 0
                        if (c.crop_r < 0) c.crop_r = 0;   //（过裁收口在装配端
                        if (c.crop_t < 0) c.crop_t = 0;   //  Transform::visibleRect）
                        if (c.crop_b < 0) c.crop_b = 0;
                        if (c.file.empty() && c.text.empty()) continue;  // 无内容片段丢弃
                        t.clips.push_back(std::move(c));
                    }
                }
                doc.scene.tracks.push_back(std::move(t));
            }
        }
        return true;
    } catch (const Poco::Exception& ex) {
        err = "bad json: " + ex.displayText();
        return false;
    } catch (const std::exception& ex) {
        err = std::string("bad json: ") + ex.what();
        return false;
    }
}

// ---------------------------------------------------------------------------
// 文档序列化（结构 → 规范 JSON；Poco stringify 紧凑输出，往返幂等）
// ---------------------------------------------------------------------------
std::string StandaloneApi::docToJson(const std::string& id, const TimelineDoc& doc) {
    Poco::JSON::Object::Ptr root = new Poco::JSON::Object;
    root->set("id", id);
    root->set("name", doc.name);
    root->set("loop", doc.loop);
    root->set("stop_position_us", static_cast<Poco::Int64>(doc.stop_position_us));
    root->set("rev", static_cast<Poco::Int64>(doc.rev));  // [W1] 乐观锁版本

    Poco::JSON::Object::Ptr scene = new Poco::JSON::Object;
    scene->set("canvas_w", doc.scene.canvas_w);
    scene->set("canvas_h", doc.scene.canvas_h);
    Poco::JSON::Array::Ptr tracks = new Poco::JSON::Array;
    for (const auto& t : doc.scene.tracks) {
        Poco::JSON::Object::Ptr tr = new Poco::JSON::Object;
        tr->set("name", t.name);
        Poco::JSON::Array::Ptr clips = new Poco::JSON::Array;
        for (const auto& c : t.clips) {
            Poco::JSON::Object::Ptr cl = new Poco::JSON::Object;
            cl->set("file", c.file);
            cl->set("text", c.text);
            cl->set("start_us", static_cast<Poco::Int64>(c.start_us));
            cl->set("duration_us", static_cast<Poco::Int64>(c.duration_us));
            cl->set("in_us", static_cast<Poco::Int64>(c.in_us));
            cl->set("x", c.x);
            cl->set("y", c.y);
            cl->set("w", c.w);
            cl->set("h", c.h);
            cl->set("opacity", c.opacity);
            cl->set("z", c.z);
            cl->set("rotation", c.rotation);
            cl->set("crop_l", c.crop_l);
            cl->set("crop_r", c.crop_r);
            cl->set("crop_t", c.crop_t);
            cl->set("crop_b", c.crop_b);
            cl->set("fade_in_us", c.fade_in_us);
            cl->set("fade_out_us", c.fade_out_us);
            clips->add(cl);
        }
        tr->set("clips", clips);
        tracks->add(tr);
    }
    scene->set("tracks", tracks);
    root->set("scene", scene);

    std::ostringstream oss;
    root->stringify(oss);
    return oss.str();
}

// ---------------------------------------------------------------------------
// .show 场景 → 网页时间轴文档（M5 工程互通导入方向）
//   映射纪律（2026-11-11 B 路线定案）：track 次序=轨序；轨内 clip 按
//   transform.z 稳定升序重排、z%1000 还原"轨内层级"文档语义（装配端
//   合成公式 (N-1-ti)*1000+cs.z 的逆，跨轨层次由轨序天然恢复）；
//   Opacity 首尾双 key tween 反推 fade_in/out（装配端正变换的逆）；
//   AudioGain tween 是 fade 的伴生通道（doc 无独立字段）直接忽略；
//   其余 tween（位置/角点/三轴动画）网页编排无承载 → 丢弃留 note；
//   Live/Unknown 素材同理。screens/displays（editor 端登记语义）忽略
//   ——standalone 单机全画布，渲染端自建本机屏。
// ---------------------------------------------------------------------------
void StandaloneApi::splitTracksForExport(ms::Scene& scene) {
    ms::Timeline merged;
    for (const auto& t : scene.timeline.tracks) {
        // Control 轨道（2026-11-13）：指令片段无素材语义、网页编排无承载
        // ——整轨静默跳过（否则按 clip 查素材落 Unknown 组，导入端再报
        // "素材缺失"误导）
        if (t.kind == ms::AssetKind::Control) continue;
        // 按 clip 素材类型分组（子轨次序先按"类型首 clip 轨内出现序"生成，
        // 再在下方按组内最大原 z 重排）
        std::vector<std::pair<ms::AssetKind, ms::Track>> groups;
        for (const auto& c : t.clips) {
            const ms::Asset* a = scene.findAsset(c.asset_id);
            const ms::AssetKind k = a ? a->kind : ms::AssetKind::Unknown;
            auto it = std::find_if(
                groups.begin(), groups.end(),
                [&](const std::pair<ms::AssetKind, ms::Track>& g) {
                    return g.first == k;
                });
            if (it == groups.end()) {
                groups.emplace_back(k, ms::Track{});
                groups.back().second.name = t.name;
                it = groups.end() - 1;
            }
            it->second.clips.push_back(c);
        }
        // [S2] 子轨组按组内最大原 z 降序：原混合轨内最高层的类型组排最前
        //（装配端 base=(N-1-ti)*1000，ti 小层高）——组间层次由组序承载，
        // 与导入端（轨序+轨内 z）的正变换互逆
        std::stable_sort(
            groups.begin(), groups.end(),
            [](const std::pair<ms::AssetKind, ms::Track>& x,
               const std::pair<ms::AssetKind, ms::Track>& y) {
                auto max_z = [](const ms::Track& tr) {
                    int32_t m = std::numeric_limits<int32_t>::min();
                    for (const auto& c : tr.clips) m = (std::max)(m, c.transform.z);
                    return m;
                };
                return max_z(x.second) > max_z(y.second);
            });
        for (auto& g : groups) {
            g.second.kind = g.first;
            // [S2] 组内按原 z 稳定升序重编 rank（0..999 封顶）：原混合轨内
            // z 交错的 clip 在"组间块 + 组内序"的 track-major 语义下按块
            // 保序；rank 已是最终序 → 一次导入回环即收敛不动点（不再漂移）
            auto& clips = g.second.clips;
            std::stable_sort(clips.begin(), clips.end(),
                             [](const ms::Clip& x, const ms::Clip& y) {
                                 return x.transform.z < y.transform.z;
                             });
            int32_t rank = 0;
            for (auto& c : clips) c.transform.z = (std::min)(rank++, static_cast<int32_t>(999));
            merged.tracks.push_back(std::move(g.second));
        }
    }
    for (std::size_t i = 0; i < merged.tracks.size(); ++i)
        merged.tracks[i].id = "t-" + std::to_string(i + 1);
    scene.timeline.tracks = std::move(merged.tracks);
}

bool StandaloneApi::sceneToDoc(const ms::Scene& scene, TimelineDoc& doc,
                               std::string& note) {
    doc = TimelineDoc{};
    doc.scene.canvas_w = scene.canvas_w > 0.0 ? scene.canvas_w : 1920.0;
    doc.scene.canvas_h = scene.canvas_h > 0.0 ? scene.canvas_h : 1080.0;

    auto add_note = [&note](const std::string& s) {
        if (!note.empty()) note += "\n";
        note += s;
    };

    for (const auto& t : scene.timeline.tracks) {
        // Control 轨道（2026-11-13）：指令片段无素材语义——整轨静默跳过，
        // 不走下方"素材缺失"note（asset_id 恒空，报出来是误导）
        if (t.kind == ms::AssetKind::Control) continue;
        TrackSpec ts;
        ts.name = t.name;

        // 轨内 clip 按 transform.z 稳定升序（同 z 保 Scene 原序——stable）
        std::vector<const ms::Clip*> ordered;
        ordered.reserve(t.clips.size());
        for (const auto& c : t.clips) ordered.push_back(&c);
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const ms::Clip* a, const ms::Clip* b) {
                             return a->transform.z < b->transform.z;
                         });

        for (const ms::Clip* cp : ordered) {
            const ms::Clip& c = *cp;
            const ms::Asset* a = scene.findAsset(c.asset_id);
            if (a == nullptr) {  // 引用完整性校验已拦——防御分支
                note += (note.empty() ? "" : "\n");
                note += "跳过片段（素材缺失）: " + c.asset_id;
                continue;
            }
            ClipSpec cs;
            if (a->kind == ms::AssetKind::Text) {
                cs.text = a->text_content;
            } else if (a->kind == ms::AssetKind::Video ||
                       a->kind == ms::AssetKind::Image ||
                       a->kind == ms::AssetKind::Audio) {
                cs.file = a->path;
            } else {
                note += (note.empty() ? "" : "\n");
                note += "跳过片段（网页编排不支持该素材类型）: " +
                        (a->path.empty() ? a->id : a->path);
                continue;
            }
            cs.start_us    = c.start_us;
            cs.duration_us = c.duration_us;
            cs.in_us       = c.in_us;
            cs.x        = c.transform.x;
            cs.y        = c.transform.y;
            cs.w        = c.transform.w;
            cs.h        = c.transform.h;
            cs.opacity  = c.transform.opacity;
            cs.rotation = c.transform.rotation;
            cs.crop_l   = (std::max)(c.transform.crop[ms::kCropL], 0.0);
            cs.crop_r   = (std::max)(c.transform.crop[ms::kCropR], 0.0);
            cs.crop_t   = (std::max)(c.transform.crop[ms::kCropT], 0.0);
            cs.crop_b   = (std::max)(c.transform.crop[ms::kCropB], 0.0);
            cs.z        = std::clamp(c.transform.z % 1000, 0, 999);

            // fade 反推：只认装配端形态——纯首淡（2 key：{0,0}→{fi,head}）/
            // 纯尾淡（2 key：{dur-fo,head}→{dur,0}）/ 双段（4 key）。首尾
            // 判定独立成立（单侧淡各自还原）；其余形态 = editor 自定义
            // 透明度动画 → 整体放弃留 note。
            for (const auto& tw : c.tweens) {
                if (tw.prop == ms::TweenProp::Opacity) {
                    const auto& k = tw.keys;
                    const bool head_ok = k.size() >= 2 && k[0].t_us == 0 &&
                                         k[0].v0 == 0.0;
                    const bool tail_ok =
                        k.size() >= 2 && k.back().t_us == c.duration_us &&
                        k.back().v0 == 0.0;
                    const bool simple =
                        k.size() <= 4 && (head_ok || tail_ok) &&
                        (!head_ok || k.size() == 2 ||
                         (tail_ok && k.size() == 4)) &&
                        (!tail_ok || k.size() == 2 ||
                         (head_ok && k.size() == 4));
                    if (simple) {
                        if (head_ok)
                            cs.fade_in_us =
                                static_cast<double>(k[1].t_us);
                        if (tail_ok)
                            cs.fade_out_us =
                                static_cast<double>(c.duration_us -
                                                    k[k.size() - 2].t_us);
                    } else {
                        // editor 自定义透明度动画：网页编排无法表达
                        note += (note.empty() ? "" : "\n");
                        note += "片段透明度动画已简化为静态值（网页编排"
                                "暂不支持关键帧动画）: " +
                                (a->path.empty() ? a->text_content : a->path);
                    }
                } else if (tw.prop != ms::TweenProp::AudioGain) {
                    note += (note.empty() ? "" : "\n");
                    note += "片段关键帧动画已简化为静态值（网页编排暂不"
                            "支持）: " +
                            (a->path.empty() ? a->text_content : a->path);
                }
            }
            ts.clips.push_back(std::move(cs));
        }
        doc.scene.tracks.push_back(std::move(ts));
    }
    if (doc.scene.tracks.empty()) {
        note = "工程无有效轨道";
        return false;
    }
    return true;
}

std::filesystem::path StandaloneApi::timelineDocPath(const std::string& id) const {
    // [S1] 目录分量与文件名分量都必须经 utf8_to_path：id 是 UTF-8 字节串，
    // fs::path 窄字符串构造在 Windows 按 ACP(GBK) 解码——中文 id 落盘成
    // mojibake NTFS 名，与装载链（loadTimelineDocs 按 UTF-8 语义读回）错位
    // → 重启后 id 乱码化（round3 S1 根因）。此前目录分量走 utf8_to_path 而
    // 文件名分量 (id + ".json") 是窄构造——save/load 双方同错时运行期自洽，
    // 重启后装载链暴露差异。统一收口于此，杜绝新调用点再犯。
    const std::string rel = id + ".json";
    return cfg_.timelines_dir.empty()
               ? media_api::utf8_to_path(rel)
               : media_api::utf8_to_path(cfg_.timelines_dir) / media_api::utf8_to_path(rel);
}

bool StandaloneApi::loadDoc(const std::string& id, TimelineDoc& doc, std::string& err) const {
    std::lock_guard<std::mutex> lock(io_mutex_);
    return loadDocLocked(id, doc, err);
}

bool StandaloneApi::loadDocLocked(const std::string& id, TimelineDoc& doc, std::string& err) const {
    // S2-1 防御层：路由侧已校验，此处兜底（id 直拼路径的任何新调用点
    // 未经校验也不会形成穿越）
    if (!validTimelineId(id, err)) return false;
    std::string content;
    // [S1] read_file 入参是 UTF-8 字符串（内部 utf8_to_path 转宽字符），
    // 此处宽路径 → UTF-8（path_to_utf8）round-trip 无损；勿用 .string()——
    // 窄转换按 ACP 解码，中文路径不可映射字符会抛 system_error
    if (!read_file(media_api::path_to_utf8(timelineDocPath(id)), content)) {
        err = "时间轴文档不存在: " + id;
        return false;
    }
    return parseDoc(content, doc, err);
}

bool StandaloneApi::saveDoc(const std::string& id, const TimelineDoc& doc, std::string& err) const {
    std::lock_guard<std::mutex> lock(io_mutex_);
    return saveDocLocked(id, doc, err);
}

bool StandaloneApi::saveDocLocked(const std::string& id, const TimelineDoc& doc, std::string& err) const {
    // S2-1 防御层（同 loadDoc；本函数在 io_mutex_ 锁内被调用）
    if (!validTimelineId(id, err)) return false;
    std::error_code ec;
    fs::create_directories(utf8_to_path(cfg_.timelines_dir), ec);
    // [S1] 文件名分量经 utf8_to_path（见 timelineDocPath 注释）
    const fs::path file = timelineDocPath(id);
    std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        err = "文档写入失败: " + media_api::path_to_utf8(file);
        return false;
    }
    ofs << docToJson(id, doc);
    return ofs.good();
}

bool StandaloneApi::removeDoc(const std::string& id, std::string& err) const {
    std::lock_guard<std::mutex> lock(io_mutex_);
    // S2-1 防御层（同 loadDoc）
    if (!validTimelineId(id, err)) return false;
    std::error_code ec;
    // [S1] 文件名分量经 utf8_to_path（见 timelineDocPath 注释）
    const fs::path file = timelineDocPath(id);
    if (fs::exists(file, ec)) {
        fs::remove(file, ec);
        if (ec) {
            err = "文档删除失败: " + media_api::path_to_utf8(file);
            return false;
        }
    }
    return true;
}

bool StandaloneApi::timelineIdCasefoldExists(const std::string& id, std::string& hit) const {
    // [S2] id 即文档文件名——Windows NTFS 不区分大小写，"ABC.json" 与
    // "abc.json" 是同一文件：仅靠注册表查重会漏掉磁盘上既有的折叠同名
    // 文档（含重启装载失败未登记者），落盘时 ofstream 直接顶掉既有文件。
    // 此处对 timelines_dir 做文件系统级折叠比对（LOCALE_INVARIANT 语义，
    // 与 NTFS 命名折叠一致）；非 Windows 文件系统区分大小写，恒无冲突。
#ifdef _WIN32
    std::error_code ec;
    const fs::path dir = utf8_to_path(cfg_.timelines_dir);
    if (!fs::exists(dir, ec) || ec) return false;
    const std::wstring target = utf8_to_path(id + ".json").wstring();
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec) || ec) continue;
        const std::wstring name = it->path().filename().wstring();
        if (name.size() != target.size()) continue;  // 折叠不改长度，长度不同即不等
        if (::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, name.c_str(),
                             static_cast<int>(name.size()), target.c_str(),
                             static_cast<int>(target.size())) == CSTR_EQUAL) {
            hit = media_api::path_to_utf8(it->path().stem());  // 磁盘上的实际 id
            return true;
        }
    }
    return false;
#else
    (void)id;
    (void)hit;
    return false;
#endif
}

std::shared_ptr<const ms::Scene> StandaloneApi::assembleDoc(const TimelineDoc& doc,
                                                            std::string& err) const {
    // [S1] 装配异常兜底（round7 S1-1）：探测链内任何 std::exception（如
    // 文件系统窄转换）不再裸抛穿透 HTTP 层——就地转结构化错误返回
    // nullptr，路由按既有"装配失败"note 路径回 400/提示，不泄漏内部
    // 异常消息（消息经 acp_to_utf8 转 UTF-8 防乱码）。
    try {
        return assembler_.assemble(doc.scene, player_id_, err);
    } catch (const std::exception& ex) {
        err = "装配异常: " + media_api::acp_to_utf8(ex.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------
// 路由总入口
// ---------------------------------------------------------------------------
bool StandaloneApi::validTimelineId(const std::string& id, std::string& err) {
    if (id.empty()) {
        err = "id 不能为空";
        return false;
    }
    if (id.size() > 128) {
        err = "id 过长（上限 128 字节）";
        return false;
    }
    // 路径分隔符/盘符冒号：穿越与绝对路径（含 NTFS 命名流 file.json:ads）
    // 一并否决——id 是稳定标识，不做 basename 化静默改写（与媒体库上传
    // 的 sanitize 先例不同：上传改写只影响新落盘文件名，而 id 改名会破坏
    // id↔文档文件的既有对应）。
    if (id.find('/') != std::string::npos ||
        id.find('\\') != std::string::npos ||
        id.find(':') != std::string::npos) {
        err = "id 禁止包含路径分隔符（/ \\ :）";
        return false;
    }
    // ".." 分量（分隔符已排除，含 ".." 即危险/无意义；也覆盖纯 ".."）
    if (id.find("..") != std::string::npos) {
        err = "id 禁止包含 \"..\" 分量";
        return false;
    }
    // Windows 文件名非法字符与控制字符
    for (const char ch : id) {
        const auto u = static_cast<unsigned char>(ch);
        if (u < 0x20 || u == 0x7f || std::strchr("<>:\"|?*", ch)) {
            err = "id 含文件名非法字符";
            return false;
        }
    }
    return true;
}

bool StandaloneApi::handle(RouteCtx& ctx) {
    if (routeStatus(ctx)) return true;
    if (routeSettings(ctx)) return true;
    // MPCDI 路由组（M4）：本机输出校准能力——门禁**前**（两模式均开放；
    // sync 受控下 display 仍有自己的输出校准，与编排面互不相干）。
    if (routeMpcdi(ctx)) return true;
    // 系统路由（M5）：进程重启——门禁**前**（两模式均开放；sync 切回
    // standalone 正是靠它，收进门禁内会形成"同步模式下无法切回"死锁）。
    if (routeSystem(ctx)) return true;
    // 模式门禁（2026-11-11 手动切换定案）：编排面（时间轴/播放/媒体库）
    // 只在独立模式开放——sync 模式下 HTTP 仅 状态/设置/静态页，编排面
    // 一律 409（网页编排台前端按 status.mode 同步收敛到设置页）。
    if (cfg_.mode != "standalone") {
        if (ctx.path.rfind("/api/v1/", 0) == 0) {
            send_json(ctx.resp, 409, err_json(
                "MODE_FORBIDDEN",
                "当前为同步模式（Editor 受控）——时间轴/媒体库/播放控制"
                "仅在独立模式下可用（设置页可切换模式，保存后重启生效）"));
            return true;
        }
        return false; // 非 API 路径（静态页/设置页资源）照常放行
    }
    if (routeTimelines(ctx)) return true;
    if (routePlayback(ctx)) return true;
    if (routeLibrary(ctx)) return true;
    return false;
}

// ---------------------------------------------------------------------------
// mpcdi —— 校准包（M4 收编自 MMC http_routes_mpcdi；语义逐条对齐）
//   GET  /api/v1/mpcdi                          —— 状态（enabled/包/会话）
//   GET  /api/v1/mpcdi/packages                 —— 包列表
//   GET  /api/v1/mpcdi/packages/{id}/regions    —— region 目录
//   GET  /api/v1/mpcdi/packages/{id}/metadata   —— viewer metadata 面板
//   POST /api/v1/mpcdi/upload                   —— 包上传（multipart/raw）
//   POST /api/v1/mpcdi/enable                   —— 启用（body: package_id/
//                                                  regions/layout/options）
//   POST /api/v1/mpcdi/disable                  —— 停用（撤几何 + 落盘）
// ---------------------------------------------------------------------------
bool StandaloneApi::routeMpcdi(RouteCtx& ctx) {
    if (!cfg_.mpcdi) return false;
    const std::string& method = ctx.method;
    const std::string& path = ctx.path;
    Poco::Net::HTTPServerResponse& resp = ctx.resp;

    if (path == "/api/v1/mpcdi" && method == "GET") {
        send_json(resp, 200, ok_json(cfg_.mpcdi->status_json()));
        return true;
    }
    if (path == "/api/v1/mpcdi/packages" && method == "GET") {
        send_json(resp, 200, ok_json(cfg_.mpcdi->packages_json()));
        return true;
    }
    // /api/v1/mpcdi/packages/{id}/(regions|metadata)
    if (path.rfind("/api/v1/mpcdi/packages/", 0) == 0 && method == "GET") {
        static constexpr char kPrefix[] = "/api/v1/mpcdi/packages/";
        std::string rest = path.substr(sizeof(kPrefix) - 1);
        const auto slash = rest.find('/');
        if (slash == std::string::npos) return false;
        const std::string id = rest.substr(0, slash);
        const std::string tail = rest.substr(slash + 1);
        if (tail != "regions" && tail != "metadata") return false;
        std::string out, err;
        const bool ok = (tail == "regions")
                            ? cfg_.mpcdi->regions_json(id, out, err)
                            : cfg_.mpcdi->metadata_json(id, out, err);
        if (!ok) {
            send_json(resp, 404, err_json("NOT_FOUND",
                                          err.empty() ? "parse failed" : err));
            return true;
        }
        send_json(resp, 200, ok_json(out));
        return true;
    }
    if (path == "/api/v1/mpcdi/upload" && method == "POST") {
        handleMpcdiUpload(ctx);
        return true;
    }
    if (path == "/api/v1/mpcdi/enable" && method == "POST") {
        // body: {package_id?, regions:"r1,r2", layout:{...}, options:{...}}
        const auto body = read_body(ctx.req);
        std::string package_id = "current", regions, layout, options;
        extract_string_field(body, "package_id", package_id);
        extract_string_field(body, "regions", regions);
        extract_json_value(body, "layout", layout);
        extract_json_value(body, "options", options);
        std::string err;
        if (!cfg_.mpcdi->enable(package_id, regions, layout, options, err)) {
            send_json(resp, 400, err_json("MPCDI_ENABLE_FAILED",
                                          err.empty() ? "enable failed" : err));
            return true;
        }
        send_json(resp, 200, ok_json(cfg_.mpcdi->status_json()));
        return true;
    }
    if (path == "/api/v1/mpcdi/disable" && method == "POST") {
        std::string err;
        if (!cfg_.mpcdi->disable(err)) {
            send_json(resp, 500, err_json("MPCDI_DISABLE_FAILED",
                                          err.empty() ? "disable failed" : err));
            return true;
        }
        send_json(resp, 200, ok_json(cfg_.mpcdi->status_json()));
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// mpcdi 包上传（multipart 优先 / raw 兜底；三段式对齐媒体库上传纪律）
// ---------------------------------------------------------------------------
void StandaloneApi::handleMpcdiUpload(RouteCtx& ctx) {
    Poco::Net::HTTPServerRequest& req = ctx.req;
    Poco::Net::HTTPServerResponse& resp = ctx.resp;
    MpcdiService& m = *cfg_.mpcdi;
    const std::size_t max_bytes =
        cfg_.max_upload_mb == 0
            ? (std::numeric_limits<std::size_t>::max)()
            : cfg_.max_upload_mb * 1024ull * 1024ull;

    auto ok_payload = [&](const std::string& package_id) {
        std::ostringstream data;
        data << "{\"uploaded\":true,\"package_id\":\""
             << json_escape(package_id) << "\"}";
        send_json(resp, 200, ok_json(data.str()));
    };

    if (req.getContentType().find("multipart/form-data") != std::string::npos) {
        // multipart 文件部件三段式（对齐媒体库 handleUpload 的接收纪律）
        class MpcdiPart : public Poco::Net::PartHandler {
        public:
            explicit MpcdiPart(MpcdiService& svc, std::size_t max_bytes)
                : svc_(svc), max_bytes_(max_bytes) {}

            void handlePart(const Poco::Net::MessageHeader& header,
                            std::istream& stream) override {
                if (got_file_) return;
                std::string disp = header.has("Content-Disposition")
                                       ? header.get("Content-Disposition")
                                       : "";
                if (disp.find("name=\"file\"") == std::string::npos &&
                    disp.find("name=file") == std::string::npos) {
                    std::stringstream sink;
                    Poco::StreamCopier::copyStream(stream, sink);
                    return;
                }
                std::string filename;
                auto pos = disp.find("filename=");
                if (pos != std::string::npos) {
                    pos += 9;
                    if (pos < disp.size() && disp[pos] == '\"') {
                        ++pos;
                        auto end = disp.find('\"', pos);
                        filename = end == std::string::npos
                                       ? disp.substr(pos)
                                       : disp.substr(pos, end - pos);
                    } else {
                        auto end = disp.find_first_of(" ;", pos);
                        filename = end == std::string::npos
                                       ? disp.substr(pos)
                                       : disp.substr(pos, end - pos);
                    }
                }
                if (filename.empty()) {
                    err_ = "missing filename";
                    return;
                }
                filename_ = filename;   // 落盘名（finish 用）
                if (!svc_.upload_begin(filename, temp_, err_)) return;
                char buf[64 * 1024];
                while (stream.good()) {
                    stream.read(buf, sizeof(buf));
                    auto n = stream.gcount();
                    if (n <= 0) break;
                    if (written_ + static_cast<std::size_t>(n) > max_bytes_) {
                        svc_.discard_upload_temp(temp_);
                        temp_.clear();
                        err_ = "upload too large";
                        return;
                    }
                    std::ofstream ofs(
                        temp_, std::ios::binary | std::ios::app);
                    if (!ofs) {
                        svc_.discard_upload_temp(temp_);
                        temp_.clear();
                        err_ = "cannot write temp file";
                        return;
                    }
                    ofs.write(buf, n);
                    written_ += static_cast<std::size_t>(n);
                }
                got_file_ = true;
            }

            MpcdiService& svc_;
            std::size_t max_bytes_;
            std::string temp_;
            std::size_t written_ = 0;
            std::string filename_ = "package.mpcdi";
            bool got_file_ = false;
            std::string err_;

            bool finish(std::string& package_id) {
                if (!got_file_) return false;
                return svc_.upload_finish(temp_, filename_, package_id, err_);
            }
        };

        MpcdiPart part(m, max_bytes);
        try {
            Poco::Net::HTMLForm form(req, req.stream(), part);
        } catch (const Poco::Exception& ex) {
            if (!part.temp_.empty()) m.discard_upload_temp(part.temp_);
            send_json(resp, 400, err_json("BAD_REQUEST", ex.displayText()));
            return;
        }
        if (!part.err_.empty()) {
            if (!part.temp_.empty()) m.discard_upload_temp(part.temp_);
            send_json(resp, part.err_ == "upload too large" ? 413 : 400,
                      err_json("BAD_REQUEST", part.err_));
            return;
        }
        if (!part.got_file_) {
            send_json(resp, 400,
                      err_json("BAD_REQUEST", "missing multipart file"));
            return;
        }
        std::string package_id;
        if (!part.finish(package_id)) {
            if (!part.temp_.empty()) m.discard_upload_temp(part.temp_);
            send_json(resp, 400, err_json("BAD_REQUEST",
                                          part.err_.empty() ? "upload failed"
                                                            : part.err_));
            return;
        }
        ok_payload(package_id);
        return;
    }

    // ---- raw octet-stream（X-Filename 头或 ?filename= 查询）------------------
    std::string filename;
    for (const auto& kv : ctx.uri.getQueryParameters()) {
        if (kv.first == "filename") filename = kv.second;
    }
    if (filename.empty() && req.has("X-Filename"))
        filename = req.get("X-Filename");
    if (filename.empty()) {
        send_json(resp, 400, err_json("BAD_REQUEST", "missing filename"));
        return;
    }
    std::string temp, err;
    if (!m.upload_begin(filename, temp, err)) {
        // S3-2（2026-11-11 测试清单）：begin 失败属客户端错误（文件名/参数
        // 问题）——400 对齐 multipart 分支与媒体库 raw 路径（原 500 误导
        // 客户端按服务端故障重试/告警）
        send_json(resp, 400, err_json("BAD_REQUEST", err));
        return;
    }
    char buf[64 * 1024];
    std::size_t written = 0;
    while (req.stream().good()) {
        req.stream().read(buf, sizeof(buf));
        auto n = req.stream().gcount();
        if (n <= 0) break;
        if (written + static_cast<std::size_t>(n) > max_bytes) {
            m.discard_upload_temp(temp);
            send_json(resp, 413, err_json("BAD_REQUEST", "upload too large"));
            return;
        }
        std::ofstream ofs(temp, std::ios::binary | std::ios::app);
        if (!ofs) {
            m.discard_upload_temp(temp);
            send_json(resp, 500, err_json("BAD_REQUEST", "cannot write temp"));
            return;
        }
        ofs.write(buf, n);
        written += static_cast<std::size_t>(n);
    }
    std::string package_id;
    if (!m.upload_finish(temp, filename, package_id, err)) {
        m.discard_upload_temp(temp);
        send_json(resp, 400,
                  err_json("BAD_REQUEST", err.empty() ? "upload failed" : err));
        return;
    }
    ok_payload(package_id);
}

// ---------------------------------------------------------------------------
// status
// ---------------------------------------------------------------------------
bool StandaloneApi::routeStatus(RouteCtx& ctx) {
    if (ctx.path != "/api/v1/status" || ctx.method != "GET") return false;

    std::ostringstream data;
    data << "{\"mode\":\"" << json_escape(cfg_.mode) << "\","
         << "\"player_id\":\"" << json_escape(player_id_) << "\","
         << "\"active_timeline\":\"" << json_escape(registry_.activeId()) << "\","
         << "\"default_timeline\":\"" << json_escape(registry_.defaultId()) << "\","
         << "\"state\":" << static_cast<int>(node_.state()) << ","
         << "\"state_name\":\"" << stateName(static_cast<int>(node_.state())) << "\","
         << "\"playhead_us\":" << node_.playheadUs() << ","
         << "\"timeline_count\":" << registry_.size() << "}";
    send_json(ctx.resp, 200, ok_json(data.str()));
    return true;
}

// ---------------------------------------------------------------------------
// settings（2026-11-11 手动模式切换定案）：设置读/写（两模式均可用）
//
//   GET  /api/v1/settings  → { mode, node:{}, display:{}, standalone:{} }
//                            （config.ini 已知键快照；文件缺失=内置默认）
//   PUT  /api/v1/settings  → body 同构（部分键可选），逐键白名单校验后
//                            saveKeyToFile 落盘；模式/端口类改动需重启生效
// ---------------------------------------------------------------------------
namespace {

/// 设置键白名单（section → 键 → 是否整数）。GET/PUT 共用同一张表——
/// 超出白名单的键拒绝（配置文件是 display 的启动真相，防网页误写）。
struct SettingKey { const char* key; bool is_int; };
const std::vector<std::pair<const char*, std::vector<SettingKey>>>&
settingSchema() {
    static const std::vector<std::pair<const char*, std::vector<SettingKey>>>
        kSchema = {
        {"node", {{"mode", false}}},
        {"display",
         {{"player_id", false},    {"listen_port", true},
          {"discovery_port", true},{"assets_root", false},
          {"asset_workers", true}, {"render_fps", true},
          {"render_backend", false},{"hw_accel", false},
          {"screen", true},        {"fullscreen", true},
          {"borderless", true},    {"window_x", true},
          {"window_y", true},      {"window_w", true},
          {"window_h", true},      {"report_interval_ms", true},
          {"diag_interval_ms", true}, {"pairing_code", false},
          {"audio_enable", true},  {"audio_device", false}}},
        {"standalone",
         {{"http_port", true},    {"max_upload_mb", true},
          {"timelines_dir", false}, {"default_timeline", false}}},
    };
    return kSchema;
}

/// 整数键值域校验（端口类 0~65535；其余宽松 0~1e9；非法返回说明）
std::string validateSetting(const std::string& sec, const std::string& key,
                            const std::string& value) {
    if (value.find('\n') != std::string::npos ||
        value.find('\r') != std::string::npos)
        return "值含换行（非法）";
    // 错误文案不带键名前缀（外层 errors 拼接已含 "sec.key:"——重复必撞测试）
    if (key == "mode" && value != "standalone" && value != "sync")
        return "仅支持 standalone/sync";
    if (key == "render_backend" && !value.empty() &&
        value != "gl" && value != "d3d11")
        return "仅支持 gl/d3d11";
    static const char* kAccel[] = {"d3d11va", "cuda", "nvdec", "vaapi",
                                   "mediacodec", "auto", "soft", "off", "none"};
    if (key == "hw_accel") {
        for (const char* a : kAccel) if (value == a) return {};
        return "无法识别的 hw_accel";
    }
    // 整数键的数值/值域校验在 PUT 循环内联完成（端口类 0~65535 专项）
    return {};
}

// ---------------------------------------------------------------------------
// 本机显示器枚举（2026-11-13 网页"显示器"设置项配套）
//   * 序 = EnumDisplayMonitors 枚举序——与预览窗口定位（PreviewWindowWin32 /
//     GL 输出窗的 --screen=N → monitors[index]）严格同源，index 直接可用；
//   * GET /settings 输出 screens 数组（网页下拉数据源）；PUT display.screen
//     值域校验（-1=系统默认主屏；0..N-1=枚举序；越界 = 网页误写防呆——
//     窗口侧本有越界回退主屏兜底，此处拦截给出可选值提示）。
// ---------------------------------------------------------------------------
struct MonitorInfoLite { int w = 0; int h = 0; bool primary = false; };

std::vector<MonitorInfoLite> enumMonitors() {
    std::vector<MonitorInfoLite> out;
#ifdef _WIN32
    struct EnumCtx {
        std::vector<MonitorInfoLite>* out;
    };
    EnumCtx ctx{&out};
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR h, HDC, LPRECT, LPARAM lp) -> BOOL {
            auto* v = reinterpret_cast<EnumCtx*>(lp)->out;
            MONITORINFOEXW mi{};
            mi.cbSize = sizeof(mi);
            if (!GetMonitorInfoW(h, &mi)) return TRUE; // 单屏取信息失败：跳过
            MonitorInfoLite m;
            m.w = mi.rcMonitor.right - mi.rcMonitor.left;
            m.h = mi.rcMonitor.bottom - mi.rcMonitor.top;
            m.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
            v->push_back(m);
            return TRUE; // 继续枚举
        },
        reinterpret_cast<LPARAM>(&ctx));
#endif
    return out; // 非 Windows：空表（网页下拉只有"主屏"，screen 校验宽松放行）
}

/// display.screen 值域校验（validateSetting 专用；非法返回说明文案）
std::string validateScreenSetting(const std::string& value) {
    char* endp = nullptr;
    const long v = std::strtol(value.c_str(), &endp, 10);
    const bool numeric = !value.empty() && endp && *endp == '\0';
    if (!numeric) return "应为整数（-1=主屏，0..N-1=显示器序号）";
    const auto monitors = enumMonitors();
    // 枚举失败（0 屏）：宽松放行（窗口定位侧有越界回退主屏兜底）
    if (monitors.empty()) return v >= -1 ? std::string() : "应 >= -1（-1=主屏）";
    if (v >= -1 && v < static_cast<long>(monitors.size())) return {};
    return "超出范围（本机 " + std::to_string(monitors.size()) +
           " 块显示器：-1=主屏，0.." +
           std::to_string(static_cast<long>(monitors.size()) - 1) + "）";
}

}  // namespace

bool StandaloneApi::routeSettings(RouteCtx& ctx) {
    const bool is_get  = ctx.path == "/api/v1/settings" && ctx.method == "GET";
    const bool is_put  = ctx.path == "/api/v1/settings" &&
                         (ctx.method == "PUT" || ctx.method == "POST");
    if (!is_get && !is_put) return false;

    // config.ini 读写与文档落盘共用 io_mutex_ 串行（HTTP 工作线程并发：
    // 并发 PUT settings 逐键重写整文件会交叉丢更新，GET 也可能读到
    // 写一半的截断文件——2026-11-11 审查补锁，兑现 hpp 线程模型声明）
    std::lock_guard<std::mutex> io_lock(io_mutex_);

    // ---- GET：config.ini 快照（缺文件/缺键 = 内置默认——与运行时同源）----
    if (is_get) {
        const ms::config::IniConfig ini = cfg_.config_path.empty()
            ? ms::config::IniConfig{}
            : ms::config::IniConfig::load(cfg_.config_path);
        std::ostringstream data;
        data << "{\"mode\":\"" << json_escape(cfg_.mode) << "\",";
        // 顶层 screens：本机显示器枚举（2026-11-13 网页"显示器"设置项配套；
        // 序 = EnumDisplayMonitors，与 --screen=N / [display] screen 同源——
        // 网页下拉选中项直接可作 screen 值，重启 display 后窗口按此定位）
        {
            data << "\"screens\":[";
            const auto monitors = enumMonitors();
            for (std::size_t i = 0; i < monitors.size(); ++i) {
                if (i) data << ",";
                data << "{\"index\":" << i
                     << ",\"w\":" << monitors[i].w
                     << ",\"h\":" << monitors[i].h
                     << ",\"primary\":" << (monitors[i].primary ? "true"
                                                               : "false")
                     << "}";
            }
            data << "],";
        }
        auto emit_str = [&](const char* sec, const char* key) {
            data << "\"" << key << "\":\""
                 << json_escape(ini.getStr(sec, key)) << "\",";
        };
        auto emit_long = [&](const char* sec, const char* key, long fb) {
            data << "\"" << key << "\":" << ini.getLong(sec, key, fb) << ",";
        };
        // node：mode 的 ini 侧登记值（当前生效值见顶层 mode 字段）
        data << "\"node\":{";
        emit_str("node", "mode");
        data << "\"__stub\":1},";
        // display（数值缺键默认与 main Args 内置默认同源）
        data << "\"display\":{";
        emit_str("display", "player_id");   emit_long("display", "listen_port", 17100);
        emit_long("display", "discovery_port", 17610);
        emit_str("display", "assets_root"); emit_long("display", "asset_workers", 3);
        emit_long("display", "render_fps", 60);
        emit_str("display", "render_backend"); emit_str("display", "hw_accel");
        emit_long("display", "screen", -1); emit_long("display", "fullscreen", 0);
        emit_long("display", "borderless", 1);
        emit_long("display", "window_x", -1); emit_long("display", "window_y", -1);
        emit_long("display", "window_w", 0);  emit_long("display", "window_h", 0);
        emit_long("display", "report_interval_ms", 250);
        emit_long("display", "diag_interval_ms", 5000);
        emit_str("display", "pairing_code"); emit_long("display", "audio_enable", 1);
        emit_str("display", "audio_device");
        data << "\"__stub\":1},";
        // standalone
        data << "\"standalone\":{";
        emit_long("standalone", "http_port", 8995);
        emit_long("standalone", "max_upload_mb", 2048);
        emit_str("standalone", "timelines_dir");
        emit_str("standalone", "default_timeline");
        data << "\"__stub\":1}}";
        send_json(ctx.resp, 200, ok_json(data.str()));
        return true;
    }

    // ---- PUT：逐键校验 + saveKeyToFile 落盘（need_restart 提示重启）-------
    Poco::JSON::Parser parser;
    try {
        const auto root = parser.parse(read_body(ctx.req));
        const auto obj = root.extract<Poco::JSON::Object::Ptr>();
        int saved = 0;
        std::string errors;
        for (const auto& kvSec : *obj) {
            const std::string sec = kvSec.first;
            if (sec != "node" && sec != "display" && sec != "standalone") {
                errors += sec + ":非法分组; ";
                continue;
            }
            const auto secObj = kvSec.second.extract<Poco::JSON::Object::Ptr>();
            if (secObj.isNull()) continue;
            for (const auto& kv : *secObj) {
                const std::string key = kv.first;
                std::string value;
                try { value = kv.second.convert<std::string>(); }
                catch (...) {
                    errors += sec + "." + key + ":类型非法; ";
                    continue;
                }
                // 白名单校验（键必须登记；整数键再做数值/值域检查）
                bool known = false, is_int = false;
                for (const auto& [sname, keys] : settingSchema()) {
                    if (sname != sec) continue;
                    for (const auto& k : keys)
                        if (key == k.key) { known = true; is_int = k.is_int; break; }
                    break;
                }
                if (!known) {
                    errors += sec + "." + key + ":未知键;";
                    continue;
                }
                if (const std::string bad = validateSetting(sec, key, value);
                    !bad.empty()) {
                    errors += sec + "." + key + ":" + bad + ";";
                    continue;
                }
                if (is_int) {
                    bool numeric = !value.empty() &&
                        value.find_first_not_of("-0123456789") == std::string::npos;
                    if (!numeric) { errors += sec + "." + key + ":非整数;"; continue; }
                    const long v = std::atol(value.c_str());
                    const bool port_like =
                        key.find("port") != std::string::npos;
                    if (port_like && (v < 0 || v > 65535)) {
                        errors += sec + "." + key + ":端口越界;"; continue;
                    }
                    if (!port_like && (v < -1 || v > 100000000)) {
                        errors += sec + "." + key + ":值域越界;"; continue;
                    }
                    // display.screen 专项值域（-1=主屏，0..N-1=枚举序——
                    // 与预览窗口定位同源；详见 validateScreenSetting）
                    if (sec == "display" && key == "screen") {
                        if (const std::string bad = validateScreenSetting(value);
                            !bad.empty()) {
                            errors += sec + "." + key + ":" + bad + ";";
                            continue;
                        }
                    }
                }
                if (!ms::config::saveKeyToFile(cfg_.config_path, sec, key,
                                               value)) {
                    errors += sec + "." + key + ":落盘失败;";
                    continue;
                }
                ++saved;
            }
        }
        std::ostringstream data;
        data << "{\"saved\":" << saved
             << ",\"need_restart\":true"
             << ",\"errors\":\"" << json_escape(errors) << "\"}";
        send_json(ctx.resp, errors.empty() ? 200 : 207,
                  ok_json(data.str()));
        return true;
    } catch (const Poco::Exception& ex) {
        send_json(ctx.resp, 400, err_json("BAD_REQUEST",
                                          "bad json: " + ex.displayText()));
        return true;
    }
}

// ---------------------------------------------------------------------------
// system：进程重启（M5 编辑器互通 2026-11-11）
//   POST /api/v1/system/restart —— 网页设置页"模式切换/端口修改"后的一键
//   生效动作。两模式均开放（sync 切回 standalone 也走这里）。重启实现由
//   main 注入（置原子标志 → 主循环 break → 优雅退出链析构全部资源 →
//   CreateProcess 拉起新实例接力）——本路由只负责鉴权语义与响应：
//   restart_fn 未装配（单测/嵌入式宿主）返回 503，不误导前端。
//   时序纪律：restart_fn 必须立即返回（HTTP 工作线程，响应尚未 flush；
//   实际退出由主循环承担，天然留给响应 ~10ms 的送达窗口）。
// ---------------------------------------------------------------------------
bool StandaloneApi::routeSystem(RouteCtx& ctx) {
    if (ctx.path != "/api/v1/system/restart" || ctx.method != "POST")
        return false;
    if (!cfg_.restart_fn) {
        send_json(ctx.resp, 503, err_json(
            "RESTART_UNSUPPORTED",
            "当前宿主未装配重启能力（restart_fn 为空）"));
        return true;
    }
    cfg_.restart_fn();
    send_json(ctx.resp, 200, ok_json("{\"restarting\":true}"));
    return true;
}

// ---------------------------------------------------------------------------
// timelines CRUD + play / set-default
// ---------------------------------------------------------------------------
bool StandaloneApi::routeTimelines(RouteCtx& ctx) {
    const std::string& path = ctx.path;
    const std::string& method = ctx.method;

    // ---- 集合操作 -----------------------------------------------------------
    if (path == "/api/v1/timelines") {
        if (method == "GET") {
            // 列表（含活跃/默认标记与时长概要）
            std::ostringstream items;
            items << "[";
            bool first = true;
            for (const auto& e : registry_.list()) {
                if (!first) items << ",";
                first = false;
                const std::int64_t dur = e.stop_position_us > 0
                                             ? e.stop_position_us
                                             : (e.scene ? e.scene->timeline.duration_us : 0);
                items << "{\"id\":\"" << json_escape(e.id) << "\","
                      << "\"name\":\"" << json_escape(e.name) << "\","
                      << "\"loop\":" << (e.loop ? "true" : "false") << ","
                      << "\"stop_position_us\":" << e.stop_position_us << ","
                      << "\"duration_us\":" << dur << ","
                      << "\"is_active\":" << (registry_.activeId() == e.id ? "true" : "false") << ","
                      << "\"is_default\":" << (registry_.defaultId() == e.id ? "true" : "false");
                // [round3 S2] 装配状态 note（非空 = 降级装载/不可直接播放），
                // 网页列表据此展示失效标记
                if (!e.note.empty())
                    items << ",\"note\":\"" << json_escape(e.note) << "\"";
                items << "}";
            }
            items << "]";
            send_json(ctx.resp, 200, ok_json(items.str()));
            return true;
        }
        if (method == "POST") {
            const auto body = read_body(ctx.req);
            // [round7 S3-1] 文档大小门禁：时间轴文档是编排意图（KB 级），
            // 媒体字节走 /library 上传（max_upload_mb 门禁）——超上限的
            // body 只可能是误用/攻击，413 拒收（对齐上传/导入门禁风格）
            if (body.size() > kMaxTimelineDocBytes) {
                send_json(ctx.resp, 413, err_json(
                    "PAYLOAD_TOO_LARGE",
                    "时间轴文档超上限（" +
                        std::to_string(kMaxTimelineDocBytes / 1024) +
                        "KB）——媒体文件请走 /api/v1/library 上传"));
                return true;
            }
            TimelineDoc doc;
            std::string err;
            if (!parseDoc(body, doc, err)) {
                send_json(ctx.resp, 400, err_json("BAD_REQUEST", err));
                return true;
            }
            // [R8-3] 名字段非空校验（双层防线的后端层：防其他客户端直写）
            if (doc.name.empty()) {
                send_json(ctx.resp, 400, err_json(
                    "NAME_INVALID", "时间轴名称不能为空（纯空格不接受）"));
                return true;
            }
            // 装配 best effort：空时间轴（新建初始态——必然先建空再编排）/
            // 素材未就位均允许创建。文档=编辑态真相，Scene=运行态产物
            //（play 时 NodeController 校验 entry.scene，null 报"未装配场景"）
            std::string assemble_note;
            auto scene = assembleDoc(doc, assemble_note);
            if (scene == nullptr)   // 非致命：note 随创建响应提示（不阻塞）
                assemble_note = "时间轴暂无可播放片段（添加素材并保存后可播）";

            ms::standalone::TimelineEntry entry;
            entry.name = doc.name;
            entry.scene = std::move(scene);
            entry.loop = doc.loop;
            entry.stop_position_us = doc.stop_position_us;
            // [round3 S2] 装配 note 随条目登记（列表保留可见性）
            entry.note = assemble_note;

            // id：请求体显式给（可选）> 自动生成；保存文件前先定 id
            {
                Poco::JSON::Parser parser;
                try {
                    const auto obj = parser.parse(body).extract<Poco::JSON::Object::Ptr>();
                    entry.id = varStr(obj, "id");
                } catch (...) {
                }
                // S2-1：显式 id 即文档文件名——穿越/非法字符一律 400（注册
                // 表零副作用；自动生成路径不含非法字符无需校验）
                if (!entry.id.empty()) {
                    std::string id_err;
                    if (!validTimelineId(entry.id, id_err)) {
                        send_json(ctx.resp, 400, err_json("ID_INVALID", id_err));
                        return true;
                    }
                }
            }

            std::lock_guard<std::mutex> lock(io_mutex_);  // id 生成→登记→落盘原子
            const std::string id = registry_.add(std::move(entry));
            if (id.empty()) {
                send_json(ctx.resp, 409, err_json("CONFLICT", "id 冲突"));
                return true;
            }
            // [S2] 大小写折叠查重（NTFS 不区分大小写）：对最终 id（显式或
            // 自动生成的 tl-N——纯小写也可能与磁盘上 "TL-N.json" 折叠同名）
            // 做文件系统级折叠比对，命中即回滚登记并 409——否则落盘会静默
            // 顶掉磁盘上折叠同名的既有文档（含重启装载失败未登记者）。
            {
                std::string fold_hit;
                if (timelineIdCasefoldExists(id, fold_hit)) {
                    registry_.remove(id);  // 回滚登记（零半态）
                    send_json(ctx.resp, 409,
                              err_json("CONFLICT",
                                       "id 与既有文档大小写折叠冲突: " + fold_hit));
                    return true;
                }
            }
            // 换回 doc 的 name（add 移动了 entry——文档落盘用本地 doc 即可）
            std::string serr;
            if (!saveDocLocked(id, doc, serr)) {
                registry_.remove(id);  // 落盘失败不留半态（回滚登记）
                send_json(ctx.resp, 500, err_json("INTERNAL", serr));
                return true;
            }
            std::ostringstream data;
            data << "{\"created\":true,\"id\":\"" << json_escape(id) << "\"";
            if (!assemble_note.empty())
                data << ",\"note\":\"" << json_escape(assemble_note) << "\"";
            // [W1] 新文档版本号从 0 起（前端首次 PUT 携带 0，成功后 bump）
            data << ",\"rev\":0}";
            send_json(ctx.resp, 200, ok_json(data.str()));
            return true;
        }
        return false;
    }

    // ---- .show 工程导入（M5 工程互通；standalone 门禁由 handle 收敛）--------
    // multipart（name=file）优先 / raw 兜底（body 即 .show JSON 文本）。
    // ShowFileIO 校验（pb 解析 + Scene 引用完整性）→ sceneToDoc → 注册+落盘。
    if (path == "/api/v1/timelines/import" && method == "POST") {
        Poco::Net::HTTPServerRequest& req = ctx.req;
        const std::size_t max_bytes =
            cfg_.max_upload_mb == 0
                ? (std::numeric_limits<std::size_t>::max)()
                : cfg_.max_upload_mb * 1024ull * 1024ull;

        std::string payload;   // .show JSON 文本
        std::string src_name = "import.show";

        if (req.getContentType().find("multipart/form-data") !=
            std::string::npos) {
            // 文件部件入内存（.show 量级几十 KB～几 MB，直读足够）
            class ShowPart : public Poco::Net::PartHandler {
            public:
                explicit ShowPart(std::size_t max_bytes)
                    : max_bytes_(max_bytes) {}
                void handlePart(const Poco::Net::MessageHeader& header,
                                std::istream& stream) override {
                    if (got_) {
                        std::stringstream sink;
                        Poco::StreamCopier::copyStream(stream, sink);
                        return;
                    }
                    std::string disp = header.has("Content-Disposition")
                                           ? header.get("Content-Disposition")
                                           : "";
                    if (disp.find("name=\"file\"") == std::string::npos &&
                        disp.find("name=file") == std::string::npos) {
                        std::stringstream sink;
                        Poco::StreamCopier::copyStream(stream, sink);
                        return;
                    }
                    auto pos = disp.find("filename=");
                    if (pos != std::string::npos) {
                        pos += 9;
                        if (pos < disp.size() && disp[pos] == '\"') {
                            ++pos;
                            auto end = disp.find('\"', pos);
                            filename = end == std::string::npos
                                           ? disp.substr(pos)
                                           : disp.substr(pos, end - pos);
                        } else {
                            auto end = disp.find_first_of(" ;", pos);
                            filename = end == std::string::npos
                                           ? disp.substr(pos)
                                           : disp.substr(pos, end - pos);
                        }
                    }
                    char buf[64 * 1024];
                    while (stream.good()) {
                        stream.read(buf, sizeof(buf));
                        auto n = stream.gcount();
                        if (n <= 0) break;
                        data.append(buf, static_cast<std::size_t>(n));
                        if (data.size() > max_bytes_) {
                            err = "upload too large";
                            return;
                        }
                    }
                    got_ = true;
                }
                std::size_t max_bytes_;
                std::string data;
                std::string filename;
                std::string err;
                bool got_ = false;
            };
            ShowPart part(max_bytes);
            try {
                Poco::Net::HTMLForm form(req, req.stream(), part);
            } catch (const Poco::Exception& ex) {
                send_json(ctx.resp, 400,
                          err_json("BAD_REQUEST", ex.displayText()));
                return true;
            }
            if (!part.err.empty()) {
                send_json(ctx.resp, 413, err_json("BAD_REQUEST", part.err));
                return true;
            }
            if (!part.got_) {
                send_json(ctx.resp, 400,
                          err_json("BAD_REQUEST", "missing multipart file"));
                return true;
            }
            payload = std::move(part.data);
            if (!part.filename.empty()) src_name = part.filename;
        } else {
            payload = read_body(req);
        }

        // .show 文本 → ShowDocument（pb JSON + 引用完整性校验）
        std::string serr;
        auto show = ms::ShowFileIO::fromJsonText(payload, serr);
        if (!show) {
            send_json(ctx.resp, 400, err_json("BAD_REQUEST", serr));
            return true;
        }

        // Scene → 网页文档（Live/复杂 tween 降级说明进 note）
        TimelineDoc doc;
        std::string note;
        if (!sceneToDoc(show->scene, doc, note)) {
            send_json(ctx.resp, 400,
                      err_json("BAD_REQUEST",
                               "工程无有效轨道" +
                                   (note.empty() ? "" : "（" + note + "）")));
            return true;
        }
        // name：.show 无时间轴名语义 → 文件名去扩展名
        {
            std::string base = src_name;
            auto slash = base.find_last_of("/\\");
            if (slash != std::string::npos) base = base.substr(slash + 1);
            auto dot = base.find_last_of('.');
            if (dot != std::string::npos && dot > 0) base = base.substr(0, dot);
            if (!base.empty()) doc.name = base;
        }
        // 装配 best effort（与创建分支同纪律：play 时才校验）
        std::string assemble_note;
        auto scene = assembleDoc(doc, assemble_note);
        if (scene == nullptr)
            assemble_note = "导入工程暂无可播放片段（素材须已上传媒体库）";

        ms::standalone::TimelineEntry entry;
        entry.name = doc.name;
        entry.scene = std::move(scene);
        entry.loop = doc.loop;
        entry.stop_position_us = doc.stop_position_us;
        // [round3 S2] 装配 note 随条目登记（列表保留可见性）
        entry.note = assemble_note;

        std::lock_guard<std::mutex> lock(io_mutex_);  // 登记→落盘原子
        const std::string id = registry_.add(std::move(entry));
        if (id.empty()) {
            send_json(ctx.resp, 409, err_json("CONFLICT", "id 分配失败"));
            return true;
        }
        // [S2] 大小写折叠查重（同创建分支）：命中回滚登记并 409
        {
            std::string fold_hit;
            if (timelineIdCasefoldExists(id, fold_hit)) {
                registry_.remove(id);
                send_json(ctx.resp, 409,
                          err_json("CONFLICT",
                                   "id 与既有文档大小写折叠冲突: " + fold_hit));
                return true;
            }
        }
        std::string derr;
        if (!saveDocLocked(id, doc, derr)) {
            registry_.remove(id);  // 落盘失败不留半态
            send_json(ctx.resp, 500, err_json("INTERNAL", derr));
            return true;
        }
        std::ostringstream data;
        data << "{\"imported\":true,\"id\":\"" << json_escape(id)
             << "\",\"name\":\"" << json_escape(doc.name) << "\"";
        const std::string merged = assemble_note.empty() ? note
                                   : (note.empty() ? assemble_note
                                                   : note + "\n" + assemble_note);
        if (!merged.empty())
            data << ",\"note\":\"" << json_escape(merged) << "\"";
        data << "}";
        send_json(ctx.resp, 200, ok_json(data.str()));
        return true;
    }

    // ---- 单条操作：/api/v1/timelines/{id}[/play|/set-default] ---------------
    static const std::string kPrefix = "/api/v1/timelines/";
    if (path.rfind(kPrefix, 0) != 0 || path.size() <= kPrefix.size()) return false;
    std::string rest = url_decode(path.substr(kPrefix.size()));
    // S2-1：URL id 在 url_decode 之后统一校验（%2F 展开的 "../" 在此拦截）
    // ——穿越/非法字符一律 400，不触达注册表与文档路径
    const auto denyBadId = [this, &ctx](const std::string& id) {
        std::string id_err;
        if (validTimelineId(id, id_err)) return false;
        send_json(ctx.resp, 400, err_json("ID_INVALID", id_err));
        return true;
    };

    // 子动作
    if (rest.size() > 5 && rest.compare(rest.size() - 5, 5, "/play") == 0) {
        const std::string id = rest.substr(0, rest.size() - 5);
        if (method != "POST") return false;
        if (denyBadId(id)) return true;
        std::string err;
        if (!node_.play(id, err)) {
            // 【2026-11-13】find 一次复用（原 find 两次）；错误码语义不变：
            // 条目存在但播不了 = ASSEMBLE_FAILED/500，不存在 = NOT_FOUND/404
            const auto entry = registry_.find(id);
            send_json(ctx.resp, entry ? 500 : 404,
                      err_json(entry ? "ASSEMBLE_FAILED" : "NOT_FOUND", err));
            return true;
        }
        send_json(ctx.resp, 200, ok_json("{\"playing\":true,\"id\":\"" + json_escape(id) + "\"}"));
        return true;
    }
    if (rest.size() > 12 && rest.compare(rest.size() - 12, 12, "/set-default") == 0) {
        const std::string id = rest.substr(0, rest.size() - 12);
        if (method != "POST") return false;
        if (denyBadId(id)) return true;
        if (!registry_.setDefault(id)) {
            send_json(ctx.resp, 404, err_json("NOT_FOUND", "时间轴不存在: " + id));
            return true;
        }
        // 持久化到 config.ini（开机自动播放恢复目标；v2.2 定案）
        std::string serr;
        if (!cfg_.config_path.empty()) {
            std::lock_guard<std::mutex> io_lock(io_mutex_);  // config 落盘串行
            if (!ms::config::saveKeyToFile(cfg_.config_path, "standalone",
                                           kDefaultKey, id)) {
                serr = "（config.ini 落盘失败，默认登记仅本次运行生效）";
            }
        }
        send_json(ctx.resp, 200,
                  ok_json("{\"default\":\"" + json_escape(id) + "\",\"note\":\"" +
                          json_escape(serr) + "\"}"));
        return true;
    }

    // ---- .show 工程导出（M5 工程互通；文档=编辑态真相）----------------------
    //   GET /api/v1/timelines/{id}/export → .show JSON 附件下载。
    //   有文档：loadDoc → assembleDoc（运行时同源 Scene，含素材元数据表）；
    //   无文档（ini/.show 装配的 readonly 条目）：注册表已有场景直出。
    if (rest.size() > 7 && rest.compare(rest.size() - 7, 7, "/export") == 0) {
        const std::string id = rest.substr(0, rest.size() - 7);
        if (method != "GET") return false;
        if (denyBadId(id)) return true;
        ms::Scene scene;
        std::string err;
        TimelineDoc doc;
        if (loadDoc(id, doc, err)) {
            std::string assemble_err;
            auto s = assembleDoc(doc, assemble_err);
            if (s) {
                scene = std::move(*s);
            } else {
                // 文档存在但装配失败（素材缺失等）——退注册表最近场景
                const auto e = registry_.find(id);
                if (e == nullptr || e->scene == nullptr) {
                    send_json(ctx.resp, 400,
                              err_json("ASSEMBLE_FAILED",
                                       "工程装配失败，无法导出: " + assemble_err));
                    return true;
                }
                scene = *e->scene;
            }
        } else {
            const auto e = registry_.find(id);
            if (e == nullptr || e->scene == nullptr) {
                send_json(ctx.resp, 404,
                          err_json("NOT_FOUND", "时间轴不存在: " + id));
                return true;
            }
            scene = *e->scene;
        }
        ms::ShowDocument show;
        show.scene = std::move(scene);
        // ---- 混合轨道拆分（.show R2 类型约束适配）---------------------------
        // 网页 MVP 轨道是混合轨（装配端 kind=Unknown），而 .show 的 R2 校验
        // 要求轨道 kind 非 Unknown 且与 clip 素材类型一致。导出侧按素材类型
        // 把混合轨拆成子轨；[S2] 拆分会改变轨序——同步重编 z（组序+组内
        // rank），保证导入回环层次单调可逆（算法详见 splitTracksForExport）。
        splitTracksForExport(show.scene);
        std::string payload, serr;
        if (!ms::ShowFileIO::toJsonText(show, payload, serr)) {
            send_json(ctx.resp, 500, err_json("INTERNAL", serr));
            return true;
        }
        // 附件名用 id（ASCII 安全；浏览器另存时可改中文名）
        ctx.resp.set("Content-Disposition",
                     "attachment; filename=\"" + id + ".show\"");
        send_text(ctx.resp, 200, "application/octet-stream", payload);
        return true;
    }

    const std::string id = rest;
    if (denyBadId(id)) return true;   // S2-1：GET/PUT/DELETE 的 {id} 统一校验
    if (method == "GET") {
        TimelineDoc doc;
        std::string err;
        if (!loadDoc(id, doc, err)) {
            // ini/.show 装配的时间轴无 JSON 文档——回条目概要（场景在注册表）
            const auto e = registry_.find(id);
            if (e == nullptr) {
                send_json(ctx.resp, 404, err_json("NOT_FOUND", err));
            } else {
                std::ostringstream data;
                data << "{\"id\":\"" << json_escape(e->id) << "\","
                     << "\"name\":\"" << json_escape(e->name) << "\","
                     << "\"loop\":" << (e->loop ? "true" : "false") << ","
                     << "\"stop_position_us\":" << e->stop_position_us << ","
                     << "\"scene\":null,\"readonly\":true}";
                send_json(ctx.resp, 200, ok_json(data.str()));
            }
            return true;
        }
        send_json(ctx.resp, 200, ok_json(docToJson(id, doc)));
        return true;
    }
    if (method == "PUT") {
        TimelineDoc doc;
        std::string err;
        const auto body = read_body(ctx.req);
        // [round7 S3-1] 文档大小门禁（同 POST 路由注释）
        if (body.size() > kMaxTimelineDocBytes) {
            send_json(ctx.resp, 413, err_json(
                "PAYLOAD_TOO_LARGE",
                "时间轴文档超上限（" +
                    std::to_string(kMaxTimelineDocBytes / 1024) +
                    "KB）——媒体文件请走 /api/v1/library 上传"));
            return true;
        }
        if (!parseDoc(body, doc, err)) {
            send_json(ctx.resp, 400, err_json("BAD_REQUEST", err));
            return true;
        }
        // [R8-3] 名字段非空校验（同 POST；parseDoc 已 trim）
        if (doc.name.empty()) {
            send_json(ctx.resp, 400, err_json(
                "NAME_INVALID", "时间轴名称不能为空（纯空格不接受）"));
            return true;
        }
        // 装配 best effort（同 POST）：清空重编/素材未就位均允许保存——
        // 文档落盘成功即保存成功；装配失败 entry.scene=null（播放时报
        // "未装配场景"，显式失效优于静默播旧内容）
        std::string assemble_note;
        auto scene = assembleDoc(doc, assemble_note);
        if (scene == nullptr)   // 非致命：note 随保存响应提示（不阻塞落盘）
            assemble_note = "时间轴暂无可播放片段（添加素材并保存后可播）";
        // 活跃时间轴热更新：直接换场景槽（编排变化即时生效）；播放头由
        // session 时钟连续推进，不回锚（理由见下方 applyLocalScene 注释）
        const bool is_active = (registry_.activeId() == id);

        ms::standalone::TimelineEntry entry;
        entry.id = id;
        entry.name = doc.name;
        entry.scene = std::move(scene);
        entry.loop = doc.loop;
        entry.stop_position_us = doc.stop_position_us;
        // [round3 S2] 装配 note 随条目登记（列表保留可见性）
        entry.note = assemble_note;
        // S3-1（2026-11-11 测试清单）：对齐 POST 分支纪律——"登记→落盘"
        // 复合原子。落盘失败回滚到旧条目，否则注册表已新/磁盘仍旧，进程
        // 重启（从磁盘装载）后编排内容"回跳"。持 io_mutex_ 期间必须调
        // saveDocLocked（saveDoc 重锁同一把 = 自死锁铁律，M2 实测踩坑）。
        const auto old = registry_.find(id);  // 旧条目快照（回滚源；shared_ptr 不悬垂）
        if (old == nullptr) {
            send_json(ctx.resp, 404, err_json("NOT_FOUND", "时间轴不存在: " + id));
            return true;
        }
        {
            std::lock_guard<std::mutex> io_lock(io_mutex_);
            // [W1] 乐观锁：提交 rev 与磁盘现行 rev 比较——不等即另一窗口
            // 已保存过（双标签页整文档 PUT 互相覆盖防线），409 让前端刷新
            // 重取。比较在锁内完成（读 rev → 比较 → 写回 bump 复合原子；
            // loadDocLocked 与 saveDocLocked 均为锁内版，不得重入外层）。
            // 兼容：旧文档/旧客户端双方 rev=0 相等放行，首次保存后 bump。
            TimelineDoc disk_doc;
            std::string lerr;
            std::int64_t new_rev = 0;
            if (loadDocLocked(id, disk_doc, lerr)) {
                if (doc.rev != disk_doc.rev) {
                    send_json(ctx.resp, 409,
                              err_json("CONFLICT",
                                       "文档已被其他窗口修改（rev 冲突），请刷新后重试"));
                    return true;
                }
                new_rev = disk_doc.rev + 1;
            }
            // 磁盘无文档（ini/.show 只读条目等）→ 不做锁校验，rev 从 0 起
            doc.rev = new_rev;
            if (!registry_.update(entry)) {
                // 理论不可达（find 已确认存在，并发 DELETE 下兜底同语义）
                send_json(ctx.resp, 404, err_json("NOT_FOUND", "时间轴不存在: " + id));
                return true;
            }
            if (!saveDocLocked(id, doc, err)) {
                registry_.update(*old);  // 落盘失败回滚登记（旧快照整体替换回来）
                send_json(ctx.resp, 500, err_json("INTERNAL", err));
                return true;
            }
        }
        if (is_active && entry.scene) {
            // 场景已替换（update 直写注册表条目——活跃指针持 shared_ptr
            // 旧快照）；装配失败（null）不注入——活跃场景保持旧内容，
            // 下次 play 重校验。
            // 【不回锚 seek（2026-11-11 "调 crop/位置易全黑"修复）】播放头
            // 由 session 时钟连续推进，场景热换后 media_us 映射天然连续；
            // 旧实现每次保存都 localSeek(resume_us)——解码游标重定位
            //（关键帧重开）期间渲染端 no_frame → 黑帧，而属性面板逐键
            // PUT，播放中调 crop/x/y/w/h 就频繁闪黑。时间结构变化导致的
            // 画面内容跳变是编排语义（预期），越界由出点逻辑收敛。
            player_.applyLocalScene(entry.scene);
        }
        std::ostringstream data;
        data << "{\"updated\":true,\"id\":\"" << json_escape(id) << "\"";
        if (!assemble_note.empty())
            data << ",\"note\":\"" << json_escape(assemble_note) << "\"";
        // [W1] 回传新版本号——前端保存成功后刷新本地 rev（下次 PUT 携带）
        data << ",\"rev\":" << doc.rev;
        data << "}";
        send_json(ctx.resp, 200, ok_json(data.str()));
        return true;
    }
    if (method == "DELETE") {
        if (registry_.find(id) == nullptr) {
            send_json(ctx.resp, 404, err_json("NOT_FOUND", "时间轴不存在: " + id));
            return true;
        }
        if (registry_.activeId() == id) node_.stop();  // 活跃先停（播放头归零）
        registry_.remove(id);
        std::string err;
        removeDoc(id, err);  // 文档删除失败不阻断（列表已一致）
        send_json(ctx.resp, 200, ok_json("{\"deleted\":true,\"id\":\"" + json_escape(id) + "\"}"));
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// playback
// ---------------------------------------------------------------------------
bool StandaloneApi::routePlayback(RouteCtx& ctx) {
    const std::string& path = ctx.path;
    const std::string& method = ctx.method;
    if (path.rfind("/api/v1/playback/", 0) != 0 || method != "POST") return false;
    const std::string action = path.substr(17);

    if (action == "pause") {
        node_.pause();
        send_json(ctx.resp, 200, ok_json("{\"paused\":true}"));
        return true;
    }
    if (action == "resume") {
        // 续播 = 从当前播放头重锚起播（localPlay 立即锚点；Playing 态幂等）
        node_.resume();
        send_json(ctx.resp, 200, ok_json("{\"playing\":true}"));
        return true;
    }
    if (action == "seek") {
        const auto body = read_body(ctx.req);
        double v = 0;
        if (!extract_number_field(body, "to_us", v) || v < 0 ||
            v > 9.0e18) {  // 上界防 double→int64 溢出 UB（int64 最大 ~9.22e18）
            send_json(ctx.resp, 400, err_json("BAD_REQUEST", "missing to_us"));
            return true;
        }
        node_.seek(static_cast<std::int64_t>(v));
        send_json(ctx.resp, 200, ok_json("{\"seeked\":true}"));
        return true;
    }
    if (action == "stop") {
        node_.stop();
        send_json(ctx.resp, 200, ok_json("{\"stopped\":true}"));
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// 媒体库
// ---------------------------------------------------------------------------
void StandaloneApi::handleUpload(RouteCtx& ctx) {
    Poco::Net::HTTPServerRequest& req = ctx.req;
    Poco::Net::HTTPServerResponse& resp = ctx.resp;

    const std::string ct = req.getContentType();
    const std::size_t max_bytes =
            cfg_.max_upload_mb == 0 ? (std::numeric_limits<std::size_t>::max)()
                                    : cfg_.max_upload_mb * 1024ull * 1024ull;

    auto send_ok = [&](const MediaItem& item) {
        std::ostringstream data;
        data << "{\"uploaded\":true,\"id\":\"" << json_escape(item.id) << "\","
             << "\"name\":\"" << json_escape(item.name) << "\","
             << "\"size\":" << item.size << ","
             << "\"kind\":\"" << json_escape(item.kind) << "\","
             << "\"count\":" << library_.count() << "}";
        send_json(resp, 200, ok_json(data.str()));
    };

    if (ct.find("multipart/form-data") != std::string::npos) {
        // multipart 文件部件三段式接收（收编 MMC UploadPartHandler）
        class UploadPart : public Poco::Net::PartHandler {
        public:
            explicit UploadPart(MediaLibrary& lib, std::size_t max_bytes)
                : lib_(lib), max_bytes_(max_bytes) {}

            void handlePart(const Poco::Net::MessageHeader& header, std::istream& stream) override {
                if (got_file_) return;
                std::string disp =
                        header.has("Content-Disposition") ? header.get("Content-Disposition") : "";
                if (disp.find("name=\"file\"") == std::string::npos &&
                    disp.find("name=file") == std::string::npos) {
                    std::stringstream sink;
                    Poco::StreamCopier::copyStream(stream, sink);
                    return;
                }

                std::string filename;
                auto pos = disp.find("filename=");
                if (pos != std::string::npos) {
                    pos += 9;
                    if (pos < disp.size() && disp[pos] == '\"') {
                        ++pos;
                        auto end = disp.find('\"', pos);
                        filename = end == std::string::npos ? disp.substr(pos)
                                                            : disp.substr(pos, end - pos);
                    } else {
                        auto end = disp.find_first_of(" ;", pos);
                        filename = end == std::string::npos ? disp.substr(pos)
                                                            : disp.substr(pos, end - pos);
                    }
                }
                if (filename.empty()) { err_ = "missing filename"; return; }

                if (!lib_.begin_upload(filename, writer_, err_)) return;
                char buf[64 * 1024];
                while (stream.good()) {
                    stream.read(buf, sizeof(buf));
                    auto n = stream.gcount();
                    if (n <= 0) break;
                    if (!lib_.write_upload_chunk(writer_, buf, static_cast<std::size_t>(n),
                                                 max_bytes_, err_)) {
                        lib_.abort_upload(writer_);
                        return;
                    }
                }
                got_file_ = true;
            }

            MediaLibrary& lib_;
            std::size_t max_bytes_;
            UploadWriter writer_;
            bool got_file_ = false;
            std::string err_;
        };

        UploadPart part(library_, max_bytes);
        try {
            Poco::Net::HTMLForm form(req, req.stream(), part);
        } catch (const Poco::Exception& ex) {
            send_json(resp, 400, err_json("BAD_REQUEST", ex.displayText()));
            return;
        }
        if (!part.err_.empty()) {
            send_json(resp, part.err_ == "upload too large" ? 413 : 400,
                      err_json("BAD_REQUEST", part.err_));
            return;
        }
        if (!part.got_file_) {
            send_json(resp, 400, err_json("BAD_REQUEST", "missing multipart file"));
            return;
        }
        MediaItem item;
        std::string err;
        if (!library_.finish_upload(part.writer_, item, err)) {
            send_json(resp, 400, err_json("BAD_REQUEST", err.empty() ? "upload failed" : err));
            return;
        }
        send_ok(item);
        return;
    }

    // ---- raw octet-stream（X-Filename 头或 ?filename= 查询）------------------
    std::string filename;
    for (const auto& kv : ctx.uri.getQueryParameters()) {
        if (kv.first == "filename") filename = kv.second;
    }
    if (filename.empty() && req.has("X-Filename")) filename = req.get("X-Filename");
    if (filename.empty()) {
        send_json(resp, 400, err_json("BAD_REQUEST", "missing filename"));
        return;
    }

    UploadWriter writer;
    std::string err;
    if (!library_.begin_upload(filename, writer, err)) {
        send_json(resp, err == "path denied" ? 400 : 400,
                  err_json(err == "path denied" ? "PATH_DENIED" : "BAD_REQUEST", err));
        return;
    }
    char buf[64 * 1024];
    auto& stream = req.stream();
    while (stream.good()) {
        stream.read(buf, sizeof(buf));
        auto n = stream.gcount();
        if (n <= 0) break;
        if (!library_.write_upload_chunk(writer, buf, static_cast<std::size_t>(n), max_bytes, err)) {
            library_.abort_upload(writer);
            send_json(resp, err == "upload too large" ? 413 : 400, err_json("BAD_REQUEST", err));
            return;
        }
    }
    MediaItem item;
    if (!library_.finish_upload(writer, item, err)) {
        send_json(resp, 400, err_json("BAD_REQUEST", err.empty() ? "upload failed" : err));
        return;
    }
    send_ok(item);
}

bool StandaloneApi::routeLibrary(RouteCtx& ctx) {
    const std::string& path = ctx.path;
    const std::string& method = ctx.method;

    if (path == "/api/v1/library" && method == "GET") {
        std::string q;
        for (const auto& kv : ctx.uri.getQueryParameters()) {
            if (kv.first == "q") q = kv.second;
        }
        send_json(ctx.resp, 200, ok_json(library_.list_json(q)));
        return true;
    }
    if (path == "/api/v1/library/refresh" && method == "POST") {
        library_.refresh();
        send_json(ctx.resp, 200, ok_json(library_.list_json("")));
        return true;
    }
    if (path == "/api/v1/library/upload" && method == "POST") {
        handleUpload(ctx);
        return true;
    }

    static const std::string kPrefix = "/api/v1/library/";
    if (path.rfind(kPrefix, 0) != 0 || path.size() <= kPrefix.size()) return false;
    const std::string id = url_decode(path.substr(kPrefix.size()));

    if (method == "GET") {
        MediaItem item;
        if (!library_.find(id, item)) {
            send_json(ctx.resp, 404, err_json("NOT_FOUND", "media not found"));
            return true;
        }
        send_json(ctx.resp, 200, ok_json(library_.item_json(item)));
        return true;
    }
    if (method == "DELETE") {
        std::string err;
        if (!library_.remove(id, err)) {
            send_json(ctx.resp, err == "media not found" ? 404 : 500,
                      err_json(err == "media not found" ? "NOT_FOUND" : "INTERNAL", err));
            return true;
        }
        std::ostringstream data;
        data << "{\"deleted\":true,\"id\":\"" << json_escape(id)
             << "\",\"count\":" << library_.count() << "}";
        send_json(ctx.resp, 200, ok_json(data.str()));
        return true;
    }
    return false;
}

}  // namespace media_api
