// ============================================================================
//  ArtnetClipSection 实现 —— Artnet 片段 DMX 配置区
//  （交互约定与 ControlClipSection 同源：轮询 diff 写回 + on_edited 落定）
// ============================================================================

#include "ArtnetClipSection.hpp"

#include <QEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

extern "C" {
#include <libavformat/avformat.h>
}

#include "Elements/SelectorComboBox/SelectorComboBox.hpp" // GUIElements
#include "Common/AppConfig/ConstantDefines.h"             // 素材根目录

namespace ms {

// ---------------------------------------------------------------------------
// 工作副本片段现查（ControlClipSection 同源：clip_lookup 优先，防 vector 搬移）
// ---------------------------------------------------------------------------
Clip* ArtnetClipSection::boundClip() {
    if (ctx_.clip_lookup) {
        if (Clip* c = ctx_.clip_lookup()) return c;
    }
    return ctx_.clip;
}

const Clip* ArtnetClipSection::boundClip() const {
    if (ctx_.clip_lookup) {
        if (Clip* c = ctx_.clip_lookup()) return c;
    }
    return ctx_.clip;
}

// ---------------------------------------------------------------------------
// 构造：控件树 + 预填 + 轮询启动（预填在轮询启动之前——首帧 diff 无变化）
// ---------------------------------------------------------------------------
ArtnetClipSection::ArtnetClipSection(const ClipSectionContext& ctx,
                                     QWidget* parent)
    : QWidget(parent), ctx_(ctx) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    auto* box = new QGroupBox(QStringLiteral("Artnet 回放"), this);
    auto* lay = new QGridLayout(box);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(4);

    // ---- DMX 视频（媒体库 DMX 分类候选；文本 = 库内相对文件名）------------
    lay->addWidget(new QLabel(QStringLiteral("DMX 视频:"), box), 0, 0);
    file_ = new SelectorComboBox(MediaLibrary::Category::DMX, box);
    file_->setObjectName("artnetFile");
    file_->setPlaceholderText(QStringLiteral("媒体库 DMX 视频名"));
    lay->addWidget(file_, 0, 1);

    // ---- 起始域 ------------------------------------------------------------
    lay->addWidget(new QLabel(QStringLiteral("回放起始域:"), box), 1, 0);
    universe_ = new QSpinBox(box);
    universe_->setObjectName("artnetUniverse");
    universe_->setRange(0, 65535);
    lay->addWidget(universe_, 1, 1);

    // ---- 目标主机（IPv4，含 .255 广播）--------------------------------------
    lay->addWidget(new QLabel(QStringLiteral("目标主机:"), box), 2, 0);
    host_ = new QLineEdit(box);
    host_->setObjectName("artnetHost");
    host_->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral(
            "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")),
        box));
    host_->setToolTip(QStringLiteral("目标主机 IP，如 192.168.1.100 或 "
                                     "192.168.1.255（广播）"));
    lay->addWidget(host_, 2, 1);

    // ---- 文件探测信息 --------------------------------------------------------
    info_ = new QLabel(box);
    info_->setObjectName("artnetInfo");
    info_->setStyleSheet(QStringLiteral("color:#888;"));
    info_->setWordWrap(true);
    lay->addWidget(info_, 3, 0, 1, 2);

    root->addWidget(box);

    // ---- 预填（片段域 clip_data → 控件）--------------------------------------
    if (const Clip* c = boundClip()) refreshFrom(c->clip_data);

    // ---- 轮询 diff 写回（仅可见时；控件无统一变更信号，diff 是唯一真值）----
    poll_ = new QTimer(this);
    poll_->setInterval(800);
    connect(poll_, &QTimer::timeout, this, &ArtnetClipSection::onEdited);
}

// ---------------------------------------------------------------------------
// 可见性驱动轮询：面板显示启动，隐藏即停 + 冲掉未落定编辑
// ---------------------------------------------------------------------------
void ArtnetClipSection::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    if (poll_) poll_->start();
}

void ArtnetClipSection::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    if (poll_) poll_->stop();
    onEdited(); // 隐藏前把未落定的编辑冲掉（页签切走的瞬间兜底）
}

// ---------------------------------------------------------------------------
// 【ClipSectionInterface】宿主广播灌值（模型→面板向）
// ---------------------------------------------------------------------------
void ArtnetClipSection::refreshFromClip(const ms::Clip* clip) {
    if (!clip) return; // 签名已类型化（P3-B），无需 static_cast 还原
    refreshFrom(clip->clip_data);
}

// ---------------------------------------------------------------------------
// 控件当前值 → 配置 JSON（host 空 = 省略 → 模型侧回默认广播地址）
// ---------------------------------------------------------------------------
QString ArtnetClipSection::currentJson() const {
    QJsonObject o;
    o[QStringLiteral("file")]         = file_ ? file_->text() : QString();
    o[QStringLiteral("targetHost")]   = host_ ? host_->text().trimmed()
                                              : QString();
    o[QStringLiteral("startUniverse")] = universe_ ? universe_->value() : 0;
    return QString::fromUtf8(
        QJsonDocument(o).toJson(QJsonDocument::Compact));
}

// ---------------------------------------------------------------------------
// clip_data → 控件灌值（空串/解析失败 = 默认值；diff 快照更新 + 探测）
// ---------------------------------------------------------------------------
void ArtnetClipSection::refreshFrom(const std::string& json) {
    const QString q = QString::fromStdString(json);
    if (q == last_json_) return; // 内容未变：不重灌（防回环 + 保控件焦点）
    last_json_ = q;

    QString file;
    QString host = QStringLiteral("192.168.1.255");
    int     universe = 0;
    if (!q.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(q.toUtf8());
        if (doc.isObject()) {
            const QJsonObject o = doc.object();
            file     = o[QStringLiteral("file")].toString();
            host     = o[QStringLiteral("targetHost")].toString(
                           QStringLiteral("192.168.1.255"));
            universe = o[QStringLiteral("startUniverse")].toInt(0);
        }
    }
    {
        const QSignalBlocker b1(file_);
        const QSignalBlocker b2(universe_);
        const QSignalBlocker b3(host_);
        if (file_) file_->setText(file);
        if (universe_) universe_->setValue(universe);
        if (host_) host_->setText(host);
    }
    updateInfoLabel();
}

// ---------------------------------------------------------------------------
// 轮询落点：serialize ≠ 快照 → 直写片段域 + on_edited 落定
// ---------------------------------------------------------------------------
void ArtnetClipSection::onEdited() {
    Clip* c = boundClip();
    if (!c) return;
    const QString now = currentJson();
    if (now == last_json_) return; // 无变化
    c->clip_data = now.toStdString();
    last_json_ = now;
    if (ctx_.on_edited) ctx_.on_edited(); // 宿主落定：payload 回推模型 +
                                          // sceneEdited + 广播
}

// ---------------------------------------------------------------------------
// 文件探测 → 信息标签（轻 avformat 探测；与模型回放解码互不依赖）
// ---------------------------------------------------------------------------
void ArtnetClipSection::updateInfoLabel() {
    if (!info_) return;
    const QString file = file_ ? file_->text().trimmed() : QString();
    if (file.isEmpty()) {
        info_->setText(QStringLiteral(
            "选择媒体库中的 DMX 视频（宽 512 × 高 = 域数，FFV1 编码）"));
        return;
    }
    const QString abs =
        AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + file;
    AVFormatContext* probe = nullptr;
    if (avformat_open_input(&probe, abs.toUtf8().constData(),
                            nullptr, nullptr) < 0 || !probe) {
        info_->setText(QStringLiteral("⚠ 文件打不开：%1").arg(file));
        if (probe) avformat_close_input(&probe);
        return;
    }
    if (avformat_find_stream_info(probe, nullptr) < 0) {
        avformat_close_input(&probe);
        info_->setText(QStringLiteral("⚠ 流信息读取失败：%1").arg(file));
        return;
    }
    QString out;
    for (unsigned i = 0; i < probe->nb_streams; ++i) {
        const AVStream* st = probe->streams[i];
        if (st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) continue;
        const double seconds =
            st->duration > 0
                ? st->duration * av_q2d(st->time_base)
                : probe->duration / (double)AV_TIME_BASE;
        const bool ffv1 = st->codecpar->codec_id == AV_CODEC_ID_FFV1;
        out = QStringLiteral("%1 · %2×%3 · %4 域 · %5 秒%6")
                  .arg(file,
                       QString::number(st->codecpar->width),
                       QString::number(st->codecpar->height),
                       QString::number(st->codecpar->height),
                       QString::number(seconds, 'f', 2),
                       ffv1 ? QString() : QStringLiteral(" · ⚠ 非 FFV1"));
        break;
    }
    if (out.isEmpty()) out = QStringLiteral("⚠ 无视频流：%1").arg(file);
    info_->setText(out);
    avformat_close_input(&probe);
}

} // namespace ms
