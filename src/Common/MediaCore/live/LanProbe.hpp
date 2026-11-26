// ============================================================================
//  LanProbe —— 本机 LAN 出口接口探测（组播 localaddr 自动钉定）
//
//  【问题 2026-11-23 实锤】Windows 组播发送/加入按 best route 选出口——
//  双网卡（物理 + WSL/Hyper-V）机器上系统自动生成的 224.0.0.0/4 →
//  127.0.0.1 路由（Loopback Pseudo-Interface，metric 331）恒压过物理
//  网卡（metric 万+），FFmpeg 不带 localaddr 的组播默认从 loopback 出：
//  本机回环自收正常、局域网其他主机零包（「本机能收、远端打不开」签名
//  症状）。route add 长前缀路由可解，但属系统运维项——每次部署重配。
//
//  【方案】收流端（NetworkIngest）打开 socket 前 probeLanIPv4() 探测
//  本机真实 LAN IPv4，动态 append "&localaddr=<ip>"——接收侧 =
//  IP_ADD_MEMBERSHIP join 接口钉定（组播/UDP 收流的通用手段；NDI 分发
//  不经此路径——NDI 收端由 SDK 自管 socket）。URL 本尊保持基础形式存
//  场景两端共用——接口是「每台机器自己的运行时事实」，不落盘、不进
//  场景，部署零配置。
//
//  枚举/过滤规则（GetAdaptersAddresses，AF_INET；单一数据源见
//  lan_detail::enumerate——2026-11-26 去重：原 listLanIfcs 与
//  probeLanIPv4 各持一份遍历 + kVirtual 过滤表，改一处漏一处）：
//    * 接口活动（IfOperStatusUp）且非软件 loopback 类型；
//    * 单播 IPv4，排除 127.x / 169.254.x（link-local）；
//    * 排除常见虚拟网卡：FriendlyName（小写化）含 vethernet / wsl /
//      hyper-v / vmware / virtualbox / virtual / loopback / pseudo——
//      Hyper-V/WSL 虚拟交换机标准命名 "vEthernet ..."；
//    * 每接口取第一个合法 IPv4；附带「有 IPv4 默认网关」标记
//      （真物理 LAN 强信号——probeLanIPv4 消费）；
//    * 全部落空 → 空（消费端不追加 = 保持系统默认行为；单网卡机器
//      本就不会选错，探测只服务双/多网卡环境）。
//
//  【平台】Windows 实现（GetAdaptersAddresses）；Android（getifaddrs，
//    NDK API 24+，minSdk 26 直接可用）——安卓手机 WiFi+蜂窝并存极常见：
//    UDP 组播 join 按默认路由可能钉到 rmnet 蜂窝接口（蜂窝不转发 LAN
//    组播）= 零包，探测把 join 钉到 wlan/eth。其余平台返回空（单接口
//    设备默认路由即正确）。
// ============================================================================

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <cwctype>
#pragma comment(lib, "iphlpapi")
#pragma comment(lib, "ws2_32")
#elif defined(__ANDROID__)
#include <ifaddrs.h>     // getifaddrs（NDK API 24+）
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>      // IFF_UP / IFF_LOOPBACK
#include <cstring>
#endif

namespace ms {

/// 本机网卡候选（下拉枚举用；ip = IPv4 点分十进制，name = 人读接口名）
struct LanIfc {
    std::string ip;
    std::string name;
};

// ---------------------------------------------------------------------------
// 统一枚举（listLanIfcs / probeLanIPv4 单一数据源——过滤规则只此一份）
// ---------------------------------------------------------------------------
namespace lan_detail {

/// 枚举条目：gw = 接口持有效 IPv4 默认网关（真物理 LAN 强信号；
/// Android getifaddrs 无网关信息恒 false，序即优先级）
struct Entry {
    std::string ip;
    std::string name;
    bool        gw = false;
};

/// 枚举本机全部合法 LAN IPv4 接口（活动 + 非 loopback + 排虚拟网卡
/// Windows / 排蜂窝隧道 Android + 排 link-local；每接口取第一个 IPv4）。
/// Android 结果按 wlan > eth > 其他稳定排序（下拉首项即自动探测所选）。
inline std::vector<Entry> enumerate() {
    std::vector<Entry> out;
#if defined(_WIN32)
    ULONG size = 15 * 1024; // 起始 15K（典型远小于此；溢出按系统提示重试）
    for (int pass = 0; pass < 4; ++pass) {
        std::vector<char> buf(size);
        auto* aa = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
        const ULONG ret = GetAdaptersAddresses(
            AF_INET,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_GATEWAYS,
            nullptr, aa, &size);
        if (ret == ERROR_BUFFER_OVERFLOW)
            continue; // size 已被系统更新为需求值，扩容重试
        if (ret != NO_ERROR)
            return out;

        for (const IP_ADAPTER_ADDRESSES* a = aa; a != nullptr; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp) continue;
            if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

            // 虚拟网卡过滤（FriendlyName 小写化后匹配关键词）
            std::wstring wname(a->FriendlyName != nullptr ? a->FriendlyName
                                                          : L"");
            std::wstring lower = wname;
            for (wchar_t& ch : lower)
                ch = static_cast<wchar_t>(::towlower(ch));
            static const wchar_t* kVirtual[] = {
                L"vethernet", L"wsl",      L"hyper-v", L"vmware",
                L"virtual",   L"loopback", L"pseudo"}; // virtual 已覆盖 virtualbox
            bool is_virtual = false;
            for (const wchar_t* k : kVirtual) {
                if (lower.find(k) != std::wstring::npos) {
                    is_virtual = true;
                    break;
                }
            }
            if (is_virtual) continue;

            // IPv4 默认网关有效性（0.0.0.0 = 无效不认）
            bool has_gw = false;
            if (a->FirstGatewayAddress != nullptr &&
                a->FirstGatewayAddress->Address.lpSockaddr != nullptr &&
                a->FirstGatewayAddress->Address.lpSockaddr->sa_family ==
                    AF_INET) {
                auto* g = reinterpret_cast<const sockaddr_in*>(
                    a->FirstGatewayAddress->Address.lpSockaddr);
                has_gw = g->sin_addr.S_un.S_addr != 0;
            }

            // 该接口第一个合法 IPv4（每接口只取一个）
            for (const IP_ADAPTER_UNICAST_ADDRESS* u =
                     a->FirstUnicastAddress;
                 u != nullptr; u = u->Next) {
                if (u->Address.lpSockaddr == nullptr ||
                    u->Address.lpSockaddr->sa_family != AF_INET)
                    continue;
                auto* sa = reinterpret_cast<const sockaddr_in*>(
                    u->Address.lpSockaddr);
                char ip[INET_ADDRSTRLEN] = {0};
                if (!inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip)))
                    break;
                const std::string s = ip;
                if (s.rfind("127.", 0) == 0 || s.rfind("169.254.", 0) == 0)
                    break;
                // FriendlyName → UTF-8（中文接口名可读）
                char utf8_name[128] = {0};
                WideCharToMultiByte(CP_UTF8, 0, wname.c_str(), -1,
                                    utf8_name, sizeof(utf8_name), nullptr,
                                    nullptr);
                out.push_back({ip, utf8_name, has_gw});
                break;
            }
        }
        return out;
    } // for(pass)：溢出按系统提示扩容重试
    return out;
#elif defined(__ANDROID__)
    struct ifaddrs* ifs = nullptr;
    if (getifaddrs(&ifs) != 0) return out;
    for (const struct ifaddrs* ifa = ifs; ifa != nullptr;
         ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr ||
            ifa->ifa_addr->sa_family != AF_INET) continue;      // 只认 IPv4
        if (!(ifa->ifa_flags & IFF_UP) ||
            (ifa->ifa_flags & IFF_LOOPBACK)) continue;
        auto* sa = reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr);
        char ip[INET_ADDRSTRLEN] = {0};
        if (!inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip))) continue;
        const std::string s = ip;
        if (s.rfind("127.", 0) == 0 || s.rfind("169.254.", 0) == 0)
            continue;

        const char* name = ifa->ifa_name != nullptr ? ifa->ifa_name : "";
        // 蜂窝/隧道接口排除（前缀匹配；clat=IPv4 over IPv6 过渡，同样
        // 不在局域网链路上——join 钉蜂窝 = 现场局域网零包）
        static const char* kCellular[] = {
            "rmnet", "ccemu", "clat", "ril", "pdp", "wwan", "v4-rmnet",
            "dummy", "tun", "tap"};
        bool cellular = false;
        for (const char* k : kCellular) {
            if (std::strncmp(name, k, std::strlen(k)) == 0) {
                cellular = true;
                break;
            }
        }
        if (cellular) continue;
        out.push_back({ip, name, false});
    }
    freeifaddrs(ifs);
    // wlan > eth > 其他（stable_sort 保枚举序——现场 WiFi 常态、盒子/
    // TV 有线次之；序即探测优先级，下拉首项与自动探测所选一致）
    std::stable_sort(out.begin(), out.end(),
                     [](const Entry& x, const Entry& y) {
                         auto rank = [](const std::string& n) {
                             if (n.rfind("wlan", 0) == 0) return 0;
                             if (n.rfind("eth", 0) == 0) return 1;
                             return 2;
                         };
                         return rank(x.name) < rank(y.name);
                     });
    return out;
#else
    return out; // 其他平台：单接口设备，默认路由即正确（返回空）
#endif
}

} // namespace lan_detail

/// 枚举本机全部合法 LAN IPv4 接口（组播 localaddr 下拉候选——editor
/// LiveClipSection「发送网卡」/ display 网页设置页数据源）。
inline std::vector<LanIfc> listLanIfcs() {
    std::vector<LanIfc> out;
    for (const lan_detail::Entry& e : lan_detail::enumerate())
        out.push_back({e.ip, e.name});
    return out;
}

/// 探测本机真实 LAN IPv4（组播 localaddr 候选）。失败/无候选 = 空串。
/// 取舍：持有效 IPv4 默认网关的接口优先（真物理 LAN 强信号）——注意
/// 多真实网卡「内网无网关 + 外网有网关」部署下该优先会钉到外网卡
///（盲区一，见文件头），此场景须由裁决链上游（场景 bind_ip / 对端
/// 同网段推断）显式覆盖；无网关候选时取枚举序首个合法接口。
inline std::string probeLanIPv4() {
    const std::vector<lan_detail::Entry> ifcs = lan_detail::enumerate();
    for (const lan_detail::Entry& e : ifcs)
        if (e.gw) return e.ip;
    return ifcs.empty() ? std::string() : ifcs.front().ip;
}

// ---------------------------------------------------------------------------
// 组播接口钉定（2026-11-26 两轮定案）
//
//   盲区一（手动指定场景）：多块真实物理网卡分属不同网段，且组播源所在
//   内网网卡【无默认网关】而外网网卡有——probeLanIPv4 优先取「有默认
//   网关」接口，会把 join 钉到外网卡 = 内网组播永远收不到。
//
//   盲区二（接收端正解，用户拍板 2026-11-26 12:00）：display 已知 Editor
//   的 TCP 连接对端 IP（ControlServer accept 时记录）——组播源（编辑器
//   编码器）就在 Editor 那台机器，**接收 join 应钉在与 Editor 同网段的
//   本机接口上**，无需任何手动配置。
//
//   裁决优先级（appendLanLocaladdr 统一裁决）：
//     1. URL 手写 localaddr=（live_url 用户可编辑——最高优先）
//     2. 进程显式配置（setExplicitLanIPv4；display ini/CLI 终极覆盖）
//     3. 与 Editor 对端 IP 同网段的本机接口（接收端自动推断——正解）
//     4. probeLanIPv4() 自动探测（现状行为）
//     5. 全部落空 → 不追加（保持系统路由；打告警）
// ---------------------------------------------------------------------------

/// 进程级显式出口 IP（空 = 自动探测）。【线程约定】须在任何 live 会话
/// 开始前设置一次（display main 解析配置后、装配 player 前调用）——
/// 之后仅读，无锁约定成立。
inline std::string& explicitLanIPv4() {
    static std::string s;
    return s;
}

/// 显式指定本机出口/加入接口 IPv4；空串 = 撤销（回落自动裁决）。
/// 进程级事实（与 LanProbe 哲学一致：不进场景 URL，每台机器自己配）。
inline void setExplicitLanIPv4(std::string ip) {
    if (ip.empty()) {
        explicitLanIPv4().clear(); // 空 = 撤销显式指定（此前实现挡掉空串，
        return;                    // 文档声明的「撤销」语义不可达——修复）
    }
    // 基础防御：含字母/非法字符直接丢弃（配置手滑不炸链路）
    if (ip.find_first_not_of("0123456789.") == std::string::npos &&
        ip.find("..") == std::string::npos) {
        explicitLanIPv4() = std::move(ip);
    }
}

/// 进程级「对端（Editor）IP」记录（接收端同网段推断数据源）。
/// ControlServer on_connected 时由 peer_host_of 注入；断开不清（组播
/// 源机器不变，断线重连 join 语义不变）。【线程约定】连接回调在网络
/// 线程、读在收流线程——写仅发生在（重）连接时刻，竞态窗口内旧值/新值
/// 均为合法接口候选（同网段判定幂等），不设锁。
inline std::string& peerLanIPv4() {
    static std::string s;
    return s;
}

/// 记录当前对端（Editor）IPv4（display 端连接建立时调用；空 = 清除）。
inline void setPeerLanIPv4(std::string ip) { peerLanIPv4() = std::move(ip); }

/// IPv4 点分十进制 → 主机序 uint32（失败 = 0）
inline std::uint32_t lanIPv4ToHost(const std::string& dotted) {
    in_addr a{};
    return inet_pton(AF_INET, dotted.c_str(), &a) == 1
               ? ntohl(a.s_addr)
               : 0;
}

/// 取与 \p peer 同 IPv4 子网（/24 简化判定——现场部署均为 C 段）的本机
/// 接口 IP。peer 无效/无同网段接口 = 空串。仅前 3 字节比对（/24）：
/// 广播域即组播可达域，跨网段组播本就依赖路由转发（不在本功能范围）。
inline std::string pickLanIPv4ForPeer(const std::string& peer) {
    const std::uint32_t p = lanIPv4ToHost(peer);
    if (p == 0) return {};
    for (const LanIfc& ifc : listLanIfcs()) {
        const std::uint32_t m = lanIPv4ToHost(ifc.ip);
        if (m != 0 && (m ^ p) >> 8 == 0) return ifc.ip; // 前 3 段相同
    }
    return {};
}

/// 组播 URL 出口/接口钉定（发送端 = IP_MULTICAST_IF，接收端 =
/// IP_ADD_MEMBERSHIP）：按上方优先级裁决并追加 localaddr 参数。
/// \param tag        日志前缀（区分 sender/ingest 链路）
/// \param caller_ip  调用方显式接口 IP（editor 场景 bind_ip 下拉值；
///                   空 = 无显式指定）。优先级介于 URL 手写与自动之间
///                   ——比进程 setter 更具体（per-asset 资产级配置）
/// \return 追加后的 URL（未追加 = 原样返回；URL 本尊保持基础形式存场景）
inline std::string appendLanLocaladdr(const std::string& url, const char* tag,
                                      const std::string& caller_ip = {}) {
    if (url.find("localaddr=") != std::string::npos) {
        // URL 显式指定最高优先：尊重用户，不追加（规避重复参数的
        // 未定义覆盖行为——FFmpeg 查询串同名参数以解析序后者生效）
        std::fprintf(stderr, "%s: localaddr 由 URL 显式指定，跳过接口探测\n",
                     tag);
        return url;
    }

    std::string ip = caller_ip;
    const char* how = "场景指定";
    if (ip.empty()) {
        ip = explicitLanIPv4();
        how = "配置指定";
    }
    if (ip.empty()) {
        // 接收端自动推断：与 Editor 对端 IP 同网段的本机接口（多网卡
        // 机器零配置正解——组播源与 Editor 同机）。peer 无效/无匹配
        // 静默回落（Editor 未连接/本机单接口等常态）
        const std::string peer = peerLanIPv4();
        if (!peer.empty()) {
            ip  = pickLanIPv4ForPeer(peer);
            if (!ip.empty())
                how = "对端同网段";
        }
    }
    if (ip.empty()) {
        how = "自动探测";
        ip  = probeLanIPv4();
    }
    if (ip.empty()) {
        std::fprintf(stderr,
                     "%s: 出口接口探测失败，交由系统路由（多网卡机器"
                     "可能选错，跨机收不到流时优先排查；可用"
                     " --mcast-localaddr=<ip> 显式指定）\n",
                     tag);
        return url;
    }
    std::fprintf(stderr, "%s: 组播接口钉定 localaddr=%s（%s）\n",
                 tag, ip.c_str(), how);
    return url + (url.find('?') == std::string::npos ? "?localaddr="
                                                     : "&localaddr=") +
           ip;
}

} // namespace ms
