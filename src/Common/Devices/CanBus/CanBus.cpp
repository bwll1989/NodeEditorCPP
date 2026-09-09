#include "CanBus.h"

#include <QRegularExpression>
#include <string>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <setupapi.h>
#endif

extern "C" {
#include "candle_api/candle.h"
}

namespace {

QString knownProductName(const QString &vid, const QString &pid)
{
    const QString key = (vid + QLatin1Char(':') + pid).toLower();
    if (key == QLatin1String("1d50:606f")) {
        return QStringLiteral("candleLight USB-CAN");
    }
    if (key == QLatin1String("1209:2323")) {
        return QStringLiteral("CANable");
    }
    if (key == QLatin1String("1d50:60a7")) {
        return QStringLiteral("candleLight FD");
    }
    return QStringLiteral("gs_usb CAN");
}

QString parseVidPidLabel(const QString &path)
{
    static const QRegularExpression re(
        QStringLiteral("vid_([0-9a-fA-F]{4}).*pid_([0-9a-fA-F]{4})"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(path);
    if (!match.hasMatch()) {
        return {};
    }
    const QString vid = match.captured(1).toUpper();
    const QString pid = match.captured(2).toUpper();
    return QStringLiteral("%1 (%2:%3)").arg(knownProductName(vid, pid), vid, pid);
}

#ifdef Q_OS_WIN
QString setupApiFriendlyName(const QString &devicePath)
{
    HDEVINFO hdi = SetupDiCreateDeviceInfoList(nullptr, nullptr);
    if (hdi == INVALID_HANDLE_VALUE) {
        return {};
    }

    SP_DEVICE_INTERFACE_DATA ifData;
    ZeroMemory(&ifData, sizeof(ifData));
    ifData.cbSize = sizeof(ifData);

    const std::wstring wpath = devicePath.toStdWString();
    if (!SetupDiOpenDeviceInterfaceW(hdi, wpath.c_str(), 0, &ifData)) {
        SetupDiDestroyDeviceInfoList(hdi);
        return {};
    }

    SP_DEVINFO_DATA devData;
    ZeroMemory(&devData, sizeof(devData));
    devData.cbSize = sizeof(devData);

    DWORD required = 0;
    SetupDiGetDeviceInterfaceDetailW(hdi, &ifData, nullptr, 0, &required, &devData);

    wchar_t buffer[256] = {};
    if (SetupDiGetDeviceRegistryPropertyW(
            hdi, &devData, SPDRP_FRIENDLYNAME, nullptr,
            reinterpret_cast<PBYTE>(buffer), sizeof(buffer), nullptr)
        || SetupDiGetDeviceRegistryPropertyW(
               hdi, &devData, SPDRP_DEVICEDESC, nullptr,
               reinterpret_cast<PBYTE>(buffer), sizeof(buffer), nullptr)) {
        SetupDiDestroyDeviceInfoList(hdi);
        return QString::fromWCharArray(buffer).trimmed();
    }

    SetupDiDestroyDeviceInfoList(hdi);
    return {};
}
#endif

QString makeDisplayName(int index, const QString &path)
{
#ifdef Q_OS_WIN
    const QString friendly = setupApiFriendlyName(path);
    if (!friendly.isEmpty()) {
        return QStringLiteral("#%1 %2").arg(index).arg(friendly);
    }
#endif
    const QString fromVidPid = parseVidPidLabel(path);
    if (!fromVidPid.isEmpty()) {
        return QStringLiteral("#%1 %2").arg(index).arg(fromVidPid);
    }
    return QStringLiteral("#%1 CAN adapter").arg(index);
}

} // namespace

CanBus::CanBus(QObject *parent)
    : QObject(parent)
{
    workerThread = new QThread();
    worker = new CanBusWorker();
    worker->moveToThread(workerThread);

    connect(this, &CanBus::openDeviceRequest, worker, &CanBusWorker::openDevice, Qt::QueuedConnection);
    connect(this, &CanBus::closeDeviceRequest, worker, &CanBusWorker::closeDevice, Qt::QueuedConnection);
    connect(this, &CanBus::sendFrameRequest, worker, &CanBusWorker::sendFrame, Qt::QueuedConnection);

    connect(worker, &CanBusWorker::isReady, this, &CanBus::isReady, Qt::QueuedConnection);
    connect(worker, &CanBusWorker::recMsg, this, &CanBus::recMsg, Qt::QueuedConnection);
    connect(worker, &CanBusWorker::errorOccurred, this, &CanBus::errorOccurred, Qt::QueuedConnection);

    workerThread->start();
    QMetaObject::invokeMethod(worker, "initialize", Qt::QueuedConnection);
}

CanBus::~CanBus()
{
    emit closeDeviceRequest();
    worker->deleteLater();

    if (workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }

    delete workerThread;
}

QList<CanBusDeviceInfo> CanBus::availableDevices()
{
    QList<CanBusDeviceInfo> result;

    candle_list_handle list = nullptr;
    if (!candle_list_scan(&list) || list == nullptr) {
        return result;
    }

    uint8_t count = 0;
    candle_list_length(list, &count);
    for (uint8_t i = 0; i < count; ++i) {
        candle_handle hdev = nullptr;
        if (!candle_dev_get(list, i, &hdev) || hdev == nullptr) {
            continue;
        }
        const wchar_t *path = candle_dev_get_path(hdev);
        CanBusDeviceInfo info;
        info.index = i;
        info.path = path ? QString::fromWCharArray(path) : QStringLiteral("unknown");
        info.displayName = makeDisplayName(i, info.path);
        result.append(info);
        candle_dev_free(hdev);
    }

    candle_list_free(list);
    return result;
}

void CanBus::openDevice(int deviceIndex, int channel, int bitrate, int modeFlags)
{
    emit openDeviceRequest(deviceIndex, channel, bitrate, modeFlags);
}

void CanBus::closeDevice()
{
    emit closeDeviceRequest();
}

void CanBus::sendFrame(quint32 canId, const QByteArray &data, bool extended, bool rtr)
{
    emit sendFrameRequest(canId, data, extended, rtr);
}
