#include "IcmpPing.hpp"

#ifdef Q_OS_WIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <vector>

namespace Nodes {
namespace {

bool ensureWinsock()
{
    static bool ready = false;
    static bool attempted = false;
    if (ready) {
        return true;
    }
    if (attempted) {
        return false;
    }
    attempted = true;
    WSADATA wsaData{};
    ready = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    return ready;
}

bool resolveIPv4(const QString &host, IPAddr &dest)
{
    const std::wstring whost = host.toStdWString();

    IN_ADDR inAddr{};
    if (InetPtonW(AF_INET, whost.c_str(), &inAddr) == 1) {
        dest = inAddr.S_un.S_addr;
        return true;
    }

    if (!ensureWinsock()) {
        return false;
    }

    ADDRINFOW hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    ADDRINFOW *result = nullptr;
    if (GetAddrInfoW(whost.c_str(), nullptr, &hints, &result) != 0 || result == nullptr) {
        return false;
    }

    bool ok = false;
    for (ADDRINFOW *ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr->ai_family != AF_INET || ptr->ai_addrlen < sizeof(sockaddr_in)) {
            continue;
        }
        auto *sin = reinterpret_cast<sockaddr_in *>(ptr->ai_addr);
        dest = sin->sin_addr.S_un.S_addr;
        ok = true;
        break;
    }

    FreeAddrInfoW(result);
    return ok;
}

} // namespace

bool IcmpPing::ping(const QString &host, unsigned timeoutMs)
{
    const QString trimmed = host.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    IPAddr dest = 0;
    if (!resolveIPv4(trimmed, dest)) {
        return false;
    }

    HANDLE const icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE) {
        return false;
    }

    static char kSendData[] = "FlowPing";
    DWORD const sendSize = static_cast<DWORD>(sizeof(kSendData) - 1);

    // 回复缓冲需足够大；过小会导致外网地址探测失败
    std::vector<char> replyBuffer(8192);
    DWORD const replySize = static_cast<DWORD>(replyBuffer.size());

    DWORD const replies = IcmpSendEcho(icmp,
                                       dest,
                                       kSendData,
                                       sendSize,
                                       nullptr,
                                       replyBuffer.data(),
                                       replySize,
                                       timeoutMs);

    bool ok = false;
    if (replies > 0) {
        auto const *reply = reinterpret_cast<ICMP_ECHO_REPLY const *>(replyBuffer.data());
        ok = (reply->Status == IP_SUCCESS);
    }

    IcmpCloseHandle(icmp);
    return ok;
}

} // namespace Nodes

#else

namespace Nodes {

bool IcmpPing::ping(const QString &, unsigned)
{
    return false;
}

} // namespace Nodes

#endif
