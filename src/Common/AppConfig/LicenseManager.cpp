#include "LicenseManager.h"

#include "LicenseClientSdk.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

namespace {

/// 清理粘贴的令牌：去除 BOM 与全部空白字符（令牌本身只含 base64url 与 "."，
/// 邮件/文档换行粘贴混入的空格、换行、制表符一律剥除后再校验）
QString cleanToken(QString token)
{
    token.remove(QChar::ByteOrderMark);
    token.remove(QChar(' '));
    token.remove(QChar('\t'));
    token.remove(QChar('\r'));
    token.remove(QChar('\n'));
    return token.trimmed();
}

/// 载荷字段取值：兼容 PascalCase / camelCase 两种序列化风格
QJsonValue payloadValue(const QJsonObject& obj, const char* name)
{
    QJsonValue v = obj.value(QLatin1String(name));
    if (v.isUndefined() || v.isNull()) {
        std::string camel = name;
        camel[0] = static_cast<char>(::tolower(camel[0]));
        v = obj.value(QLatin1String(camel.c_str()));
    }
    return v;
}

} // namespace

QString LicenseManager::machineCode()
{
    return LicenseClientSdk::instance().machineCode();
}

bool LicenseManager::isCodeValid(const QString& code)
{
    const QString token = cleanToken(code);
    if (token.isEmpty()) {
        return false;
    }
    return LicenseClientSdk::instance().verifyToken(token).valid;
}

QString LicenseManager::licenseFilePath()
{
    if (!QCoreApplication::instance()) {
        return {};
    }
    return QCoreApplication::applicationDirPath() + QStringLiteral("/license.lic");
}

bool LicenseManager::isAuthorized()
{
    auto& sdk = LicenseClientSdk::instance();
    if (!sdk.available()) {
        return false;
    }

    // 来源 1：设置页粘贴并持久化的授权令牌
    const QString stored = cleanToken(ConfigManager::instance().getLicenseCode());
    if (!stored.isEmpty() && sdk.verifyToken(stored).valid) {
        return true;
    }

    // 来源 2：exe 旁的 license.lic 文件（部署/运维友好）
    const QString path = licenseFilePath();
    if (!path.isEmpty() && QFile::exists(path)) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString content = cleanToken(QString::fromUtf8(file.readAll()));
            if (!content.isEmpty() && sdk.verifyToken(content).valid) {
                return true;
            }
        }
    }
    return false;
}

bool LicenseManager::sdkAvailable()
{
    return LicenseClientSdk::instance().available();
}

QString LicenseManager::sdkStatusText()
{
    return LicenseClientSdk::instance().statusText();
}

QString LicenseManager::verifyMessage(const QString& code)
{
    return LicenseClientSdk::instance().verifyToken(code).message;
}

QString LicenseManager::tokenSummary(const QString& code)
{
    const QString token = cleanToken(code);
    if (token.isEmpty() || !LicenseClientSdk::instance().available()) {
        return {};
    }

    // SDK 的 lc_read_license_payload 只接受 .lic 文件路径：令牌写入临时文件再读载荷
    //（该接口不验签名，仅供展示；是否有效以 isCodeValid 为准）
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return {};
    }
    const QString tempPath = tempDir.filePath(QStringLiteral("token.lic"));
    {
        QFile file(tempPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return {};
        }
        file.write(token.toUtf8());
    }

    const QString payloadJson = LicenseClientSdk::instance().licensePayload(tempPath);
    if (payloadJson.isEmpty()) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(payloadJson.toUtf8());
    if (!doc.isObject()) {
        return {};
    }
    const QJsonObject obj = doc.object();

    QString summary;
    const QString type = payloadValue(obj, "Type").toString().toLower();
    if (type == QLatin1String("trial")) {
        summary = QStringLiteral("试用授权");
    } else if (type == QLatin1String("subscription")) {
        summary = QStringLiteral("订阅授权");
    } else if (type == QLatin1String("perpetual")) {
        summary = QStringLiteral("永久授权");
    } else {
        summary = QStringLiteral("授权");
    }

    const QString expires = payloadValue(obj, "ExpiresAt").toString();
    if (!expires.isEmpty()) {
        QString datePart = expires;
        const int t = datePart.indexOf(QLatin1Char('T'));
        if (t > 0) {
            datePart.truncate(t);
        }
        summary += QStringLiteral(" · 有效期至 ") + datePart;
    }

    const QString customer = payloadValue(obj, "CustomerCode").toString();
    if (!customer.isEmpty()) {
        summary += QStringLiteral(" · 客户 ") + customer;
    }

    return summary;
}

bool LicenseManager::generateActivationRequest(const QString& activationCode, const QString& outPath,
                                               QString* error)
{
    QString reason;
    const bool ok = LicenseClientSdk::instance().generateRequest(activationCode, outPath, &reason);
    if (error) {
        *error = ok ? QString()
                    : (reason.isEmpty() ? QStringLiteral("生成激活请求文件失败") : reason);
    }
    return ok;
}
