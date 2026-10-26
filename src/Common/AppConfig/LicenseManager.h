#pragma once

#include <QString>
#include "ConfigManager.h"

/**
 * 根据 CPU 信息生成机器码，并用内置密钥校验授权码。
 * 授权码 = SHA256("FlowLicense.v1|" + 去掉横线的机器码) 的前 8 字节，格式为 XXXX-XXXX-XXXX-XXXX。
 */
class APPCONFIG_EXPORT LicenseManager
{
public:
    static QString machineCode();
    static QString licenseFor(const QString& machineCode);
    static bool isCodeValid(const QString& code);
    static bool isAuthorized();

private:
    LicenseManager() = default;
};
