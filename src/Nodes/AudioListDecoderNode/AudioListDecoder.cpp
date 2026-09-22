/**
 * @file AudioListDecoder.cpp
 * @brief 播放列表引擎实现
 */
#include "AudioListDecoder.hpp"

#include "Common/AppConfig/ConstantDefines.h"

#include <QFileInfo>
#include <QtGlobal>

AudioListDecoder::AudioListDecoder(QObject *parent)
    : QObject(parent)
    , m_decoder(std::make_shared<AudioDecoder>())
{
    connect(m_decoder.get(), &AudioDecoder::playbackFinished,
            this, &AudioListDecoder::onTrackFinished, Qt::QueuedConnection);
    connect(m_decoder.get(), &AudioDecoder::playbackProgress,
            this, &AudioListDecoder::playbackProgress, Qt::QueuedConnection);
}

AudioListDecoder::~AudioListDecoder()
{
    m_suppressFinished = true;
    if (m_decoder && m_decoder->getPlaying()) {
        m_decoder->stopPlay(true);
    }
}

void AudioListDecoder::setPlaylist(const QVector<AudioListItem> &items)
{
    const bool wasPlaying = isPlaying();
    if (wasPlaying) {
        stop(true);
    }
    m_items = items;
    if (m_currentIndex >= m_items.size()) {
        m_currentIndex = m_items.isEmpty() ? 0 : m_items.size() - 1;
    }
    m_ready = false;
    emit playlistChanged();
    emit currentIndexChanged(m_currentIndex);
}

AudioListItem AudioListDecoder::itemAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return {};
    }
    return m_items.at(index);
}

bool AudioListDecoder::setItemGain(int index, double gainDb)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }
    if (qFuzzyCompare(m_items[index].gainDb + 1.0, gainDb + 1.0)) {
        return true;
    }
    m_items[index].gainDb = gainDb;
    if (index == m_currentIndex) {
        applyGainToDecoder();
    }
    // 不 emit playlistChanged，避免拖动增益时整表重建
    return true;
}

bool AudioListDecoder::setItemPath(int index, const QString &relativePath)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }
    const QString trimmed = relativePath.trimmed();
    if (m_items[index].relativePath == trimmed) {
        return true;
    }
    const bool needReload = (index == m_currentIndex);
    const bool wasPlaying = needReload && isPlaying();
    if (needReload && wasPlaying) {
        stop(true);
    }
    m_items[index].relativePath = trimmed;
    // 不 emit playlistChanged，避免选文件时整表重建导致控件失焦
    if (needReload) {
        m_ready = false;
        if (wasPlaying) {
            playIndex(index);
        } else {
            prepareIndex(index);
        }
    }
    return true;
}

void AudioListDecoder::appendItem(const AudioListItem &item)
{
    m_items.append(item);
    emit playlistChanged();
}

bool AudioListDecoder::removeItem(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }
    const bool removingCurrent = (index == m_currentIndex);
    const bool wasPlaying = removingCurrent && isPlaying();
    if (wasPlaying) {
        stop(true);
    }
    m_items.removeAt(index);
    if (m_items.isEmpty()) {
        m_currentIndex = 0;
        m_ready = false;
    } else if (m_currentIndex >= m_items.size()) {
        m_currentIndex = m_items.size() - 1;
        m_ready = false;
    } else if (index < m_currentIndex) {
        --m_currentIndex;
    }
    emit playlistChanged();
    emit currentIndexChanged(m_currentIndex);
    return true;
}

bool AudioListDecoder::moveItem(int from, int to)
{
    if (from < 0 || from >= m_items.size() || to < 0 || to >= m_items.size() || from == to) {
        return false;
    }
    m_items.move(from, to);
    if (m_currentIndex == from) {
        m_currentIndex = to;
    } else if (from < m_currentIndex && to >= m_currentIndex) {
        --m_currentIndex;
    } else if (from > m_currentIndex && to <= m_currentIndex) {
        ++m_currentIndex;
    }
    emit playlistChanged();
    emit currentIndexChanged(m_currentIndex);
    return true;
}

void AudioListDecoder::setLoopMode(AudioListLoopMode mode)
{
    if (m_loopMode == mode) {
        return;
    }
    m_loopMode = mode;
    if (m_decoder) {
        m_decoder->setLooping(mode == AudioListLoopMode::Single);
    }
    emit loopModeChanged(mode);
}

bool AudioListDecoder::isPlaying() const
{
    return m_decoder && m_decoder->getPlaying();
}

bool AudioListDecoder::prepareIndex(int index)
{
    return loadTrack(index, false);
}

bool AudioListDecoder::playIndex(int index)
{
    if (index < 0) {
        index = m_currentIndex;
    }
    return loadTrack(index, true);
}

void AudioListDecoder::stop(bool resetPosition)
{
    m_suppressFinished = true;
    if (m_decoder) {
        m_decoder->stopPlay(resetPosition);
    }
    m_suppressFinished = false;
    emit playingChanged(false);
}

void AudioListDecoder::applyGainToDecoder()
{
    if (!m_decoder || m_currentIndex < 0 || m_currentIndex >= m_items.size()) {
        return;
    }
    m_decoder->setVolume(m_items[m_currentIndex].gainDb);
}

std::shared_ptr<AudioTimestampRingQueue> AudioListDecoder::getAudioBuffer(int channel)
{
    return m_decoder->getAudioBuffer(channel);
}

QString AudioListDecoder::absolutePathFor(const AudioListItem &item) const
{
    if (item.relativePath.isEmpty()) {
        return {};
    }
    if (QFileInfo::exists(item.relativePath)) {
        return item.relativePath;
    }
    return AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QStringLiteral("/") + item.relativePath;
}

bool AudioListDecoder::loadTrack(int index, bool startPlayback)
{
    if (index < 0 || index >= m_items.size()) {
        emit statusMessage(QStringLiteral("无效的播放序号"));
        return false;
    }
    const AudioListItem item = m_items.at(index);
    if (item.relativePath.trimmed().isEmpty()) {
        emit statusMessage(QStringLiteral("曲目路径为空"));
        return false;
    }

    const QString absPath = absolutePathFor(item);
    if (!QFileInfo::exists(absPath)) {
        m_ready = false;
        emit statusMessage(QStringLiteral("文件不存在: %1").arg(item.relativePath));
        return false;
    }

    const bool switching = (index != m_currentIndex) || !m_ready || startPlayback;
    if (m_decoder->getPlaying()) {
        m_suppressFinished = true;
        m_decoder->stopPlay(true);
        m_suppressFinished = false;
    }

    if (switching || !m_ready) {
        auto *res = m_decoder->initializeFFmpeg(absPath);
        if (!res) {
            m_ready = false;
            emit statusMessage(QStringLiteral("初始化失败: %1").arg(item.relativePath));
            return false;
        }
        delete res;
        m_ready = true;
    }

    if (m_currentIndex != index) {
        m_currentIndex = index;
        emit currentIndexChanged(m_currentIndex);
    }

    m_decoder->setVolume(item.gainDb);
    m_decoder->setLooping(m_loopMode == AudioListLoopMode::Single);

    if (startPlayback) {
        m_decoder->startPlay();
        emit playingChanged(true);
        emit statusMessage(QStringLiteral("播放 [%1] %2").arg(index).arg(item.relativePath));
    } else {
        emit statusMessage(QStringLiteral("就绪 [%1] %2").arg(index).arg(item.relativePath));
    }
    return true;
}

void AudioListDecoder::onTrackFinished()
{
    if (m_suppressFinished) {
        return;
    }
    if (m_loopMode == AudioListLoopMode::Single) {
        playIndex(m_currentIndex);
        return;
    }
    advanceAfterTrackEnd();
}

void AudioListDecoder::advanceAfterTrackEnd()
{
    if (m_items.isEmpty()) {
        emit playingChanged(false);
        emit playlistFinished();
        return;
    }

    const int next = m_currentIndex + 1;
    if (next < m_items.size()) {
        playIndex(next);
        return;
    }

    if (m_loopMode == AudioListLoopMode::List) {
        playIndex(0);
        return;
    }

    emit playingChanged(false);
    emit statusMessage(QStringLiteral("播放列表结束"));
    emit playlistFinished();
}
