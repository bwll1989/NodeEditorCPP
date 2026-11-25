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
//  探测规则（GetAdaptersAddresses，AF_INET）：
//    * 接口活动（IfOperStatusUp）且非软件 loopback 类型；
//    * 单播 IPv4，排除 127.x / 169.254.x（link-local）；
//    * 排除常见虚拟网卡：FriendlyName（小写化）含 vethernet / wsl /
//      hyper-v / vmware / virtualbox / virtual / loopback / pseudo——
//      Hyper-V/WSL 虚拟交换机标准命名 "vEthernet ..."；
//    * 优先取「有默认网关」的接口（真物理 LAN 强信号）；多候选取首个；
//    * 全部落空 → 空串（消费端不追加 = 保持系统默认行为；单网卡机器
//      本就不会选错，探测只服务双/多网卡环境）。
//
//  【平台】Windows 实现（GetAdaptersAddresses）；Android（getifaddrs，
//    NDK API 24+，minSdk 26 直接可用）——安卓手机 WiFi+蜂窝并存极常见：
//    UDP 组播 join 按默认路由可能钉到 rmnet 蜂窝接口（蜂窝不转发 LAN
//    组播）= 零包，探测把 join 钉到 wlan/eth。其余平台返回空（单接口
//    设备默认路由即正确）。
// ============================================================================

#pragma once

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

/// 探测本机真实 LAN IPv4（组播 localaddr 候选）。失败/无候选 = 空串。
inline std::string probeLanIPv4() {
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
            return {};

        const IP_ADAPTER_ADDRESSES* hit_gw   = nullptr; // 有默认网关的候选
        const sockaddr_in*          addr_gw  = nullptr;
        const IP_ADAPTER_ADDRESSES* hit_any  = nullptr; // 任意合法候选（兜底）
        const sockaddr_in*          addr_any = nullptr;

        for (const IP_ADAPTER_ADDRESSES* a = aa; a != nullptr; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp) continue;
            if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

            // 虚拟网卡过滤（FriendlyName 小写化后匹配关键词）
            std::wstring name(a->FriendlyName != nullptr ? a->FriendlyName
                                                         : L"");
            for (wchar_t& ch : name)
                ch = static_cast<wchar_t>(::towlower(ch));
            static const wchar_t* kVirtual[] = {
                L"vethernet", L"wsl",      L"hyper-v", L"vmware",
                L"virtual",   L"loopback", L"pseudo"}; // virtual 已覆盖 virtualbox
            bool is_virtual = false;
            for (const wchar_t* k : kVirtual) {
                if (name.find(k) != std::wstring::npos) {
                    is_virtual = true;
                    break;
                }
            }
            if (is_virtual) continue;

            // 该接口第一个合法 IPv4（每接口只取一个）
            for (const IP_ADAPTER_UNICAST_ADDRESS* u = a->FirstUnicastAddress;
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

                // 有默认网关的接口优先（仅认 IPv4 网关；0.0.0.0 = 无效不认）
                if (hit_gw == nullptr && a->FirstGatewayAddress != nullptr &&
                    a->FirstGatewayAddress->Address.lpSockaddr != nullptr &&
                    a->FirstGatewayAddress->Address.lpSockaddr->sa_family ==
                        AF_INET) {
                    auto* g = reinterpret_cast<const sockaddr_in*>(
                        a->FirstGatewayAddress->Address.lpSockaddr);
                    if (g->sin_addr.S_un.S_addr != 0) {
                        hit_gw  = a;
                        addr_gw = sa;
                    }
                }
                if (hit_any == nullptr) {
                    hit_any  = a;
                    addr_any = sa;
                }
                break; // 每接口只取第一个合法 IPv4
            }
        }

        const sockaddr_in* pick = addr_gw != nullptr ? addr_gw : addr_any;
        if (pick == nullptr) return {};
        char ip[INET_ADDRSTRLEN] = {0};
        if (!inet_ntop(AF_INET, &pick->sin_addr, ip, sizeof(ip))) return {};
        return ip;
    } // for(pass)：溢出按系统提示扩容重试
    return {};
#elif defined(__ANDROID__)
    // ---- Android：getifaddrs 枚举（WiFi+蜂窝并存免疫）--------------------
    // 优先级 wlan* > eth* > 其他非蜂窝；排除 loopback / link-local /
    // 蜂窝数据接口（rmnet*/cc*/ril*/pdp*/wwan*/v4-rmnet*——组播 join
    // 钉蜂窝 = 现场局域网零包）。蜂窝+WiFi 并存时安卓默认路由可能走
    // 蜂窝（现场无外网时系统仍倾向数据网络），FFmpeg 无 localaddr 的
    // join 按 best route 选接口——必须显式钉定。
    struct ifaddrs* ifs = nullptr;
    if (getifaddrs(&ifs) != 0) return {};

    const char* wlan_ip = nullptr; char wlan_buf[INET_ADDRSTRLEN] = {0};
    const char* eth_ip  = nullptr; char eth_buf[INET_ADDRSTRLEN]  = {0};
    const char* any_ip  = nullptr; char any_buf[INET_ADDRSTRLEN]  = {0};

    for (const struct ifaddrs* ifa = ifs; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr ||
            ifa->ifa_addr->sa_family != AF_INET) continue;      // 只认 IPv4
        if (!(ifa->ifa_flags & IFF_UP) ||
            (ifa->ifa_flags & IFF_LOOPBACK)) continue;
        auto* sa = reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr);
        char ip[INET_ADDRSTRLEN] = {0};
        if (!inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip))) continue;
        const std::string s = ip;
        if (s.rfind("127.", 0) == 0 || s.rfind("169.254.", 0) == 0) continue;

        const char* name = ifa->ifa_name != nullptr ? ifa->ifa_name : "";
        // 蜂窝/隧道接口排除（前缀匹配；clat=IPv4 over IPv6 过渡，同样
        // 不在局域网链路上）
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

        if (any_ip == nullptr) std::strncpy(any_buf, ip, sizeof(any_buf) - 1);
        if (wlan_ip == nullptr && std::strncmp(name, "wlan", 4) == 0) {
            std::strncpy(wlan_buf, ip, sizeof(wlan_buf) - 1);
            wlan_ip = wlan_buf;
        }
        if (eth_ip == nullptr && std::strncmp(name, "eth", 3) == 0) {
            std::strncpy(eth_buf, ip, sizeof(eth_buf) - 1);
            eth_ip = eth_buf;
        }
    }
    freeifaddrs(ifs);
    if (wlan_ip) return wlan_ip;   // 现场 WiFi（显示节点常态）
    if (eth_ip)  return eth_ip;    // 盒子/TV 有线
    return any_ip ? any_ip : "";   // 兜底：首个合法接口（USB 共享网等）
#else
    return {}; // 其他平台：单接口设备默认路由即正确，不追加 localaddr
#endif
}

} // namespace ms
