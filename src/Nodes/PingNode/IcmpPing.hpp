/**
 * @file IcmpPing.hpp
 * @brief Windows ICMP 探测（IcmpSendEcho），供 Ping 节点后台线程调用
 */
#pragma once

#include <QString>

namespace Nodes {

class IcmpPing
{
public:
    /// 同步 ping 一次；应在工作线程调用。timeoutMs 默认 3000。
    static bool ping(const QString &host, unsigned timeoutMs = 3000);
};

} // namespace Nodes
