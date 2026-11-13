// ============================================================================
//  SceneLoader.hpp —— 场景序列化/反序列化与引用完整性校验（Core 组件）
//
//  设计依据：《media-server-sync-design.md》§3 / §5.1
//    * 职责：Protobuf Scene ↔ 运行时内存模型（SceneModel）的双向转换；
//      Editor 侧 TimelineExporter 下发前经 toProto 序列化（§4.3），
//      Player 侧 SceneLoader 接收后 fromProto 还原（§5.1）；
//    * 校验是本组件的核心价值之一：加载期就把"clip 引用不存在的素材"、
//      "clip 素材类型与轨道约束不符"、"时长/入点非法"等配置错误拦截，
//      让播放主循环可以放心假设"场景引用完整"（fail-fast 原则）。
//
//  与时间轴总时长的约定：
//    Timeline.duration_us == 0 表示"未显式指定"，加载时按片段自动推算
//    （computeDurationUs），推算结果回填进内存模型（不回写 pb）。
// ============================================================================

#pragma once

#include <optional>
#include <string>

#include "model/SceneModel.hpp"
#include "scene.pb.h" // protoc 生成（输出于 build 目录，见 core/CMakeLists.txt）

namespace ms {

class SceneLoader {
public:
    // ---- pb ↔ 内存模型 双向转换 -------------------------------------------

    /// 内存模型 → Protobuf（Editor 导出/下发用）
    static ms_proto::Scene toProto(const Scene& scene);

    /// Protobuf → 内存模型（Player 接收用）
    /// 执行引用完整性校验；失败时返回 nullopt 并给出错误描述（err）
    static std::optional<Scene> fromProto(const ms_proto::Scene& pb,
                                          std::string& err);

    // ---- 子结构转换（公开静态；与 Scene 级映射完全一致）--------------------

    static ms_proto::Asset  toProtoAsset(const Asset& a);
    static Asset            fromProtoAsset(const ms_proto::Asset& pa);
    static ms_proto::Screen toProtoScreen(const Screen& s);
    static Screen           fromProtoScreen(const ms_proto::Screen& ps);
    static ms_proto::Track  toProtoTrack(const Track& t);
    static Track            fromProtoTrack(const ms_proto::Track& pt);
    static ms_proto::Timeline toProtoTimeline(const Timeline& tl);

    // ---- 场景完整性校验（一切场景数据入口统一走此校验）----------------------
    /// 校验 R1（素材引用存在）/ R2（轨道类型约束，含 Unknown 拦截）/
    /// R3（时间与 transform 数值合法）/ R7（id 非空 + 各集合内唯一）。
    /// \return true = 全部通过；false 时 err 给出「规则编号 + 上下文」
    static bool validateScene(const Scene& scene, std::string& err);

    // ---- 二进制序列化 -----------------------------------------------------

    /// 序列化为二进制（可写文件或经网络下发）
    static std::string serialize(const Scene& scene);

    /// 从二进制反序列化（含 fromProto 全部校验）
    static std::optional<Scene> parse(const std::string& bytes, std::string& err);

    // ---- 文件 IO（Editor 存档 / 测试夹具 / --print 冒烟）------------------

    /// 从 .scene 文件加载；失败返回 nullopt 并填充 err
    static std::optional<Scene> loadFromFile(const std::string& path,
                                             std::string& err);

    /// 保存为 .scene 文件；失败返回 false 并填充 err
    static bool saveToFile(const Scene& scene, const std::string& path,
                           std::string& err);
};

} // namespace ms
