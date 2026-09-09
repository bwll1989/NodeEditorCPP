#include "LogRingBuffer.hpp"

#include "../../Common/AppConfig/ConstantDefines.h"

LogRingBuffer& LogRingBuffer::instance()
{
    static LogRingBuffer buffer;
    return buffer;
}

LogRingBuffer::LogRingBuffer()
    : _capacity(qMax(200, AppConfigs::MAX_LOG_ENTRIES * 5))
    , _nextSeq(1)
{
}

qint64 LogRingBuffer::append(const QString& timestamp, const QString& level, const QString& message)
{
    Entry entry;
    {
        QMutexLocker locker(&_mutex);
        entry.seq = _nextSeq++;
        entry.timestamp = timestamp;
        entry.level = level;
        entry.message = message;

        _entries.push_back(entry);
        while (_entries.size() > _capacity) {
            _entries.removeFirst();
        }
    }

    invokeBroadcast(entry);
    return entry.seq;
}

QJsonObject LogRingBuffer::entryToJson(const Entry& entry)
{
    QJsonObject obj;
    obj[QStringLiteral("seq")] = static_cast<double>(entry.seq);
    obj[QStringLiteral("timestamp")] = entry.timestamp;
    obj[QStringLiteral("level")] = entry.level;
    obj[QStringLiteral("message")] = entry.message;
    return obj;
}

void LogRingBuffer::invokeBroadcast(const Entry& entry)
{
    std::function<void(const QJsonObject&)> callback;
    {
        QMutexLocker locker(&_mutex);
        callback = _broadcast;
    }
    if (!callback) {
        return;
    }

    QJsonObject payload = entryToJson(entry);
    payload[QStringLiteral("event")] = QStringLiteral("log");
    callback(payload);
}

QJsonArray LogRingBuffer::tail(int limit, const QString& levelFilter, qint64 sinceSeq) const
{
    const int safeLimit = qBound(1, limit, _capacity);
    QMutexLocker locker(&_mutex);

    QJsonArray items;

    for (const Entry& entry : _entries) {
        if (entry.seq <= sinceSeq) {
            continue;
        }
        if (levelFilter != QStringLiteral("All") && entry.level != levelFilter) {
            continue;
        }
        items.append(entryToJson(entry));
    }

    if (items.size() <= safeLimit) {
        return items;
    }

    QJsonArray trimmed;
    const int start = items.size() - safeLimit;
    for (int i = start; i < items.size(); ++i) {
        trimmed.append(items.at(i));
    }
    return trimmed;
}

LogRingBuffer::Entry LogRingBuffer::entryBySeq(qint64 seq) const
{
    QMutexLocker locker(&_mutex);
    for (const Entry& entry : _entries) {
        if (entry.seq == seq) {
            return entry;
        }
    }
    return {};
}

void LogRingBuffer::setBroadcastCallback(std::function<void(const QJsonObject&)> callback)
{
    QMutexLocker locker(&_mutex);
    _broadcast = std::move(callback);
}
