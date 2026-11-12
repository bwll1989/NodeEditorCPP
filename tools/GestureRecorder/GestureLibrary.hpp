#pragma once

/**
 * @file GestureLibrary.hpp
 * @brief .gesture 动作库文件读写（GestureRecorder.exe 与 GestureNode 各持一份副本）
 *
 * ⚠️ 文件格式 v1 —— 两份副本必须严格一致：
 * {
 *   "format": "flow-gesture",
 *   "version": 1,
 *   "type": "hand",
 *   "featureDim": 64,
 *   "gestures": [
 *     { "label": "fist", "threshold": 0.9, "samples": [[64 个浮点数], ...] }
 *   ]
 * }
 * samples 内每个样本为 GestureFeature::kDim 维浮点数组；threshold 缺省时
 * 消费端回退到 GestureFeature::kDefaultThreshold。
 */

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <array>
#include "GestureFeature.hpp"

namespace GestureLibrary {

struct Gesture
{
    QString label;
    double threshold = GestureFeature::kDefaultThreshold;
    QVector<std::array<float, GestureFeature::kDim>> samples;
};

struct Library
{
    QString type = QStringLiteral("hand");
    int featureVersion = 1; ///< 旧文件无此字段时视为 v1（特征不可信，建议重录）
    QVector<Gesture> gestures;

    int indexOf(const QString& label) const
    {
        for (int i = 0; i < gestures.size(); ++i) {
            if (gestures[i].label == label) {
                return i;
            }
        }
        return -1;
    }

    int sampleCount() const
    {
        int n = 0;
        for (const auto& g : gestures) {
            n += g.samples.size();
        }
        return n;
    }

    Gesture* find(const QString& label)
    {
        const int idx = indexOf(label);
        return idx >= 0 ? &gestures[idx] : nullptr;
    }
};

inline bool load(const QString& path, Library& out, QString* error = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("无法打开文件: %1").arg(path);
        }
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        if (error) {
            *error = QStringLiteral("不是有效的 JSON 对象: %1").arg(path);
        }
        return false;
    }
    const QJsonObject root = doc.object();
    if (root.value("format").toString() != QStringLiteral("flow-gesture")) {
        if (error) {
            *error = QStringLiteral("format 不是 flow-gesture: %1").arg(path);
        }
        return false;
    }

    Library lib;
    lib.type = root.value("type").toString(QStringLiteral("hand"));
    lib.featureVersion = root.value("featureVersion").toInt(1);

    const QJsonArray arr = root.value("gestures").toArray();
    for (const auto& gv : arr) {
        const QJsonObject go = gv.toObject();
        Gesture g;
        g.label = go.value("label").toString();
        if (g.label.isEmpty()) {
            continue;
        }
        g.threshold = go.value("threshold").toDouble(GestureFeature::kDefaultThreshold);
        const QJsonArray samples = go.value("samples").toArray();
        for (const auto& sv : samples) {
            const QJsonArray dims = sv.toArray();
            if (dims.size() < GestureFeature::kDim) {
                continue;
            }
            std::array<float, GestureFeature::kDim> f{};
            for (int d = 0; d < GestureFeature::kDim; ++d) {
                f[d] = static_cast<float>(dims.at(d).toDouble());
            }
            g.samples.push_back(f);
        }
        if (!g.samples.isEmpty()) {
            lib.gestures.push_back(std::move(g));
        }
    }

    out = std::move(lib);
    return true;
}

inline bool save(const QString& path, const Library& lib, QString* error = nullptr)
{
    QJsonObject root;
    root.insert("format", QStringLiteral("flow-gesture"));
    root.insert("version", 1);
    root.insert("featureVersion", GestureFeature::kFeatureVersion);
    root.insert("type", lib.type);
    root.insert("featureDim", GestureFeature::kDim);

    QJsonArray arr;
    for (const auto& g : lib.gestures) {
        QJsonObject go;
        go.insert("label", g.label);
        go.insert("threshold", g.threshold);
        QJsonArray samples;
        for (const auto& f : g.samples) {
            QJsonArray dims;
            for (int d = 0; d < GestureFeature::kDim; ++d) {
                dims.append(static_cast<double>(f[d]));
            }
            samples.append(dims);
        }
        go.insert("samples", samples);
        arr.append(go);
    }
    root.insert("gestures", arr);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("无法写入文件: %1").arg(path);
        }
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

} // namespace GestureLibrary
