#pragma once

#include <QString>
#include "ConfigManager.h"

/**
 * @brief 授权管理：基于 LicenseClient SDK（licensingclient.dll，C ABI）。
 *
 * 授权模型（替代旧版 CPU 指纹 + 对称授权码方案）：
 *  - 机器码由 SDK 提供（Windows MachineGuid 派生，16 位 Crockford Base32），交由运营核对；
 *  - 授权凭证为平台签发的 LP1. 令牌（.lic 文件内容），SDK 用内嵌公钥做 RSA-PSS 验签，
 *    并校验机器码绑定与有效期（含时钟回拨防护），客户端无私钥、无法伪造；
 *  - 授权来源两处：设置页粘贴的令牌（存于配置 License/Code），或 exe 旁的 license.lic 文件；
 *  - 激活闭环：设置页输入激活码 → 生成加密请求 .ft → 平台离线激活 → 粘贴/导入签发的令牌。
 *
 * SDK 原生库缺失时 available()=false，isAuthorized() 恒为 false（fail-closed）。
 */
class APPCONFIG_EXPORT LicenseManager
{
public:
    /// 机器码（SDK 提供）；SDK 不可用时返回空串
    static QString machineCode();

    /// 校验授权令牌文本（.lic 文件内容，LP1. 开头；自动去首尾空白）
    static bool isCodeValid(const QString& code);

    /// 当前是否已授权：配置中的令牌有效，或 exe 旁 license.lic 有效
    static bool isAuthorized();

    /// SDK 原生库是否可用（缺失时授权校验恒为未授权）
    static bool sdkAvailable();

    /// SDK 诊断信息（未加载原因）
    static QString sdkStatusText();

    /// 令牌校验失败原因（供 UI 提示）
    static QString verifyMessage(const QString& code);

    /// 令牌摘要信息（不验签，仅供展示）：形如"订阅授权 · 有效期至 2026-12-31 · 客户 XXX"；
    /// 令牌无效、SDK 不可用或载荷解析失败时返回空串
    static QString tokenSummary(const QString& code);

    /// 生成加密激活请求文件（.ft）；失败时经 error 返回原因
    static bool generateActivationRequest(const QString& activationCode, const QString& outPath,
                                          QString* error = nullptr);

    /// exe 旁授权文件的约定路径（license.lic）
    static QString licenseFilePath();

private:
    LicenseManager() = default;
};
