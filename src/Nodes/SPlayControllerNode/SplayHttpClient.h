/**
 * @file SplayHttpClient.h
 * @brief ENTTEC S-Play HTTP API 客户端（POST /api + JSON command）
 *
 * 文档：https://github.com/ENTTEC/SplayApi
 * 典型请求：POST http://192.168.1.13/api  Body: {"command":88}
 */
#pragma once

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariant>

namespace Nodes
{
    /**
     * @class SplayHttpClient
     * @brief 异步 POST JSON 客户端；每个 reply 携带可选 context 便于区分请求类型
     *
     * 用 epoch 丢弃换主机 / abort 之后才返回的过期应答，避免「先已连接再被旧超时打成未连接」。
     */
    class SplayHttpClient : public QObject
    {
        Q_OBJECT
    public:
        static constexpr int kTransferTimeoutMs = 8000;

        explicit SplayHttpClient(QObject *parent = nullptr)
            : QObject(parent)
        {
            // 设备/本机模拟器走直连，避免系统 HTTP 代理把 127.0.0.1 请求挂死
            manager_.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
            connect(&manager_, &QNetworkAccessManager::finished,
                    this, &SplayHttpClient::onReplyFinished);
        }

        /**
         * @brief 向 /api 发送 JSON 命令
         * @param url     完整 URL，如 http://host:80/api
         * @param body    请求体（须含 command）
         * @param context 可选上下文（如 "poll" / "play" / "stop"），原样回传
         */
        void postJson(const QUrl &url, const QJsonObject &body, const QVariant &context = {})
        {
            if (!url.isValid() || url.host().isEmpty()) {
                emit requestFailed(
                    QStringLiteral("Invalid URL: %1").arg(url.toString()), 0, context);
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            request.setTransferTimeout(kTransferTimeoutMs);
#endif

            const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
            QNetworkReply *reply = manager_.post(request, payload);
            reply->setProperty(kContextProperty, context);
            reply->setProperty(kBodyProperty, payload);
            reply->setProperty(kEpochProperty, m_epoch);
        }

        /**
         * @brief 作废并中止所有未完成请求（换主机时调用）
         *
         * 先递增 epoch，后 abort：即使旧 reply 以 TransferTimeout 结束而非 Canceled，
         * 也会因 epoch 不匹配被丢弃，不会回调 requestFailed。
         */
        void abortAll()
        {
            ++m_epoch;
            const auto replies = findChildren<QNetworkReply *>();
            for (QNetworkReply *reply : replies) {
                if (!reply) {
                    continue;
                }
                reply->setProperty(kAbortedProperty, true);
                reply->abort();
            }
        }

        int epoch() const { return m_epoch; }

    Q_SIGNALS:
        /** 请求成功且响应为 JSON 对象 */
        void jsonReady(QJsonObject json, QVariant context);
        /** 网络错误、HTTP 错误、JSON 解析失败，或 result==false */
        void requestFailed(QString errorMessage, int httpStatus, QVariant context);

    private:
        static constexpr char kContextProperty[] = "splayContext";
        static constexpr char kBodyProperty[] = "splayBody";
        static constexpr char kAbortedProperty[] = "splayAborted";
        static constexpr char kEpochProperty[] = "splayEpoch";

        void onReplyFinished(QNetworkReply *reply)
        {
            const QVariant context = reply->property(kContextProperty);
            const int replyEpoch = reply->property(kEpochProperty).toInt();
            const bool aborted = reply->property(kAbortedProperty).toBool();
            const auto error = reply->error();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QString errorString = reply->errorString();

            // 过期 / 主动 abort：静默丢弃（含换主机后迟到的 timeout）
            if (aborted
                || replyEpoch != m_epoch
                || error == QNetworkReply::OperationCanceledError) {
                reply->deleteLater();
                return;
            }

            // 失败时不要 readAll：未打开的 QNetworkReply 会打
            // "QIODevice::read (QNetworkReplyHttpImpl): device not open"
            if (error != QNetworkReply::NoError) {
                reply->deleteLater();
                emit requestFailed(
                    QStringLiteral("%1 (HTTP %2)").arg(errorString).arg(httpStatus),
                    httpStatus,
                    context);
                return;
            }

            const QByteArray body = reply->isOpen() ? reply->readAll() : QByteArray();
            reply->deleteLater();

            // 读完后再确认一次 epoch，避免处理过程中被 abortAll
            if (replyEpoch != m_epoch) {
                return;
            }

            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                const QString preview = QString::fromUtf8(body.left(120)).trimmed();
                emit requestFailed(
                    QStringLiteral("JSON 解析失败 (HTTP %1): %2。片段: %3")
                        .arg(httpStatus)
                        .arg(parseError.errorString())
                        .arg(preview),
                    httpStatus,
                    context);
                return;
            }

            const QJsonObject obj = doc.object();
            // GET_ALL_PLAYLISTS 等查询可能无 result 字段，仅在明确 false 时判失败
            if (obj.contains(QStringLiteral("result")) && !obj.value(QStringLiteral("result")).toBool()) {
                const QString err = obj.value(QStringLiteral("error")).toString(QStringLiteral("result=false"));
                emit requestFailed(err, httpStatus, context);
                return;
            }

            emit jsonReady(obj, context);
        }

        QNetworkAccessManager manager_;
        int m_epoch = 0;
    };
}
