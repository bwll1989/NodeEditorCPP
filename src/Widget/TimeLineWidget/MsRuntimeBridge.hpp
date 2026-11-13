// ============================================================================
//  MsRuntimeBridge.hpp —— Flow 主窗口 ↔ EditorRuntime 桥接壳（2026-11-12）
//
//  定位：原 media_editor 的 EditorWindow 承担"编辑 GUI ↔ 下发运行时"装配，
//  收编进 Flow 后 EditorWindow 已删，本桥以最小接口承接其运行时半边：
//    * EditorRuntime（显示端连接/场景下发/素材 HTTP 分发/锚点命令）；
//    * ClockProvider（全场对齐时钟——命令 now 注入与播放权威时钟）；
//    * SceneLoader（Scene pb ↔ base64——.flow 工程内嵌场景段）。
//
//  【为何需要本壳——C++ 标准分界】Flow 主程序按 C++17 编译，而
//  EditorRuntime.hpp 的 include 链（AssetHttpServer/ControlClient…）带
//  Poco/protobuf 与 C++20 特性（同 FfmpegMediaSource 的 atomic<shared_ptr>
//  先例）——C++17 TU 不可 include。本壳住 ms_editor_ui（C++20），对外
//  头文件只暴露 SceneModel 纯结构 + Qt 类型，MainWindow（C++17）可安全
//  include；运行时全部细节走 PIMPL（Impl 在 .cpp）。
//
//  线程模型：EditorRuntime 内部自管 recv/看门狗/发送线程；本桥方法全部
//  GUI 线程调用（rt 的 cmd*/add 等仅入队，天然线程安全）。displayRows 等
//  快照读取内部带锁，安全。
// ============================================================================

#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <memory>

#include "model/SceneModel.hpp" // ms::Scene/Asset（纯 C++ 结构，C++17 兼容）

namespace ms {

class MsRuntimeBridge : public QObject {
    Q_OBJECT

public:
    /// 显示端登记行（POD；GUI 侧转 SceneTreeWidget::DisplayState）。
    /// 【2026-11-12 统一入口】.flow 工程的 mediaDisplays 段以此为准：
    /// id/host/port/enabled/pairingCode 为登记态（随工程持久化），
    /// connected/registered 为运行态（不持久化）。
    struct DisplayRow {
        QString id;                ///< 显示端槽位 id（= 工程绑定 player_id）
        QString host;              ///< 主机/IP
        int     port = 17100;      ///< 监听端口
        bool    enabled   = true;  ///< 启用（禁用 = 断开且不重连）
        bool    connected = false; ///< TCP 在线
        bool    registered = false; ///< 已注册（收到过注册帧）
        QString pairingCode;       ///< 配对码（空 = 不认证；随工程持久化）
    };

    explicit MsRuntimeBridge(QObject* parent = nullptr);
    ~MsRuntimeBridge() override;

    // ---- 生命周期 -----------------------------------------------------------
    /// 启动下发运行时（UDP 心跳 + 素材 HTTP 分发 + 显示端连接看门狗）。
    /// 端口占用等失败 → false（GUI 可继续用，仅无下发；qWarning 留痕）。
    bool start();
    void stop();
    bool running() const;

    // ---- 场景 / 素材（只入队，GUI 零阻塞）-----------------------------------
    /// 下发场景：全量广播给全部在线显示端（0 端在线 = 0 份送达天然合法）；
    /// 发送线程内幂等注册素材（推模型）+ serialize + 广播。
    void publishScene(const Scene& scene);
    /// 同步媒体：注册素材 + 推清单（display 收到即比对拉取，幂等）。
    /// 返回本次新注册素材数（0 = 无新增文件，全部已就位；宿主状态栏反馈用）
    std::size_t syncMedia(const Scene& scene);
    /// 权威场景快照（最后一次发布的；锁内深拷贝）。false = 尚无发布
    bool authoritySnapshot(Scene& out) const;

    // ---- 播放控制（对齐时钟内部注入；语义对齐原 EditorRuntime::cmd*）--------
    void cmdPlay();                     // 从头/续播（锚点 = now + lead）
    void cmdPause();                    // 冻结于锚点时刻
    /// 跳转：Playing 续播 / Paused 冻结于新位置 / Idle 只记录定位点
    void cmdSeekToUs(std::int64_t session_time_us);
    void cmdStop();                     // 回 Idle
    /// 会话是否播放中（区别于本地预览状态机）
    bool playing() const;
    /// 播放权威时钟：Playing 时返回会话播放头（us），否则 -1
    ///（GUI 侧据此统一喂预览 seekTo/时间轴回显——单一时间权威）
    std::int64_t sessionTimeUs() const;

    // ---- 显示端管理 ----------------------------------------------------------
    bool addDisplay(const QString& id, const QString& host, int port,
                    const QString& pairing_code = {});
    bool removeDisplay(const QString& id);
    /// 启停：停用立即断开，启用交看门狗重连（场景树右键"连接/断开"）
    bool setDisplayEnabled(const QString& id, bool enabled);
    QList<DisplayRow> displayRows() const;

    // ---- 素材同步进度（200ms UI 拍；display_id → "⤓ 同步 N/M · P%"）--------
    QMap<QString, QString> assetSyncTexts() const;

    // ---- .flow 工程序列化（静态；SceneLoader pb ↔ base64）-------------------
    /// Scene → base64（pb 字节）。失败返回空串并填 err。
    static QString sceneToBase64(const Scene& scene, QString* err = nullptr);
    /// base64 → Scene（R1 引用完整性校验含内）。失败返回 false。
    static bool sceneFromBase64(const QString& b64, Scene& out, QString& err);

private:
    class Impl;                  ///< PIMPL：EditorRuntime/ClockProvider 住 .cpp
    std::unique_ptr<Impl> impl_;
};

} // namespace ms
