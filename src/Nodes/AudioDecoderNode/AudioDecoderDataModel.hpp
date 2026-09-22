#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "Common/DataTypes/NodeDataList.hpp"
#include "AudioDecoderInterface.hpp"
#include <QFile>
#include <memory>

#include "QtNodes/Definitions"

#include "QTimer"
#include "AudioDecoder.hpp"
#include "QThread"
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "AudioPeakDecoder.hpp"
#include "Elements/AudioWaveformWidget/AudioWaveformPeaks.hpp"

using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::NodeId;
using QtNodes::NodeRole;

using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    class AudioDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString file READ filePathProperty WRITE setFilePathProperty NOTIFY filePathChanged)
        Q_PROPERTY(bool loop READ loopProperty WRITE setLoopProperty NOTIFY loopChanged)
        Q_PROPERTY(double volume READ volumeProperty WRITE setVolumeProperty NOTIFY volumeChanged)
        Q_PROPERTY(bool playing READ playingProperty WRITE setPlayingProperty NOTIFY playingChanged)
        Q_PROPERTY(double progress READ progressProperty NOTIFY progressChanged)
    public:
        /**
       * @brief 构造函数，初始化音频解码Node，支持动态多通道分离输出
       */
        AudioDecoderDataModel(){
            InPortCount = 3;
            OutPortCount = 2;  // 初始输出端口数，可动态调整
            CaptionVisible = true;
            Caption = "Audio Decoder";
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = true;

            // 初始化波形异步解码 watcher（生命周期随节点，析构时自动取消等待）
            m_peakWatcher.reset(new QFutureWatcher<std::shared_ptr<AudioWaveformPeaks>>());
            connect(m_peakWatcher.data(), &QFutureWatcherBase::finished,
                    this, &AudioDecoderDataModel::onPeakDecodingFinished, Qt::DirectConnection);

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "file";
                b.control = widget->fileSelectComboBox;
                AbstractDelegateModel::registerExternalBinding("/file", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "volume";
                b.control = widget->volumeSlider;
                AbstractDelegateModel::registerExternalBinding("/volume", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "loop";
                b.control = widget->loopCheckBox;
                AbstractDelegateModel::registerExternalBinding("/loop", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "playing";
                b.control = widget->playButton;
                AbstractDelegateModel::registerExternalBinding("/play", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "progress";
                AbstractDelegateModel::registerExternalBinding("/progress", this, b);
            }

            connect(widget->fileSelectComboBox, &SelectorComboBox::textChanged,
                    this, &AudioDecoderDataModel::setFilePathProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::filePathChanged, this, [this](const QString&){
                {
                    QSignalBlocker blocker(widget->fileSelectComboBox);
                    widget->fileSelectComboBox->setText(m_filePath);
                }
                widget->waveformWidget->setPlayheadSec(-1.0);
                if (!m_filePath.isEmpty()) {
                    if (player->getPlaying()) {
                        player->stopPlay();
                    }
                    auto res = player->initializeFFmpeg(AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + m_filePath);
                    if (!res) {
                        isReady = false;
                        widget->waveformWidget->setPeaks(nullptr);
                        widget->waveformWidget->setDurationSec(0.0);
                        widget->waveformWidget->setPlaceholderText(QStringLiteral("音频初始化失败"));
                        return;
                    }
                    isReady = true;
                    // 根据 FFmpeg 初始化返回的实际时长，先立即生成 demo 占位波形（解码真波形期间用户能看到轮廓 + 进度）
                    rebuildDemoWaveformFromJson(res);
                    if (res->contains(QStringLiteral("duration"))) {
                        widget->waveformWidget->setDurationSec(res->value(QStringLiteral("duration")).toDouble());
                    }
                    // 异步解码真正的波形（命中缓存直接秒返回）
                    startPeakDecodingForPath(AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QStringLiteral("/") + m_filePath);
                } else {
                    isReady = false;
                    widget->waveformWidget->setPeaks(nullptr);
                    widget->waveformWidget->setDurationSec(0.0);
                    widget->waveformWidget->setPlaceholderText(QStringLiteral("请先选择音频文件"));
                    // 换空文件时取消正在进行的解码，避免回调把旧 peaks 写回来
                    abortPeakDecoding();
                }
            });

            connect(widget->volumeSlider, &FloatDragValueWidget::valueChanged,
                    this, &AudioDecoderDataModel::setVolumeProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::volumeChanged, this, [this](double){
                {
                    QSignalBlocker blocker(widget->volumeSlider);
                    widget->volumeSlider->setValue(m_volume);
                }
                player->setVolume(m_volume);
            });

            connect(widget->loopCheckBox, &QCheckBox::toggled,
                    this, &AudioDecoderDataModel::setLoopProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::loopChanged, this, [this](bool){
                {
                    QSignalBlocker blocker(widget->loopCheckBox);
                    widget->loopCheckBox->setChecked(m_loop);
                }
                player->setLooping(m_loop);
            });

            connect(widget->playButton, &QPushButton::toggled,
                    this, &AudioDecoderDataModel::setPlayingProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::playingChanged, this, [this](bool playing){
                {
                    QSignalBlocker blocker(widget->playButton);
                    // 避免 autoExclusive 导致无法取消勾选
                    widget->playButton->setAutoExclusive(false);
                    widget->playButton->setChecked(playing);
                    widget->playButton->setText(playing ? QStringLiteral("Stop") : QStringLiteral("Play"));
                }
                // EOF 复位只同步属性/按钮，避免再次 stopPlay 与线程 wait 纠缠
                if (m_suppressPlaySideEffects) {
                    return;
                }
                if (playing) {
                    if (!isReady) {
                        return;
                    }
                    // 保留拖拽定位的起点，避免 stopPlay 把它清零
                    player->stopPlay(/*resetPosition=*/false);
                    player->startPlay();
                } else {
                    player->stopPlay(/*resetPosition=*/true);
                    onPlaybackProgress(0, player->durationSec() > 0.0 ? player->durationSec() : 1.0);
                }
            });

            connect(widget->waveformWidget, &AudioWaveformWidget::seekRequested,
                    this, [this](double sec, bool /*dragging*/) {
                if (!isReady) {
                    return;
                }
                player->seekTo(sec);
                const double total = widget->waveformWidget->durationSec();
                if (total > 0.0) {
                    const double p = qBound(0.0, sec / total, 1.0);
                    if (std::fabs(p - m_progress) >= 5e-5) {
                        m_progress = p;
                        Q_EMIT progressChanged(m_progress);
                    }
                }
                // 未播放时只更新播放头；播放中由 decoder 的 progress 继续刷新
                if (!m_playing && widget->waveformWidget) {
                    widget->waveformWidget->setPlayheadSec(sec);
                }
            }, Qt::QueuedConnection);

            connect(player.get(), &AudioDecoder::playbackProgress,
                    this, &AudioDecoderDataModel::onPlaybackProgress, Qt::QueuedConnection);
            // 播放自然结束：强制复位按钮/进度（不依赖 setPlayingProperty 的 early-return）
            connect(player.get(), &AudioDecoder::playbackFinished,
                    this, [this]() {
                m_playing = false;
                m_progress = 0.0;
                {
                    QSignalBlocker blocker(widget->playButton);
                    widget->playButton->setAutoExclusive(false);
                    widget->playButton->setChecked(false);
                    widget->playButton->setText(QStringLiteral("Play"));
                }
                if (widget->waveformWidget) {
                    widget->waveformWidget->setPlayheadSec(0.0);
                }
                Q_EMIT progressChanged(0.0);
                // 清理缓冲并复位起点（此时解码线程多半已退出或正在收尾）
                player->stopPlay(/*resetPosition=*/true);
                // 仅通知属性/OSC，跳过 playingChanged 里的二次 stop/start
                m_suppressPlaySideEffects = true;
                Q_EMIT playingChanged(false);
                m_suppressPlaySideEffects = false;
            }, Qt::QueuedConnection);
        }

        /**
         * @brief 析构函数，释放资源（停播 + 结束/忽略波形解码回调）
         */
        ~AudioDecoderDataModel(){
            abortPeakDecoding();
            if (m_peakWatcher) {
                // 析构时若 watcher 还在跑，断开 finished 信号避免回调到已销毁 this
                QObject::disconnect(m_peakWatcher.data(), nullptr, this, nullptr);
            }
            if (player->getPlaying()){
                player->stopPlay();
            }
        }

        /**
     * @brief 获取端口标题
     * @param portType 端口类型（输入/输出）
     * @param portIndex 端口索引
     * @return 端口标题字符串
     */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return AudioData().type();
            case PortType::None:
                break;
            }

            return VariableData().type();
        }

        /**
         * @brief 获取指定端口的输出数据（建立连接时调用）
         * @param port 端口索引 (0-N对应不同声道)
         * @return 包含共享环形缓冲区的音频数据
         */
        std::shared_ptr<NodeData> outData(PortIndex port) override
        {

            
            // 创建新的AudioData并设置共享环形缓冲区
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(player->getAudioBuffer(port));

            return audioData;

        }


        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "PLAY";
                case 1:
                    return "LOOP";
                case 2:
                    return "GAIN";
                default:
                    return "";
                }
            case PortType::Out:
                    return "CH "+QString::number(portIndex);
            default:
                return "";
            }


        }
        /**
         * @brief 设置端口输入
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            switch (portIndex) {
            case 0: {
                    auto d = std::dynamic_pointer_cast<VariableData>(data);
                    if (d != nullptr) {
                        setPlayingProperty(d->asBool());
                    }
                    return;
            }

            case 1:{
                    auto d = std::dynamic_pointer_cast<VariableData>(data);
                    if (d != nullptr) {
                        setLoopProperty(d->asBool());
                    }
                    return;
            }
            case 2:{
                    auto d = std::dynamic_pointer_cast<VariableData>(data);
                    if (d != nullptr) {
                        setVolumeProperty(d->asNumber());
                    }
                    return;
            }
            }
        }

        QWidget *embeddedWidget() override
        {

            return widget;
        }

        /**
     * @brief 保存节点状态
     */
        QJsonObject save() const override
        {
            QJsonObject values;
            values["filePath"] = m_filePath;
            values["isLoop"] = loopProperty();
            values["volume"] = volumeProperty();
            values["play"] = m_playing;
            values["autoPlay"] = m_playing;

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        /**
     * @brief 加载节点状态
     */
        void load(const QJsonObject &p) override
        {
            NodeDelegateModel::load(p);

            QJsonObject values;
            const QJsonValue vv = p.value("values");
            if (vv.isObject()) {
                values = vv.toObject();
            } else {
                values = p;
            }

            bool shouldPlay = false;
            if (values.contains(QStringLiteral("play"))) {
                shouldPlay = values[QStringLiteral("play")].toBool();
            } else if (values.contains(QStringLiteral("autoPlay"))) {
                shouldPlay = values[QStringLiteral("autoPlay")].toBool();
            }
            autoPlay = shouldPlay;

            if (values.contains(QStringLiteral("isLoop"))) {
                setLoopProperty(values[QStringLiteral("isLoop")].toBool());
            }

            if (values.contains(QStringLiteral("volume"))) {
                setVolumeProperty(values[QStringLiteral("volume")].toDouble());
            }

            const QString savedFile = values.value(QStringLiteral("filePath")).toString();
            if (!savedFile.isEmpty() && savedFile != m_filePath) {
                m_playing = false;
                setFilePathProperty(savedFile);
            } else if (!savedFile.isEmpty() && !isReady) {
                m_playing = false;
                m_filePath = savedFile;
                {
                    QSignalBlocker blocker(widget->fileSelectComboBox);
                    widget->fileSelectComboBox->setText(m_filePath);
                }
                if (player->getPlaying()) {
                    player->stopPlay();
                }
                widget->waveformWidget->setPlayheadSec(-1.0);
                const QString absPath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QStringLiteral("/") + m_filePath;
                const auto res = player->initializeFFmpeg(absPath);
                isReady = res != nullptr;
                if (isReady) {
                    rebuildDemoWaveformFromJson(res);
                    startPeakDecodingForPath(absPath);
                } else {
                    widget->waveformWidget->setPeaks(nullptr);
                    widget->waveformWidget->setPlaceholderText(QStringLiteral("音频初始化失败"));
                }
            }

            const auto applyPlayState = [this, shouldPlay]() {
                if (shouldPlay && isReady) {
                    m_playing = false;
                    playAudio();
                } else {
                    stopAudio();
                }
            };

            if (!savedFile.isEmpty()) {
                const QString absPath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QStringLiteral("/") + savedFile;
                if (QFile::exists(absPath)) {
                    QTimer::singleShot(100, this, applyPlayState);
                    return;
                }
            }

            applyPlayState();
        }

    public slots:
        /**
         * 函数级注释：接收解码器播放进度，同步到波形播放头与 progress 属性
         *  1. progress 属性：按 0.005% 粒度变化判断是否 emit，降低下游压力
         *  2. 波形播放头/右下角时间文本：无条件调用 setPlayheadSec(currentSec)，
         *     由 Widget 内部判断"播放头像素位置或时间文本是否变化"再决定是否重绘，
         *     解决"文字刷新不及时"(旧实现被 progress 百分位阈值挡住)
         */
        void onPlaybackProgress(double currentSec, double totalSec) {
            const double p = (totalSec > 0.0) ? qBound(0.0, currentSec / totalSec, 1.0) : 0.0;
            // 注意：局部变量名不能叫 progressChanged，否则和同名 signal 冲突，触发 C2064 "项不会计算为接受 1 个参数的函数"
            const bool needEmitProgress = std::fabs(p - m_progress) >= 5e-5;
            if (needEmitProgress) {
                m_progress = p;
                Q_EMIT progressChanged(m_progress);
            }
            // 没有 peaks 也要刷新时间文本(右下角显示 cur/total)，所以不用 isValid 门槛
            if (widget && widget->waveformWidget) {
                widget->waveformWidget->setPlayheadSec(currentSec);
            }
        }

        /**
         * 选则音频文件
         */
        void select_audio_file(QString fileName)
        {
            if(fileName!="")
            {
                setFilePathProperty(fileName);
            }

        }

        /**
         * 播放音频
         */
        void playAudio() {
            setPlayingProperty(true);
        }
        /**
         * 停止播放
         */
        void stopAudio(){
            setPlayingProperty(false);
        }
        /**
         * @brief 音量改变槽函数
         * @param value 音量值 (0-100)
         */
        void onVolumeChanged(double value) {
            setVolumeProperty(value);
        }

        /**
         * @brief 循环播放切换槽函数
         * @param checked 是否启用循环播放
         */
        void onLoopToggled(bool checked) {
            setLoopProperty(checked);
            qDebug() << "循环播放:" << (checked ? "启用" : "禁用");
        }

        QString filePathProperty() const
        {
            return m_filePath;
        }

        /**
         * 函数级注释：获取播放进度属性（0.0~1.0）
         */
        double progressProperty() const
        {
            return m_progress;
        }

        void setFilePathProperty(const QString& relative)
        {
            const QString trimmed = relative.trimmed();
            if (trimmed == m_filePath) {
                return;
            }
            m_filePath = trimmed;
            Q_EMIT filePathChanged(trimmed);
        }

        bool loopProperty() const
        {
            return m_loop;
        }

        void setLoopProperty(bool loop)
        {
            if (loop == m_loop) {
                return;
            }
            m_loop = loop;
            Q_EMIT loopChanged(loop);
        }

        double volumeProperty() const
        {
            return m_volume;
        }

        void setVolumeProperty(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_volume + 1.0)) {
                return;
            }
            m_volume = value;
            Q_EMIT volumeChanged(value);
        }

        /**
         * @brief 获取当前播放状态属性
         * @return true 为正在播放，false 为停止
         */
        bool playingProperty() const
        {
            return m_playing;
        }

        /**
         * @brief 设置播放状态属性
         * @param playing true 开始播放，false 停止播放
         */
        void setPlayingProperty(bool playing)
        {
            if (playing == m_playing) {
                return;
            }
            if (playing && !isReady) {
                return;
            }
            m_playing = playing;
            if (!m_playing) {
                if (!qFuzzyCompare(0.0 + 1.0, m_progress + 1.0)) {
                    m_progress = 0.0;
                    Q_EMIT progressChanged(m_progress);
                }
                // 停止时把波形播放头归零（回到 0 秒位置，波形总时长仍在）
                widget->waveformWidget->setPlayheadSec(0.0);
            }
            Q_EMIT playingChanged(playing);
        }

    signals:
        void filePathChanged(const QString& path);
        void loopChanged(bool loop);
        void volumeChanged(double volume);
        void playingChanged(bool playing);
        void progressChanged(double progress);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/file"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/loop"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/volume"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/play"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addrFile = makeFullOscAddress("/file");
            const QString addrLoop = makeFullOscAddress("/loop");
            const QString addrVolume = makeFullOscAddress("/volume");
            const QString addrPlay = makeFullOscAddress("/play");
            const QString addrStop = makeFullOscAddress("/stop");

            if (ev.address == addrFile) {
                setFilePathProperty(ev.payload.toString());
            } else if (ev.address == addrLoop) {
                setLoopProperty(ev.payload.toBool());
            } else if (ev.address == addrVolume) {
                setVolumeProperty(ev.payload.toDouble());
            } else if (ev.address == addrPlay) {
                setPlayingProperty(ev.payload.toBool());
            } else if (ev.address == addrStop) {
                if (ev.payload.toBool()) {
                    setPlayingProperty(false);
                }
            }
        }

        /**
         * 函数级注释：异步解码 watcher 完成回调，把 peaks 写入 waveformWidget；失败回退到 demo 占位
         */
        void onPeakDecodingFinished()
        {
            if (!m_peakWatcher) return;
            std::shared_ptr<AudioWaveformPeaks> peaks;
            try {
                peaks = m_peakWatcher->result();
            } catch (...) {
                peaks = nullptr;
            }
            const QString path = m_peakDecodingPath;
            m_peakDecodingPath.clear();
            // ABA 保护：解码期间如果用户切到别的文件，不要把旧结果覆盖回来
            const QString currentAbs = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QStringLiteral("/") + m_filePath;
            if (m_filePath.isEmpty() || QFileInfo(path) != QFileInfo(currentAbs)) {
                return;
            }
            if (peaks && peaks->isValid()) {
                widget->waveformWidget->setPeaks(std::move(peaks));
                widget->waveformWidget->setPlayheadSec(-1.0);
            } else {
                // 失败：保留之前的 demo 波形，右下角提示"解码失败，显示占位波形"
                widget->waveformWidget->setPlaceholderText(QStringLiteral("波形解码失败，显示占位波形"));
            }
        }


    private:
        /**
         * 函数级注释：开始异步解码波形（先查缓存，命中直接同步返回；未命中在线程池 decodeSync + 写缓存）
         * 期间用占位波形显示"生成波形中 xx%"进度文本
         */
        void startPeakDecodingForPath(const QString& absoluteFilePath)
        {
            if (!m_peakWatcher) return;
            const QFileInfo fi(absoluteFilePath);
            if (!fi.exists()) {
                widget->waveformWidget->setPlaceholderText(QStringLiteral("文件不存在"));
                return;
            }
            // 1. 缓存命中：直接同步 setPeaks，不走异步
            std::shared_ptr<AudioWaveformPeaks> cached;
            if (AudioPeakDecoder::loadFromCache(absoluteFilePath, cached) && cached && cached->isValid()) {
                widget->waveformWidget->setPeaks(std::move(cached));
                widget->waveformWidget->setPlayheadSec(-1.0);
                m_peakDecodingPath.clear();
                return;
            }
            // 2. 取消之前的解码（如存在），启动新解码
            if (m_peakWatcher->isRunning()) {
                // QtConcurrent::run 的 future 默认不支持 cancel，我们只靠 ABA 检查在 finished 时丢弃
            }
            widget->waveformWidget->setPlaceholderText(QStringLiteral("正在生成波形…"));
            m_peakDecodingPath = absoluteFilePath;

            QFuture<std::shared_ptr<AudioWaveformPeaks>> future = QtConcurrent::run(
                [absoluteFilePath]() -> std::shared_ptr<AudioWaveformPeaks> {
                    AudioPeakDecoder dec;
                    QString err;
                    auto p = dec.decodeSync(absoluteFilePath, &err);
                    if (p && p->isValid()) {
                        AudioPeakDecoder::saveToCache(absoluteFilePath, *p);
                    }
                    Q_UNUSED(err);
                    return p;
                });
            m_peakWatcher->setFuture(future);
        }

        /**
         * 函数级注释：切换到空文件/析构前，取消等待中的 watcher（取消不会真正中断后台线程，仅防止 finished 写入 UI）
         */
        void abortPeakDecoding()
        {
            m_peakDecodingPath.clear(); // ABA：后续 finished 回调会因 path != current 自动丢弃
            // 这里不调用 future.waitForFinished()，避免 UI 阻塞
        }

        /**
         * 函数级注释：根据 FFmpeg 初始化返回的时长/采样率生成假波形占位
         * 目的：音频选择后立即可见波形（真正的解码 peaks 生成留待后续迭代）
         */
        void rebuildDemoWaveformFromJson(QJsonObject* res) {
            if (!res || !widget || !widget->waveformWidget) return;
            const QVariantMap vmap = res->toVariantMap();
            bool okDur = false;
            double totalSec = 0.0;
            QVariant vDur = vmap.value("duration");
            if (vDur.isValid()) totalSec = vDur.toDouble(&okDur);
            if (!okDur || totalSec <= 0.0) totalSec = 0.0;
            bool okSr = false;
            int sampleRate = 48000;
            QVariant vSr = vmap.value("sample_rate");
            if (vSr.isValid()) {
                const int s = vSr.toInt(&okSr);
                if (okSr && s > 0) sampleRate = s;
            }
            int bins = 8192;
            if (totalSec > 0.0) {
                bins = std::max(1024, static_cast<int>(
                                    (totalSec * static_cast<double>(sampleRate)) / static_cast<double>(AudioWaveformPeaks::kSamplesPerBin) + 0.5));
            }
            widget->waveformWidget->setDurationSec(totalSec);
            widget->waveformWidget->setDemoData(bins, sampleRate);
        }

        AudioDecoderInterface *widget=new AudioDecoderInterface();
        //    界面控件
        std::shared_ptr<AudioDecoder> player=std::make_shared<AudioDecoder>();
        QString m_filePath="";
        bool m_loop=false;
        double m_volume = 0.0;
        bool m_playing = false;
        double m_progress = 0.0;
        bool autoPlay=false;
        bool isReady= false;
        bool m_suppressPlaySideEffects = false;

        // 波形解码异步对象与当前路径（用于 ABA 检查）
        QScopedPointer<QFutureWatcher<std::shared_ptr<AudioWaveformPeaks>>> m_peakWatcher;
        QString m_peakDecodingPath;

    };
}
