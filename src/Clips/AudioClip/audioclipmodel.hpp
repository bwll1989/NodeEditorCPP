#ifndef AUDIOCLIPMODEL_H
#define AUDIOCLIPMODEL_H


#include "TimeLineWidget/abstractclipmodel.hpp"

class AudioClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    explicit AudioClipModel(int start, int end, const QString& filePath = QString(), QObject* parent = nullptr)
        : AbstractClipModel(start, end, "Audio", parent), m_filePath(filePath), m_sampleRate(44100), m_channels(2) {
           EMBEDWIDGET = false;
        }
    ~AudioClipModel() override = default;
    // Getters
    QString filePath() const { return m_filePath; }
    int sampleRate() const { return m_sampleRate; }
    int channels() const { return m_channels; }
    QString format() const { return m_format; }

    // Setters
    void setFilePath(const QString& path) { m_filePath = path; }
    void setSampleRate(int rate) { m_sampleRate = rate; }
    void setChannels(int count) { m_channels = count; }
    void setFormat(const QString& format) { m_format = format; }

    // Save/Load
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();
        json["filePath"] = m_filePath;
        json["sampleRate"] = m_sampleRate;
        json["channels"] = m_channels;
        json["format"] = m_format;
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_filePath = json["filePath"].toString();
        m_sampleRate = json["sampleRate"].toInt();
        m_channels = json["channels"].toInt();
        m_format = json["format"].toString();
    }

    QVariant data(int role) const override {
        if (role == Qt::DisplayRole) {
            return m_filePath;
        }
        return QVariant();
    }

    QVariant currentData(int currentFrame) const override {
        return QVariant();
    }

private:
    QString m_filePath;
    int m_sampleRate;
    int m_channels;
    QString m_format;
};

#endif // AUDIOCLIPMODEL_H 