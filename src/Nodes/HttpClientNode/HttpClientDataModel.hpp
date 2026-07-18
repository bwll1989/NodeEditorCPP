/**
 * @file HttpClientDataModel.hpp
 * @brief HTTP Client 节点数据模型
 *
 * 向指定 URL 发送 HTTP 请求（GET / POST / PUT / DELETE / PATCH），
 * 将响应解析为 VariableData 输出。
 *
 * 特性：
 * - 5 个输入端口：TRIGGER / PATH / BODY / BASE_URL / HEADERS
 * - 任意输入或界面参数变化时自动发送请求
 * - 请求进行中若有新输入，排队并在当前请求完成后自动重发
 * - 支持外部 OSC 地址控制（/baseUrl、/path、/body、/headers、/method、/send）
 *
 * 请求参数优先级（高 → 低）：
 * - Path：PATH 端口 > TRIGGER.path > 界面 Path
 * - Base URL：BASE_URL 端口 > 界面 Base URL
 * - Body：BODY 端口 > TRIGGER.body > 界面 Body
 * - Headers：HEADERS 端口 > TRIGGER.headers > 界面 Headers
 */
#pragma once

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QSignalBlocker>
#include <QTimer>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "HttpClientInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::ConnectionPolicy;

using namespace QtNodes;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @class HttpClientDataModel
     * @brief HTTP 客户端节点，基于 QNetworkAccessManager 异步发请求
     */
    class HttpClientDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        /** @brief HTTP 请求方法枚举，与界面 ComboBox 索引一一对应 */
        enum class HttpMethod : int {
            Get = 0,
            Post = 1,
            Put = 2,
            Delete = 3,
            Patch = 4
        };
        Q_ENUM(HttpMethod)

        /** @brief 输入端口索引 */
        enum InputPort : PortIndex {
            TriggerPort = 0,  ///< 触发端口，收到数据即发起请求
            PathPort = 1,     ///< 请求路径覆盖
            BodyPort = 2,     ///< 请求体覆盖（POST/PUT/PATCH）
            BaseUrlPort = 3,  ///< 基础 URL 覆盖
            HeadersPort = 4   ///< 请求头覆盖（JSON 对象字符串或 map）
        };

        /** @brief 输出端口索引 */
        enum OutputPort : PortIndex {
            ResponsePort = 0, ///< 完整响应数据（statusCode、success、JSON 字段等）
            DonePort = 1      ///< 请求完成脉冲（success 布尔值）
        };

        Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
        Q_PROPERTY(HttpMethod method READ method WRITE setMethod NOTIFY methodChanged)
        Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
        Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
        Q_PROPERTY(QString headers READ headers WRITE setHeaders NOTIFY headersChanged)

    public:
        /**
         * @brief 构造函数：初始化端口、网络管理器、界面信号与外部绑定
         */
        HttpClientDataModel()
        {
            InPortCount = 5;
            OutPortCount = 2;
            Caption = QStringLiteral("HTTP Client");
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = true;
            PortEditable = false;

            m_response = std::make_shared<VariableData>();
            m_doneOutput = std::make_shared<VariableData>(QVariant(false));

            m_manager = new QNetworkAccessManager(this);
            connect(m_manager, &QNetworkAccessManager::finished, this, &HttpClientDataModel::onReplyFinished);

            // 从界面控件同步初始值
            m_baseUrl = widget->baseUrlEdit->text().trimmed();
            m_path = widget->pathEdit->text().trimmed();
            m_body = widget->bodyEdit->toPlainText();
            m_headers = widget->headersEdit->text().trimmed();

            // Body 编辑防抖：连续输入 400ms 后才发请求，避免频繁请求
            m_bodyDebounce = new QTimer(this);
            m_bodyDebounce->setSingleShot(true);
            m_bodyDebounce->setInterval(400);
            connect(m_bodyDebounce, &QTimer::timeout, this, &HttpClientDataModel::sendRequest);

            // 界面参数变化时自动发送请求
            connect(widget->baseUrlEdit, &QLineEdit::editingFinished, this, [this]() {
                setBaseUrl(widget->baseUrlEdit->text());
                sendRequest();
            });
            connect(widget->pathEdit, &QLineEdit::editingFinished, this, [this]() {
                setPath(widget->pathEdit->text());
                sendRequest();
            });
            connect(widget->headersEdit, &QLineEdit::editingFinished, this, [this]() {
                setHeaders(widget->headersEdit->text());
                sendRequest();
            });
            connect(widget->bodyEdit, &QPlainTextEdit::textChanged, this, [this]() {
                setBody(widget->bodyEdit->toPlainText());
                m_bodyDebounce->start();
            });
            connect(widget->method, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int index) {
                        setMethod(static_cast<HttpMethod>(index));
                        sendRequest();
                    });
            connect(widget->sendButton, &QPushButton::clicked, this, &HttpClientDataModel::sendRequest);

            // 注册外部 OSC 地址绑定
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "baseUrl";
                b.control = widget->baseUrlEdit;
                AbstractDelegateModel::registerExternalBinding("/baseUrl", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "method";
                b.control = widget->method;
                AbstractDelegateModel::registerExternalBinding("/method", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "path";
                b.control = widget->pathEdit;
                AbstractDelegateModel::registerExternalBinding("/path", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "body";
                AbstractDelegateModel::registerExternalBinding("/body", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "headers";
                b.control = widget->headersEdit;
                AbstractDelegateModel::registerExternalBinding("/headers", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "send";
                b.control = widget->sendButton;
                AbstractDelegateModel::registerExternalBinding("/send", this, b);
            }

        }

        ~HttpClientDataModel() override = default;

        /** @brief 所有端口均使用 VariableData 类型 */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        /** @brief 返回端口显示名称 */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case TriggerPort: return QStringLiteral("TRIGGER");
                case PathPort: return QStringLiteral("PATH");
                case BodyPort: return QStringLiteral("BODY");
                case BaseUrlPort: return QStringLiteral("BASE_URL");
                case HeadersPort: return QStringLiteral("HEADERS");
                default: return QString();
                }
            case PortType::Out:
                switch (portIndex) {
                case ResponsePort: return QStringLiteral("RESPONSE");
                case DonePort: return QStringLiteral("DONE");
                default: return QString();
                }
            default:
                return QString();
            }
        }

        /** @brief 返回指定输出端口的当前数据 */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex) {
            case ResponsePort:
                return m_response;
            case DonePort:
                return m_doneOutput;
            default:
                return std::make_shared<VariableData>();
            }
        }

        /**
         * @brief 接收输入端口数据
         *
         * 任意输入端口数据变化时均自动调用 sendRequest()。
         * TRIGGER 上单纯的 true/false 仅作触发，不会误当作 path。
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }

            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }

            switch (portIndex) {
            case TriggerPort:
                m_triggerData = var;
                sendRequest();
                break;
            case PathPort:
                m_pathData = var;
                applyPathFromData(var);
                sendRequest();
                break;
            case BodyPort:
                m_bodyData = var;
                applyBodyFromData(var);
                sendRequest();
                break;
            case BaseUrlPort:
                m_baseUrlData = var;
                applyBaseUrlFromData(var);
                sendRequest();
                break;
            case HeadersPort:
                m_headersData = var;
                applyHeadersFromData(var);
                sendRequest();
                break;
            default:
                break;
            }
        }

        /** @brief 输入/输出端口均允许多连接 */
        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            Q_UNUSED(index)
            switch (portType) {
            case PortType::In:
            case PortType::Out:
                return ConnectionPolicy::Many;
            default:
                return ConnectionPolicy::One;
            }
        }

        /** @brief 序列化节点配置到 JSON */
        QJsonObject save() const override
        {
            QJsonObject values;
            values["baseUrl"] = m_baseUrl;
            values["method"] = static_cast<int>(m_method);
            values["path"] = m_path;
            values["body"] = m_body;
            values["headers"] = m_headers;

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        /** @brief 从 JSON 恢复节点配置 */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                setBaseUrl(v["baseUrl"].toString(QStringLiteral("http://127.0.0.1:8080")));
                setMethod(static_cast<HttpMethod>(v["method"].toInt(0)));
                setPath(v["path"].toString());
                setBody(v["body"].toString());
                setHeaders(v["headers"].toString());
            }
        }

        /** @brief 返回内嵌界面控件 */
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QString baseUrl() const { return m_baseUrl; }
        HttpMethod method() const { return m_method; }
        QString path() const { return m_path; }
        QString body() const { return m_body; }
        QString headers() const { return m_headers; }

    public slots:
        /** @brief 设置基础 URL 并同步到界面 */
        void setBaseUrl(const QString &url)
        {
            if (m_baseUrl == url) {
                return;
            }
            m_baseUrl = url;
            {
                QSignalBlocker blocker(widget->baseUrlEdit);
                widget->baseUrlEdit->setText(url);
            }
            Q_EMIT baseUrlChanged(url);
        }

        /** @brief 设置 HTTP 方法并同步到界面 */
        void setMethod(HttpMethod method)
        {

            m_method = method;
            {
                QSignalBlocker blocker(widget->method);
                widget->method->setCurrentIndex(static_cast<int>(method));
            }
            Q_EMIT methodChanged(method);
        }

        /** @brief 设置请求路径并同步到界面 */
        void setPath(const QString &path)
        {

            m_path = path;
            {
                QSignalBlocker blocker(widget->pathEdit);
                widget->pathEdit->setText(path);
            }
            Q_EMIT pathChanged(path);
        }

        /** @brief 设置请求体并同步到界面 */
        void setBody(const QString &body)
        {

            m_body = body;
            {
                QSignalBlocker blocker(widget->bodyEdit);
                widget->bodyEdit->setPlainText(body);
            }
            Q_EMIT bodyChanged(body);
        }

        /** @brief 设置请求头 JSON 并同步到界面 */
        void setHeaders(const QString &headers)
        {

            m_headers = headers;
            {
                QSignalBlocker blocker(widget->headersEdit);
                widget->headersEdit->setText(headers);
            }
            Q_EMIT headersChanged(headers);
        }

    signals:
        void baseUrlChanged(const QString &url);
        void methodChanged(HttpMethod method);
        void pathChanged(const QString &path);
        void bodyChanged(const QString &body);
        void headersChanged(const QString &headers);

    protected:
        /** @brief 模型就绪后订阅全局事件总线（OSC 外部控制） */
        void afterModelReady() override
        {
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/baseUrl"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/method"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/path"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/body"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/headers"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private slots:
        /**
         * @brief 发起 HTTP 请求
         *
         * 若当前已有请求进行中，设置 m_resendPending 排队，
         * 待 onReplyFinished 完成后自动重发。
         */
        void sendRequest()
        {
            if (m_pendingRequest) {
                m_resendPending = true;
                widget->statusLabel->setText(QStringLiteral("Queued..."));
                return;
            }

            syncParametersFromWidget();

            const QUrl url = buildUrl();
            if (!url.isValid() || url.host().isEmpty()) {
                widget->statusLabel->setText(QStringLiteral("Invalid URL"));
                publishError(QStringLiteral("Invalid URL: ") + url.toString());
                return;
            }

            QNetworkRequest request(url);
            applyHeaders(request);

            const QByteArray bodyBytes = buildBody();
            if (!bodyBytes.isEmpty()) {
                request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            }

            QNetworkReply *reply = nullptr;
            switch (m_method) {
            case HttpMethod::Get:
                reply = m_manager->get(request);
                break;
            case HttpMethod::Post:
                reply = m_manager->post(request, bodyBytes);
                break;
            case HttpMethod::Put:
                reply = m_manager->put(request, bodyBytes);
                break;
            case HttpMethod::Delete:
                reply = m_manager->deleteResource(request);
                break;
            case HttpMethod::Patch:
                reply = m_manager->sendCustomRequest(request, "PATCH", bodyBytes);
                break;
            }

            if (!reply) {
                widget->statusLabel->setText(QStringLiteral("Request failed"));
                publishError(QStringLiteral("Failed to create request"));
                return;
            }

            m_pendingRequest = true;
            widget->statusLabel->setText(QStringLiteral("Sending %1 %2").arg(widget->method->currentText(), url.toString()));
            widget->sendButton->setEnabled(false);
            AbstractDelegateModel::stateFeedBack("/send", true);
        }

        /**
         * @brief 网络请求完成回调
         *
         * 解析响应 JSON/文本，更新 RESPONSE 和 DONE 输出端口，
         * 若有排队请求则立即重发。
         */
        void onReplyFinished(QNetworkReply *reply)
        {
            m_pendingRequest = false;
            widget->sendButton->setEnabled(true);
            AbstractDelegateModel::stateFeedBack("/send", false);

            QVariantMap result;
            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const bool success = reply->error() == QNetworkReply::NoError;

            result.insert(QStringLiteral("statusCode"), statusCode);
            result.insert(QStringLiteral("success"), success);
            result.insert(QStringLiteral("url"), reply->url().toString());

            if (!success) {
                result.insert(QStringLiteral("error"), reply->errorString());
            }

            const QByteArray responseData = reply->readAll();
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

            // JSON 对象：顶层字段展开到 result；数组：放入 data；其他：放入 body
            if (doc.isObject()) {
                const QVariantMap obj = doc.object().toVariantMap();
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    result.insert(it.key(), it.value());
                }
            } else if (doc.isArray()) {
                result.insert(QStringLiteral("data"), doc.array().toVariantList());
            } else if (!responseData.isEmpty()) {
                result.insert(QStringLiteral("body"), QString::fromUtf8(responseData));
            }

            m_response = std::make_shared<VariableData>(result);
            m_doneOutput = std::make_shared<VariableData>(QVariant(success));
            Q_EMIT dataUpdated(ResponsePort);
            Q_EMIT dataUpdated(DonePort);

            widget->statusLabel->setText(
                success ? QStringLiteral("OK %1").arg(statusCode)
                        : QStringLiteral("Error %1: %2").arg(statusCode).arg(reply->errorString()));

            reply->deleteLater();

            // 处理排队中的请求
            if (m_resendPending) {
                m_resendPending = false;
                QTimer::singleShot(0, this, &HttpClientDataModel::sendRequest);
            }
        }

        /**
         * @brief 处理全局事件总线命令
         *
         * 外部 OSC 地址变更参数后自动发送请求；
         * /send 仅在 payload 为 true 时触发。
         */
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addr = ev.address;
            if (addr == makeFullOscAddress("/baseUrl")) {
                setBaseUrl(ev.payload.toString());
                sendRequest();
            } else if (addr == makeFullOscAddress("/path")) {
                setPath(ev.payload.toString());
                sendRequest();
            } else if (addr == makeFullOscAddress("/body")) {
                setBody(ev.payload.toString());
                sendRequest();
            } else if (addr == makeFullOscAddress("/headers")) {
                setHeaders(ev.payload.toString());
                sendRequest();
            } else if (addr == makeFullOscAddress("/method")) {
                setMethod(static_cast<HttpMethod>(ev.payload.toInt()));
                sendRequest();
            } else if (addr == makeFullOscAddress("/send")) {
                if (ev.payload.toBool()) {
                    sendRequest();
                }
            }
        }

    private:
        /** @brief 从界面控件同步最新参数到成员变量（发送前调用） */
        void syncParametersFromWidget()
        {
            m_baseUrl = widget->baseUrlEdit->text().trimmed();
            m_path = widget->pathEdit->text().trimmed();
            m_body = widget->bodyEdit->toPlainText();
            m_headers = widget->headersEdit->text().trimmed();
        }

        /**
         * @brief 从 VariableData 提取路径值
         *
         * 支持 `path` 键，或 `default` 中以 `/`、`http://`、`https://` 开头的字符串。
         * 纯布尔 true/false 不会被视为路径。
         */
        static QString extractPathLikeValue(const std::shared_ptr<VariableData> &data)
        {
            if (!data || data->isEmpty()) {
                return {};
            }

            if (data->hasKey(QStringLiteral("path"))) {
                return data->value(QStringLiteral("path")).toString().trimmed();
            }

            if (data->hasKey(QStringLiteral("default"))) {
                const QVariant value = data->value(QStringLiteral("default"));
                if (value.typeId() == QMetaType::QString) {
                    const QString text = value.toString().trimmed();
                    if (text.startsWith(QLatin1Char('/'))
                        || text.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
                        || text.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
                        return text;
                    }
                }
            }

            return {};
        }

        /**
         * @brief 从 VariableData 提取基础 URL
         *
         * 支持 `baseUrl` 键，或 `default` 中以 `http://`、`https://` 开头的字符串。
         */
        static QString extractBaseUrlValue(const std::shared_ptr<VariableData> &data)
        {
            if (!data || data->isEmpty()) {
                return {};
            }

            if (data->hasKey(QStringLiteral("baseUrl"))) {
                return data->value(QStringLiteral("baseUrl")).toString().trimmed();
            }

            if (data->hasKey(QStringLiteral("default"))) {
                const QVariant value = data->value(QStringLiteral("default"));
                if (value.typeId() == QMetaType::QString) {
                    const QString text = value.toString().trimmed();
                    if (text.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
                        || text.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
                        return text;
                    }
                }
            }

            return {};
        }

        /** @brief 从端口数据提取路径并同步到界面 */
        void applyPathFromData(const std::shared_ptr<VariableData> &data)
        {
            const QString path = extractPathLikeValue(data);
            if (!path.isEmpty()) {
                setPath(path);
            }
        }

        /**
         * @brief 从端口数据提取请求体并同步到界面
         *
         * 支持 `body` 键（字符串或 map），或 `default` 字符串。
         */
        void applyBodyFromData(const std::shared_ptr<VariableData> &data)
        {
            if (!data || data->isEmpty()) {
                return;
            }

            if (data->hasKey(QStringLiteral("body"))) {
                const QVariant bodyVar = data->value(QStringLiteral("body"));
                if (bodyVar.typeId() == QMetaType::QString) {
                    setBody(bodyVar.toString());
                } else {
                    const QJsonObject obj = QJsonObject::fromVariantMap(bodyVar.toMap());
                    setBody(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
                }
                return;
            }

            if (data->hasKey(QStringLiteral("default")) && data->value().typeId() == QMetaType::QString) {
                setBody(data->value().toString());
            }
        }

        /** @brief 从端口数据提取基础 URL 并同步到界面 */
        void applyBaseUrlFromData(const std::shared_ptr<VariableData> &data)
        {
            const QString baseUrl = extractBaseUrlValue(data);
            if (!baseUrl.isEmpty()) {
                setBaseUrl(baseUrl);
            }
        }

        /** @brief 将 QVariantMap 序列化为紧凑 JSON 字符串 */
        static QString serializeHeadersMap(const QVariantMap &map)
        {
            return QString::fromUtf8(
                QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact));
        }

        /**
         * @brief 从 VariableData 提取请求头 JSON 字符串
         *
         * 支持：
         * - `headers` 键（JSON 字符串或 map）
         * - `default` 键（以 `{` 或 `[` 开头的 JSON 字符串，或 map）
         * - 整个 map 直接作为请求头键值对
         */
        static QString extractHeadersValue(const std::shared_ptr<VariableData> &data)
        {
            if (!data || data->isEmpty()) {
                return {};
            }

            if (data->hasKey(QStringLiteral("headers"))) {
                const QVariant value = data->value(QStringLiteral("headers"));
                if (value.typeId() == QMetaType::QString) {
                    return value.toString().trimmed();
                }
                if (value.typeId() == QMetaType::QVariantMap) {
                    return serializeHeadersMap(value.toMap());
                }
            }

            if (data->hasKey(QStringLiteral("default"))) {
                const QVariant value = data->value(QStringLiteral("default"));
                if (value.typeId() == QMetaType::QString) {
                    const QString text = value.toString().trimmed();
                    if (text.startsWith(QLatin1Char('{')) || text.startsWith(QLatin1Char('['))) {
                        return text;
                    }
                } else if (value.typeId() == QMetaType::QVariantMap) {
                    return serializeHeadersMap(value.toMap());
                }
            }

            const QVariantMap map = data->getMap();
            if (!map.isEmpty()) {
                return serializeHeadersMap(map);
            }

            return {};
        }

        /** @brief 从端口数据提取请求头并同步到界面 */
        void applyHeadersFromData(const std::shared_ptr<VariableData> &data)
        {
            const QString headers = extractHeadersValue(data);
            if (!headers.isEmpty()) {
                setHeaders(headers);
            }
        }

        /**
         * @brief 解析最终请求头文本
         *
         * 优先级：HEADERS 端口 > TRIGGER.headers > 界面 Headers。
         */
        QString resolveHeadersText() const
        {
            QString headersText = extractHeadersValue(m_headersData);
            if (headersText.isEmpty() && m_triggerData) {
                headersText = extractHeadersValue(m_triggerData);
            }
            if (headersText.isEmpty()) {
                headersText = m_headers.trimmed();
            }
            return headersText;
        }

        /**
         * @brief 组装最终请求 URL
         *
         * Path 优先级：PATH 端口 > TRIGGER.path > 界面 Path。
         * Base URL 优先级：BASE_URL 端口 > 界面 Base URL。
         * Path 为完整 URL 时直接使用，否则与 Base URL 拼接。
         */
        QUrl buildUrl() const
        {
            QString path = extractPathLikeValue(m_pathData);
            if (path.isEmpty()) {
                path = extractPathLikeValue(m_triggerData);
            }
            if (path.isEmpty()) {
                path = m_path.trimmed();
            }

            QString base = extractBaseUrlValue(m_baseUrlData);
            if (base.isEmpty()) {
                base = m_baseUrl.trimmed();
            }

            QString combined;
            if (path.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
                || path.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
                combined = path;
            } else {
                while (base.endsWith(QLatin1Char('/'))) {
                    base.chop(1);
                }
                if (!path.startsWith(QLatin1Char('/')) && !path.isEmpty()) {
                    path.prepend(QLatin1Char('/'));
                }
                combined = base + path;
            }

            QUrl url = QUrl::fromUserInput(combined);
            if (!url.isValid() && !combined.isEmpty()) {
                url = QUrl(combined);
            }
            return url;
        }

        /**
         * @brief 组装请求体字节流
         *
         * GET/DELETE 不携带 body。
         * Body 优先级：BODY 端口 > TRIGGER.body > 界面 Body。
         */
        QByteArray buildBody() const
        {
            if (m_method == HttpMethod::Get || m_method == HttpMethod::Delete) {
                return {};
            }

            QString bodyText = m_body;
            if (m_bodyData && !m_bodyData->isEmpty()) {
                if (m_bodyData->value().typeId() == QMetaType::QString) {
                    bodyText = m_bodyData->value().toString();
                } else {
                    const QJsonObject obj = QJsonObject::fromVariantMap(m_bodyData->getMap());
                    bodyText = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                }
            } else if (m_triggerData && m_triggerData->hasKey(QStringLiteral("body"))) {
                const QVariant bodyVar = m_triggerData->value(QStringLiteral("body"));
                if (bodyVar.typeId() == QMetaType::QString) {
                    bodyText = bodyVar.toString();
                } else {
                    const QJsonObject obj = QJsonObject::fromVariantMap(bodyVar.toMap());
                    bodyText = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                }
            }

            return bodyText.toUtf8();
        }

        /**
         * @brief 将 JSON 请求头应用到 QNetworkRequest
         *
         * Headers 文本须为 JSON 对象，如 {"Authorization":"Bearer token"}。
         */
        void applyHeaders(QNetworkRequest &request) const
        {
            const QString headersText = resolveHeadersText();
            if (headersText.isEmpty()) {
                return;
            }

            QJsonParseError error;
            const QJsonDocument doc = QJsonDocument::fromJson(headersText.toUtf8(), &error);
            if (!doc.isObject()) {
                return;
            }

            const QJsonObject headers = doc.object();
            for (auto it = headers.begin(); it != headers.end(); ++it) {
                request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
            }
        }

        /**
         * @brief 发布本地错误到输出端口（URL 无效、请求创建失败等）
         * @param message 错误描述
         */
        void publishError(const QString &message)
        {
            QVariantMap result;
            result.insert(QStringLiteral("success"), false);
            result.insert(QStringLiteral("error"), message);
            m_response = std::make_shared<VariableData>(result);
            m_doneOutput = std::make_shared<VariableData>(QVariant(false));
            Q_EMIT dataUpdated(ResponsePort);
            Q_EMIT dataUpdated(DonePort);
        }

        HttpClientInterface *widget = new HttpClientInterface(); ///< 内嵌界面
        QNetworkAccessManager *m_manager = nullptr;                    ///< 异步网络管理器

        std::shared_ptr<VariableData> m_triggerData;   ///< TRIGGER 端口缓存数据
        std::shared_ptr<VariableData> m_pathData;      ///< PATH 端口缓存数据
        std::shared_ptr<VariableData> m_bodyData;      ///< BODY 端口缓存数据
        std::shared_ptr<VariableData> m_baseUrlData;   ///< BASE_URL 端口缓存数据
        std::shared_ptr<VariableData> m_headersData;   ///< HEADERS 端口缓存数据
        std::shared_ptr<VariableData> m_response;      ///< RESPONSE 输出数据
        std::shared_ptr<VariableData> m_doneOutput;    ///< DONE 输出数据

        QTimer *m_bodyDebounce = nullptr; ///< Body 编辑防抖定时器

        QString m_baseUrl = QStringLiteral("http://127.0.0.1:8080"); ///< 基础 URL
        QString m_path;                                              ///< 请求路径
        QString m_body;                                              ///< 请求体
        QString m_headers;                                           ///< 请求头 JSON
        HttpMethod m_method = HttpMethod::Get;                       ///< HTTP 方法
        bool m_pendingRequest = false;                               ///< 是否有请求进行中
        bool m_resendPending = false;                              ///< 是否有排队待重发的请求
    };
}
