#include "LicenseManager.h"

#include <QCryptographicHash>
#include <QSettings>
#include <QStringList>
#include <QSysInfo>

#include <cstring>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <intrin.h>
#include <windows.h>
#endif

namespace {

constexpr char kLicensePrefix[] = "FlowLicense.v1|";

QString cpuFingerprint()
{
    QStringList parts;
#ifdef Q_OS_WIN
    int regs[4] = {};
    __cpuid(regs, 0);
    char vendor[13] = {};
    std::memcpy(vendor, &regs[1], 4);
    std::memcpy(vendor + 4, &regs[3], 4);
    std::memcpy(vendor + 8, &regs[2], 4);
    parts << QString::fromLatin1(vendor).trimmed();

    __cpuid(regs, 0x80000000);
    const unsigned maxExt = static_cast<unsigned>(regs[0]);
    if (maxExt >= 0x80000004u) {
        char brand[49] = {};
        for (int i = 0; i < 3; ++i) {
            __cpuid(regs, 0x80000002 + i);
            std::memcpy(brand + i * 16, regs, 16);
        }
        parts << QString::fromLatin1(brand).trimmed();
    }

    __cpuid(regs, 1);
    parts << QString::number(static_cast<quint32>(regs[0]), 16);
    parts << QString::number(static_cast<quint32>(regs[2]), 16);
    parts << QString::number(static_cast<quint32>(regs[3]), 16);

    QSettings reg(QStringLiteral(R"(HKEY_LOCAL_MACHINE\HARDWARE\DESCRIPTION\System\CentralProcessor\0)"),
                  QSettings::NativeFormat);
    parts << reg.value(QStringLiteral("VendorIdentifier")).toString().trimmed();
    parts << reg.value(QStringLiteral("ProcessorNameString")).toString().simplified();
    parts << reg.value(QStringLiteral("Identifier")).toString().trimmed();

    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    parts << QString::number(sysInfo.dwNumberOfProcessors);
#else
    parts << QSysInfo::currentCpuArchitecture();
#endif
    return parts.join(QLatin1Char('|'));
}

QString formatDigest(const QByteArray& digest)
{
    const QString hex = QString::fromLatin1(digest.left(8).toHex()).toUpper();
    return hex.left(4) + QLatin1Char('-')
        + hex.mid(4, 4) + QLatin1Char('-')
        + hex.mid(8, 4) + QLatin1Char('-')
        + hex.mid(12, 4);
}

QString normalizeCode(QString code)
{
    code = code.trimmed().toUpper();
    code.remove(QLatin1Char('-'));
    code.remove(QLatin1Char(' '));
    return code;
}

} // namespace

QString LicenseManager::machineCode()
{
    static const QString code = formatDigest(
        QCryptographicHash::hash(cpuFingerprint().toUtf8(), QCryptographicHash::Sha256));
    return code;
}

QString LicenseManager::licenseFor(const QString& code)
{
    const QByteArray material = QByteArray(kLicensePrefix) + normalizeCode(code).toUtf8();
    return formatDigest(QCryptographicHash::hash(material, QCryptographicHash::Sha256));
}

bool LicenseManager::isCodeValid(const QString& code)
{
    const QString given = normalizeCode(code);
    if (given.isEmpty()) {
        return false;
    }
    const QString internalCode = normalizeCode(QString::fromUtf8(AppConfigs::InternalAuthorization));
    if (!internalCode.isEmpty() && given == internalCode) {
        return true;
    }
    const QString expected = normalizeCode(licenseFor(machineCode()));
    return given == expected;
}

bool LicenseManager::isAuthorized()
{
    return isCodeValid(ConfigManager::instance().getLicenseCode());
}
