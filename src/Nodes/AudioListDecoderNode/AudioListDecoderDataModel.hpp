/**
 * @file AudioListDecoderDataModel.hpp
 * @brief 独立音频播放列表节点（QListView 行内：文件选择 + 增益 + 上下移）
 *
 * Index 仅供 PLAY；列表编辑与自动切曲不改 Index。
 * 「添加」在列表右上角；每行含 SelectorComboBox 与增益调节。
 *
 * 端口：PLAY / INDEX / LOOP / LIST_LOOP → CH0…
 */
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QModelIndex>
#include <QSignalBlocker>
#include <QSize>
#include <QStandardItem>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "AudioListDecoder.hpp"
#include "AudioListDecoderInterface.hpp"
#include "PluginDefinition.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    class AudioListDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        enum InputPort : PortIndex {
            PlayPort = 0,
            IndexPort = 1,
            LoopPort = 2,
            ListLoopPort = 3
        };

        Q_PROPERTY(int index READ indexProperty WRITE setIndexProperty NOTIFY indexChanged)
        Q_PROPERTY(bool loop READ loopProperty WRITE setLoopProperty NOTIFY loopChanged)
        Q_PROPERTY(bool listLoop READ listLoopProperty WRITE setListLoopProperty NOTIFY listLoopChanged)
        Q_PROPERTY(bool playing READ playingProperty WRITE setPlayingProperty NOTIFY playingChanged)

    public:
        AudioListDecoderDataModel()
        {
            InPortCount = 4;
            OutPortCount = 2;
            Caption = PLUGIN_NAME;
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = true;

            m_player = std::make_shared<AudioListDecoder>();

            connectUiSignals();
            registerOscBindings();

            connect(m_player.get(), &AudioListDecoder::statusMessage,
                    this, [this](const QString &msg) {
                widget->setStatusText(msg);
            }, Qt::QueuedConnection);

            connect(m_player.get(), &AudioListDecoder::currentIndexChanged,
                    this, [this](int) {
                refreshPlayingHighlight();
            }, Qt::QueuedConnection);

            connect(m_player.get(), &AudioListDecoder::playingChanged,
                    this, [this](bool playing) {
                m_playing = playing;
                widget->syncPlayButton(playing);
                refreshPlayingHighlight();
                Q_EMIT playingChanged(playing);
            }, Qt::QueuedConnection);

            connect(m_player.get(), &AudioListDecoder::playlistChanged,
                    this, [this]() {
                rebuildPlaylistView();
                clampIndexToPlaylist();
            }, Qt::QueuedConnection);

            connect(m_player.get(), &AudioListDecoder::playlistFinished,
                    this, [this]() {
                m_playing = false;
                widget->syncPlayButton(false);
                refreshPlayingHighlight();
                Q_EMIT playingChanged(false);
            }, Qt::QueuedConnection);

            rebuildPlaylistView();
            widget->setStatusText(QStringLiteral("空列表"));
        }

        ~AudioListDecoderDataModel() override
        {
            if (m_player) {
                m_player->stop(true);
            }
        }

        NodeDataType dataType(PortType portType, PortIndex) const override
        {
            if (portType == PortType::Out) {
                return AudioData().type();
            }
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                static const char *const kIn[] = {
                    "PLAY", "INDEX", "LOOP", "LIST_LOOP"
                };
                return (portIndex >= 0 && portIndex < 4)
                    ? QString::fromLatin1(kIn[portIndex])
                    : QString();
            }
            if (portType == PortType::Out) {
                return QStringLiteral("CH %1").arg(portIndex);
            }
            return {};
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(m_player->getAudioBuffer(static_cast<int>(port)));
            return audioData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }

            switch (portIndex) {
            case PlayPort:
                setPlayingProperty(isTriggerTrue(*var));
                break;
            case IndexPort: {
                const int idx = extractIndexFromData(var);
                if (idx >= 0) {
                    setIndexProperty(idx);
                }
                break;
            }
            case LoopPort:
                setLoopProperty(isTriggerTrue(*var));
                break;
            case ListLoopPort:
                setListLoopProperty(isTriggerTrue(*var));
                break;
            default:
                break;
            }
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            return (portType == PortType::In || portType == PortType::Out)
                ? ConnectionPolicy::Many
                : ConnectionPolicy::One;
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("index")] = m_index;
            values[QStringLiteral("loop")] = m_loop;
            values[QStringLiteral("listLoop")] = m_listLoop;
            values[QStringLiteral("playing")] = m_playing;

            QJsonArray items;
            for (const AudioListItem &it : m_player->playlist()) {
                QJsonObject o;
                o[QStringLiteral("path")] = it.relativePath;
                o[QStringLiteral("gainDb")] = it.gainDb;
                items.append(o);
            }
            values[QStringLiteral("playlist")] = items;

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            NodeDelegateModel::load(p);
            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isObject()) {
                return;
            }
            const QJsonObject values = v.toObject();

            QVector<AudioListItem> items;
            const QJsonArray arr = values.value(QStringLiteral("playlist")).toArray();
            for (const QJsonValue &jv : arr) {
                if (!jv.isObject()) {
                    continue;
                }
                const QJsonObject o = jv.toObject();
                AudioListItem it;
                it.relativePath = o.value(QStringLiteral("path")).toString();
                it.gainDb = o.value(QStringLiteral("gainDb")).toDouble(0.0);
                if (!it.relativePath.isEmpty()) {
                    items.append(it);
                }
            }
            m_player->setPlaylist(items);

            if (values.contains(QStringLiteral("loop"))) {
                setLoopProperty(values.value(QStringLiteral("loop")).toBool());
            }
            if (values.contains(QStringLiteral("listLoop"))) {
                setListLoopProperty(values.value(QStringLiteral("listLoop")).toBool());
            }

            setIndexOnly(values.value(QStringLiteral("index")).toInt(0));

            const bool shouldPlay = values.value(QStringLiteral("playing")).toBool(false);
            if (shouldPlay && !items.isEmpty()) {
                playByIndex(m_index);
            } else if (!items.isEmpty()) {
                m_player->prepareIndex(qBound(0, m_index, items.size() - 1));
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        int indexProperty() const { return m_index; }
        bool loopProperty() const { return m_loop; }
        bool listLoopProperty() const { return m_listLoop; }
        bool playingProperty() const { return m_playing; }

        void setIndexProperty(int index) { setIndexOnly(index); }

        void setLoopProperty(bool loop)
        {
            if (m_loop == loop) {
                return;
            }
            m_loop = loop;
            {
                QSignalBlocker b(widget->loopCheckBox);
                widget->loopCheckBox->setChecked(loop);
            }
            if (loop && m_listLoop) {
                m_listLoop = false;
                QSignalBlocker b(widget->listLoopCheckBox);
                widget->listLoopCheckBox->setChecked(false);
                Q_EMIT listLoopChanged(false);
            }
            applyLoopMode();
            Q_EMIT loopChanged(loop);
        }

        void setListLoopProperty(bool listLoop)
        {
            if (m_listLoop == listLoop) {
                return;
            }
            m_listLoop = listLoop;
            {
                QSignalBlocker b(widget->listLoopCheckBox);
                widget->listLoopCheckBox->setChecked(listLoop);
            }
            if (listLoop && m_loop) {
                m_loop = false;
                QSignalBlocker b(widget->loopCheckBox);
                widget->loopCheckBox->setChecked(false);
                Q_EMIT loopChanged(false);
            }
            applyLoopMode();
            Q_EMIT listLoopChanged(listLoop);
        }

        void setPlayingProperty(bool playing)
        {
            if (playing == m_playing) {
                return;
            }
            if (playing) {
                playByIndex(m_index);
            } else {
                stopPlayback();
            }
        }

    signals:
        void indexChanged(int index);
        void loopChanged(bool loop);
        void listLoopChanged(bool listLoop);
        void playingChanged(bool playing);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/index"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/listLoop"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address == makeFullOscAddress("/index")) {
                setIndexProperty(ev.payload.toInt());
            } else if (ev.address == makeFullOscAddress("/play")) {
                setPlayingProperty(ev.payload.toBool());
            } else if (ev.address == makeFullOscAddress("/loop")) {
                setLoopProperty(ev.payload.toBool());
            } else if (ev.address == makeFullOscAddress("/listLoop")) {
                setListLoopProperty(ev.payload.toBool());
            }
        }

    private:
        void connectUiSignals()
        {
            connect(widget->playButton, &QPushButton::toggled, this, [this](bool checked) {
                setPlayingProperty(checked);
            });
            connect(widget->indexSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this](int v) {
                if (m_index != v) {
                    m_index = v;
                    Q_EMIT indexChanged(v);
                }
            });
            connect(widget->loopCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setLoopProperty(checked);
            });
            connect(widget->listLoopCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setListLoopProperty(checked);
            });

            connect(widget->addButton, &QPushButton::clicked, this, [this]() {
                AudioListItem it;
                it.relativePath.clear();
                it.gainDb = 0.0;
                m_player->appendItem(it);
                widget->setStatusText(QStringLiteral("已添加第 %1 条，请选择文件")
                                          .arg(m_player->itemCount() - 1));
            });

            connect(widget->playlistView, &QListView::doubleClicked,
                    this, [this](const QModelIndex &index) {
                if (!index.isValid()) {
                    return;
                }
                playByIndex(index.data(AudioListDecoderInterface::kRoleTrackIndex).toInt());
            });
        }

        void registerOscBindings()
        {
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "index";
                b.control = widget->indexSpinBox;
                AbstractDelegateModel::registerExternalBinding("/index", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "loop";
                b.control = widget->loopCheckBox;
                AbstractDelegateModel::registerExternalBinding("/loop", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "listLoop";
                b.control = widget->listLoopCheckBox;
                AbstractDelegateModel::registerExternalBinding("/listLoop", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "playing";
                b.control = widget->playButton;
                AbstractDelegateModel::registerExternalBinding("/play", this, b);
            }
        }

        void rebuildPlaylistView()
        {
            auto *model = widget->playlistModel;
            auto *view = widget->playlistView;

            for (int r = 0; r < model->rowCount(); ++r) {
                const QModelIndex idx = model->index(r, 0);
                if (QWidget *w = view->indexWidget(idx)) {
                    view->setIndexWidget(idx, nullptr);
                    w->deleteLater();
                }
            }
            model->clear();

            const auto items = m_player->playlist();
            const int count = items.size();
            widget->indexSpinBox->setMaximum(count > 0 ? count - 1 : 0);

            for (int i = 0; i < count; ++i) {
                const AudioListItem &it = items.at(i);
                auto *item = new QStandardItem();
                item->setData(i, AudioListDecoderInterface::kRoleTrackIndex);
                item->setEditable(false);
                model->appendRow(item);

                auto *row = new PlaylistTrackRowWidget(
                    i, it.relativePath, it.gainDb,
                    i > 0, i < count - 1, view);
                row->setPlayingHighlight(m_playing && i == m_player->currentIndex());
                item->setSizeHint(row->sizeHint().expandedTo(QSize(0, 36)));

                connect(row, &PlaylistTrackRowWidget::moveUpRequested, this, [this, i]() {
                    if (i > 0) {
                        m_player->moveItem(i, i - 1);
                    }
                });
                connect(row, &PlaylistTrackRowWidget::moveDownRequested, this, [this, i]() {
                    if (i < m_player->itemCount() - 1) {
                        m_player->moveItem(i, i + 1);
                    }
                });
                connect(row, &PlaylistTrackRowWidget::removeRequested, this, [this, i]() {
                    m_player->removeItem(i);
                });
                connect(row, &PlaylistTrackRowWidget::pathChanged, this, [this, i](const QString &path) {
                    m_player->setItemPath(i, path.trimmed());
                });
                connect(row, &PlaylistTrackRowWidget::gainChanged, this, [this, i](double gainDb) {
                    m_player->setItemGain(i, gainDb);
                });

                const QModelIndex modelIndex = model->index(model->rowCount() - 1, 0);
                view->setIndexWidget(modelIndex, row);
            }
        }

        void refreshPlayingHighlight()
        {
            auto *model = widget->playlistModel;
            auto *view = widget->playlistView;
            const int playingIdx = m_player->currentIndex();
            for (int r = 0; r < model->rowCount(); ++r) {
                const QModelIndex idx = model->index(r, 0);
                auto *row = qobject_cast<PlaylistTrackRowWidget *>(view->indexWidget(idx));
                if (!row) {
                    continue;
                }
                const int track = idx.data(AudioListDecoderInterface::kRoleTrackIndex).toInt();
                row->setPlayingHighlight(m_playing && track == playingIdx);
            }
        }

        void applyLoopMode()
        {
            AudioListLoopMode mode = AudioListLoopMode::Off;
            if (m_loop) {
                mode = AudioListLoopMode::Single;
            } else if (m_listLoop) {
                mode = AudioListLoopMode::List;
            }
            m_player->setLoopMode(mode);
        }

        void setIndexOnly(int index)
        {
            const int count = m_player->itemCount();
            if (count > 0) {
                index = qBound(0, index, count - 1);
            } else {
                index = 0;
            }
            {
                QSignalBlocker b(widget->indexSpinBox);
                widget->indexSpinBox->setMaximum(count > 0 ? count - 1 : 0);
                widget->indexSpinBox->setValue(index);
            }
            if (m_index != index) {
                m_index = index;
                Q_EMIT indexChanged(index);
            }
        }

        void clampIndexToPlaylist()
        {
            const int count = m_player->itemCount();
            widget->indexSpinBox->setMaximum(count > 0 ? count - 1 : 0);
            if (count <= 0) {
                if (m_index != 0) {
                    m_index = 0;
                    QSignalBlocker b(widget->indexSpinBox);
                    widget->indexSpinBox->setValue(0);
                    Q_EMIT indexChanged(0);
                }
                return;
            }
            if (m_index >= count) {
                setIndexOnly(count - 1);
            }
        }

        void playByIndex(int index)
        {
            if (m_player->itemCount() <= 0) {
                widget->setStatusText(QStringLiteral("播放列表为空"));
                m_playing = false;
                widget->syncPlayButton(false);
                return;
            }
            index = qBound(0, index, m_player->itemCount() - 1);
            if (!m_player->playIndex(index)) {
                // 引擎未发出 playingChanged，本地复位按钮
                m_playing = false;
                widget->syncPlayButton(false);
                return;
            }
            // 成功时由 AudioListDecoder::playingChanged 同步按钮与 m_playing
            refreshPlayingHighlight();
        }

        void stopPlayback()
        {
            m_player->stop(true);
            // playingChanged(false) 由引擎发出
            widget->setStatusText(QStringLiteral("已停止"));
        }

        static bool isTriggerTrue(const VariableData &data)
        {
            const QVariant value = data.value();
            if (value.typeId() == QMetaType::Bool) {
                return value.toBool();
            }
            if (value.canConvert<int>()) {
                return value.toInt() != 0;
            }
            const QString s = value.toString().trimmed().toLower();
            return s == QLatin1String("true") || s == QLatin1String("1");
        }

        static int extractIndexFromData(const std::shared_ptr<VariableData> &data)
        {
            if (!data || data->isEmpty()) {
                return -1;
            }
            if (data->hasKey(QStringLiteral("index"))) {
                bool ok = false;
                const int index = data->value(QStringLiteral("index")).toInt(&ok);
                return ok ? index : -1;
            }
            if (!data->hasKey(QStringLiteral("default"))) {
                return -1;
            }
            const QVariant value = data->value(QStringLiteral("default"));
            if (value.typeId() == QMetaType::Bool) {
                return -1;
            }
            bool ok = false;
            const int index = value.toInt(&ok);
            return ok ? index : -1;
        }

        AudioListDecoderInterface *widget = new AudioListDecoderInterface();
        std::shared_ptr<AudioListDecoder> m_player;

        int m_index = 0;
        bool m_loop = false;
        bool m_listLoop = false;
        bool m_playing = false;
    };
}
