// ============================================================================
//  FrameCodec.hpp —— TCP 流分帧（长度前缀 + Protobuf Envelope）
//
//  设计依据：《media-server-sync-design.md》§6
//    TCP 是字节流，需要应用层分帧。本编解码器用最简单的
//      [4 字节小端 payload 长度][payload]
//    格式；payload = ControlEnvelope.SerializeAsString()。
//
//  线上格式（小端）：
//      +--------+------------------+
//      | len(4) | envelope bytes   |     len ≤ kMaxPayload（防恶意/错误流）
//      +--------+------------------+
//
//  使用模式（见 ControlServer/ControlClient）：
//    收：SocketInputStream → receiveBytes → append 到接收缓冲 →
//        FrameCodec::tryExtract 逐帧弹出（可能一次收到多帧半包）
//    发：FrameCodec::encode(envelope) → 一次 sendBytes
// ============================================================================

#pragma once

#include <cstdint>
#include <string>

#include "sync.pb.h" // ms_proto::Envelope（sync.proto，protoc 生成）

namespace ms {

class FrameCodec {
public:
    /// 单帧 payload 上限（4MB）：Scene 含素材清单时也在几 KB~几百 KB 量级，
    /// 4MB 充裕且防止坏流把长度字段解读成天文数字耗尽内存
    static constexpr std::uint32_t kMaxPayload = 4u * 1024u * 1024u;

    /// 信封 → 线上帧（4 字节长度头 + payload 二进制）
    static std::string encode(const ms_proto::Envelope& envelope);

    /// 信封 → payload 二进制（不含长度头；与 encode 配合测试）
    static std::string encodePayload(const ms_proto::Envelope& envelope);

    /// 从接收缓冲尝试提取一帧完整 envelope
    /// \param buffer 累积的接收字节（函数会原地消费掉已处理的字节）
    /// \param out    成功时填充解析结果
    /// \return 1=取到一帧；0=数据不足（等下一次 socket 读）；-1=协议错误
    ///         （长度超限或 pb 解析失败——调用方应断开连接）
    static int tryExtract(std::string& buffer, ms_proto::Envelope& out);

    /// 帧头字节数
    static constexpr std::size_t kHeaderSize = 4;
};

} // namespace ms
