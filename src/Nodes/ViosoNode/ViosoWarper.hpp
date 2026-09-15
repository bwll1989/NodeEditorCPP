#pragma once

/**
 * @file ViosoWarper.hpp
 * @brief VIOSO WarpBlend 低层封装
 *
 * 职责：
 * - 加载 / 切换 `.vwf` 与投影机索引（calibIndex）
 * - 在当前 OpenGL 上下文中把源纹理 Warp+Blend 到离屏 FBO 纹理
 *
 * 线程 / 上下文约束（必须遵守）：
 * - 所有方法均须在 `ImageGpuUpload::runGl()` 回调内调用（共享 GL 上下文已 makeCurrent）
 * - 禁止在 runGl 回调内创建再销毁「多余的」临时 `GpuTextureHandle`
 *   （其 RAII deleter 会再进 `destroyTexture` 抢同一把非递归锁 → 死锁）
 * - V 轴与显示管线的对齐（输入翻 / 输出翻）在节点层 `ViosoDataModel` 完成，本类只产出 FBO 原始结果
 *
 * @see https://github.com/vioso/VIOSO_API
 * @see ViosoDataModel.hpp
 */

#include "GpuTextureHandle.h"

#include <QByteArray>
#include <QString>

class QOpenGLFunctions;

namespace Nodes
{

/**
 * @brief 单个 `VWB_Warper` 的生命周期与离屏渲染
 *
 * 典型用法（均在 runGl 内）：
 * @code
 *   warper.ensureReady(f, absVwfPath, projectorIndex, cryptoKey16);
 *   auto tex = warper.render(f, srcTexture);
 * @endcode
 */
class ViosoWarper
{
public:
    ViosoWarper() = default;
    ~ViosoWarper();

    ViosoWarper(const ViosoWarper&) = delete;
    ViosoWarper& operator=(const ViosoWarper&) = delete;

    /**
     * @brief 销毁底层 warper 并清空状态
     * @param f 当前 GL 函数集（OpenGL 路径下 Init/Destroy 要求上下文 current；参数目前未直接使用）
     */
    void destroy(QOpenGLFunctions* f);

    /**
     * @brief 解析 UI 输入的 Passkey 为 16 字节 AES key
     * @param text     空=关闭加密；32 位十六进制；或任意文本（UTF-8 截断/补零到 16 字节）
     * @param outKey16 成功时：空=关闭；否则恰好 16 字节
     * @param error    失败原因（可选）
     * @return false 表示输入无效（例如奇数位十六进制）
     */
    static bool parseCryptoKey(const QString& text, QByteArray& outKey16, QString* error = nullptr);

    /**
     * @brief 确保已按给定 `.vwf` + 投影机索引 + Passkey 初始化；任一变化时会重建
     * @param f              当前 GL 函数集
     * @param vwfPath        `.vwf` 绝对路径
     * @param projectorIndex 文件内通道索引（从 0 起，对应 ini 中的 calibIndex）
     * @param cryptoKey16    空=关闭加密；否则须为 16 字节（见 parseCryptoKey）
     * @return 成功可调用 render；失败时见 lastError()
     */
    bool ensureReady(QOpenGLFunctions* f,
                     const QString& vwfPath,
                     int projectorIndex,
                     const QByteArray& cryptoKey16 = {});

    /**
     * @brief 将源纹理 Warp+Blend 到新建 RGBA8 输出纹理
     * @param f   当前 GL 函数集
     * @param src 与共享上下文兼容的输入纹理（调用方负责 V 约定）
     * @return 有效 GpuTextureHandle；失败返回空句柄
     *
     * 实现要点：
     * - 输出尺寸来自 `.vwf` 头中该通道的 width/height
     * - 禁止传入 VWB_STATEMASK_VIEWPORT（库会把 viewport 改成输入尺寸，结果挤在角落）
     * - 返回的是 FBO 原始结果，尚未做显示管线 V 翻转
     * - 失败路径不得让带 makeTextureLifetime 的句柄在 runGl 内析构
     */
    NodeDataTypes::GpuTextureHandle render(QOpenGLFunctions* f,
                                           const NodeDataTypes::GpuTextureHandle& src);

    bool isReady() const { return m_ready; }
    int outputWidth() const { return m_outWidth; }
    int outputHeight() const { return m_outHeight; }
    QString lastError() const { return m_lastError; }

    /**
     * @brief 读取 `.vwf` 内投影机通道数（纯 CPU，不需 GL 上下文）
     * @return 成功时 channelCount >= 1
     */
    static bool queryChannelCount(const QString& vwfPath, int& channelCount, QString* error = nullptr);

    /**
     * @brief 读取指定通道的 warp 图宽高（纯 CPU）
     */
    static bool queryChannelSize(const QString& vwfPath,
                                 int projectorIndex,
                                 int& outW,
                                 int& outH,
                                 QString* error = nullptr);

private:
    /** @brief 实例侧包装：失败写入 m_lastError */
    bool queryOutputSize(const QString& vwfPath, int projectorIndex, int& outW, int& outH);

    /**
     * @brief VWB_CreateA(NULL,…) → setCryptoKey → 填写 calibFile/calibIndex → VWB_Init
     * OpenGL 下 device 传 nullptr，要求调用时上下文已 current
     */
    bool createWarper(QOpenGLFunctions* f,
                      const QString& vwfPath,
                      int projectorIndex,
                      const QByteArray& cryptoKey16);

    /** @brief 进程级密钥缓冲：VWB_setCryptoKey 只保留指针，须在 Init 期间保持有效 */
    static bool applyProcessCryptoKey(const QByteArray& cryptoKey16, QString* error);

    void* m_warper = nullptr; ///< 实际类型为 VWB_Warper*，避免头文件污染 Qt 侧
    QString m_loadedPath;     ///< 已加载的 .vwf 绝对路径
    int m_loadedIndex = -1;   ///< 已加载的投影机索引
    QByteArray m_loadedCryptoKey; ///< 已应用到本实例的 16 字节 key（空=关闭）
    int m_outWidth = 0;       ///< 当前通道输出宽
    int m_outHeight = 0;      ///< 当前通道输出高
    bool m_ready = false;     ///< Init 成功且可 render
    QString m_lastError;      ///< 最近一次失败原因（便于节点侧排查）
};

} // namespace Nodes
