#include "PingDataModel.hpp"

#ifdef Q_OS_WIN
#include "IcmpPing.hpp"
#include <QtConcurrent/QtConcurrent>
#else
#include <QStringList>
#endif

namespace Nodes {

PingDataModel::PingDataModel()
{
    InPortCount = 1;
    OutPortCount = 1;
    Caption = PLUGIN_NAME;
    CaptionVisible = true;
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = false;

    widget = new PingInterface();
    m_connectedOutput = std::make_shared<VariableData>(QVariant(false));
    m_host = widget->hostEdit->text().trimmed();
    widget->updateConnectionStatus(false);

    m_timer = new QTimer(this);
    m_timer->setInterval(kPingIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &PingDataModel::startPing);

#ifdef Q_OS_WIN
    m_pingWatcher = new QFutureWatcher<PingResult>(this);
    connect(m_pingWatcher, &QFutureWatcher<PingResult>::finished, this, &PingDataModel::onIcmpPingFinished);
#else
    m_process = new QProcess(this);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &PingDataModel::onPingProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            setConnected(false);
        }
    });
#endif

    connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(widget->hostEdit->text());
    });
    connect(widget->enableCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        setEnabled(checked);
    });

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("host");
        b.control = widget->hostEdit;
        AbstractDelegateModel::registerExternalBinding("/host", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("enabled");
        b.control = widget->enableCheckBox;
        AbstractDelegateModel::registerExternalBinding("/enable", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("connected");
        b.control = widget->connectionLabel;
        AbstractDelegateModel::registerExternalBinding("/connected", this, b);
    }
}

PingDataModel::~PingDataModel()
{
    m_timer->stop();
    ++m_pingGeneration;

#ifdef Q_OS_WIN
    if (m_pingWatcher->isRunning()) {
        m_pingWatcher->waitForFinished();
    }
#else
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(500);
    }
#endif
}

NodeDataType PingDataModel::dataType(PortType, PortIndex) const
{
    return VariableData().type();
}

QString PingDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In && portIndex == EnablePort) {
        return QStringLiteral("ENABLE");
    }
    if (portType == PortType::Out && portIndex == ConnectedPort) {
        return QStringLiteral("CONNECTED");
    }
    return {};
}

std::shared_ptr<NodeData> PingDataModel::outData(PortIndex const portIndex)
{
    if (portIndex == ConnectedPort) {
        return m_connectedOutput;
    }
    return nullptr;
}

void PingDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
{
    if (portIndex != EnablePort) {
        return;
    }
    auto var = std::dynamic_pointer_cast<VariableData>(data);
    if (!var) {
        setEnabled(false);
        return;
    }
    setEnabled(isBoolTrue(*var));
}

ConnectionPolicy PingDataModel::portConnectionPolicy(PortType portType, PortIndex) const
{
    return (portType == PortType::In || portType == PortType::Out) ? ConnectionPolicy::Many
                                                                   : ConnectionPolicy::One;
}

QJsonObject PingDataModel::save() const
{
    QJsonObject values;
    values[QStringLiteral("host")] = m_host;
    values[QStringLiteral("enabled")] = m_enabled;
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson[QStringLiteral("values")] = values;
    return modelJson;
}

void PingDataModel::load(const QJsonObject &p)
{
    const QJsonValue v = p.value(QStringLiteral("values"));
    if (!v.isObject()) {
        return;
    }
    const QJsonObject values = v.toObject();
    setHost(values.value(QStringLiteral("host")).toString(m_host));
    if (values.contains(QStringLiteral("enabled"))) {
        setEnabled(values.value(QStringLiteral("enabled")).toBool());
    }
}

QWidget *PingDataModel::embeddedWidget()
{
    return widget;
}

void PingDataModel::setHost(const QString &host)
{
    const QString trimmed = host.trimmed();
    if (m_host == trimmed) {
        return;
    }
    m_host = trimmed;
    {
        QSignalBlocker blocker(widget->hostEdit);
        widget->hostEdit->setText(m_host);
    }
    Q_EMIT hostChanged(m_host);
}

void PingDataModel::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    {
        QSignalBlocker blocker(widget->enableCheckBox);
        widget->enableCheckBox->setChecked(m_enabled);
    }
    Q_EMIT enabledChanged(m_enabled);

    if (m_enabled) {
        startPing();
        m_timer->start();
    } else {
        m_timer->stop();
        invalidatePendingPing();
        setConnected(false);
    }
}

void PingDataModel::invalidatePendingPing()
{
#ifdef Q_OS_WIN
    ++m_pingGeneration;
    m_pingInFlight = false;
#else
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
#endif
}

void PingDataModel::startPing()
{
#ifdef Q_OS_WIN
    if (m_pingInFlight.load()) {
        return;
    }

    const QString host = widget->hostEdit->text().trimmed();
    m_host = host;
    if (host.isEmpty()) {
        setConnected(false);
        return;
    }

    const int generation = m_pingGeneration.load();
    m_pingInFlight = true;

    QFuture<PingResult> future = QtConcurrent::run([host, generation]() {
        const bool ok = IcmpPing::ping(host, kPingTimeoutMs);
        return PingResult{generation, ok};
    });
    m_pingWatcher->setFuture(future);
#else
    if (m_process->state() != QProcess::NotRunning) {
        return;
    }

    const QString host = widget->hostEdit->text().trimmed();
    m_host = host;
    if (host.isEmpty()) {
        setConnected(false);
        return;
    }

    QStringList args;
    args << QStringLiteral("-c") << QStringLiteral("1") << QStringLiteral("-W") << QStringLiteral("3")
         << host;
    m_process->start(QStringLiteral("ping"), args);
#endif
}

#ifdef Q_OS_WIN
void PingDataModel::onIcmpPingFinished()
{
    m_pingInFlight = false;

    const PingResult result = m_pingWatcher->result();
    if (result.first != m_pingGeneration.load()) {
        return;
    }
    setConnected(result.second);
}
#else
void PingDataModel::onPingProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);
    setConnected(ok);
}
#endif

void PingDataModel::setConnected(bool connected)
{
    if (m_connected == connected) {
        return;
    }
    m_connected = connected;
    widget->updateConnectionStatus(m_connected);
    m_connectedOutput = std::make_shared<VariableData>(QVariant(m_connected));
    Q_EMIT dataUpdated(ConnectedPort);
    Q_EMIT connectedChanged(m_connected);
}

bool PingDataModel::isBoolTrue(const VariableData &data)
{
    const QVariant value = data.value();
    if (value.typeId() == QMetaType::Bool) {
        return value.toBool();
    }
    if (value.canConvert<int>()) {
        return value.toInt() != 0;
    }
    const QString text = value.toString().trimmed().toLower();
    return text == QLatin1String("true") || text == QLatin1String("1");
}

} // namespace Nodes
