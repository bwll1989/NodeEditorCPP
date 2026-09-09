#include "CanBusWorker.h"

#include <QString>
#include <cstring>

extern "C" {
#include "candle_api/candle.h"
}

CanBusWorker::CanBusWorker(QObject *parent)
    : QObject(parent)
{
}

CanBusWorker::~CanBusWorker()
{
    releaseDevice();
}

void CanBusWorker::initialize()
{
    if (!m_pollTimer) {
        m_pollTimer = new QTimer(this);
        m_pollTimer->setInterval(5);
        connect(m_pollTimer, &QTimer::timeout, this, &CanBusWorker::pollFrames);
    }
}

void CanBusWorker::openDevice(int deviceIndex, int channel, int bitrate, int modeFlags)
{
    releaseDevice();

    candle_list_handle list = nullptr;
    if (!candle_list_scan(&list) || list == nullptr) {
        emit errorOccurred(QStringLiteral("Failed to scan candleLight devices"));
        emit isReady(false);
        return;
    }

    uint8_t count = 0;
    candle_list_length(list, &count);
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(count)) {
        candle_list_free(list);
        emit errorOccurred(QStringLiteral("Device index out of range"));
        emit isReady(false);
        return;
    }

    candle_handle hdev = nullptr;
    if (!candle_dev_get(list, static_cast<uint8_t>(deviceIndex), &hdev) || hdev == nullptr) {
        candle_list_free(list);
        emit errorOccurred(QStringLiteral("Failed to get candleLight device"));
        emit isReady(false);
        return;
    }
    candle_list_free(list);

    if (!candle_dev_open(hdev)) {
        const auto err = candle_dev_last_error(hdev);
        candle_dev_free(hdev);
        emit errorOccurred(QStringLiteral("Failed to open device (err=%1)").arg(static_cast<int>(err)));
        emit isReady(false);
        return;
    }

    if (!candle_channel_set_bitrate(hdev, static_cast<uint8_t>(channel), static_cast<uint32_t>(bitrate))) {
        const auto err = candle_dev_last_error(hdev);
        candle_dev_close(hdev);
        candle_dev_free(hdev);
        emit errorOccurred(QStringLiteral("Unsupported bitrate or timing (err=%1)").arg(static_cast<int>(err)));
        emit isReady(false);
        return;
    }

    if (!candle_channel_start(hdev, static_cast<uint8_t>(channel), static_cast<uint32_t>(modeFlags))) {
        const auto err = candle_dev_last_error(hdev);
        candle_dev_close(hdev);
        candle_dev_free(hdev);
        emit errorOccurred(QStringLiteral("Failed to start CAN channel (err=%1)").arg(static_cast<int>(err)));
        emit isReady(false);
        return;
    }

    m_device = hdev;
    m_channel = channel;
    if (m_pollTimer) {
        m_pollTimer->start();
    }
    emit isReady(true);
}

void CanBusWorker::closeDevice()
{
    releaseDevice();
    emit isReady(false);
}

void CanBusWorker::sendFrame(quint32 canId, const QByteArray &data, bool extended, bool rtr)
{
    if (!m_device) {
        return;
    }

    candle_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.can_id = canId & (extended ? 0x1FFFFFFFu : 0x7FFu);
    if (extended) {
        frame.can_id |= CANDLE_ID_EXTENDED;
    }
    if (rtr) {
        frame.can_id |= CANDLE_ID_RTR;
    }

    const int len = qMin(8, data.size());
    frame.can_dlc = static_cast<uint8_t>(len);
    if (len > 0) {
        memcpy(frame.data, data.constData(), static_cast<size_t>(len));
    }

    auto *hdev = static_cast<candle_handle>(m_device);
    if (!candle_frame_send(hdev, static_cast<uint8_t>(m_channel), &frame)) {
        emit errorOccurred(QStringLiteral("Failed to send CAN frame (err=%1)")
                               .arg(static_cast<int>(candle_dev_last_error(hdev))));
    }
}

void CanBusWorker::pollFrames()
{
    if (!m_device) {
        return;
    }

    auto *hdev = static_cast<candle_handle>(m_device);
    candle_frame_t frame;

    // Drain available frames without long blocking.
    while (candle_frame_read(hdev, &frame, 0)) {
        const candle_frametype_t frameType = candle_frame_type(&frame);
        // RECEIVE = 总线收到的帧；ECHO = 本机发送回显（Loopback 自测主要靠它）
        if (frameType != CANDLE_FRAMETYPE_RECEIVE && frameType != CANDLE_FRAMETYPE_ECHO) {
            continue;
        }

        QVariantMap map;
        const quint32 id = candle_frame_id(&frame);
        const bool extended = candle_frame_is_extended_id(&frame);
        const bool rtr = candle_frame_is_rtr(&frame);
        const uint8_t dlc = candle_frame_dlc(&frame);
        const uint8_t *bytes = candle_frame_data(&frame);
        const QByteArray payload(reinterpret_cast<const char *>(bytes), dlc);

        map.insert(QStringLiteral("type"),
                   frameType == CANDLE_FRAMETYPE_ECHO ? QStringLiteral("echo")
                                                     : QStringLiteral("rx"));
        map.insert(QStringLiteral("id"), id);
        map.insert(QStringLiteral("id_hex"),
                   QStringLiteral("0x%1").arg(id, extended ? 8 : 3, 16, QLatin1Char('0')).toUpper());
        map.insert(QStringLiteral("extended"), extended);
        map.insert(QStringLiteral("rtr"), rtr);
        map.insert(QStringLiteral("dlc"), dlc);
        map.insert(QStringLiteral("data"), QString(payload.toHex(' ').toUpper()));
        map.insert(QStringLiteral("data_raw"), payload);
        map.insert(QStringLiteral("timestamp_us"), candle_frame_timestamp_us(&frame));
        map.insert(QStringLiteral("channel"), m_channel);
        map.insert(QStringLiteral("default"), map.value(QStringLiteral("data")));

        emit recMsg(map);
    }
}

void CanBusWorker::releaseDevice()
{
    if (m_pollTimer) {
        m_pollTimer->stop();
    }

    if (!m_device) {
        return;
    }

    auto *hdev = static_cast<candle_handle>(m_device);
    candle_channel_stop(hdev, static_cast<uint8_t>(m_channel));
    candle_dev_close(hdev);
    candle_dev_free(hdev);
    m_device = nullptr;
}
