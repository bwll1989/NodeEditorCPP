#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QString>
#include <QVector>
#include <functional>

/** @brief 线程安全环形缓冲，供桌面日志与网页 tail / WebSocket 共用 */
class LogRingBuffer {
public:
    struct Entry {
        qint64 seq = 0;
        QString timestamp;
        QString level;
        QString message;
    };

    static LogRingBuffer& instance();

    /** @return 新条目的 seq */
    qint64 append(const QString& timestamp, const QString& level, const QString& message);

    QJsonArray tail(int limit, const QString& levelFilter = QStringLiteral("All"), qint64 sinceSeq = 0) const;

    Entry entryBySeq(qint64 seq) const;

    void setBroadcastCallback(std::function<void(const QJsonObject&)> callback);

    int capacity() const { return _capacity; }

private:
    LogRingBuffer();

    static QJsonObject entryToJson(const Entry& entry);
    void invokeBroadcast(const Entry& entry);

    mutable QMutex _mutex;
    QVector<Entry> _entries;
    int _capacity;
    qint64 _nextSeq;
    std::function<void(const QJsonObject&)> _broadcast;
};
