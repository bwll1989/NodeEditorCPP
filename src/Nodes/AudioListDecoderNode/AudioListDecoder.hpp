/**
 * @file AudioListDecoder.hpp
 * @brief 基于 AudioDecoder 的播放列表引擎
 */
#pragma once

#include "AudioDecoder.hpp"

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

struct AudioListItem
{
    QString relativePath;
    double gainDb = 0.0;
};

enum class AudioListLoopMode
{
    Off = 0,
    Single = 1,
    List = 2
};

class AudioListDecoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioListDecoder(QObject *parent = nullptr);
    ~AudioListDecoder() override;

    QVector<AudioListItem> playlist() const { return m_items; }
    void setPlaylist(const QVector<AudioListItem> &items);

    int itemCount() const { return m_items.size(); }
    AudioListItem itemAt(int index) const;
    bool setItemGain(int index, double gainDb);
    bool setItemPath(int index, const QString &relativePath);
    void appendItem(const AudioListItem &item);
    bool removeItem(int index);
    bool moveItem(int from, int to);

    int currentIndex() const { return m_currentIndex; }
    AudioListLoopMode loopMode() const { return m_loopMode; }
    void setLoopMode(AudioListLoopMode mode);

    bool isPlaying() const;
    bool isReady() const { return m_ready; }

    bool prepareIndex(int index);
    bool playIndex(int index = -1);
    void stop(bool resetPosition = true);
    void applyGainToDecoder();

    AudioDecoder *decoder() const { return m_decoder.get(); }
    std::shared_ptr<AudioTimestampRingQueue> getAudioBuffer(int channel);

signals:
    void playlistChanged();
    void currentIndexChanged(int index);
    void loopModeChanged(AudioListLoopMode mode);
    void playingChanged(bool playing);
    void statusMessage(const QString &message);
    void playbackProgress(double currentSec, double totalSec);
    void playlistFinished();

private slots:
    void onTrackFinished();

private:
    QString absolutePathFor(const AudioListItem &item) const;
    bool loadTrack(int index, bool startPlayback);
    void advanceAfterTrackEnd();

    std::shared_ptr<AudioDecoder> m_decoder;
    QVector<AudioListItem> m_items;
    int m_currentIndex = 0;
    AudioListLoopMode m_loopMode = AudioListLoopMode::Off;
    bool m_ready = false;
    bool m_suppressFinished = false;
};
