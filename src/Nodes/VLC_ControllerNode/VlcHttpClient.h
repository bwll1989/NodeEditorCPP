/**
 * @file VlcHttpClient.h
 * @brief VLC HTTP Web 接口专用客户端
 *
 * 封装对 VLC Lua HTTP 接口的 GET 请求，负责：
 * - HTTP Basic 认证（兼容两种常见格式）
 * - JSON 响应解析
 * - 401 时自动切换认证方式重试
 * - 并发请求隔离（每个 QNetworkReply 独立保存上下文）
 *
 * VLC 接口文档：https://wiki.videolan.org/VLC_HTTP_requests/
 *
 * 典型请求示例：
 *   GET http://127.0.0.1:8080/requests/status.json?command=volume&val=128
 *   Authorization: Basic base64(":password")
 */
#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace Nodes
{
    /**
     * @class VlcHttpClient
     * @brief 异步 GET 客户端，向 VLC 发送 HTTP 请求并解析 JSON 响应
     *
     * VLC Web 接口常见两种 Basic 认证方式：
     * 1. 用户名为空、密码为 Lua HTTP 密码 —— 对应浏览器登录表单（:password）
     * 2. 用户名即密码、密码为空 —— 对应部分文档示例 http://password@host
     *
     * 收到 401 时会从方式 1 自动切换到方式 2 重试一次。
     */
    class VlcHttpClient : public QObject
    {
        Q_OBJECT
    public:
        /** @brief HTTP Basic 认证模式 */
        enum class AuthMode
        {
            EmptyUserPassword,  ///< 凭证格式 ":password"
            PasswordAsUser      ///< 凭证格式 "password:"
        };

        explicit VlcHttpClient(QObject *parent = nullptr)
            : QObject(parent)
        {
            connect(&manager_, &QNetworkAccessManager::finished,
                    this, &VlcHttpClient::onReplyFinished);
        }

        /**
         * @brief 发起 GET 请求
         * @param url      完整请求 URL（含 query 参数）
         * @param password VLC Lua HTTP 密码；为空时不附加 Authorization 头
         */
        void sendGet(const QUrl &url, const QString &password)
        {
            sendRequest(url, password, AuthMode::EmptyUserPassword);
        }

    Q_SIGNALS:
        /** @brief 请求成功且 JSON 解析完成 */
        void jsonReady(QJsonObject json);
        /** @brief 网络错误、HTTP 错误或 JSON 解析失败 */
        void requestFailed(QString errorMessage, int httpStatus);

    private:
        /** @brief 保存在 QNetworkReply 上的动态属性键名 */
        static constexpr char kPasswordProperty[] = "vlcPassword";
        static constexpr char kAuthModeProperty[] = "vlcAuthMode";
        static constexpr char kUrlProperty[] = "vlcUrl";

        /**
         * @brief 向请求头写入 Authorization: Basic ...
         * @param mode EmptyUserPassword → ":password"；PasswordAsUser → "password:"
         */
        static void applyBasicAuth(QNetworkRequest &request,
                                   const QString &password,
                                   AuthMode mode)
        {
            if (password.isEmpty()) {
                return;
            }

            QString credentials;
            if (mode == AuthMode::PasswordAsUser) {
                credentials = password + QLatin1Char(':');
            } else {
                credentials = QLatin1Char(':') + password;
            }

            const QByteArray token = credentials.toUtf8().toBase64();
            request.setRawHeader("Authorization", "Basic " + token);
        }

        /**
         * @brief 实际发送 GET 请求，并将上下文绑定到 reply 对象
         *
         * 清除 URL 内嵌的用户名/密码，统一通过 Authorization 头认证，
         * 避免 QUrl 自动编码与 Basic 头冲突。
         */
        void sendRequest(const QUrl &url, const QString &password, AuthMode mode)
        {
            QUrl requestUrl = url;
            requestUrl.setUserName(QString());
            requestUrl.setPassword(QString());

            QNetworkRequest request(requestUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            applyBasicAuth(request, password, mode);

            QNetworkReply *reply = manager_.get(request);
            reply->setProperty(kPasswordProperty, password);
            reply->setProperty(kAuthModeProperty, static_cast<int>(mode));
            reply->setProperty(kUrlProperty, url);
        }

        /**
         * @brief QNetworkAccessManager::finished 回调
         *
         * 处理流程：
         * 1. 401 + 首次认证 → 切换 PasswordAsUser 重试
         * 2. 其他网络错误 → requestFailed
         * 3. JSON 对象 → jsonReady（status.json 命令响应）
         * 4. JSON 数组 → 包装为 {"_playlistItems": [...]} 后 jsonReady
         */
        void onReplyFinished(QNetworkReply *reply)
        {
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray body = reply->readAll();
            const QString password = reply->property(kPasswordProperty).toString();
            const auto authMode = static_cast<AuthMode>(reply->property(kAuthModeProperty).toInt());
            const QUrl originalUrl = reply->property(kUrlProperty).toUrl();

            if (reply->error() != QNetworkReply::NoError) {
                if (httpStatus == 401
                    && !password.isEmpty()
                    && authMode == AuthMode::EmptyUserPassword) {
                    reply->deleteLater();
                    sendRequest(originalUrl, password, AuthMode::PasswordAsUser);
                    return;
                }

                reply->deleteLater();
                emit requestFailed(
                    QString("%1 (HTTP %2)").arg(reply->errorString()).arg(httpStatus),
                    httpStatus);
                return;
            }

            reply->deleteLater();

            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                const QString preview = QString::fromUtf8(body.left(120)).trimmed();
                emit requestFailed(
                    QString("JSON 解析失败 (HTTP %1): %2。片段: %3")
                        .arg(httpStatus)
                        .arg(parseError.errorString())
                        .arg(preview),
                    httpStatus);
                return;
            }

            if (doc.isObject()) {
                emit jsonReady(doc.object());
                return;
            }

            if (doc.isArray()) {
                // playlist.json 部分版本返回扁平数组，包装后统一交给节点解析
                QJsonObject wrapped;
                wrapped.insert(QStringLiteral("_playlistItems"), doc.array());
                emit jsonReady(wrapped);
                return;
            }

            emit requestFailed(QString("未知 JSON 格式 (HTTP %1)").arg(httpStatus), httpStatus);
        }

        QNetworkAccessManager manager_;
    };
}
