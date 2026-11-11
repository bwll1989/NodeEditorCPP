#include "LicenseClientSdk.h"

#include <QCoreApplication>
#include <cstdlib>
#include <vector>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

constexpr int kMachineCodeCapacity = 128;
constexpr int kHashCapacity = 128;
constexpr int kVersionCapacity = 64;
constexpr int kLastSeenCapacity = 64;
constexpr int kErrorCapacity = 1024;
constexpr int kPayloadCapacity = 16384;

/// 调用文本型 C ABI 导出：返回值 >=0 为写入字节数，<0 为错误
QString callText(int (*fn)(char*, int), int capacity)
{
    if (!fn) {
        return {};
    }
    std::vector<char> buffer(static_cast<size_t>(capacity) + 1, 0);
    const int written = fn(buffer.data(), capacity);
    if (written <= 0) {
        return {};
    }
    return QString::fromUtf8(buffer.data(), written);
}

} // namespace

LicenseClientSdk& LicenseClientSdk::instance()
{
    static LicenseClientSdk sdk;
    return sdk;
}

LicenseClientSdk::LicenseClientSdk()
{
#ifdef Q_OS_WIN
    // 候选路径：环境变量 → 应用程序目录 → 默认 DLL 搜索（exe 同目录）
    QStringList candidates;
    const QString envLib = qEnvironmentVariable("LICENSE_SDK_LIB");
    if (!envLib.trimmed().isEmpty()) {
        candidates << envLib.trimmed();
    }
    if (QCoreApplication::instance()) {
        candidates << QCoreApplication::applicationDirPath()
                            + QStringLiteral("/licensingclient.dll");
    }
    candidates << QStringLiteral("licensingclient.dll");

    for (const QString& candidate : candidates) {
        HMODULE lib = ::LoadLibraryW(reinterpret_cast<const wchar_t*>(candidate.utf16()));
        if (!lib) {
            continue;
        }
        auto resolve = [lib](const char* name) -> void* {
            return reinterpret_cast<void*>(::GetProcAddress(lib, name));
        };
        m_getMachineCode = reinterpret_cast<int (*)(char*, int)>(resolve("lc_get_machine_code"));
        m_getMachineHash = reinterpret_cast<int (*)(char*, int)>(resolve("lc_get_machine_hash"));
        m_sdkVersion = reinterpret_cast<int (*)(char*, int)>(resolve("lc_sdk_version"));
        m_checkClock = reinterpret_cast<int (*)(const char*, char*, int)>(resolve("lc_check_clock"));
        m_generateRequest = reinterpret_cast<int (*)(const char*, const char*, const char*)>(
            resolve("lc_generate_request"));
        m_verifyTokenEmbedded = reinterpret_cast<int (*)(const char*, const char*, char*, int)>(
            resolve("lc_verify_token_embedded"));
        m_verifyLicenseEmbedded = reinterpret_cast<int (*)(const char*, const char*, char*, int)>(
            resolve("lc_verify_license_embedded"));
        m_readLicensePayload = reinterpret_cast<int (*)(const char*, char*, int)>(
            resolve("lc_read_license_payload"));

        // 核心导出必须齐全，否则视为加载失败
        const bool complete = m_getMachineCode && m_generateRequest
                              && m_verifyTokenEmbedded && m_verifyLicenseEmbedded;
        if (!complete) {
            m_status = QStringLiteral("licensingclient.dll 缺少必需导出函数（%1），请核对 SDK 版本")
                           .arg(QString::fromLatin1(candidate.toUtf8()));
            ::FreeLibrary(lib);
            m_lib = nullptr;
            continue;
        }

        m_lib = lib;
        m_loaded = true;
        m_status.clear();
        return;
    }

    m_status = QStringLiteral("未找到 SDK 原生库 licensingclient.dll"
                              "（需与 Flow.exe 同目录，或用环境变量 LICENSE_SDK_LIB 指定完整路径）。"
                              "授权功能不可用。");
#else
    m_status = QStringLiteral("当前平台未实现 LicenseClient SDK 加载");
#endif
}

LicenseClientSdk::~LicenseClientSdk()
{
#ifdef Q_OS_WIN
    if (m_lib) {
        ::FreeLibrary(static_cast<HMODULE>(m_lib));
        m_lib = nullptr;
    }
#endif
}

QString LicenseClientSdk::machineCode() const
{
    return callText(m_getMachineCode, kMachineCodeCapacity);
}

QString LicenseClientSdk::hardwareHash() const
{
    return callText(m_getMachineHash, kHashCapacity);
}

QString LicenseClientSdk::sdkVersion() const
{
    return callText(m_sdkVersion, kVersionCapacity);
}

LicenseClientSdk::VerifyResult LicenseClientSdk::verifyToken(const QString& token) const
{
    VerifyResult result;
    if (!m_loaded || !m_verifyTokenEmbedded) {
        result.message = m_status;
        return result;
    }
    const QByteArray utf8 = token.trimmed().toUtf8();
    if (utf8.isEmpty()) {
        result.message = QStringLiteral("授权内容为空");
        return result;
    }
    std::vector<char> error(kErrorCapacity, 0);
    const int rc = m_verifyTokenEmbedded(utf8.constData(), nullptr, error.data(), kErrorCapacity);
    result.valid = (rc == 0);
    result.message = QString::fromUtf8(error.data());
    if (result.message.isEmpty()) {
        result.message = result.valid ? QStringLiteral("授权有效") : QStringLiteral("授权无效");
    }
    return result;
}

LicenseClientSdk::VerifyResult LicenseClientSdk::verifyLicenseFile(const QString& path) const
{
    VerifyResult result;
    if (!m_loaded || !m_verifyLicenseEmbedded) {
        result.message = m_status;
        return result;
    }
    const QByteArray utf8 = path.toUtf8();
    std::vector<char> error(kErrorCapacity, 0);
    const int rc = m_verifyLicenseEmbedded(utf8.constData(), nullptr, error.data(), kErrorCapacity);
    result.valid = (rc == 0);
    result.message = QString::fromUtf8(error.data());
    if (result.message.isEmpty()) {
        result.message = result.valid ? QStringLiteral("授权有效") : QStringLiteral("授权无效");
    }
    return result;
}

bool LicenseClientSdk::generateRequest(const QString& activationCode, const QString& outPath,
                                       QString* error) const
{
    if (!m_loaded || !m_generateRequest) {
        if (error) {
            *error = m_status;
        }
        return false;
    }
    const QByteArray code = activationCode.toUtf8();
    QByteArray path = outPath.toUtf8();
    const int rc = m_generateRequest(code.constData(), path.constData(), nullptr);
    if (rc == 0) {
        return true;
    }
    if (error) {
        switch (rc) {
            case 2:
                *error = QStringLiteral("生成激活请求失败：参数错误（请检查激活码与保存路径）");
                break;
            case -1:
                *error = QStringLiteral("生成激活请求失败：SDK 内部异常");
                break;
            default:
                *error = QStringLiteral("生成激活请求失败（错误码 %1）").arg(rc);
                break;
        }
    }
    return false;
}

QString LicenseClientSdk::checkClock() const
{
    if (!m_loaded || !m_checkClock) {
        return m_status;
    }
    std::vector<char> lastSeen(kLastSeenCapacity, 0);
    const int rc = m_checkClock(nullptr, lastSeen.data(), kLastSeenCapacity);
    switch (rc) {
        case 0:
            return {};
        case 1:
            return QStringLiteral("检测到系统时间回拨");
        case 2:
            return QStringLiteral("时钟存储不可用");
        default:
            return QStringLiteral("时钟检查异常");
    }
}

QString LicenseClientSdk::licensePayload(const QString& path) const
{
    if (!m_loaded || !m_readLicensePayload) {
        return {};
    }
    std::vector<char> buffer(kPayloadCapacity, 0);
    const QByteArray utf8 = path.toUtf8();
    const int written = m_readLicensePayload(utf8.constData(), buffer.data(), kPayloadCapacity);
    if (written <= 0) {
        return {};
    }
    return QString::fromUtf8(buffer.data(), written);
}
