#pragma once

/**
 * @file GestureFeature.hpp
 * @brief 手势特征提取与相似度（GestureRecorder.exe 与 GestureNode 各持一份副本）
 *
 * ⚠️ 特征约定 v2 —— 两份副本 + .gesture 文件三处必须严格一致，任一端改动必须同步：
 *   dims 0..62 : worldLandmarks 21 个手部关键点按索引顺序展平 [x0,y0,z0,...]
 *                （米制坐标，原点≈手部几何中心，天然平移/尺度不变）
 *   dim 63     : 恒为 0（手性已在 v2 中镜像归一，见下）
 *
 * v2 相比 v1 的两个修正（v1 识别成功率差的根因）：
 *   1. **左右手镜像归一**：handedness 为 Left 时把所有点的 x 取反（水平镜像），
 *      等效变换成右手特征——库只需录一只手，左右手都能匹配。
 *   2. **手性维度归零**：v1 中 handedness 维为 ±1，而米制坐标仅 ~0.05，
 *      余弦相似度被手性维主导（约 93% 权重），手型差异被淹没导致分不开手势。
 *      v2 归零后相似度纯比手型。
 *
 * 旧 v1 库文件仍可加载，但精度不可信，建议用录制工具重新导出。
 *
 * 相似度 = 余弦（dot/(|a||b|)），对整体缩放不敏感。
 */

#include <QVector>
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <opencv2/core.hpp>

namespace GestureFeature {

inline constexpr int kDim = 64;
inline constexpr int kFeatureVersion = 2;
inline constexpr float kDefaultThreshold = 0.9f;

/** 从 HandResult 的 worldLandmarks + handedness 提取 64 维特征（左手自动镜像成右手等效） */
inline bool extract(const cv::Mat& worldLandmarks, float handedness, std::array<float, kDim>& out)
{
    if (worldLandmarks.empty()) {
        return false;
    }
    cv::Mat f32;
    if (worldLandmarks.type() != CV_32F) {
        worldLandmarks.convertTo(f32, CV_32F);
    } else {
        f32 = worldLandmarks;
    }
    if (f32.total() < 63) {
        return false;
    }
    const bool mirror = handedness < 0.5f; // Left
    const float* p = f32.isContinuous() ? f32.ptr<float>() : f32.clone().ptr<float>();
    for (int k = 0; k < 21; ++k) {
        const float x = p[k * 3 + 0];
        const float y = p[k * 3 + 1];
        const float z = p[k * 3 + 2];
        out[k * 3 + 0] = mirror ? -x : x;
        out[k * 3 + 1] = y;
        out[k * 3 + 2] = z;
    }
    out[63] = 0.f;
    return true;
}

/** 余弦相似度（纯手型比对，整体尺度无关） */
inline float cosine(const std::array<float, kDim>& a, const std::array<float, kDim>& b)
{
    float dot = 0.f;
    float na = 0.f;
    float nb = 0.f;
    for (int i = 0; i < kDim; ++i) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    if (na <= 0.f || nb <= 0.f) {
        return 0.f;
    }
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

/**
 * 水平镜像副本（x 分量取反 = 手性等效变换）。
 * 匹配端对特征同时测试原样与镜像两个朝向、取较大相似度，
 * 保证库无论以哪只手录制（含旧版 v1 未镜像库）、识别用哪只手，成功率一致。
 */
inline std::array<float, kDim> mirrored(const std::array<float, kDim>& f)
{
    std::array<float, kDim> m = f;
    for (int k = 0; k < 21; ++k) {
        m[k * 3 + 0] = -f[k * 3 + 0];
    }
    return m;
}

/**
 * 采样增广：一次采集入库 5 个模板（原样 / 平面旋转 ±12° / 缩放 0.9 / 1.1，各加微噪声），
 * 等效于多角度多距离采样，提升识别鲁棒性。只变换前 63 维的手部点。
 */
inline void augmentVariants(const std::array<float, kDim>& base, QVector<std::array<float, kDim>>& out)
{
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<float> noise(0.f, 0.002f);

    const float angles[5] = {0.f, 12.f, -12.f, 0.f, 0.f};
    const float scales[5] = {1.f, 1.f, 1.f, 0.9f, 1.1f};

    for (int v = 0; v < 5; ++v) {
        std::array<float, kDim> f = base;
        const float rad = angles[v] * 3.14159265f / 180.f;
        const float c = std::cos(rad) * scales[v];
        const float s = std::sin(rad) * scales[v];
        for (int k = 0; k < 21; ++k) {
            const float x = base[k * 3 + 0];
            const float y = base[k * 3 + 1];
            const float z = base[k * 3 + 2];
            f[k * 3 + 0] = x * c - y * s + noise(rng);
            f[k * 3 + 1] = x * s + y * c + noise(rng);
            f[k * 3 + 2] = z + noise(rng);
        }
        f[63] = 0.f;
        out.push_back(f);
    }
}

} // namespace GestureFeature
