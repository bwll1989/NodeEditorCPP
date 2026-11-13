#include "VideoTranscoder.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

#include "Common/ProcessGuard/JobGuard.hpp"

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif

Q_LOGGING_CATEGORY(logTranscoder, "flow.medialibrary.transcoder")

namespace {

/**
 * @brief 判断 ffmpeg 像素格式名是否含 alpha 分量
 *        覆盖常见家族：rgba/bgra/argb/abgr（含 64 位变体）、yuva*、gbrap*、ya8/ya16
 */
bool pixFmtHasAlpha(const QString& pixFmt)
{
    if (pixFmt.isEmpty()) return false;
    static const QStringList markers = {
        QStringLiteral("rgba"), QStringLiteral("bgra"), QStringLiteral("argb"),
        QStringLiteral("abgr"), QStringLiteral("yuva"), QStringLiteral("gbrap"),
        QStringLiteral("ya8"), QStringLiteral("ya16"),
    };
    for (const QString& m : markers) {
        if (pixFmt.contains(m)) return true;
    }
    return false;
}

/**
 * @brief 按候选路径列表定位可执行文件
 */
QString locateExecutable(const QStringList& candidates)
{
    for (const QString& c : candidates) {
        if (QFileInfo::exists(c)) {
            return QFileInfo(c).absoluteFilePath();
        }
    }
    return QString();
}

/**
 * @brief 收集 FFmpeg DLL 所在目录（进程已加载 avcodec DLL，通过 GetModuleHandle 定位），
 *        供 ffmpeg.exe / ffprobe.exe 的查找：运行时 exe 与 DLL 同目录发布
 */
QStringList ffmpegDllDirs()
{
    QStringList dirs;
#ifdef Q_OS_WIN
    static const wchar_t* dllNames[] = {
        L"avcodec-62.dll", L"avcodec-61.dll", L"avcodec-60.dll",
    };
    for (const wchar_t* name : dllNames) {
        HMODULE mod = ::GetModuleHandleW(name);
        if (!mod) continue;
        wchar_t buf[MAX_PATH] = {0};
        const DWORD len = ::GetModuleFileNameW(mod, buf, MAX_PATH);
        if (len > 0) {
            dirs << QFileInfo(QString::fromWCharArray(buf, static_cast<int>(len))).absolutePath();
        }
    }
#endif
    return dirs;
}

} // namespace

VideoTranscoder::VideoTranscoder(QObject* parent)
    : QObject(parent)
{
}

VideoTranscoder::~VideoTranscoder()
{
    // 进程退出时终止仍在运行的 ffmpeg，避免留下孤儿进程
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

QString VideoTranscoder::ffmpegExecutable()
{
    const QString exeName = QStringLiteral("ffmpeg.exe");
    QStringList candidates;
    for (const QString& dir : ffmpegDllDirs()) {
        candidates << dir + QLatin1Char('/') + exeName;
    }
    const QString appDir = QCoreApplication::applicationDirPath();
    candidates << appDir + QLatin1Char('/') + exeName
               << appDir + QStringLiteral("/bin/") + exeName
               << appDir + QStringLiteral("/ffmpeg/bin/") + exeName
               << appDir + QStringLiteral("/../ffmpeg/bin/") + exeName
               << appDir + QStringLiteral("/../bin/") + exeName;
    return locateExecutable(candidates);
}

QString VideoTranscoder::ffprobeExecutable()
{
    const QString ffmpeg = ffmpegExecutable();
    if (!ffmpeg.isEmpty()) {
        const QString candidate = QFileInfo(ffmpeg).absolutePath() + QStringLiteral("/ffprobe.exe");
        if (QFileInfo::exists(candidate)) return candidate;
        return QString(); // 无 ffprobe 时仅退化为无进度/无跳过检测，不影响转码
    }
    return QString();
}

QString VideoTranscoder::outputPathFor(const QString& srcPath)
{
    const QFileInfo fi(srcPath);
    const QString base = fi.completeBaseName();
    if (fi.suffix().compare(QLatin1String("mov"), Qt::CaseInsensitive) == 0) {
        // 源本身是 mov：先写 _HAP 临时名，转码成功删除原文件后改回原名（finalPathFor）
        return fi.absolutePath() + QLatin1Char('/') + base + QStringLiteral("_HAP.mov");
    }
    return fi.absolutePath() + QLatin1Char('/') + base + QStringLiteral(".mov");
}

QString VideoTranscoder::finalPathFor(const QString& srcPath)
{
    const QFileInfo fi(srcPath);
    return fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName() + QStringLiteral(".mov");
}

bool VideoTranscoder::enqueue(const QString& absPath)
{
    if (absPath.isEmpty() || !QFileInfo::exists(absPath)) return false;
    if (m_current.src == absPath) return false;
    for (const Job& j : m_queue) {
        if (j.src == absPath) return false;
    }
    m_queue.enqueue({ absPath, outputPathFor(absPath) });
    qCInfo(logTranscoder) << "导入文件:" << absPath;
    startNext();
    return true;
}

void VideoTranscoder::startNext()
{
    if (m_process || m_queue.isEmpty()) return;

    m_current = m_queue.dequeue();
    m_durationSec = 0.0;
    m_lastPercent = -1;
    m_audioCopyAttempted = false;
    m_sourceHasAlpha = false;
    m_progressBuf.clear();

    // 无 ffprobe：跳过探测，直接转码（无跳过检测与进度百分比）
    const QString probe = ffprobeExecutable();
    if (probe.isEmpty()) {
        emit transcodeStarted(m_current.src);
        emit transcodeProgress(m_current.src, -1);
        startTranscode(m_current, true);
        return;
    }
    startProbe(m_current);
}

void VideoTranscoder::startProbe(const Job& job)
{
    m_phase = Phase::Probe;
    m_process = new QProcess(this);
    m_process->setProgram(ffprobeExecutable());
    m_process->setArguments({
        QStringLiteral("-v"), QStringLiteral("error"),
        QStringLiteral("-select_streams"), QStringLiteral("v:0"),
        QStringLiteral("-show_entries"),
        QStringLiteral("stream=codec_name,pix_fmt:format=duration"),
        QStringLiteral("-of"), QStringLiteral("json"),
        job.src,
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        const QString src = m_current.src;
        QString out, errTail;
        if (m_process) {
            out = QString::fromUtf8(m_process->readAllStandardOutput());
            errTail = QString::fromUtf8(m_process->readAllStandardError()).simplified().left(300);
        }

        bool ok = (status == QProcess::NormalExit && exitCode == 0);
        QString codecName;
        QString pixFmt;
        double duration = 0.0;
        if (ok) {
            const QJsonObject root = QJsonDocument::fromJson(out.toUtf8()).object();
            const QJsonArray streams = root.value(QStringLiteral("streams")).toArray();
            if (!streams.isEmpty()) {
                const QJsonObject st = streams.at(0).toObject();
                codecName = st.value(QStringLiteral("codec_name")).toString();
                pixFmt = st.value(QStringLiteral("pix_fmt")).toString();
            }
            duration = root.value(QStringLiteral("format")).toObject()
                           .value(QStringLiteral("duration")).toString().toDouble();
        }

        if (!ok) {
            // 探测失败不阻断：记录日志后仍尝试转码（无进度百分比）
            qCWarning(logTranscoder) << "ffprobe 失败，直接转码:" << src << errTail;
            emit transcodeStarted(src);
            emit transcodeProgress(src, -1);
            startTranscode(m_current, true);
            return;
        }

        // 已是 HAP 编码：跳过（success=true 且 outPath 为空表示跳过，不动任何文件）
        if (codecName.startsWith(QLatin1String("hap"))) {
            qCInfo(logTranscoder) << "源已是 HAP，跳过转码:" << src;
            finishCurrent(true, QString(), QStringLiteral("已是 HAP 编码，跳过转码"));
            return;
        }

        m_durationSec = duration;
        m_sourceHasAlpha = pixFmtHasAlpha(pixFmt);
        qCInfo(logTranscoder) << "探测完成:" << src << "codec=" << codecName
                              << "pix_fmt=" << pixFmt << "alpha=" << m_sourceHasAlpha;
        emit transcodeStarted(src);
        startTranscode(m_current, true);
    });

    // 【幽灵 socket 治本 2026-11-13】ffmpeg/ffprobe 纳入陪葬 job
    connect(m_process, &QProcess::started, m_process, [this]() {
        if (m_process) {
            ms::assignChildToJob(static_cast<unsigned long>(m_process->processId()));
        }
    });

    m_process->start();
}

void VideoTranscoder::startTranscode(const Job& job, bool audioCopy)
{
    m_phase = Phase::Transcode;
    m_progressBuf.clear();
    m_lastPercent = -1;

    QStringList args;
    args << QStringLiteral("-y")
         << QStringLiteral("-hide_banner")
         << QStringLiteral("-loglevel") << QStringLiteral("error")
         << QStringLiteral("-nostdin")
         << QStringLiteral("-progress") << QStringLiteral("pipe:1")
         << QStringLiteral("-i") << job.src
         << QStringLiteral("-map") << QStringLiteral("0:v:0")
         << QStringLiteral("-map") << QStringLiteral("0:a?")
         // DXT 纹理要求宽高为 4 的倍数；已是 4 倍数时该滤镜为无操作
         << QStringLiteral("-vf") << QStringLiteral("scale=trunc(iw/4)*4:trunc(ih/4)*4")
         << QStringLiteral("-c:v") << QStringLiteral("hap")
         << QStringLiteral("-format") << (m_sourceHasAlpha
             ? QStringLiteral("hap_alpha")   // DXT5 含 alpha（本构建不支持 hap_q_alpha）
             : QStringLiteral("hap_q"));     // DXT5-YCoCg 高质量
    if (audioCopy) {
        args << QStringLiteral("-c:a") << QStringLiteral("copy");
    } else {
        args << QStringLiteral("-c:a") << QStringLiteral("aac") << QStringLiteral("-b:a") << QStringLiteral("256k");
    }
    args << job.out;

    m_process = new QProcess(this);
    m_process->setProgram(ffmpegExecutable());
    m_process->setArguments(args);
    m_process->setWorkingDirectory(QFileInfo(job.out).absolutePath());

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        if (!m_process) return;
        m_progressBuf += m_process->readAllStandardOutput();
        // -progress pipe:1 按行输出 key=value
        int idx;
        while ((idx = m_progressBuf.indexOf('\n')) >= 0) {
            const QByteArray line = m_progressBuf.left(idx).trimmed();
            m_progressBuf.remove(0, idx + 1);
            if (!line.startsWith(QByteArrayLiteral("out_time_us="))) continue;
            const double us = line.mid(12).toDouble();
            if (m_durationSec > 0.0 && us > 0) {
                int percent = static_cast<int>(us / 1.0e6 / m_durationSec * 100.0);
                percent = qBound(0, percent, 99); // 100% 留给完成事件
                if (percent != m_lastPercent) {
                    m_lastPercent = percent;
                    emit transcodeProgress(m_current.src, percent);
                }
            }
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, audioCopy](int exitCode, QProcess::ExitStatus status) {
        QString errTail;
        if (m_process) {
            errTail = QString::fromUtf8(m_process->readAllStandardError()).simplified().left(300);
        }
        const bool ok = (status == QProcess::NormalExit && exitCode == 0);
        if (ok) {
            // qCInfo(logTranscoder) << "转码完成:" << m_current.src << "->" << m_current.out;
            finishCurrent(true, m_current.out, QString());
            return;
        }
        qCWarning(logTranscoder) << "转码失败(exit" << exitCode << "):" << m_current.src << errTail;

        // 音频 copy 失败（mov 容器不接受源音频编码等）→ 回退 AAC 重试一次
        if (audioCopy) {
            m_audioCopyAttempted = true;
            cleanupProcess();
            emit transcodeProgress(m_current.src, -1);
            startTranscode(m_current, false);
            return;
        }
        finishCurrent(false, QString(),
                      errTail.isEmpty() ? QStringLiteral("ffmpeg 退出码 %1").arg(exitCode) : errTail);
    });

    // 【幽灵 socket 治本 2026-11-13】ffmpeg 纳入陪葬 job（同 startProbe）
    connect(m_process, &QProcess::started, m_process, [this]() {
        if (m_process) {
            ms::assignChildToJob(static_cast<unsigned long>(m_process->processId()));
        }
    });

    // qCInfo(logTranscoder) << "开始转码:" << m_current.src << "->" << m_current.out;
    m_process->start();
}

void VideoTranscoder::finishCurrent(bool success, const QString& outPath, const QString& message)
{
    const QString src = m_current.src;
    QString out = outPath;

    if (success && !out.isEmpty()) {
        // 删除原始文件，产物保持原始文件名（仅扩展名变 .mov）
        const QString finalPath = finalPathFor(src);
        if (!QFile::remove(src)) {
            // 原文件被占用（如正在播放）等：删除失败则保留 _HAP 临时命名，不阻断流程
            qCWarning(logTranscoder) << "删除原始视频失败，保留 HAP 临时命名:" << src;
        } else if (out != finalPath) {
            if (!QFile::rename(out, finalPath)) {
                qCWarning(logTranscoder) << "HAP 产物改名为最终文件名失败，保留:" << out;
            } else {
                out = finalPath;
            }
        }
    } else if (!success) {
        // 清理转码失败的半成品输出
        QFile::remove(m_current.out);
        out.clear();
    }

    cleanupProcess();
    emit transcodeFinished(src, out, success, message);
    startNext();
}

void VideoTranscoder::cleanupProcess()
{
    if (!m_process) return;
    m_process->deleteLater();
    m_process = nullptr;
}
