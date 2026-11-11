#pragma once

#include <QString>

/**
 * @brief LicenseClient SDK 原生库（licensingclient.dll）动态加载封装。
 *
 * SDK 由授权平台提供（.NET NativeAOT 编译的 C ABI 动态库），负责：
 *  - 获取机器码（Windows MachineGuid 派生，16 位 Crockford Base32，不随网卡/VPN 变化）
 *  - 生成加密激活请求文件（.ft，AES-256-GCM）
 *  - 校验授权令牌 / 授权文件（.lic = LP1. 令牌；内嵌公钥 RSA-PSS 验签 + 机器码绑定 + 有效期）
 *  - 时钟回拨检测
 *
 * 加载顺序：环境变量 LICENSE_SDK_LIB → 应用程序目录 → 系统默认 DLL 搜索路径。
 * 库缺失或导出不完整时 available() 返回 false，所有接口安全返回"未授权/失败"，
 * 不会令宿主程序崩溃（授权因此处于关闭状态，属于有意的 fail-closed 语义）。
 *
 * C ABI 返回值约定（见 3rdParty/license-sdk-c-windows-1.0.0/include/license_client.h）：
 *  - 文本函数：>= 0 为写入缓冲区字节数（UTF-8，不含结尾 0）；< 0 为错误
 *  - 状态函数：0 = 成功/有效；1 = 授权无效；2 = 参数或文件错误；-1 = 未知异常
 */
class LicenseClientSdk
{
public:
    struct VerifyResult {
        bool valid = false;
        QString message; ///< SDK 给出的校验说明（成功确认或失败原因）
    };

    /// 全局单例；首次调用时加载原生库（线程安全）
    static LicenseClientSdk& instance();

    /// 原生库是否已成功加载且导出齐全
    bool available() const { return m_loaded; }

    /// 诊断信息：未加载时的原因说明；已加载时为空
    QString statusText() const { return m_status; }

    /// 机器码（16 位分组码，形如 A1B2-C3D4-E5F6-G7H8）；库不可用时返回空串
    QString machineCode() const;

    /// 硬件指纹（机器码源标识的 SHA-256 十六进制，排障用）；库不可用时返回空串
    QString hardwareHash() const;

    /// SDK 版本号；库不可用时返回空串
    QString sdkVersion() const;

    /// 校验授权令牌文本（.lic 文件内容，LP1. 开头），内嵌公钥验签
    VerifyResult verifyToken(const QString& token) const;

    /// 校验授权文件（.lic），内嵌公钥验签
    VerifyResult verifyLicenseFile(const QString& path) const;

    /// 生成加密激活请求文件（.ft）；key 为空时使用 SDK 内置口令（可用环境变量 LICENSE_REQUEST_KEY 覆盖）
    bool generateRequest(const QString& activationCode, const QString& outPath, QString* error) const;

    /// 时钟回拨检测：返回空串表示正常，否则为问题描述
    QString checkClock() const;

    /// 读取 .lic 载荷 JSON（不校验签名，仅供展示/排障）
    QString licensePayload(const QString& path) const;

private:
    LicenseClientSdk();
    ~LicenseClientSdk();
    LicenseClientSdk(const LicenseClientSdk&) = delete;
    LicenseClientSdk& operator=(const LicenseClientSdk&) = delete;

    void* m_lib = nullptr;   ///< HMODULE
    bool m_loaded = false;   ///< 加载且导出齐全
    QString m_status;        ///< 诊断信息

    // 与 license_client.h 对应的函数指针
    int (*m_getMachineCode)(char*, int) = nullptr;
    int (*m_getMachineHash)(char*, int) = nullptr;
    int (*m_sdkVersion)(char*, int) = nullptr;
    int (*m_checkClock)(const char*, char*, int) = nullptr;
    int (*m_generateRequest)(const char*, const char*, const char*) = nullptr;
    int (*m_verifyTokenEmbedded)(const char*, const char*, char*, int) = nullptr;
    int (*m_verifyLicenseEmbedded)(const char*, const char*, char*, int) = nullptr;
    int (*m_readLicensePayload)(const char*, char*, int) = nullptr;
};
