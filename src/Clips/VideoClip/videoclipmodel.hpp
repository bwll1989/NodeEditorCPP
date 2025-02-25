#ifndef VIDEOCLIPMODEL_H
#define VIDEOCLIPMODEL_H

#include "TimeLineWidget/AbstractClipModel.hpp"

class VideoClipModel : public AbstractClipModel {
    Q_OBJECT
public:
    explicit VideoClipModel(int start, int end, const QString& filePath = QString(), QObject* parent = nullptr)
        : AbstractClipModel(start, end, "Video", parent), m_filePath(filePath), m_frameRate(25.0), m_width(1920), m_height(1080) {
            EMBEDWIDGET = false;
        }
    ~VideoClipModel() override = default;
    // Getters
    QString filePath() const { return m_filePath; }
    double frameRate() const { return m_frameRate; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    // Setters
    void setFilePath(const QString& path) { m_filePath = path; }
    void setFrameRate(double rate) { m_frameRate = rate; }
    void setWidth(int width) { m_width = width; }
    void setHeight(int height) { m_height = height; }

    // 重写保存和加载函数
    QJsonObject save() const override {
        QJsonObject json = AbstractClipModel::save();
        json["filePath"] = m_filePath;
        json["frameRate"] = m_frameRate;
        json["width"] = m_width;
        json["height"] = m_height;
        return json;
    }

    void load(const QJsonObject& json) override {
        AbstractClipModel::load(json);
        m_filePath = json["filePath"].toString();
        m_frameRate = json["frameRate"].toDouble();
        m_width = json["width"].toInt();
        m_height = json["height"].toInt();
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
    double m_frameRate;
    int m_width;
    int m_height;
};

#endif // VIDEOCLIPMODEL_H 