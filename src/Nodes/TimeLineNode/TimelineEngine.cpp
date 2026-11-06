// ============================================================================
// TimelineEngine.cpp —— 内置时间线引擎实现（原 9 个 .cpp 合并）
// ============================================================================
#include "TimelineEngine.h"

// ---------- AbstractClipModel.cpp ----------
//
// Created by WuBin on 2026/10/30.
// AbstractClipModel 的库内实现。
//
// 说明：initPropertyWidget / showPropertyWidget 原先是头文件内联实现。
// 由于它们会直接构造 ClipTimePanel / ClipAppearancePanel 等 Q_OBJECT 窗口类，
// 宿主插件（.node/.plugin 动态库）在内联展开时会实例化这些面板类的
// tr() / staticMetaObject 等符号——其中 staticMetaObject 是静态数据符号，
// 无法通过导入库解析，导致插件链接失败（LNK2019）。
// 因此将实现移入库内（QtTimeLine.dll），宿主仅按普通导出函数调用。
//


// 面板类完整定义只在库内使用（头文件中仅为前置声明，见 AbstractClipModel.hpp）
// startDrag 中 qobject_cast<IntDragValueWidget*> 需要宿主控件完整类型，仅库内使用
#include "IntDragValueWidget/IntDragValueWidget.hpp"

#include <QClipboard>
#include <QDataStream>
#include <QDrag>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

void AbstractClipModel::initPropertyWidget()
{
    m_standardPropertyWidget = new QWidget();
    m_layout = new QVBoxLayout(m_standardPropertyWidget);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    // 创建一个容器 widget 来放置主要内容
    auto contentWidget = new QWidget(m_standardPropertyWidget);
    auto contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(4);

    // 1. 时间属性组（重构后：由 ClipTimePanel 管理）
    timeGroupBox = new ClipTimePanel(this, m_standardPropertyWidget);
    m_layout->addWidget(timeGroupBox);
    m_layout->addWidget(new ClipAppearancePanel(this, m_standardPropertyWidget));
    registerExternalControl("/start_frame", timeGroupBox->m_startFrameSpinBox);
    registerExternalControl("/end_frame", timeGroupBox->m_endFrameSpinBox);
    registerExternalControl("/position_frame", timeGroupBox->m_positionFrameSpinBox);
    registerExternalControl("/start_time_code", timeGroupBox->m_startTimeCodeLineEdit);
    registerExternalControl("/end_time_code", timeGroupBox->m_endTimeCodeLineEdit);
    registerExternalControl("/position_time_code", timeGroupBox->m_positionTimeLineEdit);

    // 添加代理编辑器的占位符
    if (!m_clipPropertyWidget) {
        m_clipPropertyWidget = clipPropertyWidget();
        contentLayout->addWidget(m_clipPropertyWidget);
    }

    // 将内容 widget 添加到主布局
    m_layout->addWidget(contentWidget);
    m_standardPropertyWidget->setWindowFlags(Qt::WindowStaysOnTopHint);
}

void AbstractClipModel::showPropertyWidget()
{
    if (m_clipPropertyWidget) {
        m_standardPropertyWidget->show();
    } else {
        initPropertyWidget();
        m_standardPropertyWidget->show();
    }
}


// 函数说明：把 OSC 控件拖拽数据打包为 mime 并启动拖拽（Ctrl+左键拖动控件时触发）。
// 实现说明：原为头文件内联实现；因 qobject_cast<IntDragValueWidget*> 需要控件完整类型，
// 而该控件属于宿主 GUIElements 库，公共头不得引入（会连带展开其 Q_OBJECT 符号，
// 导致消费方链接失败 LNK2019），故移入库内实现。
void AbstractClipModel::startDrag(QWidget* widget)
{
        // 找到对应的OSC地址
        QString oscAddress;
        for (const auto& pair : _OscMapping) {
            if (pair.second == widget) {
                oscAddress = pair.first;
                break;
            }
        }
        
        if (oscAddress.isEmpty()) return;
    
        OSCMessage message;
        const QString modelAlias = m_timelineModel
            ? m_timelineModel->data(QModelIndex(), TimelineRoles::TimelineModelAliasRole).toString()
            : QStringLiteral("default");
        message.address = modelAlias + "/" + QString::number(m_id) + oscAddress;
        message.host = "127.0.0.1";
        message.port = 8991;
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(message.address);
        // 获取控件的值
        if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
            message.value = button->isChecked();
            message.type = "Int";
        } else if (auto* slider = qobject_cast<QAbstractSlider*>(widget)) {
            message.value = slider->value();
            message.type = "Int";
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(widget)) {
            message.value = spinBox->value();
            message.type = "Int";
        } else if (auto* dragInt = qobject_cast<IntDragValueWidget*>(widget)) {
            message.value = dragInt->value();
            message.type = "Int";
        } else if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
            message.value = lineEdit->text();
            message.type = "String";
        } else if (auto* label = qobject_cast<QLabel*>(widget)) {
            message.value = label->text();
            message.type = "String";
        } else {
            message.value = QVariant();
            message.type = "String";
        }
    
        QByteArray itemData;
        QDataStream dataStream(&itemData, QIODevice::WriteOnly);
        dataStream << message.host << message.port << message.address <<message.type<<message.value;
    
        QMimeData* mimeData = new QMimeData;
        mimeData->setData("application/x-osc-address", itemData);
    
        QDrag* drag = new QDrag(widget);
        drag->setMimeData(mimeData);
        QPixmap pixmap(200, 30);
        pixmap.fill(Qt::transparent);
        
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 绘制背景
        QColor bgColor(40, 40, 40, 200);  // 半透明深灰色
        painter.setBrush(bgColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(pixmap.rect(), 5, 5);  // 圆角矩形
        // 绘制文本
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(9);
        painter.setFont(font);
        QRect textRect = pixmap.rect().adjusted(30, 0, -8, 0);  // 图标右侧的文本区域
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, message.address);
    
        // 设置拖拽预览
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));  // 热点在中心
    
        drag->exec(Qt::CopyAction);
}

// ---------- ClipTimePanel.cpp ----------
// ClipTimePanel.cpp
// 函数说明：实现 ClipTimePanel 构造逻辑，布局“时间属性”控件并保持原有功能：
// - 显示为 00:00:00:000（毫秒格式）
// - 文本框 textChanged 与 spinbox 一样调用 setStart/setEnd
// - 响应 AbstractClipModel 的 timelinePositionChanged/lengthChanged 信号更新 UI

#include <QObject>
#include <QString>

ClipTimePanel::ClipTimePanel(AbstractClipModel* owner, QWidget* parent)
    : m_owner(owner),
    QGroupBox(parent)
{
    setTitle(tr("时间属性"));
    m_timeLayout = new QGridLayout(this);

    // 开始帧显示
    auto startLabel = new QLabel(tr("开始帧:"), this);
    m_startFrameSpinBox = new IntDragValueWidget(this);
    m_startFrameSpinBox->setRange(0, 9999999);
    // m_startFrameSpinBox->setValue(m_owner->start());
    m_startTimeCodeLineEdit = new QTimeEdit(this);
    m_startTimeCodeLineEdit->setDisplayFormat("hh:mm:ss:zzz");
    m_timeLayout->addWidget(startLabel, 0, 0,1,1);
    m_timeLayout->addWidget(m_startFrameSpinBox, 0, 1,1,1);
    m_timeLayout->addWidget(m_startTimeCodeLineEdit, 0, 2,1,1);

    // 片段总长显示（绝对时间：毫秒）
    auto lengthLabel = new QLabel(tr("片段位置:"), this);
    m_positionFrameSpinBox = new IntDragValueWidget(this);
    m_positionFrameSpinBox->setRange(0, 9999999);
    m_positionTimeLineEdit = new QTimeEdit(this);
    m_positionTimeLineEdit->setDisplayFormat("hh:mm:ss:zzz");
    m_timeLayout->addWidget(lengthLabel, 1, 0,1,1);
    m_timeLayout->addWidget(m_positionFrameSpinBox, 1 ,1,1,1);
    m_timeLayout->addWidget(m_positionTimeLineEdit, 1, 2,1,1);

    // 结束帧显示（绝对时间：毫秒）
    auto endLabel = new QLabel(tr("结束帧:"), this);
    m_endFrameSpinBox = new IntDragValueWidget(this);
    m_endFrameSpinBox->setRange(0, 9999999);
    m_endTimeCodeLineEdit = new QTimeEdit(this);
    m_endTimeCodeLineEdit->setDisplayFormat("hh:mm:ss:zzz");
    m_timeLayout->addWidget(endLabel, 2, 0,1,1);
    m_timeLayout->addWidget(m_endFrameSpinBox, 2, 1,1,1);
    m_timeLayout->addWidget(m_endTimeCodeLineEdit, 2, 2,1,1);
    setMaximumHeight(150);
    updateStartFrame(m_owner->start());
    updateEndFrame(m_owner->end());
    updatePositionFrame(m_owner->start());
    initSignalConnections();
}
void ClipTimePanel::initSignalConnections() {
    connect(m_startFrameSpinBox,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updateStartFrame);
    connect(m_startTimeCodeLineEdit,&QTimeEdit::timeChanged,this,&ClipTimePanel::updateStartTimeCode);
    connect(m_endFrameSpinBox,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updateEndFrame);
    connect(m_endTimeCodeLineEdit,&QTimeEdit::timeChanged,this,&ClipTimePanel::updateEndTimeCode);
    connect(m_positionFrameSpinBox,&IntDragValueWidget::valueChanged,this,&ClipTimePanel::updatePositionFrame);
    connect(m_positionTimeLineEdit,&QTimeEdit::timeChanged,this,&ClipTimePanel::updatePositionTimeCode);
    connect(m_owner, &AbstractClipModel::timelinePositionChanged, this, [this](int frames){
        updateStartFrame(frames);
        updatePositionFrame(frames);
    });
    connect(m_owner, &AbstractClipModel::lengthChanged, this, [this](int frames){
       updateEndFrame(frames);
    });

}

int ClipTimePanel::clampStartFrame(int frames) const
{
    if (!m_owner || !m_owner->isResizable())
        return frames;
    if (frames < 0)
        frames = 0;
    const int maxStart = m_owner->end() - 1;
    if (frames > maxStart)
        frames = qMax(0, maxStart);
    return frames;
}

int ClipTimePanel::clampEndFrame(int frames) const
{
    if (!m_owner || !m_owner->isResizable())
        return frames;
    const int minEnd = m_owner->start() + 1;
    if (frames < minEnd)
        frames = minEnd;
    return frames;
}

void ClipTimePanel::refreshFrameRanges()
{
    if (!m_owner || !m_owner->isResizable() || !m_startFrameSpinBox || !m_endFrameSpinBox)
        return;
    QSignalBlocker blockStart(m_startFrameSpinBox);
    QSignalBlocker blockEnd(m_endFrameSpinBox);
    m_startFrameSpinBox->setRange(0, qMax(0, m_owner->end() - 1));
    m_endFrameSpinBox->setRange(m_owner->start() + 1, 9999999);
}

void ClipTimePanel::updateStartFrame(int frames) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    frames = clampStartFrame(frames);
    QSignalBlocker b1(m_startFrameSpinBox);
    QSignalBlocker b2(m_startTimeCodeLineEdit);
    m_startFrameSpinBox->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, m_owner->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, m_owner->getTimeCodeType());
    m_startTimeCodeLineEdit->setTime(time);

    m_owner->setStart(frames);
    refreshFrameRanges();
}

void ClipTimePanel::updateStartTimeCode(const QTime& tc) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    QSignalBlocker b1(m_startFrameSpinBox);
    QSignalBlocker b2(m_startTimeCodeLineEdit);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, m_owner->getTimeCodeType());
    int newStart = static_cast<int>(timecode_frame_to_frames(timecode, m_owner->getTimeCodeType()));
    newStart = clampStartFrame(newStart);
    m_startFrameSpinBox->setValue(newStart);
    const auto clampedTc = frames_to_timecode_frame(newStart, m_owner->getTimeCodeType());
    m_startTimeCodeLineEdit->setTime(timecode_frame_to_qtime(clampedTc, m_owner->getTimeCodeType()));
    m_owner->setStart(newStart);
    refreshFrameRanges();

}
void ClipTimePanel::updateEndFrame(int frames) {
    if (!m_owner) {
        return;
    }
    frames = clampEndFrame(frames);
    QSignalBlocker b1(m_endFrameSpinBox);
    QSignalBlocker b2(m_endTimeCodeLineEdit);
    m_endFrameSpinBox->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, m_owner->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, m_owner->getTimeCodeType());
    m_endTimeCodeLineEdit->setTime(time);
    m_owner->setEnd(frames);
    refreshFrameRanges();
}
void ClipTimePanel::updateEndTimeCode(const QTime& tc) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    QSignalBlocker b1(m_endFrameSpinBox);
    QSignalBlocker b2(m_endTimeCodeLineEdit);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, m_owner->getTimeCodeType());
    int newEnd = static_cast<int>(timecode_frame_to_frames(timecode, m_owner->getTimeCodeType()));
    newEnd = clampEndFrame(newEnd);
    m_endFrameSpinBox->setValue(newEnd);
    const auto clampedTc = frames_to_timecode_frame(newEnd, m_owner->getTimeCodeType());
    m_endTimeCodeLineEdit->setTime(timecode_frame_to_qtime(clampedTc, m_owner->getTimeCodeType()));
    m_owner->setEnd(newEnd);
    refreshFrameRanges();
}
void ClipTimePanel::updatePositionFrame(int frames) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    int length=m_owner->length();
    QSignalBlocker b1(m_positionFrameSpinBox);
    QSignalBlocker b2(m_positionTimeLineEdit);
    m_positionFrameSpinBox->setValue(frames);
    auto tc = frames_to_timecode_frame(frames, m_owner->getTimeCodeType());
    QTime time=timecode_frame_to_qtime(tc, m_owner->getTimeCodeType());
    m_positionTimeLineEdit->setTime(time);
    m_owner->setStart(frames);
    m_owner->setEnd(frames+length);
}
void ClipTimePanel::updatePositionTimeCode(const QTime& tc) {
    if (!m_owner) {
        qDebug() << "m_owner is nullptr";
        return;
    }
    int length=m_owner->length();
    QSignalBlocker b1(m_positionFrameSpinBox);
    QSignalBlocker b2(m_positionTimeLineEdit);
    TimeCodeFrame timecode = qtime_to_timecode_frame(tc, m_owner->getTimeCodeType());
    int newEnd = static_cast<int>(timecode_frame_to_frames(timecode, m_owner->getTimeCodeType()));
    m_positionFrameSpinBox->setValue(newEnd);
    m_owner->setStart(newEnd);
    m_owner->setEnd(newEnd+length);
}

// ---------- ClipAppearancePanel.cpp ----------
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
bool sameRgb(const QColor& a, const QColor& b)
{
    return a.isValid() && b.isValid() && (a.rgb() & 0x00ffffff) == (b.rgb() & 0x00ffffff);
}
}

ClipAppearancePanel::ClipAppearancePanel(AbstractClipModel* owner, QWidget* parent)
    : QGroupBox(parent)
    , m_owner(owner)
{
    setTitle(tr("片段属性"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* colorRow = new QHBoxLayout();
    colorRow->setSpacing(4);
    colorRow->addWidget(new QLabel(tr("颜色:"), this));

    m_currentColor = new QLabel(this);
    m_currentColor->setFixedSize(22, 22);
    colorRow->addWidget(m_currentColor);

    m_colors = new QButtonGroup(this);
    m_colors->setExclusive(true);
    const QList<QColor> presets = titleColorPresets();
    for (int i = 0; i < presets.size(); ++i) {
        const QColor preset = presets.at(i);
        auto* button = new QToolButton(this);
        button->setCheckable(true);
        button->setFixedSize(22, 22);
        button->setToolTip(preset.name(QColor::HexRgb));
        m_colors->addButton(button, i);
        colorRow->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, preset]() {
            if (m_syncing || !m_owner)
                return;
            m_owner->setColor(preset);
        });
    }
    colorRow->addStretch(1);
    layout->addLayout(colorRow);

    auto* noteRow = new QHBoxLayout();
    noteRow->setSpacing(4);
    noteRow->addWidget(new QLabel(tr("备注名:"), this));
    m_noteEdit = new QLineEdit(this);
    m_noteEdit->setPlaceholderText(tr("留空则显示类型名"));
    noteRow->addWidget(m_noteEdit, 1);
    layout->addLayout(noteRow);

    connect(m_noteEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_syncing || !m_owner)
            return;
        m_owner->setNoteName(text);
    });
    if (m_owner) {
        connect(m_owner, &AbstractClipModel::appearanceChanged, this, [this]() {
            syncFromOwner();
        });
    }
    syncFromOwner();
}

void ClipAppearancePanel::changeEvent(QEvent* event)
{
    QGroupBox::changeEvent(event);
    // QSS（qproperty-*）重新生效后，用新的边框颜色重建色块样式
    if (event->type() == QEvent::StyleChange) {
        syncFromOwner();
    }
}

void ClipAppearancePanel::syncFromOwner()
{
    if (!m_owner)
        return;
    m_syncing = true;

    const QColor current = m_owner->color();

    // 当前颜色指示块：背景为片段颜色数据，边框样式来自 QSS 可配置属性
    m_currentColor->setStyleSheet(QStringLiteral(
        "background-color: %1; border: 1px solid %2; border-radius: 2px;")
        .arg(current.name(QColor::HexRgb),
             m_currentColorBorderColor.name(QColor::HexRgb)));

    // 预设色块：背景为预设颜色数据，边框样式来自 QSS 可配置属性
    const QList<QColor> presets = titleColorPresets();
    const QList<QAbstractButton*> buttons = m_colors->buttons();
    for (int i = 0; i < buttons.size() && i < presets.size(); ++i) {
        buttons.at(i)->setStyleSheet(QStringLiteral(
            "QToolButton { background-color: %1; border: 1px solid %2; border-radius: 2px; }"
            "QToolButton:checked { border: 2px solid %3; }")
            .arg(presets.at(i).name(QColor::HexRgb),
                 m_swatchBorderColor.name(QColor::HexRgb),
                 m_swatchCheckedBorderColor.name(QColor::HexRgb)));
    }

    int matched = -1;
    if (m_owner->hasCustomColor()) {
        for (int i = 0; i < presets.size(); ++i) {
            if (sameRgb(presets.at(i), current)) {
                matched = i;
                break;
            }
        }
    }
    m_colors->setExclusive(false);
    for (QAbstractButton* button : buttons)
        button->setChecked(false);
    if (matched >= 0) {
        if (QAbstractButton* button = m_colors->button(matched))
            button->setChecked(true);
    }
    m_colors->setExclusive(true);

    {
        QSignalBlocker blocker(m_noteEdit);
        if (m_noteEdit->text() != m_owner->noteName())
            m_noteEdit->setText(m_owner->noteName());
    }
    m_syncing = false;
}

// ---------- BasePluginLoader.cpp ----------



// TimeLineNode 私有副本：内置剪辑工厂（静态注册，替代 QPluginLoader 动态加载）。
// 这些 clip 继承本目录静态副本的 AbstractClipModel / NodeClipDelegateModel，
// 与宿主 plugins/TimeLine/*.plugin（继承 QtTimeLine.dll 的类）互不兼容，
// 因此 loadPlugins() 不再扫描磁盘目录。
#include "artnetclipplugin.hpp"
#include "curveclipplugin.hpp"
#include "imageclipplugin.hpp"
#include "mappingclipplugin.hpp"
#include "triggerclipplugin.hpp"
#include "videoclipplugin.hpp"

    BasePluginLoader::BasePluginLoader(QObject *parent) : QObject(parent) {}


    void BasePluginLoader::registerPlugin(ClipPlugInterface* plugin) {
        if (!plugin) {
            return;
        }
        // 工厂类同时继承 QObject，挂到 loader 下随加载器一起析构
        QObject* pluginObj = dynamic_cast<QObject*>(plugin);
        if (!pluginObj) {
            delete plugin;
            return;
        }
        pluginObj->setParent(this);
        const QString type = plugin->clipType();
        m_pluginsByType[type] = plugin;
        m_loadedPlugins.append(pluginObj);
        emit pluginLoaded(pluginObj);
    }

    void BasePluginLoader::loadPlugins() {
        // TimeLineNode 私有副本：clip 全部静态注册，绝不加载宿主 plugins/TimeLine 目录
        //（那里的 .plugin 继承 QtTimeLine.dll 的类，与静态副本 ABI 不兼容）。
        m_pluginsByType.clear();
        m_loadedPlugins.clear();
        registerPlugin(new ArtnetClipPlugin());
        registerPlugin(new CurveClipPlugin());
        registerPlugin(new ImageClipPlugin());
        registerPlugin(new MappingClipPlugin());
        registerPlugin(new TriggerClipPlugin());
        registerPlugin(new VideoClipPlugin());
        emit pluginsLoaded();
    }

    QList<QObject*> BasePluginLoader::getLoadedPlugins() const {
        return m_loadedPlugins;
    }

    QStringList BasePluginLoader::getAvailableTypes() const {
        return m_pluginsByType.keys();
    }

    ClipPlugInterface* BasePluginLoader::getPluginByType(const QString& type) const {
        return m_pluginsByType.value(type, nullptr);
    }

    AbstractClipModel* BasePluginLoader::createModelForType(const QString& type, int start) const {
        ClipPlugInterface* plugin = getPluginByType(type);
        if (plugin) {
            return plugin->createModel(start);
        }
        return nullptr;
    }

    // AbstractClipDelegate* BasePluginLoader::createDelegateForType(const QString& type) const {
    //     ClipPlugInterface* plugin = getPluginByType(type);
    //     if (plugin) {
    //         return plugin->createDelegate();
    //     }
    //     return nullptr;
    // }



// ---------- BaseTimeLineModel.cpp ----------
#include <QTimer>


BaseTimeLineModel::BaseTimeLineModel(QObject* parent) : QAbstractItemModel(parent), m_pluginLoader(nullptr)
{
      // 创建并初始化插件加载器
    m_pluginLoader = new BasePluginLoader(this);
    m_pluginLoader->loadPlugins();
}
// 获取播放头位置
qint64 BaseTimeLineModel::getPlayheadPos() const{
    return m_currentFrame;
}
// 设置播放头位置
void BaseTimeLineModel::onSetPlayheadPos(int newPlayheadPos)
{
    m_currentFrame = newPlayheadPos;
    emit S_playheadMoved(newPlayheadPos);

}

QModelIndex BaseTimeLineModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    if (!parent.isValid()) {
        // Track level
        if (row < m_tracks.size()) {
            return createIndex(row, column, m_tracks[row]);
        }
    } else {
        // Clip level
        TrackData* track = static_cast<TrackData*>(parent.internalPointer());
        if (track && row < track->clips.size()) {
            return createIndex(row, column, track->clips[row]);
        }
    }
    return QModelIndex();
}

QModelIndex BaseTimeLineModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};

    void* ptr = child.internalPointer();
    if (!ptr || isTrackPointer(ptr))
        return {};

    const auto it = m_clipLocation.find(static_cast<AbstractClipModel*>(ptr));
    if (it == m_clipLocation.end())
        return {};

    const int trackRow = it->second.trackRow;
    if (trackRow < 0 || trackRow >= m_tracks.size())
        return {};
    return createIndex(trackRow, 0, m_tracks[trackRow]);
}

int BaseTimeLineModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_tracks.size();

    void* ptr = parent.internalPointer();
    if (!isTrackPointer(ptr))
        return 0;
    return static_cast<TrackData*>(ptr)->clips.size();
}

int BaseTimeLineModel::columnCount(const QModelIndex &parent) const
{
    // 如果父索引无效，返回1
    Q_UNUSED(parent);
    return 1; // Only one column for both tracks and clips
}

QVariant BaseTimeLineModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        switch (role) {
            case Qt::ToolTipRole:
                return QVariant::fromValue(QString("Length:%1").arg(m_lengthFrame));
            case TimelineLengthRole:
                return QVariant::fromValue(m_lengthFrame);
            case TimeCodeTypeRole:
                return QVariant::fromValue(getTimeCodeType());
            case TimelineModelAliasRole:
                return QVariant::fromValue(m_modelAlias);
            default:
                return QVariant();
        }
    }

    void* ptr = index.internalPointer();
    if (!ptr)
        return {};

    if (!isTrackPointer(ptr)) {
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(ptr);
        if (!clip)
            return {};
        switch (role) {
            case Qt::ToolTipRole:
                return QVariant::fromValue(QString(clip->type()+ ":%1 to %2").arg(clip->start()).arg(clip->end()));
            //获取剪辑模型
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(clip);
            //获取剪辑开始时间
            case TimelineRoles::ClipInRole:
                return QVariant::fromValue(clip->start());
            //获取剪辑结束时间
            case TimelineRoles::ClipOutRole:
                return QVariant::fromValue(clip->end());
            //获取剪辑长度
            case TimelineRoles::ClipLengthRole:
                return QVariant::fromValue(clip->length());
            //获取剪辑类型
            case TimelineRoles::ClipTypeRole:
                return QVariant::fromValue(clip->type());
            //获取剪辑是否显示小部件
            case TimelineRoles::ClipShowWidgetRole:
                return QVariant::fromValue(clip->isEmbedWidget());
            //获取剪辑是否可调整大小
            case TimelineRoles::ClipResizableRole:
                return QVariant::fromValue(clip->isResizable());
            //获取剪辑是否显示边框
            case TimelineRoles::ClipShowBorderRole:
                return QVariant::fromValue(clip->isShowBorder());
            //获取剪辑ID
            case TimelineRoles::ClipIdRole:
                return QVariant::fromValue(clip->id());
            case TimelineRoles::TimeCodeTypeRole:
                return QVariant::fromValue(clip->getTimeCodeType());
            case TimelineRoles::ClipColorRole:
                return clip->color();
            case TimelineRoles::ClipNoteRole:
                return clip->noteName();
            default:
                return clip->data(role);
        }
    } else {
        TrackData* track = static_cast<TrackData*>(ptr);
            switch (role){
            //获取轨道工具提示
            case Qt::ToolTipRole:
                return QVariant::fromValue(track->name);
            //获取轨道类型
            case TrackTypeRole:
                return QVariant::fromValue(TrackTypeString(track->type));
            //获取轨道名称
            case TrackNameRole:
                return QVariant::fromValue(track->name);
            //获取轨道模型
            case TrackModelRole:
                return QVariant::fromValue(track);
            //获取轨道剪辑数量
            case TrackClipsCountRole:
                return QVariant::fromValue(track->clips.size());
            //获取轨道剪辑列表
            case TrackClipsRole:
                return QVariant::fromValue(track->clips);
            default:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant BaseTimeLineModel::clipData(ClipId clipID, TimelineRoles role) const
{
    QVariant result;
    // 遍历所有轨道查找指定的片段
    for (const auto& track : m_tracks) {
        for (const auto& clip : track->clips) {
            if (clip->id() == clipID) {
                switch (role) {
                    case ClipIdRole:
                        result=clip->id();
                        break;
                    case ClipInRole:
                        result=clip->start();
                        break;
                    case ClipOutRole:
                        result=clip->end();
                        break;
                    case ClipLengthRole:
                        result=clip->end() - clip->start();
                        break;
                    case ClipTypeRole:
                        result=clip->type();
                        break;
                    case ClipShowWidgetRole:
                        result=clip->isEmbedWidget();
                        break;
                    // case ClipResizableRole:
                    //     return QVariant::fromValue(clip->isResizeable());
                    case ClipShowBorderRole:
                        result=clip->isShowBorder();
                        break;
                    case ClipModelRole:
                        result= QVariant::fromValue(clip);
                        break;
                    case ClipColorRole:
                        result = clip->color();
                        break;
                    case ClipNoteRole:
                        result = clip->noteName();
                        break;
                    case ClipOscWidgetsRole:
                        result=QVariant::fromValue(clip->getExternalControlAddressMapping());
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return result;
}

bool BaseTimeLineModel::setClipData(ClipId clipID, TimelineRoles role, QVariant value)
{
    // 遍历所有轨道查找指定的片段
    for (auto& track : m_tracks) {
        for (auto& clip : track->clips) {
            if (clip->id() == clipID) {
                bool success = false;
                const QModelIndex clipIndex = indexForClip(clip);
                switch (role) {
                    case ClipInRole:
                        clip->setStart(value.toInt());
                        return true;
                    case ClipOutRole:
                        clip->setEnd(value.toInt());
                        return true;
                    case ClipShowWidgetRole:
                        clip->setEmbedWidget(value.toBool());
                        success = true;
                        break;
                    case ClipShowBorderRole:
                        clip->setShowBorder(value.toBool());
                        success = true;
                        break;
                    case ClipColorRole: {
                        QColor color = value.value<QColor>();
                        if (!color.isValid())
                            color = QColor(value.toString());
                        clip->setColor(color);
                        return true;
                    }
                    case ClipNoteRole:
                        clip->setNoteName(value.toString());
                        return true;
                    default:
                        break;
                }
                if (success && clipIndex.isValid()) {
                    emit dataChanged(clipIndex, clipIndex, {role});
                    return true;
                }
            }
        }
    }
    return false;
}

void BaseTimeLineModel::onStartPlay(){
   qDebug()<<"onStartPlay";
}

void BaseTimeLineModel::onPausePlay(){
    qDebug()<<"onPausePlay";
}

void BaseTimeLineModel::onStopPlay(){
   qDebug()<<"onStopPlay";
}

// 设置数据
bool BaseTimeLineModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //如果索引是时间轴本身
    if (!index.isValid()) {
        if (role == TimelineRoles::TimelineModelAliasRole) {
            setModelAlias(value.toString());
            return true;
        }
        return false;
    }
    if (index.parent().isValid()){
        //如果索引是剪辑
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());

        if (!clip)
            return false;

        switch (role) {
            // 设置剪辑开始时间
            case TimelineRoles::ClipInRole: {
                int newStart = value.toInt();
                clip->setStart(newStart);
                return true;
            }
            case TimelineRoles::ClipPosRole:{
                int newPos=value.toInt();
                int clipLength = clip->length();
                clip->setStart(newPos);
                clip->setEnd(newPos+clipLength);
                return true;
            }
            case TimelineRoles::ClipIdRole:{
                clip->setId(value.toInt());
                emit dataChanged(index, index, {TimelineRoles::ClipIdRole});
                return true;
            }
            // 设置剪辑是否显示小部件（AbstractClipModel::setEmbedWidget 不发信号，需要 Model 主动通知）
            case TimelineRoles::ClipShowWidgetRole: {
                const bool newShowWidget = value.toBool();
                clip->setEmbedWidget(newShowWidget);
                emit dataChanged(index, index, {TimelineRoles::ClipShowWidgetRole});
                return true;
            }
            // 设置剪辑是否可调整大小（AbstractClipModel::setResizable 不发信号，需要 Model 主动通知）
            case TimelineRoles::ClipResizableRole: {
                const bool newResizable = value.toBool();
                clip->setResizable(newResizable);
                emit dataChanged(index, index, {TimelineRoles::ClipResizableRole});
                return true;
            }
            // 设置剪辑结束时间（clip->setEnd 会触发 lengthChanged，Model 已监听并转为 dataChanged）
            case TimelineRoles::ClipOutRole: {
                const int newEnd = value.toInt();
                clip->setEnd(newEnd);
                return true;
            }
            // 设置剪辑长度（clip->setEnd 会触发 lengthChanged，Model 已监听并转为 dataChanged）
            case TimelineRoles::ClipLengthRole: {
                const int newLength = value.toInt();
                clip->setEnd(clip->start()+newLength);
                return true;
            }
            // 设置剪辑是否显示边框（AbstractClipModel::setShowBorder 不发信号，需要 Model 主动通知）
            case TimelineRoles::ClipShowBorderRole: {
                const bool newShowBorder = value.toBool();
                clip->setShowBorder(newShowBorder);
                emit dataChanged(index, index, {TimelineRoles::ClipShowBorderRole});
                return true;
            }
            // 设置时间码类型（使用 TimeCodeTypeRole 表达，避免误用 ClipTypeRole）
            case TimelineRoles::TimeCodeTypeRole: {
                const TimeCodeType type = static_cast<TimeCodeType>(value.toInt());
                clip->setTimeCodeType(type);
                emit dataChanged(index, index, {TimelineRoles::TimeCodeTypeRole});
                return true;
            }
            case TimelineRoles::ClipColorRole: {
                QColor color = value.value<QColor>();
                if (!color.isValid())
                    color = QColor(value.toString());
                clip->setColor(color);
                return true;
            }
            case TimelineRoles::ClipNoteRole:
                clip->setNoteName(value.toString());
                return true;
            default:
                return false;
        }

    }else{
        TrackData* track = static_cast<TrackData*>(index.internalPointer());
        if (!track)
            return false;
        switch (role){
            //设置轨道名称
            case TimelineRoles::TrackNameRole: {
                /**
                 * @brief 更新轨道名称
                 * @details
                 * TrackListView 使用 openPersistentEditor 创建持久编辑器。
                 * 若这里不发 dataChanged，编辑器可能一直显示创建时的旧文本，
                 * 导致“重新导入后顺序对但 TrackName 显示错误”。
                 */
                track->name = value.toString();
                emit dataChanged(index, index, {role});
                return true;
            }
            default:
                return false;
        }

    }
}

// 获取支持的拖放操作
Qt::DropActions BaseTimeLineModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

// 获取项目标志
Qt::ItemFlags BaseTimeLineModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.isValid() && !index.parent().isValid())
        flags |= Qt::ItemIsEditable;
    return flags; 
}


void BaseTimeLineModel::clear(){
    while(rowCount()>0){
        onDeleteTrack(0);
    }
    
}

// 保存模型
/**
 * @brief 保存时间线模型的状态到 JSON 对象
 * @return 包含模型状态的 JSON 对象
 */
QJsonObject BaseTimeLineModel::save() const {
    QJsonObject modelJson;
    QJsonArray trackArray;
    modelJson["displayFormat"] = static_cast<int>(m_timeDisplayFormat);
    
    for (const auto& track : m_tracks) {
        QJsonObject trackJson = track->save();
        QJsonArray clipArray;
        // 修正: 使用 const_reference 遍历避免拷贝，直接访问智能指针
        for (const AbstractClipModel* clip : track->clips) {
            clipArray.append(clip->save());
        }
        trackJson["clips"] = clipArray;
        trackArray.append(trackJson);
    }
    
    modelJson["tracks"] = trackArray;
    return modelJson;
}

void BaseTimeLineModel::load(const QJsonObject &modelJson) {
    /**
     * @brief 从 JSON 加载时间线模型
     * @details
     * 重新导入后“顺序正确但 TrackName 显示错误”的根因之一是：导入过程触发了大量插入/删除信号，
     * 视图在中间态创建了 persistent editor，随后 trackName 再被 setData 修改，编辑器未及时同步。
     *
     * 这里改为标准的 beginResetModel/endResetModel：
     * - 导入期间视图不处理中间态，避免错位/错名。
     * - 导入完成后一次性刷新视图。
     */

    beginResetModel();

    // 1) 释放旧数据（不走 onDeleteTrack，避免导入过程中发射大量自定义信号）
    for (TrackData* track : m_tracks) {
        if (!track) continue;
        for (AbstractClipModel* clip : track->clips) {
            delete clip;
        }
        track->clips.clear();
        delete track;
    }
    m_tracks.clear();
    m_clipLocation.clear();
    m_trackSet.clear();

    // 2) 重置导入相关状态
    m_clipNextId = 0;
    m_timeDisplayFormat = static_cast<TimedisplayFormat>(modelJson["displayFormat"].toInt());

    // 3) 构建新数据
    if (!getPluginLoader()) {
        endResetModel();
        return;
    }

    const QJsonArray trackArray = modelJson["tracks"].toArray();
    for (const QJsonValue &trackValue : trackArray) {
        const QJsonObject trackJson = trackValue.toObject();
        const QString type = trackJson["type"].toString();
        const QString name = trackJson["trackName"].toString();

        auto* track = new TrackData();
        track->type = type;
        track->name = name;

        const QJsonArray clipArray = trackJson["clips"].toArray();
        for (const QJsonValue &clipValue : clipArray) {
            const QJsonObject clipJson = clipValue.toObject();

            AbstractClipModel* clip = getPluginLoader()->createModelForType(
                clipJson["type"].toString(),
                clipJson["start"].toInt()
            );
            if (!clip) {
                continue;
            }

            clip->setTimelineModel(this);
            clip->setTimeCodeType(getTimeCodeType());
            clip->load(clipJson);

            // 更新下一个可用 ID（以 JSON 中的 Id 为准）
            m_clipNextId = qMax(static_cast<qint64>(m_clipNextId), static_cast<qint64>(clipJson["Id"].toInt()) + 1);

            watchClip(clip);
            track->clips.push_back(clip);
        }

        m_tracks.push_back(track);
    }

    rebuildIndex();
    endResetModel();

    // 4) 更新时间线长度（会在变化时发出 S_LengthChanged）
    onUpdateTimeLineLength();
}
// 计算时间线长度
qint64 BaseTimeLineModel::onUpdateTimeLineLength()
{
    // qDebug()<<"onTimelineLengthChanged";
    int max = 0;
    for(const auto& track : m_tracks){
        for(const auto& clip : track->clips){
            max = qMax(max, clip->end());
        }
    }
    if(max!=m_lengthFrame){
        m_lengthFrame = max;
        emit S_LengthChanged(m_lengthFrame);

    }
    return m_lengthFrame;
}

void BaseTimeLineModel::onDeleteTrack(int trackIndex) {
    /**
     * @brief 删除轨道
     * @details
     * 旧实现的问题：
     * - 通过 createIndex(0,0,clip) 传给 onDeleteClip，parent 无效，导致 onDeleteClip 直接 return，片段不会被释放。
     * - 结果是删除轨道时 clip 内存泄漏，并且视图/选择状态也可能出现悬挂指针。
     *
     * 新实现策略：
     * - 删除轨道行时（beginRemoveRows/endRemoveRows）由父行移除负责通知视图；
     * - 在移除轨道前，直接释放该轨道拥有的所有 clip 对象，确保内存与指针安全。
     */

    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        qDebug() << "Invalid track index";
        return;
    }

    beginRemoveRows(QModelIndex(), trackIndex, trackIndex);

    TrackData* track = m_tracks[trackIndex];
    if (track) {
        // 释放该轨道上的所有片段
        for (AbstractClipModel* clip : track->clips) {
            delete clip;
        }
        track->clips.clear();
    }

    // 从容器中移除轨道并释放轨道对象
    m_tracks.removeAt(trackIndex);
    delete track;
    rebuildIndex();

    endRemoveRows();
}  
// 通过类型名创建轨道
void BaseTimeLineModel::onAddTrack(const QString& type) {
    if (!m_pluginLoader) return;
        
    beginInsertRows(QModelIndex(), m_tracks.size(), m_tracks.size());
    auto track = new TrackData();
    track->type = type;
    track->name = type+" " + QString::number(m_tracks.size());
    m_tracks.push_back(std::move(track));
    rebuildIndex();
    endInsertRows();
}

void BaseTimeLineModel::onMoveTrack(int sourceRow, int targetRow) {
    /**
     * @brief 移动轨道
     * @details
     * 使用 Qt 标准的 beginMoveRows/endMoveRows 通知视图更新，
     * 避免仅依赖自定义信号导致视图状态（选择、持久编辑器、滚动位置）不同步。
     */
    if (sourceRow < 0 || sourceRow >= m_tracks.size()) {
        return;
    }
    if (targetRow < 0 || targetRow >= m_tracks.size()) {
        return;
    }
    if (sourceRow == targetRow) {
        return;
    }

    // Qt 约定：当在同一 parent 内向下移动时，destinationRow 需要 +1（因为 source 行先被移除）
    const int destinationRow = (sourceRow < targetRow) ? (targetRow + 1) : targetRow;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow, QModelIndex(), destinationRow);
    m_tracks.move(sourceRow, targetRow);
    rebuildIndex();
    endMoveRows();


}

void BaseTimeLineModel::onAddClip(int trackIndex, int startFrame) {
    /**
     * @brief 在指定轨道添加片段
     * @details
     * 本次重构的核心目标是“让 Qt Model/View 的绑定回到标准路径”：
     * - 插入子项必须使用 beginInsertRows/endInsertRows 通知视图，而不是仅依赖自定义信号全局刷新。
     * - index/createIndex 的 row 必须是“片段行号”，旧代码用 trackIndex 作为 row 创建 clip 索引，会造成索引语义错误。
     */

    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        qDebug() << "Invalid track index:" << trackIndex;
        return;
    }
    if (!getPluginLoader()) {
        qDebug() << "Plugin loader is not initialized";
        return;
    }

    TrackData* track = m_tracks[trackIndex];
    if (!track) {
        qDebug() << "Track pointer is null";
        return;
    }

    AbstractClipModel* newClip = getPluginLoader()->createModelForType(track->type, startFrame);
    if (!newClip) {
        qDebug() << "Failed to create clip model";
        return;
    }

    newClip->setTimelineModel(this);
    newClip->setTimeCodeType(getTimeCodeType());

    watchClip(newClip);

    // 插入到 track->clips 的尾部
    const int insertRow = track->clips.size();
    const QModelIndex trackModelIndex = index(trackIndex, 0, QModelIndex());

    beginInsertRows(trackModelIndex, insertRow, insertRow);
    track->clips.push_back(newClip);
    rebuildIndex();
    endInsertRows();

    // 为新片段设置唯一 ID（使用标准 index(row,col,parent) 获取正确索引）
    const QModelIndex clipIndex = index(insertRow, 0, trackModelIndex);
    setData(clipIndex, QVariant::fromValue(m_clipNextId++), TimelineRoles::ClipIdRole);
    applyLengthChange(0, newClip->end());
}


void BaseTimeLineModel::onDeleteClip(QModelIndex clipIndex){
    if (!clipIndex.isValid() || !clipIndex.parent().isValid())
        return;

    // 获取轨道索引和轨道
    QModelIndex trackIndex = clipIndex.parent();
    TrackData* track = m_tracks[trackIndex.row()];
    if (!track)
        return;
        
    // 开始删除行
    beginRemoveRows(trackIndex, clipIndex.row(), clipIndex.row());

    // 获取要删除的片段
    if(clipIndex.row() < track->clips.size()) {
        AbstractClipModel* clip = track->clips[clipIndex.row()];
        // 从轨道中移除片段
        track->clips.erase(track->clips.begin() + clipIndex.row());
        m_clipLocation.erase(clip);
        delete clip;

    }

    rebuildIndex();
    endRemoveRows();
    //更新时间线长度
    onUpdateTimeLineLength();
}

/**
 * @brief 通过 clip 指针反查其在模型中的索引
 * @param clip 片段指针
 * @return clip 对应的 QModelIndex；若未找到返回无效索引
 */
QModelIndex BaseTimeLineModel::indexForClip(AbstractClipModel* clip) const
{
    if (!clip)
        return {};

    const auto it = m_clipLocation.find(clip);
    if (it == m_clipLocation.end())
        return {};

    const int trackRow = it->second.trackRow;
    const int clipRow = it->second.clipRow;
    if (trackRow < 0 || trackRow >= m_tracks.size())
        return {};

    const QModelIndex trackIndex = index(trackRow, 0, QModelIndex());
    return index(clipRow, 0, trackIndex);
}

/**
 * @brief 查找片段所在的轨道
 * @param clip 要查找的片段指针
 * @return 返回包含该片段的轨道的智能指针，如果未找到则返回nullptr
 */
void BaseTimeLineModel::rebuildIndex()
{
    m_clipLocation.clear();
    m_trackSet.clear();
    for (int trackRow = 0; trackRow < m_tracks.size(); ++trackRow) {
        TrackData* track = m_tracks[trackRow];
        if (!track)
            continue;
        m_trackSet.insert(track);
        for (int clipRow = 0; clipRow < track->clips.size(); ++clipRow) {
            AbstractClipModel* clip = track->clips[clipRow];
            if (!clip)
                continue;
            m_clipLocation[clip] = ClipLocation{trackRow, clipRow, clip->end()};
        }
        rebuildTrackClipOrder(track);
    }
}

void BaseTimeLineModel::rebuildTrackClipOrder(TrackData* track)
{
    if (!track)
        return;
    track->clipsByStart.clear();
    track->maxClipSpan = 0;
    for (int row = 0; row < track->clips.size(); ++row) {
        AbstractClipModel* clip = track->clips[row];
        if (!clip)
            continue;
        track->clipsByStart.push_back(row);
        track->maxClipSpan = qMax(track->maxClipSpan, clip->end() - clip->start());
    }
    std::sort(track->clipsByStart.begin(), track->clipsByStart.end(), [track](int a, int b) {
        const int sa = track->clips[a] ? track->clips[a]->start() : 0;
        const int sb = track->clips[b] ? track->clips[b]->start() : 0;
        if (sa != sb)
            return sa < sb;
        return a < b;
    });
}

void BaseTimeLineModel::beginInteractiveGeometryChange(AbstractClipModel* clip)
{
    if (!clip)
        return;
    if (!m_deferGeometryNotify)
        m_interactiveOriginalEnd = clip->end();
    m_interactiveClip = clip;
    m_deferGeometryNotify = true;
}

void BaseTimeLineModel::endInteractiveGeometryChange()
{
    AbstractClipModel* clip = m_interactiveClip;
    const int originalEnd = m_interactiveOriginalEnd;
    m_interactiveClip = nullptr;
    m_deferGeometryNotify = false;
    if (!clip)
        return;

    const auto located = m_clipLocation.find(clip);
    if (located != m_clipLocation.end())
        located->second.end = clip->end();
    noteClipGeometryChanged(clip);
    applyLengthChange(originalEnd, clip->end());

    const QModelIndex idx = indexForClip(clip);
    if (idx.isValid()) {
        emit dataChanged(idx, idx, {
            TimelineRoles::ClipInRole,
            TimelineRoles::ClipOutRole,
            TimelineRoles::ClipPosRole,
            TimelineRoles::ClipLengthRole
        });
    }
}

void BaseTimeLineModel::syncInteractiveClipOrder()
{
    if (m_interactiveClip)
        noteClipGeometryChanged(m_interactiveClip);
}

void BaseTimeLineModel::watchClip(AbstractClipModel* clip)
{
    if (!clip)
        return;
    connect(clip, &AbstractClipModel::lengthChanged, this, [this, clip](int) {
        queueClipGeometryNotify(clip);
    });
    connect(clip, &AbstractClipModel::timelinePositionChanged, this, [this, clip](int) {
        queueClipGeometryNotify(clip);
    });
    connect(clip, &AbstractClipModel::appearanceChanged, this, [this, clip]() {
        if (m_clipLocation.find(clip) == m_clipLocation.end())
            return;
        const QModelIndex idx = indexForClip(clip);
        if (!idx.isValid())
            return;
        emit dataChanged(idx, idx, {TimelineRoles::ClipColorRole, TimelineRoles::ClipNoteRole});
    });
}

void BaseTimeLineModel::queueClipGeometryNotify(AbstractClipModel* clip)
{
    if (!clip || m_deferGeometryNotify)
        return;
    if (m_clipLocation.find(clip) == m_clipLocation.end())
        return;
    if (!m_pendingGeometryClips.contains(clip))
        m_pendingGeometryClips.append(clip);
    if (m_geometryNotifyQueued)
        return;
    m_geometryNotifyQueued = true;
    // setStart 和 setEnd 会连着发两个信号。等当前调用返回后合并成一次通知。
    QTimer::singleShot(0, this, [this] { flushClipGeometryNotify(); });
}

void BaseTimeLineModel::flushClipGeometryNotify()
{
    m_geometryNotifyQueued = false;
    const QList<AbstractClipModel*> clips = m_pendingGeometryClips;
    m_pendingGeometryClips.clear();
    for (AbstractClipModel* clip : clips) {
        const auto located = m_clipLocation.find(clip);
        if (located == m_clipLocation.end())
            continue;
        const int oldEnd = located->second.end;
        const int newEnd = clip->end();
        located->second.end = newEnd;
        noteClipGeometryChanged(clip);
        applyLengthChange(oldEnd, newEnd);
        const QModelIndex idx = indexForClip(clip);
        if (idx.isValid()) {
            emit dataChanged(idx, idx, {
                TimelineRoles::ClipInRole,
                TimelineRoles::ClipOutRole,
                TimelineRoles::ClipPosRole,
                TimelineRoles::ClipLengthRole
            });
        }
    }
}

void BaseTimeLineModel::applyLengthChange(int oldEnd, int newEnd)
{
    if (newEnd > m_lengthFrame) {
        m_lengthFrame = newEnd;
        emit S_LengthChanged(m_lengthFrame);
        return;
    }
    if (oldEnd >= m_lengthFrame && newEnd < m_lengthFrame)
        onUpdateTimeLineLength();
}

void BaseTimeLineModel::noteClipGeometryChanged(AbstractClipModel* clip)
{
    if (!clip)
        return;
    const auto it = m_clipLocation.find(clip);
    if (it == m_clipLocation.end())
        return;
    const int trackRow = it->second.trackRow;
    if (trackRow < 0 || trackRow >= m_tracks.size())
        return;
    rebuildTrackClipOrder(m_tracks[trackRow]);
}

bool BaseTimeLineModel::isTrackPointer(void* ptr) const
{
    return ptr && m_trackSet.find(static_cast<TrackData*>(ptr)) != m_trackSet.end();
}

TrackData* BaseTimeLineModel::findParentTrackOfClip(AbstractClipModel* clip) const {
    if (!clip)
        return nullptr;
    const auto it = m_clipLocation.find(clip);
    if (it == m_clipLocation.end())
        return nullptr;
    const int trackRow = it->second.trackRow;
    if (trackRow < 0 || trackRow >= m_tracks.size())
        return nullptr;
    return m_tracks[trackRow];
}
// 查找轨道行
int BaseTimeLineModel::findTrackRow(TrackData* track) const {
    // 错误: 直接比较智能指针是不正确的
    // 修正: 需要比较指针值
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i] == track) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void BaseTimeLineModel::setPluginLoader(BasePluginLoader* loader) {
    if (m_pluginLoader) {
        delete m_pluginLoader;
    }
    m_pluginLoader = loader;
    m_pluginLoader->loadPlugins();
}

BasePluginLoader* BaseTimeLineModel::getPluginLoader() const {
    return m_pluginLoader;
}

// 在析构函数中清理
BaseTimeLineModel::~BaseTimeLineModel()
{
    /**
     * @brief 析构函数
     * @details
     * 当前数据结构使用裸指针（TrackData* / AbstractClipModel*），因此必须显式释放。
     * 旧代码仅 m_tracks.clear() 会导致轨道与片段全部泄漏。
     */
    m_clipLocation.clear();
    m_trackSet.clear();
    for (TrackData* track : m_tracks) {
        if (!track) continue;
        for (AbstractClipModel* clip : track->clips) {
            delete clip;
        }
        track->clips.clear();
        delete track;
    }
    m_tracks.clear();

    delete m_pluginLoader;
    m_pluginLoader = nullptr;
}

int BaseTimeLineModel::getTrackCount() const {
    return m_tracks.size();
}



// ---------- BaseTimeLineView.cpp ----------
#include <QMenu>
#include <QAction>
#include <QtMath>
#include <QTimer>
#include <algorithm>

namespace {
void clipRowsInFrameRange(const TrackData* track, int frameLo, int frameHi, QList<int>& rows)
{
    rows.clear();
    if (!track)
        return;

    const int clipCount = track->clips.size();
    if (clipCount <= 0)
        return;

    auto overlaps = [&](int row) {
        AbstractClipModel* clip = track->clips[row];
        return clip && clip->end() >= frameLo && clip->start() <= frameHi;
    };

    if (track->clipsByStart.size() != clipCount) {
        for (int row = 0; row < clipCount; ++row) {
            if (overlaps(row))
                rows.push_back(row);
        }
        return;
    }

    const int minStart = frameLo - qMax(0, track->maxClipSpan);
    const QList<int>& order = track->clipsByStart;
    const auto first = std::lower_bound(order.begin(), order.end(), minStart, [track](int row, int value) {
        const AbstractClipModel* clip = track->clips[row];
        return (clip ? clip->start() : 0) < value;
    });
    const auto last = std::upper_bound(order.begin(), order.end(), frameHi, [track](int value, int row) {
        const AbstractClipModel* clip = track->clips[row];
        return value < (clip ? clip->start() : 0);
    });
    for (auto it = first; it != last; ++it) {
        if (overlaps(*it))
            rows.push_back(*it);
    }
    std::sort(rows.begin(), rows.end());
}
}
BaseTimelineView::BaseTimelineView(BaseTimeLineModel *viewModel, QWidget *parent)
        : Model(viewModel), QAbstractItemView{parent}
    {
        setObjectName("TimelineView");
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt:: ScrollBarAlwaysOff);
        setModel(Model);
        horizontalScrollBar()->setSingleStep(10);
        horizontalScrollBar()->setPageStep(100);
        verticalScrollBar()->setSingleStep(trackHeight);
        verticalScrollBar()->setPageStep(trackHeight * 5);
        viewport()->setMinimumHeight(trackHeight + rulerHeight + toolbarHeight);
        // setMinimumHeight(trackHeight + rulerHeight + toolbarHeight);

        /**
         * @brief 选择模型说明
         * @details
         * 本视图不再在构造函数里创建 selectionModel。
         * - 由 BaseTimelineWidget 统一创建共享 QItemSelectionModel，并同时注入到 TrackListView 与 TimelineView。
         * - 这样可以保证“选择状态”只有一份，避免绑定链路复杂、状态不同步。
         */
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectItems);
        setMouseTracking(true);
        // 确保有选择模型
        setEditTriggers(QAbstractItemView::AllEditTriggers);
        setAutoScroll(true);
        setAutoScrollMargin(5);
        setAcceptDrops(true);
        // setDragEnabled(true);

    // 连接循环播放信号

        installEventFilter(this);

        // ========== 新增：初始化重绘合并定时器 ==========
        m_redrawCoalesceTimer = new QTimer(this);
        m_redrawCoalesceTimer->setSingleShot(true);
        connect(m_redrawCoalesceTimer, &QTimer::timeout, this, [this]() {
            performRedraw();
        });
        // ========== 新增结束 ==========

        // 从当前属性应用颜色，支持 QSS 的 qproperty- 语法
        applyStyleFromProperties();
    }

BaseTimelineView::~BaseTimelineView()
{
    finishClipDrag();
    delete m_toolbar;
    // delete timer;

}

void BaseTimelineView::initToolBar(BaseTimelineToolbar* new_toolbar)
{
    m_toolbar=new_toolbar;
    m_toolbar->setFixedHeight(toolbarHeight-6);
    // 设置工具栏位置
    m_toolbar->move(4, 4);

}

QRect BaseTimelineView::visualRect(const QModelIndex &index) const
{
    /**
     * @brief 获取索引在 viewport 内的可视矩形
     * @details
     * 与 TrackListView 保持一致：以滚动条 value 为唯一真值来源计算偏移，
     * 避免 m_scrollOffset 缓存状态与滚动条不同步导致命中测试/绘制错位。
     */
    const QPoint offset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    return itemRect(index).translated(-offset);
}

 QModelIndex BaseTimelineView::indexAt(const QPoint &point) const
    {
        /**
         * 函数说明：根据鼠标点返回对应的模型索引
         * - 原逻辑：优先命中轨道，再命中该轨道下的剪辑矩形（visualRect）
         * - 修改点：为避免在极小缩放下片段过窄导致无法选中，
         *   在剪辑的命中测试时应用“最小命中宽度”保护：
         *   如果 clipRect.width() < minHitWidth，则围绕其中心点临时扩展命中矩形到 minHitWidth，
         *   仅用于contains判断，不影响绘制或布局的真实宽度。
         */
        if (point.y() < contentTop() || trackHeight <= 0 || !model())
            return {};

        const int contentY = point.y() + verticalScrollBar()->value() - contentTop();
        if (contentY < 0)
            return {};
        const int row = contentY / trackHeight;
        if (row < 0 || row >= model()->rowCount())
            return {};

        const QModelIndex trackIndex = model()->index(row, 0);
        const QRect trackRect = visualRect(trackIndex);
        if (!trackRect.contains(point))
            return {};

        auto* track = static_cast<TrackData*>(trackIndex.internalPointer());
        const int minHitWidth = 15;
        const int contentX = point.x() + horizontalScrollBar()->value();
        const int frameLo = pointToFrame(qMax(0, contentX - minHitWidth)) - 1;
        const int frameHi = pointToFrame(contentX) + 1;
        QList<int> clipRows;
        clipRowsInFrameRange(track, frameLo, frameHi, clipRows);

        for (int clipRow : clipRows) {
            const QModelIndex clipIndex = model()->index(clipRow, 0, trackIndex);
            QRect clipRect = visualRect(clipIndex);
            if (clipRect.width() < minHitWidth) {
                const int pad = minHitWidth - clipRect.width();
                clipRect.adjust(0, 0, pad, 0);
                clipRect = clipRect.intersected(trackRect);
            }
            if (clipRect.contains(point))
                return clipIndex;
        }
        return trackIndex;
    }

QRect BaseTimelineView::itemRect(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QRect();
    }
    // 获取轨道宽度
    int trackwidth = getTrackWdith()+ viewport()->width();
    if(index.parent()==QModelIndex())
        // 如果轨道索引
    {
        // 返回轨道矩形
        return QRect(0, (index.row() * trackHeight) + contentTop(), trackwidth, trackHeight);
    }
     // 如果剪辑索引
    else{
        // 获取剪辑
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());
        if (clip) {
            // 获取剪辑开始帧
            int startFrame = clip->start();
            // 获取剪辑结束帧
            int endFrame = clip->end()+1;
            // 获取剪辑所在轨道行
            int trackRow = index.parent().row();
            // 获取剪辑开始点x
            int clipStartX = frameToPoint(startFrame);
            // 获取剪辑开始点y
            int clipStartY = (trackRow*trackHeight + contentTop());
            // 获取剪辑宽度
            int clipWidth = frameToPoint(endFrame - startFrame);
            // 获取剪辑所在轨道行开始点
            QPoint topLeft(clipStartX,clipStartY);
            // 返回剪辑矩形
            // qDebug()<<"rect"<<QRect(topLeft,QSize(clipWidth,trackHeight));
            return QRect(topLeft,QSize(clipWidth,trackHeight));
        }

    }
    return QRect();
}

int BaseTimelineView::getTrackWdith() const
{
    int timelineLength = model()->data(QModelIndex(), TimelineRoles::TimelineLengthRole).toInt();
    if (timelineLength <= 0) {
        return viewport()->width();  // 如果没有片段，返回视口宽度
    }
    return frameToPoint(timelineLength + 1);  // 否则返回基于时间轴长度的宽度
}

// 移动选定的剪辑
void BaseTimelineView::moveSelectedClip(int dx, int dy, bool isMouse)
{
    QModelIndexList list = selectionModel()->selectedIndexes();
    if(list.isEmpty())
        return;
    int newPos;
    if(isMouse){
        newPos = dx;
    }else{
        newPos = model()->data(list[0],TimelineRoles::ClipInRole).toInt() + dx;
    }
    // 确保不会移动到负值位置
    newPos = std::max(0, newPos);
    // 更新模型数据
    // 非拖拽时 dataChanged 会局部刷新。拖拽期间模型先不发通知，由鼠标移动自己脏矩形。
    getModel()->setData(list.first(), newPos, TimelineRoles::ClipPosRole);
}

void BaseTimelineView::movePlayheadToFrame(int frame)
{
    // 直接设置时间码生成器的帧位置。S_playheadMoved 会走局部刷新。
    getModel()->onSetPlayheadPos(frame);
}

void BaseTimelineView::onUpdateViewport(){
    scheduleRedraw();
}

// 滚动视图
void BaseTimelineView::scheduleRedraw(int delayMs) {
    m_staticCacheDirty = true;
    if (!m_redrawCoalesceTimer) return;
    // 如果定时器已在等待中，无需重复启动；让多次请求自然合并
    if (!m_redrawCoalesceTimer->isActive()) {
        m_redrawCoalesceTimer->start(delayMs);
    }
}

void BaseTimelineView::performRedraw() {
    if (m_isRedrawing) return;
    m_isRedrawing = true;

    // 内容变化才更新滚动条范围。时间轴没有持久编辑器。
    updateScrollBars();
    viewport()->update();

    m_isRedrawing = false;
}

void BaseTimelineView::onPlayheadMoved(int frame)
{
    const int previous = m_paintedPlayheadFrame;
    m_paintedPlayheadFrame = frame;
    if (!staticCacheReady()) {
        viewport()->update();
        return;
    }
    if (previous >= 0 && previous != frame)
        viewport()->update(playheadDirtyRect(previous));
    viewport()->update(playheadDirtyRect(frame));
}

bool BaseTimelineView::staticCacheReady() const
{
    if (m_staticCacheDirty || m_staticCache.isNull() || !viewport())
        return false;
    const qreal dpr = viewport()->devicePixelRatio();
    const QSize px(qRound(viewport()->width() * dpr), qRound(viewport()->height() * dpr));
    return m_staticCache.size() == px;
}

QRect BaseTimelineView::playheadDirtyRect(int frame) const
{
    if (frame < 0 || !viewport())
        return {};
    const int x = frameToPoint(frame) - horizontalScrollBar()->value();
    const int pad = playheadwidth + 2;
    const int top = qMax(0, contentTop() - playheadheight);
    return QRect(x - pad, top, pad * 2 + 1, viewport()->height() - top).intersected(viewport()->rect());
}

void BaseTimelineView::ensureStaticCache()
{
    QWidget* vp = viewport();
    if (!vp || vp->width() <= 0 || vp->height() <= 0)
        return;
    if (staticCacheReady())
        return;
    if (m_interactiveDrag && Model)
        Model->syncInteractiveClipOrder();

    const qreal dpr = vp->devicePixelRatio();
    m_staticCache = QPixmap(qRound(vp->width() * dpr), qRound(vp->height() * dpr));
    m_staticCache.setDevicePixelRatio(dpr);
    m_staticCache.fill(m_timelineBgColor);

    QPainter cachePainter(&m_staticCache);
    const QRect rect(0, 0, vp->width(), vp->height());
    drawBackground(&cachePainter, rect);
    drawVerticalTimeLines(&cachePainter, rect);
    drawTracks(&cachePainter);
    drawTimeRuler(&cachePainter, rect);
    m_staticCacheDirty = false;
}

QRect BaseTimelineView::clipContentBounds(AbstractClipModel* clip, int trackRow) const
{
    if (!clip)
        return {};
    const int x = frameToPoint(clip->start());
    const int width = qMax(1, frameToPoint(clip->end() - clip->start() + 1));
    const int y = trackRow * trackHeight + contentTop();
    // 右侧多留一截，盖住画在片段矩形外面的标题。
    return QRect(x, y, width, trackHeight).adjusted(-4, -4, 44, 4);
}

void BaseTimelineView::updateClipGeometry(const QModelIndex& index)
{
    if (!index.isValid() || !index.parent().isValid() || !viewport())
        return;
    auto* clip = static_cast<AbstractClipModel*>(index.internalPointer());
    if (!clip)
        return;
    if (!staticCacheReady()) {
        scheduleRedraw();
        return;
    }

    const QRect now = clipContentBounds(clip, index.parent().row());
    const auto found = m_clipContentRects.find(clip);
    const QRect old = found == m_clipContentRects.end() ? now : found->second;
    m_clipContentRects[clip] = now;

    const QPoint scroll(horizontalScrollBar()->value(), verticalScrollBar()->value());
    QRegion dirty(old.translated(-scroll));
    dirty += now.translated(-scroll);
    dirty &= viewport()->rect();
    if (dirty.isEmpty())
        return;
    repaintStaticCache(dirty);
    viewport()->update(dirty.boundingRect());
}

void BaseTimelineView::syncScrollOffsetFromScrollBars()
{
    /**
     * @brief 从滚动条同步 m_scrollOffset
     * @details
     * 滚动条（horizontalScrollBar/verticalScrollBar）是 QAbstractItemView 的内建滚动状态。
     * 当前项目历史代码同时维护了 m_scrollOffset，因此必须在滚动发生后同步缓存，保证绘制/命中测试一致。
     */
    m_scrollOffset = QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
}

void BaseTimelineView::setScrollOffsetClamped(const QPoint& offset)
{
    /**
     * @brief 设置滚动偏移并同步滚动条（自动夹紧）
     * @details
     * 禁止直接修改 m_scrollOffset；正确做法是设置滚动条 value，让 QAbstractItemView 走标准滚动路径。
     * 这里做夹紧，避免出现负值或超过 maximum 的情况。
     */
    const int x = qBound(horizontalScrollBar()->minimum(), offset.x(), horizontalScrollBar()->maximum());
    const int y = qBound(verticalScrollBar()->minimum(),   offset.y(), verticalScrollBar()->maximum());

    horizontalScrollBar()->setValue(x);
    verticalScrollBar()->setValue(y);

    syncScrollOffsetFromScrollBars();
}

void BaseTimelineView::onScroll(int dx, int dy){
    /**
     * @brief 外部滚动同步入口（兼容旧接口）
     * @details
     * 旧实现直接调用 QAbstractItemView::scrollContentsBy 并手动维护 m_scrollOffset，容易导致状态错乱。
     * 新实现以滚动条为真值来源：将 dx/dy 转换为目标 offset，再通过 setScrollOffsetClamped 同步。
     */
    // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
    setScrollOffsetClamped(m_scrollOffset - QPoint(dx, dy));
}
void BaseTimelineView::horizontalScroll(double position)
{
    // position 是 0-1 之间的值，表示滚动条的相对位置
    int maxScroll = horizontalScrollBar()->maximum();
    int newScrollValue = static_cast<int>(maxScroll * position);

    // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
    horizontalScrollBar()->setValue(newScrollValue);
}
// 更新滚动条
void BaseTimelineView::updateScrollBars()
{
    /**
     * @brief 更新滚动条范围（与 TrackListView 使用相同几何口径）
     * @details
     * 以滚动条为唯一真值来源：
     * - 垂直内容总高度 = contentTop(标尺+工具栏) + trackCount * trackHeight
     * - 两个视图必须使用同一公式，否则会出现滚动不同步、行对不齐
     */
    if (!model())
        return;

    const int maxX = qMax(0, getTrackWdith() - viewport()->width());
    horizontalScrollBar()->setRange(0, maxX);

    const int contentTop = rulerHeight + toolbarHeight;
    const int totalHeight = model()->rowCount() * trackHeight + contentTop;
    const int maxY = qMax(0, totalHeight - viewport()->height());
    verticalScrollBar()->setRange(0, maxY);

    // range 变化可能导致 value 被 Qt 自动夹紧，这里同步一次缓存
    syncScrollOffsetFromScrollBars();
}

bool BaseTimelineView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
    // 轨道名在左侧列表里双击编辑。时间轴上选中轨道时不要弹出输入框。
    if (!index.parent().isValid())
        return false;
    return QAbstractItemView::edit(index, trigger, event);
}

void BaseTimelineView::scrollContentsBy(int dx, int dy)
{
    /**
     * @brief 滚动条变化后的视口滚动
     * @details
     * dx/dy 是内容相对视口的位移：滚动条增大时为负，已有像素向左/上移动。
     * 缓存能平移时只重画新露出的条带；缩放、改内容或跨度超过视口时整页重建。
     */
    QAbstractItemView::scrollContentsBy(dx, dy);
    syncScrollOffsetFromScrollBars();
    if (dx == 0 && dy == 0)
        return;
    if (!shiftStaticCache(dx, dy))
        m_staticCacheDirty = true;
    if (viewport())
        viewport()->update();
}

bool BaseTimelineView::shiftStaticCache(int dx, int dy)
{
    QWidget* vp = viewport();
    if (!vp || !staticCacheReady())
        return false;

    const int w = vp->width();
    const int h = vp->height();
    if (w <= 0 || h <= 0)
        return false;
    // 一次跨过整个视口时，平移省不下绘制。
    if (qAbs(dx) >= w || qAbs(dy) >= h)
        return false;

    const int header = qBound(0, contentTop(), h);
    // 刻度横线画在 contentTop() 这一像素上。裁切高度若正好是 contentTop()，
    // 这一行会落进轨道区域，上下滚动时被轨道背景盖住。
    const int rulerBottom = qMin(h, header + 1);
    const qreal dpr = m_staticCache.devicePixelRatio();
    QPixmap shifted(m_staticCache.size());
    shifted.setDevicePixelRatio(dpr);
    shifted.fill(m_timelineBgColor);
    {
        QPainter blit(&shifted);
        // 标尺固定在顶部，只跟横向滚动走。底边横线留在标尺里。
        if (rulerBottom > 0) {
            blit.setClipRect(0, 0, w, rulerBottom);
            blit.drawPixmap(dx, 0, m_staticCache);
        }
        if (rulerBottom < h) {
            blit.setClipRect(0, rulerBottom, w, h - rulerBottom);
            blit.drawPixmap(dx, dy, m_staticCache);
        }
    }
    m_staticCache = shifted;

    // 多画 1 像素，盖住条带接缝上的刻度线。
    QRegion exposed;
    if (dx > 0)
        exposed += QRect(0, 0, qMin(w, dx + 1), h);
    else if (dx < 0)
        exposed += QRect(qMax(0, w + dx - 1), 0, qMin(w, -dx + 1), h);
    if (dy != 0 && rulerBottom < h) {
        const int contentH = h - rulerBottom;
        const int band = qMin(contentH, qAbs(dy) + 1);
        if (dy > 0)
            exposed += QRect(0, rulerBottom, w, band);
        else
            exposed += QRect(0, h - band, w, band);
        // 轨道条带的重绘会铺背景。标尺整段再画一次，底边横线才不会被盖住。
        exposed += QRect(0, 0, w, rulerBottom);
    }
    repaintStaticCache(exposed);
    return true;
}

void BaseTimelineView::repaintStaticCache(const QRegion& region)
{
    if (region.isEmpty() || m_staticCache.isNull() || !viewport())
        return;
    QPainter painter(&m_staticCache);
    painter.setClipRegion(region);
    const QRect rect(0, 0, viewport()->width(), viewport()->height());
    drawBackground(&painter, rect);
    drawVerticalTimeLines(&painter, rect);
    drawTracks(&painter);
    drawTimeRuler(&painter, rect);
}

int BaseTimelineView::pointToFrame(int point) const
{
    /**
     * 将像素位置转换为帧数（使用实例级 m_timescale，避免跨窗口干扰）
     * - 当 m_timescale 越小，表示更"缩小"，则每个像素代表更多帧
     */
    return qRound(static_cast<double>(point) / m_timescale);
}

int BaseTimelineView::frameToPoint(int frame) const
{
    /**
     * 将帧数转换为像素坐标（使用实例级 m_timescale，避免跨窗口干扰）
     * - 当 m_timescale 越小，表示更"缩小"，则每帧占据更少的像素
     */
    return qRound(static_cast<double>(frame) * m_timescale);
}

void BaseTimelineView::contextMenuEvent(QContextMenuEvent* event) {
    // 构建并弹出右键菜单（统一一个 QMenu 实例）
    QModelIndex index = indexAt(event->pos());
    QMenu contextMenu(this);

    // Focus cursor：滚动视图到播放头位置（尽量居中）
    QAction* focusCursorAction = new QAction("Focus cursor", this);
    focusCursorAction->setIcon(QIcon(":/tlnode/icons/location.png"));
    connect(focusCursorAction, &QAction::triggered, [this]() {
        const int playheadX = frameToPoint(getModel()->getPlayheadPos());
        const int viewWidth = viewport()->width();
        const int maxScroll = horizontalScrollBar()->maximum();
        int targetScroll = playheadX - viewWidth / 2;
        targetScroll = qBound(0, targetScroll, maxScroll);
        // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
        horizontalScrollBar()->setValue(targetScroll);
    });
    contextMenu.addAction(focusCursorAction);

    // 如果点击在轨道（无父）上，添加“Add Item”
    if (index.isValid() && !index.parent().isValid()) {
        QAction* addClipAction = new QAction("Add Item", this);
        addClipAction->setIcon(QIcon(":/tlnode/icons/add.png"));
        connect(addClipAction, &QAction::triggered, [this, index, event]() {
            addClipAtPosition(index, event->pos());
        });
        contextMenu.addAction(addClipAction);
    }

    // 如果点击在已选中的剪辑上，添加“Delete Clip”
    if (index.isValid() && selectionModel()->isSelected(index)) {
        QAction* deleteClipAction = new QAction("Delete Clip", this);
        deleteClipAction->setIcon(QIcon(":/tlnode/icons/trash.png"));
        connect(deleteClipAction, &QAction::triggered, [this, index]() {
            getModel()->onDeleteClip(index);
            selectionModel()->clearSelection();
            viewport()->update();
        });
        contextMenu.addAction(deleteClipAction);
    }

    // 统一显示菜单
    contextMenu.exec(event->globalPos());

    // 调用基类处理（保持与原逻辑一致）
    QAbstractItemView::contextMenuEvent(event);
}
//void BaseTimelineView::keyPressEvent(QKeyEvent *event)
//{
//    if(selectionModel()->selectedIndexes().isEmpty())
//        return;
//    if (event->key() == Qt::Key_Delete) {
//        // 处理删除键
//
//        QModelIndex currentIndex = selectionModel()->currentIndex();
//        getModel()->onDeleteClip(currentIndex);
//
//        // 清除选择并发送 nullptr
//        selectionModel()->clearSelection();
//        emit currentClipChanged(nullptr);
//        viewport()->update();
//    }
//
//}

void BaseTimelineView::addClipAtPosition(const QModelIndex& index, const QPoint& pos) {
    if (!index.isValid() || index.parent().isValid()) {
        qDebug() << "Invalid index: Click was not on a valid track";
        return;
    }

    int trackIndex = index.row();
    if (trackIndex < 0 || trackIndex >= getModel()->getTrackCount()) {
        qDebug() << "Invalid track index: " << trackIndex;
        return;
    }
    // Calculate the start frame based on the mouse position
    int startFrame = pointToFrame(pos.x() + m_scrollOffset.x());

    getModel()->onAddClip(trackIndex,startFrame);

    // 更新视图（优化为合并重绘）
    scheduleRedraw();
}

void BaseTimelineView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        // 处理左键按下的情况
        QAbstractItemView::mouseReleaseEvent(event);
        return;
    }

    finishClipDrag();

    // 结束画布平移模式
    

        m_isPanning = false;
        // m_panMoved = false;
  

    mouseHeld = false;
    m_playheadSelected = false;
    m_mouseEnd = event->pos();
    updateCursorShape();
    //pressed outside of selection
    QAbstractItemView::mouseReleaseEvent(event);
}

void BaseTimelineView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    // 创建并显示属性对话框
    showClipProperty(index);
    QAbstractItemView::mouseDoubleClickEvent(event);
}

void BaseTimelineView::leaveEvent(QEvent *event)
{
    finishClipDrag();
    mouseHeld = false;
    // selectionModel()->clear();
    m_hoverIndex = QModelIndex();
    QAbstractItemView::leaveEvent(event);
}

// 设置缩放比例
void BaseTimelineView::setScale(double value)
{
    /**
     * 根据归一化比例更新本视图实例的时间刻度（像素/帧）
     * - 不再修改全局 timescale，避免不同窗口互相影响
     * - 保持播放头焦点在缩放后视觉位置不变
     */
    // 1. 先保存旧的 timescale 用于计算旧焦点像素位置
    double oldTimescale = m_timescale;

    // 2. 将输入值钳制到 [0.0, 1.0]
    double ratio = qBound(0.0, value, 1.0);

    // 3. 将 ratio 线性映射到 [m_minTimescale, m_baseTimeScale]
    //    ratio=0 -> m_timescale=m_minTimescale；ratio=1 -> m_timescale=m_baseTimeScale
    m_timescale = m_minTimescale + ratio * (static_cast<double>(m_baseTimeScale) - m_minTimescale);

    // 4. 保持播放头焦点位置不变
    int focusFrame = getModel()->getPlayheadPos();
    int oldPointFocus = qRound(static_cast<double>(focusFrame) * oldTimescale);
    int newPointFocus = qRound(static_cast<double>(focusFrame) * m_timescale);
    int diff = newPointFocus - oldPointFocus;

    // 5. 刻度已经变了，旧缓存不能按像素平移。先标脏，滚动条夹紧时不要拿旧图去补条带。
    m_staticCacheDirty = true;
    setScrollOffsetClamped(QPoint(m_scrollOffset.x() + diff, m_scrollOffset.y()));

    // 6. 更新界面
    currentScale = ratio;
    // qDebug()<<"currentScale:"<<currentScale;
    scheduleRedraw();
}

void BaseTimelineView::resizeEvent(QResizeEvent *event)
{
    scheduleRedraw();
    QAbstractItemView::resizeEvent(event);

    // 更新ZoomController
    // emit timelineInfoChanged(getTrackWdith(), viewport()->width(), m_scrollOffset.x());
}

void BaseTimelineView::showEvent(QShowEvent *event)
{
    /**
     * @brief 显示事件
     * @details
     * 兜底逻辑：
     * - 如果外部没有注入共享 selectionModel，则创建一个本地 selectionModel。
     * - 如果本视图未被 Controller 托管（独立使用），则在此处按需绑定“模型变化 -> 合并重绘”。
     */
    if (!selectionModel() && model()) {
        setSelectionModel(new QItemSelectionModel(model(), this));
    }

    // 独立使用场景：没有 Controller 时，视图需要自己监听模型变化来刷新
    if (!property("qtimeline_managed").toBool() && !property("qtimeline_model_bound").toBool() && Model) {
        connect(Model, &QAbstractItemModel::modelReset,   this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::rowsInserted, this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::rowsRemoved,  this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::rowsMoved,    this, [this]() { scheduleRedraw(); });
        connect(Model, &QAbstractItemModel::dataChanged,  this, [this]() { scheduleRedraw(); });

        connect(Model, &BaseTimeLineModel::S_playheadMoved, this, [this](int frame) { onPlayheadMoved(frame); });
        connect(Model, &BaseTimeLineModel::S_LengthChanged,       this, [this]() { scheduleRedraw(); });

        setProperty("qtimeline_model_bound", true);
    }

    updateScrollBars();
    QAbstractItemView::showEvent(event);
}

void BaseTimelineView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        QAbstractItemView::mousePressEvent(event);
        return;
    }

    m_mouseStart = event->pos();
    m_mouseEnd = event->pos();
    mouseHeld = true;

    // 清除当前选择（selectionModel 可能由外部注入；兜底情况也可能尚未创建）
    if (selectionModel()) {
        selectionModel()->clearSelection();
    }
    // // 获取播放头位置
    // int playheadPos = frameToPoint(((BaseTimeLineModel*)model())->getPlayheadPos());
    // // 获取播放头矩形
    // QRect playheadHitBox(QPoint(playheadPos-3,rulerHeight),QPoint(playheadPos+2,viewport()->height()));
    // // 获取播放头矩形2
    // QRect playheadHitBox2(QPoint(playheadPos-playheadwidth,-playheadheight + rulerHeight),QPoint(playheadPos+playheadwidth,rulerHeight));
    // // 如果点击在播放头上
    // if(playheadHitBox.contains(m_mouseStart) || playheadHitBox2.contains(m_mouseStart)) 

    //鼠标点击位置在标尺上就移动播放头
    if (m_mouseStart.y() <= rulerHeight + toolbarHeight) 
    {
        m_playheadSelected = true;
        return;
    }
    QModelIndex item = indexAt(event->pos());
    //如果点击在轨道上的的空白处
    if(item.isValid() && !item.parent().isValid()) {
        m_playheadSelected = true;
        return;
    }
    m_playheadSelected = false;

    // 如果点击到了片段
    if(item.isValid() && item.parent().isValid()) {
        // 设置当前项和选择
        selectionModel()->setCurrentIndex(item, QItemSelectionModel::ClearAndSelect);
        setCursor(Qt::ClosedHandCursor);
        m_mouseOffset.setX(frameToPoint(item.data(TimelineRoles::ClipInRole).toInt()) - m_mouseStart.x());
        viewport()->update();
        return;
    }
    if(!item.isValid() && !item.parent().isValid()) {
        // 点击在空白处，不在轨道、片段、播放头上
        m_isPanning = true;
        m_panStart = event->pos();
        // 开始平移前同步一次滚动偏移缓存，避免历史状态不一致
        syncScrollOffsetFromScrollBars();
        m_panInitialOffset = m_scrollOffset;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    
}

/**
 * 鼠标移动事件处理函数
 * 处理鼠标移动时的各种交互行为
 * @param event 鼠标事件对象
 */
void BaseTimelineView::mouseMoveEvent(QMouseEvent *event)
{
    // 处理右键事件
    if (event->button() == Qt::RightButton) {
        QAbstractItemView::mouseMoveEvent(event);
        return;
    }
    // 处理鼠标按住拖动的情况
    if (mouseHeld) {
        handleMouseDrag(event);
        return;
    }

    // 处理鼠标悬停状态
    updateMouseHoverState(event);
    updateCursorShape();
    
    QAbstractItemView::mouseMoveEvent(event);
}

/**
 * 处理鼠标拖动操作
 * @param event 鼠标事件对象
 */
void BaseTimelineView::handleMouseDrag(QMouseEvent *event)
{
    m_mouseEnd = event->pos();
    
    // 处理播放头拖动
    if (m_playheadSelected) {
        movePlayheadToFrame(pointToFrame(std::max(0, m_mouseEnd.x() + m_scrollOffset.x())));
        return;
    }
    // 处理画布平移
    if (m_isPanning) {
        const int dx = event->pos().x() - m_panStart.x();
        const int dy = event->pos().y() - m_panStart.y();
        // 仅水平平移
        QPoint newOffset = m_panInitialOffset - QPoint(dx, 0);

        // 约束滚动范围：水平范围由滚动条/视口宽度决定
        const int minX = 0;
        const int maxX = qMax(minX, getTrackWdith() - viewport()->width());
        newOffset.setX(qBound(minX, newOffset.x(), maxX));

        // 通过滚动条驱动平移，避免直接写 m_scrollOffset 导致状态不同步
        if (newOffset != m_scrollOffset) {
            // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
            setScrollOffsetClamped(newOffset);
        }
        event->accept();
        return;
    }
    if (!selectionModel())
        return;
    const QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty())
        return;

    const QModelIndex clipIndex = selectedIndexes.first();
    auto* clip = static_cast<AbstractClipModel*>(clipIndex.internalPointer());
    if (!clip)
        return;

    const bool moving = m_mouseUnderClipEdge == hoverState::CENTER;
    const bool resizing = m_mouseUnderClipEdge == hoverState::LEFT || m_mouseUnderClipEdge == hoverState::RIGHT;
    if (!moving && !resizing)
        return;

    if (!m_interactiveDrag)
        beginClipDrag(clip, clipIndex);

    // Trigger 等固定长度片段会把标题画到几何矩形右侧外（向右约 40px）。
    // 只刷新 visualRect 时，这段旧文字清不掉，拖动就会叠成一串。
    const QRect oldRect = visualRect(clipIndex).adjusted(-4, -4, 44, 4);
    switch (m_mouseUnderClipEdge) {
        case hoverState::CENTER:
            moveSelectedClip(pointToFrame(m_mouseEnd.x() + m_mouseOffset.x()),
                             m_mouseEnd.y() + m_mouseOffset.y());
            break;
        case hoverState::LEFT:
            getModel()->setData(clipIndex, pointToFrame(m_mouseEnd.x() + m_scrollOffset.x()), TimelineRoles::ClipInRole);
            break;
        case hoverState::RIGHT:
            getModel()->setData(clipIndex, pointToFrame(m_mouseEnd.x() + m_scrollOffset.x()), TimelineRoles::ClipOutRole);
            break;
        default:
            break;
    }
    const QRect nextRect = visualRect(clipIndex).adjusted(-4, -4, 44, 4);
    viewport()->update(oldRect.united(nextRect));
}

void BaseTimelineView::beginClipDrag(AbstractClipModel* clip, const QModelIndex& index)
{
    if (!clip || !index.isValid() || !Model || m_interactiveDrag)
        return;
    m_interactiveDrag = true;
    m_dragClip = clip;
    m_dragIndex = index;
    Model->beginInteractiveGeometryChange(clip);
    // 不在 QAbstractScrollArea 上 grabMouse。按下时鼠标已经由视口捕获，
    // 再抢到外层视图会让松手事件到不了 mouseReleaseEvent，拖拽无法结束。
    m_staticCacheDirty = true;
}

void BaseTimelineView::finishClipDrag()
{
    if (!m_interactiveDrag)
        return;
    m_interactiveDrag = false;
    m_dragClip = nullptr;
    m_dragIndex = QPersistentModelIndex();
    // dataChanged 会按旧位置和新位置补进缓存，避免松手后再整页重建。
    if (Model)
        Model->endInteractiveGeometryChange();
}

/**
 * 更新鼠标悬停状态
 * @param event 鼠标事件对象
 */
void BaseTimelineView::updateMouseHoverState(QMouseEvent *event)
{
    QPoint pos = event->pos();
    m_hoverIndex = indexAt(pos);
    QRect rect = visualRect(m_hoverIndex);
    
    // 默认情况下无悬浮状态
    m_mouseUnderClipEdge = hoverState::NONE;
    
    // 检查是否悬停在片段上
    // bool isValidClip = m_hoverIndex.isValid() && m_hoverIndex.parent().isValid();
    if (m_hoverIndex.isValid() && m_hoverIndex.parent().isValid())
     {
        // 获取片段对象
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(m_hoverIndex.internalPointer());
        if (!clip) {
            return;
        }
        // 设置悬停状态为中心
        m_mouseUnderClipEdge = hoverState::CENTER;
        // 检查是否可调整大小的片段
        if (clip->isResizable()) {
            // 检查是否在左边缘
            if (abs(pos.x() - rect.left()) <= 5) {
                m_mouseUnderClipEdge = hoverState::LEFT;
            }
            // 检查是否在右边缘
            else if (abs(pos.x() - rect.right()) <= 5) {
                m_mouseUnderClipEdge = hoverState::RIGHT;
            }
        }
        return;
    }
    if (m_hoverIndex.isValid() && !m_hoverIndex.parent().isValid())
    {
        //悬浮在轨道上时
        return;
    }
    if (!m_hoverIndex.isValid() && !m_hoverIndex.parent().isValid())
    {
        return;
    }
}

/**
 * 根据鼠标悬停状态更新鼠标指针形状
 */
void BaseTimelineView::updateCursorShape()
{
    switch (m_mouseUnderClipEdge) {
        case hoverState::LEFT:
            setCursor(Qt::SizeHorCursor);
            break;
        case hoverState::RIGHT:
            setCursor(Qt::SizeHorCursor);
            break;
        case hoverState::CENTER:
            setCursor(Qt::OpenHandCursor);
            break;
        case hoverState::NONE:
        default:
            unsetCursor();
            break;
    }
}

void BaseTimelineView::dragEnterEvent(QDragEnterEvent *event)
{

        event->acceptProposedAction();
        m_isSupportMedia = true;
        // QAbstractItemView::dragEnterEvent(event);

}

void BaseTimelineView::dragMoveEvent(QDragMoveEvent *event)
{

    m_lastDragPos = event->position().toPoint();

    // 检查当前位置是否在轨道上
    QModelIndex trackIndex;


    // 排除工具栏和标尺区域
    if (m_lastDragPos.y() < rulerHeight + toolbarHeight) {
        event->ignore();
        m_isSupportMedia = false;
        return;
    }

    // 查找轨道
    bool onTrack = false;
    for (int i = 0; i < getModel()->rowCount(); i++) {
        if (visualRect(getModel()->index(i, 0)).contains(m_lastDragPos)) {
            trackIndex = getModel()->index(i, 0);
            onTrack = true;
            break;
        }
    }
    if (onTrack) {

        // 获取文件类型
        QString fileType = isMimeAcceptable(event->mimeData());
        // 获取轨道类型
        QString trackType = getModel()->data(trackIndex, TimelineRoles::TrackTypeRole).toString();

        // 判断文件类型和轨道类型是否匹配
        if (!fileType.isEmpty() && fileType == trackType) {
            // 类型匹配时接受拖动
            m_isSupportMedia = true;
            event->acceptProposedAction();

        }
        else {
            // 类型不匹配时拒绝拖动
            m_isSupportMedia = false;
            event->ignore();
            unsetCursor();
        }

    } else {
        QAbstractItemView::dragMoveEvent(event);
    }
}

void BaseTimelineView::dropEvent(QDropEvent *event)
{
    if(m_isSupportMedia){
        m_lastDragPos = event->position().toPoint();
        QModelIndex trackIndex;
        QRect rullerRect(-m_scrollOffset.x(),0,viewport()->width() + m_scrollOffset.x(),rulerHeight);
        /* If above or on the ruler drop on the first track*/
        if(m_lastDragPos.y()<0 || rullerRect.contains(m_lastDragPos)){
            if(getModel()->rowCount()>0)
                trackIndex = model()->index(0, 0);
        }else{
            /* Find track at drop point */
            for(int i = 0; i < getModel()->rowCount(); i++){
                if (visualRect(getModel()->index(i, 0)).contains(m_lastDragPos)){
                    trackIndex = getModel()->index(i,0);
                }
            }
        }
        /* If dropped out side of tracks */
        if(!trackIndex.isValid()){
            return;
        }
        int pos = pointToFrame(m_lastDragPos.x());

        getModel()->onAddClip(trackIndex.row(),pos);
        viewport()->update();

    }else{
        QAbstractItemView::dropEvent(event);
    }
}

 bool BaseTimelineView::eventFilter(QObject *watched, QEvent *event)
 {
      if (event->type() == QEvent::KeyPress) {
          QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
          QModelIndexList list = selectionModel()->selectedIndexes();
          BaseTimeLineModel* timelinemodel = (BaseTimeLineModel*)model();
          if (watched == this) {
              switch (keyEvent->key()){
                  case Qt::Key_Right :
                      moveSelectedClip(1,0,false);
                      return true;  // 统一返回 true
                  case Qt::Key_Left:
                      moveSelectedClip(-1,0,false);
                      return true;
                  case Qt::Key_Delete: {
                      QModelIndex currentIndex = selectionModel()->currentIndex();
                      getModel()->onDeleteClip(currentIndex);
                      // 清除选择并发送 nullptr
                      selectionModel()->clearSelection();
                      emit currentClipChanged(nullptr);
                      viewport()->update();
                      return true;
                  }
                  default:
                      break;
              }
          }
      }
      return QObject::eventFilter(watched, event);
 }

void BaseTimelineView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    syncScrollOffsetFromScrollBars();
    ensureStaticCache();

    QPainter painter(viewport());
    if (!m_staticCache.isNull())
        painter.drawPixmap(QPoint(0, 0), m_staticCache);
    if (m_interactiveDrag && m_dragClip && m_dragIndex.isValid()) {
        const QRect clipRect = visualRect(m_dragIndex).adjusted(0, clipoffset, 0, -clipoffset);
        const bool selected = selectionModel() && selectionModel()->isSelected(m_dragIndex);
        m_dragClip->paint(&painter, clipRect, selected);
    }
    drawPlayhead(&painter);

    if (Model)
        m_paintedPlayheadFrame = static_cast<int>(Model->getPlayheadPos());
}

void BaseTimelineView::drawBackground(QPainter* painter, const QRect& rect)
{
    // 绘制背景
    painter->fillRect(rect, m_timelineBgColor);

    // 设置分割线颜色并绘制轨道分隔线
    painter->setPen(m_timelineSeparatorColor);
    int firstRow = 0;
    int lastRow = -1;
    visibleTrackRange(firstRow, lastRow);
    if (lastRow < firstRow)
        return;
    for (int i = qMax(0, firstRow - 1); i <= lastRow; ++i) {
        const int trackSplitter = (i + 1) * trackHeight + contentTop() - m_scrollOffset.y();
        if (trackSplitter < contentTop() || trackSplitter > rect.bottom())
            continue;
        painter->drawLine(0, trackSplitter, rect.width(), trackSplitter);
    }
}

void BaseTimelineView::drawVerticalTimeLines(QPainter* painter, const QRect& rect)
{
    int lineheight = model()->rowCount() * trackHeight + contentTop();

    // 计算时间线间隔
    double frameRate = timecode_frames_per_sec(Model->getTimeCodeType());
    int frameStep = calculateFrameStep(frameRate);

    // 计算起始和结束位置
    int startMarker = static_cast<int>(pointToFrame(m_scrollOffset.x())) * m_timescale + 1;
    int endMarker = rect.width() + m_scrollOffset.x();
    startMarker = pointToFrame(startMarker);
    startMarker -= (startMarker % frameStep);
    startMarker = frameToPoint(startMarker);

    // 绘制垂直时间线
    for (int i = startMarker; i < endMarker; i += m_timescale * frameStep) {
        painter->drawLine(i - m_scrollOffset.x(),
                         std::max(rulerHeight, rect.top()),
                         i - m_scrollOffset.x(),
                         lineheight);
    }
}

int BaseTimelineView::calculateFrameStep(double frameRate) const
{
    /**
     * 函数说明：
     * - 根据当前 m_timescale（像素/帧）和字体大小，计算一个不会导致标签重叠的帧步长。
     * - 再从一组"漂亮步长"（1、2、5、10、25、50、100、帧率）中选取第一个不小于该步长的值。
     */

    /**
     * 按当前实例的 m_timescale 计算合适的刻度步长，避免跨窗口共享状态
     */
    // 估算标签的最小像素间距：与字体大小相关（也可改为用 QFontMetrics 的度量）
    const int minPixelSpacing = qMax(30, static_cast<int>(fontSize * 1.8));

    // 像素间距 => 帧间距（ceil 保证足够间距）
    int framesByPixels = qMax(1, static_cast<int>(std::ceil(static_cast<double>(minPixelSpacing) / m_timescale)));

    // "漂亮"的步长集合（可按需扩展）
    const int fpsRounded = static_cast<int>(std::round(frameRate));
    const QVector<int> niceSteps = { fpsRounded/2, fpsRounded, fpsRounded * 5,fpsRounded * 10,fpsRounded * 30, fpsRounded * 60};

    // 选择第一个 >= framesByPixels 的漂亮步长；若都小于，则用最后一个
    int step = niceSteps.last();
    for (int s : niceSteps) {
        if (s >= framesByPixels) { step = s; break; }
    }

    return step;
}

void BaseTimelineView::drawTimeRuler(QPainter* painter, const QRect& rect)
{
    /**
     * 函数说明：
     * - 绘制标尺背景，计算起止范围与帧步长。
     * - 使用当前 timescale 与帧步长确定像素间距，避免文本重叠。
     */

    // 绘制标尺背景
    painter->setPen(m_timelineRulerColor);
    painter->setBrush(QBrush(m_timelineBgColor));
    painter->drawRect(-m_scrollOffset.x(), 0,
                     rect.width() + m_scrollOffset.x(),
                     contentTop());

    // 计算时间线间隔（帧步长）
    const double frameRate = timecode_frames_per_sec(Model->getTimeCodeType());
    const int frameStep = calculateFrameStep(frameRate);

    // 计算起始/结束位置（以帧为单位，规整到步长边界）
    int startFrame = pointToFrame(m_scrollOffset.x());
    startFrame -= (startFrame % frameStep);

    const int startX = frameToPoint(startFrame);
    const int endX   = rect.width() + m_scrollOffset.x();

    // 绘制时间标记和文本（根据像素间距决定是否绘制文本）
    drawTimeMarkers(painter, startX, endX, frameStep);
}

void BaseTimelineView::drawTimeMarkers(QPainter* painter, int startMarker, int endMarker, int frameStep)
{
    /**
     * 函数说明：
     * - 按帧步长绘制刻度线。
     * - 当像素间距不足最小间距时，仅绘制刻度线不绘制文本，避免重叠。
     */

    // 计算每个刻度的像素间距
    /**
     * 使用实例级 m_timescale 计算刻度像素间距，避免跨窗口共享状态导致的干扰
     */
    const int tickPixelStep = static_cast<int>(std::round(m_timescale * frameStep));

    // 依据字体大小设定最小文本间距
    const int minLabelPixelSpacing = qMax(30, static_cast<int>(fontSize * 1.8));
    const bool canDrawText = tickPixelStep >= minLabelPixelSpacing;

    for (int x = startMarker; x < endMarker; x += tickPixelStep) {
        const int frameNumber = pointToFrame(x);

        // 画刻度线
        painter->drawLine(x - m_scrollOffset.x(),
                          contentTop() - textoffset,
                          x - m_scrollOffset.x(),
                          contentTop());

        if (canDrawText) {
           // 文本格式化为 mm:ss.ff（分钟:秒.帧）
            const double frameRate = timecode_frames_per_sec(Model->getTimeCodeType());
            const int fps = static_cast<int>(std::round(frameRate)); // 将非整数帧率四舍五入用于显示
            const qint64 totalFrames  = static_cast<qint64>(frameNumber);
            const qint64 totalSeconds = (fps > 0) ? (totalFrames / fps) : 0;
            const int ff = (fps > 0) ? static_cast<int>(totalFrames % fps) : 0;
            const int mm = static_cast<int>(totalSeconds / 60);
            const int ss = static_cast<int>(totalSeconds % 60);

            const QString text = QString("%1:%2.%3")
                                    .arg(mm, 2, 10, QChar('0'))
                                    .arg(ss, 2, 10, QChar('0'))
                                    .arg(ff, 2, 10, QChar('0'));

            QRect textRect = painter->fontMetrics().boundingRect(text);

            // 将文本居中到刻度线
            textRect.translate(-m_scrollOffset.x(), 0);
            textRect.translate(x - textRect.width() / 2, contentTop() - textoffset);

            painter->drawText(textRect, text);
        }
    }
}

void BaseTimelineView::drawPlayhead(QPainter* painter)
{
    // 绘制播放头三角形
    QPoint kite[5] = {
        QPoint(0,0),
        QPoint(-playheadwidth, -playheadCornerHeight),
        QPoint(-playheadwidth, -playheadheight),
        QPoint(playheadwidth, -playheadheight),
        QPoint(playheadwidth, -playheadCornerHeight)
    };

    int playheadPos = frameToPoint(getModel()->getPlayheadPos()) - m_scrollOffset.x();
    
    // 移动播放头到正确位置
    for (QPoint &p : kite) {
        p.setX(p.x() + playheadPos);
        p.setY(p.y() + contentTop());
    }

    // 设置画笔和画刷颜色并绘制
    painter->setPen(m_timelinePlayheadColor);
    painter->setBrush(m_timelinePlayheadColor);
    painter->drawConvexPolygon(kite, 5);
    painter->drawLine(QPoint(playheadPos, contentTop()),
                     QPoint(playheadPos, viewport()->height()));
}

/**
 * 设置时间线背景色
 * @param QColor color 背景色
 */
void BaseTimelineView::setTimelineBgColor(const QColor& color) {
    m_timelineBgColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置时间线分隔线颜色
 * @param QColor color 分隔线颜色
 */
void BaseTimelineView::setTimelineSeparatorColor(const QColor& color) {
    m_timelineSeparatorColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置标尺颜色
 * @param QColor color 标尺颜色
 */
void BaseTimelineView::setTimelineRulerColor(const QColor& color) {
    m_timelineRulerColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置播放头颜色
 * @param QColor color 播放头颜色
 */
void BaseTimelineView::setTimelinePlayheadColor(const QColor& color) {
    m_timelinePlayheadColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置片段填充颜色
 * @param QColor color 片段填充颜色
 */
void BaseTimelineView::setClipFillColor(const QColor& color) {
    m_clipFillColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置片段边框颜色
 * @param QColor color 片段边框颜色
 */
void BaseTimelineView::setClipBorderColor(const QColor& color) {
    m_clipBorderColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 设置轨道选中颜色
 * @param QColor color 轨道选中颜色
 */
void BaseTimelineView::setTrackSelectedColor(const QColor& color) {
    m_trackSelectedColor = color;
    applyStyleFromProperties();
    viewport()->update();
}
/**
 * 样式变化事件处理，确保 QSS 生效后刷新颜色
 * @param QEvent* e 事件
 */
void BaseTimelineView::changeEvent(QEvent* e) {
    QAbstractItemView::changeEvent(e);
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::PaletteChange) {
        applyStyleFromProperties();
        viewport()->update();
    }
}
/**
 * 从属性应用颜色到全局样式（兼容旧绘制逻辑和其他文件使用的全局色）
 */
void BaseTimelineView::applyStyleFromProperties() {
    // 更新旧代码使用的全局颜色（TimeLineStyle.h）
    bgColour            = m_timelineBgColor;
    seperatorColour     = m_timelineSeparatorColor;
    rulerColour         = m_timelineRulerColor;
    playheadColour      = m_timelinePlayheadColor;
    ClipColor           = m_clipFillColor;
    ClipBorderColour    = m_clipBorderColor;
    trackSelectedColour = m_trackSelectedColor;
    m_staticCacheDirty = true;
}

void BaseTimelineView::visibleTrackRange(int& firstRow, int& lastRow) const
{
    firstRow = 0;
    lastRow = -1;
    if (!model() || trackHeight <= 0)
        return;
    const int count = model()->rowCount();
    if (count <= 0)
        return;

    const int scrollY = verticalScrollBar()->value();
    const int top = contentTop();
    const int bottomContent = scrollY + viewport()->height() - top - 1;
    if (bottomContent < 0)
        return;

    lastRow = qMin(count - 1, bottomContent / trackHeight);
    firstRow = scrollY <= top ? 0 : qMin(count - 1, (scrollY - top) / trackHeight);
    if (firstRow > lastRow)
        lastRow = -1;
}

void BaseTimelineView::drawTracks(QPainter* painter)
{
    if (!model() || !painter)
        return;

    int firstRow = 0;
    int lastRow = -1;
    visibleTrackRange(firstRow, lastRow);
    if (lastRow < firstRow)
        return;

    const int scrollX = horizontalScrollBar()->value();
    const int scrollY = verticalScrollBar()->value();
    const int viewRight = scrollX + viewport()->width();
    const int padFrames = qMax(1, pointToFrame(4));
    const int frameLo = pointToFrame(qMax(0, scrollX)) - padFrames;
    const int frameHi = pointToFrame(viewRight) + padFrames;
    const bool clipped = painter->hasClipping();
    const QRect clipBounds = clipped ? painter->clipBoundingRect().toAlignedRect() : QRect();
    const int top = contentTop();
    const AbstractClipModel* selectedClip = nullptr;
    if (selectionModel()) {
        const QModelIndex current = selectionModel()->currentIndex();
        if (current.parent().isValid())
            selectedClip = static_cast<AbstractClipModel*>(current.internalPointer());
    }
    QList<int> clipRows;

    for (int i = firstRow; i <= lastRow; ++i) {
        const int trackTop = i * trackHeight + top - scrollY;
        if (clipped && (trackTop + trackHeight <= clipBounds.top() || trackTop >= clipBounds.bottom()))
            continue;

        const QModelIndex trackIndex = model()->index(i, 0);
        auto* track = static_cast<TrackData*>(trackIndex.internalPointer());
        if (!track)
            continue;
        clipRowsInFrameRange(track, frameLo, frameHi, clipRows);
        for (int clipRow : clipRows) {
            if (clipRow < 0 || clipRow >= track->clips.size())
                continue;
            AbstractClipModel* clip = track->clips.at(clipRow);
            if (!clip || (m_interactiveDrag && clip == m_dragClip))
                continue;

            const int startX = frameToPoint(clip->start());
            const int clipWidth = frameToPoint(clip->end() - clip->start() + 1);
            if (startX + clipWidth <= scrollX || startX >= viewRight)
                continue;
            const int paintWidth = qMax(clipWidth, 1);
            if (clipped && !QRect(startX - scrollX, trackTop, paintWidth, trackHeight).intersects(clipBounds))
                continue;

            // 与 clipContentBounds 同一套几何，避免再走 visualRect / parent()。
            m_clipContentRects[clip] = QRect(startX, trackTop + scrollY, paintWidth, trackHeight).adjusted(-4, -4, 44, 4);
            const QRect clipRect(startX - scrollX, trackTop + clipoffset, clipWidth, trackHeight - clipoffset * 2);
            clip->paint(painter, clipRect, clip == selectedClip);
        }
    }
}

void BaseTimelineView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QAbstractItemView::selectionChanged(selected, deselected);
    
    // 获取当前选中的索引
    QModelIndex currentIndex = selectionModel()->currentIndex();
    
    // 发送选中片段变化信号
    if (currentIndex.isValid() && currentIndex.parent().isValid()) {
        auto* clip = static_cast<AbstractClipModel*>(currentIndex.internalPointer());
        emit currentClipChanged(clip);
    } else {
        emit currentClipChanged(nullptr);
    }

    m_staticCacheDirty = true;
    viewport()->update();
}

void BaseTimelineView::wheelEvent(QWheelEvent *event){
    /**
     * 函数说明：支持 Ctrl+滚轮进行缩放；否则进行水平滚动
     * - 缩放使用乘法步进，并钳制到 [minNormalized, 1.0]，避免无限缩小
     * - 保持与 setScale 的最小阈值一致
     */
    if (event->modifiers() & Qt::ControlModifier) {
        // 缩放操作
        QPoint pos = event->position().toPoint();
        double delta = event->angleDelta().y() / 120.0; // 标准滚轮步长
        double scaleChange = (delta > 0 ? 1.1 : 0.9);    // 每次滚动的缩放倍率

        // 允许达到指定下限，从而触达 setScale 的最小可见缩放
        const double minNormalized = minTimescale; // 与 setScale 保持一致
        currentScale = qBound(minNormalized, currentScale * scaleChange, 1.0);

        setScale(currentScale);
    } else {
        /**
         * @brief 非 Ctrl 情况下滚轮用于水平滚动
         * @details
         * 以滚动条为真值来源：通过设置 horizontalScrollBar()->setValue 驱动滚动，
         * 避免只改 m_scrollOffset 造成“视觉滚动了但滚动条没动/命中测试不一致”。
         */
        const int deltaX = -event->angleDelta().y() / 2; // 减小滚动速度

        const int oldX = horizontalScrollBar()->value();
        const int newX = qBound(horizontalScrollBar()->minimum(), oldX + deltaX, horizontalScrollBar()->maximum());

        if (newX != oldX) {
            // 滚动条变化会进入 scrollContentsBy，在那里平移缓存。
            horizontalScrollBar()->setValue(newX);
            emit scrolled(newX - oldX, 0);
        }
    }
    event->accept();
}

void BaseTimelineView::onFrameChanged(qint64 frame)
{
    onPlayheadMoved(static_cast<int>(frame));
}

void BaseTimelineView::onPlaybackStateChanged(bool isPlaying)
{
    // 工具栏通过基类虚接口接收播放状态（DefaultTimeLineToolBar 已删除，
    // 节点使用自己的 TimeLineNodeToolBar，走多态调用避免空指针转换）
    if (m_toolbar) {
        m_toolbar->setPlaybackState(isPlaying);
    }
}

void BaseTimelineView::showClipProperty(const QModelIndex& index)
{
    //判断索引不为为片段时
    if(!index.isValid() || !index.parent().isValid()){
        return;
    }
    //获取片段
    AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());
    if (!clip) return;
        //获取属性窗口
        clip->showPropertyWidget();
}

BaseTimeLineModel* BaseTimelineView::getModel() const
{
    return Model;
}

/**
 * @brief 检查文件是否可接受
 * @param QString filePath 文件路径
 * @return bool 是否可接受
 */
QString BaseTimelineView::isMimeAcceptable(const QMimeData *Mime) const
{
    // 获取拖拽的文件路径

    if (Mime->hasUrls())
    {
        QString filePath = Mime->urls().first().toLocalFile();
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();
        // 根据后缀判断文件类型
        if(VideoTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Video";
        }
        else if(AudioTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Audio";
        }
        else if(ImageTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Image";
        }
        else if(ControlTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Control";
        }
    }else if (Mime->hasFormat("application/x-osc-address"))
        {
            return "Control";
        }
    return "";
}

// ---------- BaseTrackDelegate.cpp ----------
#include <QTimer>

BaseTrackDelegate::BaseTrackDelegate(QObject *parent): QAbstractItemDelegate{parent}{};

void BaseTrackDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {};

QSize BaseTrackDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    return option.rect.size();
};


QWidget *BaseTrackDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    /**
     * @brief 创建轨道行持久编辑器
     * @details
     * 重新导入/轨道移动后“显示的 TrackName 错误”的常见原因是：
     * - 旧代码在 textChanged 的 lambda 中捕获了 QModelIndex（临时索引）。
     * - 当行被 move/reset 后，该 index 可能已指向错误的行或失效，导致写回/显示错乱。
     *
     * 解决方案：
     * - 使用 QPersistentModelIndex 作为长期索引引用，能够在 beginMoveRows 等操作后自动跟随。
     */

    QWidget* editor = new QWidget(parent);

    QHBoxLayout* layout = new QHBoxLayout(editor);
    layout->setContentsMargins(0,0,0,0);

    QLineEdit* lineEdit = new QLineEdit();
    lineEdit->setText(index.data(TimelineRoles::TrackNameRole).toString());
    editor->setToolTip(index.data(TimelineRoles::TrackTypeRole).toString());
    lineEdit->setAlignment(Qt::AlignCenter);

    const QPersistentModelIndex pIndex(index);
    connect(lineEdit, &QLineEdit::textChanged, this, [pIndex](const QString& text){
        /**
         * @brief 轨道名编辑回写模型
         * @details
         * - 通过 QPersistentModelIndex 确保行移动/导入重载后仍写到正确轨道。
         * - 若索引失效（例如 model reset），则忽略本次回写。
         */
        if (!pIndex.isValid()) {
            return;
        }
        QAbstractItemModel* m = const_cast<QAbstractItemModel*>(pIndex.model());
        if (!m) {
            return;
        }
        if (m->data(pIndex, TimelineRoles::TrackNameRole).toString() == text) {
            return;
        }
        m->setData(pIndex, text, TimelineRoles::TrackNameRole);
    });

    layout->addWidget(lineEdit);
    editor->setFocusProxy(lineEdit);
    QTimer::singleShot(0, lineEdit, [lineEdit]() {
        lineEdit->setFocus();
        lineEdit->selectAll();
    });

    auto* self = const_cast<BaseTrackDelegate*>(this);
    connect(lineEdit, &QLineEdit::editingFinished, editor, [self, editor]() {
        if (editor->property("qtl_closing").toBool())
            return;
        editor->setProperty("qtl_closing", true);
        emit self->closeEditor(editor, QAbstractItemDelegate::NoHint);
    });

    // 移动轨道按钮
    QLabel* moveLabel = new QLabel();
    moveLabel->setFixedSize(20,20);
    moveLabel->setPixmap(QPixmap(":/tlnode/icons/move.png").scaled(moveLabel->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    moveLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(moveLabel);

    editor->show();
    editor->setMouseTracking(true);
    return editor;
};


void BaseTrackDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {

    // if (option.state & QStyle::State_MouseOver) {
    //     qDebug()<<"updateEditorGeometry";
    //     // 鼠标悬停时，编辑器略大一些，边距更小
    //     editor->setGeometry(option.rect.adjusted(1,1,0,0));
    // } else {
    //     qDebug()<<"updateEditorGeometr12";
        // 正常状态，保持原有边距
        editor->setGeometry(option.rect.adjusted(5,1,0,-1));
    // }
    editor->clearMask();
};


void BaseTrackDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    /**
     * @brief 将模型数据同步到编辑器
     * @details
     * TrackListView 使用 openPersistentEditor 创建持久编辑器。
     * - 当模型在 load() 里 setData(TrackNameRole) 更新轨道名时，若 delegate 不实现 setEditorData，
     *   编辑器会一直显示“创建时的旧文本”，导致导入后显示错误。
     */
    if (!editor) {
        return;
    }

    QLineEdit* lineEdit = editor->findChild<QLineEdit*>();
    if (!lineEdit) {
        return;
    }

    const QString newText = index.data(TimelineRoles::TrackNameRole).toString();
    if (lineEdit->text() == newText) {
        return;
    }

    const QSignalBlocker blocker(lineEdit);
    lineEdit->setText(newText);
};

// ---------- BaseTrackListView.cpp ----------
#include <QDrag>
#include <QMimeData>
#include <QMenu>
BaseTracklistView::BaseTracklistView(BaseTimeLineModel *viemModel, QWidget *parent): Model(viemModel), QAbstractItemView{parent}{
    /**
     * @brief 构造轨道列表视图
     * @details
     * 关键点：本视图不再“自己 new selectionModel”。
     * - 以前 TrackListView 和 TimelineView 各自持有 selectionModel，会产生两套选择状态，导致逻辑越来越乱。
     * - 重构后 selectionModel 由外部容器（BaseTimelineWidget）统一创建并注入，从而保证选择状态唯一。
     */
    setObjectName("TrackListView");
    setModel(Model);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    // selectionModel 由外部注入（BaseTimelineWidget 中的共享 selectionModel）
    // 如果有人单独使用 BaseTracklistView（不通过 BaseTimelineWidget），则会在 showEvent 中补一个兜底。

    horizontalScrollBar()->setSingleStep(10);
    horizontalScrollBar()->setPageStep(100);
    verticalScrollBar()->setSingleStep(trackHeight);
    verticalScrollBar()->setPageStep(trackHeight );
    viewport()->setMinimumHeight(trackHeight + rulerHeight+toolbarHeight);
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);

    // Create delete track action
    m_deleteTrackAction = new QAction("Delete track", this);
    m_deleteTrackAction->setIcon(QIcon(":/tlnode/icons/trash.png"));
    m_deleteTrackAction->setShortcut(QKeySequence(Qt::Key_Delete));
    QObject::connect(m_deleteTrackAction, &QAction::triggered, this, &BaseTracklistView::onDeleteTrack);
    setItemDelegate(new BaseTrackDelegate(this));
    setEditTriggers(QAbstractItemView::DoubleClicked);
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded ) ;
    setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded) ;
    installEventFilter(this);
    
    
}

void BaseTracklistView::setTrackColor(const QColor &color)
{
    m_trackColor = color;
    viewport()->update();
}

void BaseTracklistView::setTrackSelectedColor(const QColor &color)
{
    m_trackSelectedColor = color;
    viewport()->update();
}

void BaseTracklistView::setTrackHoverColor(const QColor &color)
{
    m_trackHoverColor = color;
    viewport()->update();
}

void BaseTracklistView::setTrackTitleColor(const QColor &color)
{
    m_trackTitleColor = color;
    viewport()->update();
}

void BaseTracklistView::setTimelinePlayheadColor(const QColor& color)
{
    m_timelinePlayheadColor = color;
    viewport()->update();
}

/**
 * @brief 样式变化事件：QSS qproperty- 生效/主题切换后重绘视口
 */
void BaseTracklistView::changeEvent(QEvent *event)
{
    QAbstractItemView::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) {
        viewport()->update();
    }
}

/**
 * @brief 删除轨道
 */
void BaseTracklistView::onDeleteTrack() {
    /**
     * @brief 删除当前选中的轨道
     * @details
     * persistent editor 的创建/关闭统一收敛到 onUpdateViewport/ensureVisiblePersistentEditors。
     */
    if (!Model || !selectionModel()) {
        return;
    }

    const QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }

    const int deleteRow = selected.first().row();
    if (deleteRow < 0 || deleteRow >= Model->rowCount()) {
        return;
    }

    const QModelIndex deleteIndex = Model->index(deleteRow, 0);
    if (indexWidget(deleteIndex)) {
        closePersistentEditor(deleteIndex);
    }

    // 从缓存中移除该 editor（若启用了可视范围 editor 缓存）
    for (int i = m_openPersistentEditors.size() - 1; i >= 0; --i) {
        if (!m_openPersistentEditors[i].isValid() || m_openPersistentEditors[i] == deleteIndex) {
            m_openPersistentEditors.remove(i);
        }
    }

    Model->onDeleteTrack(deleteRow);
    selectionModel()->clearSelection();
    m_hoverIndex = QModelIndex();
    onUpdateViewport();
}
/**
 * @brief 上下文菜单事件
 */
void BaseTracklistView::contextMenuEvent(QContextMenuEvent* event) {
        QMenu contextMenu(this);
        //qDebug()<<"contextMenuEvent";
        // Add track creation submenu
        QMenu* addTrackMenu = contextMenu.addMenu("Add Track ");
        addTrackMenu->setIcon(QIcon(":/tlnode/icons/add.png"));
        // Get available track types from BasePluginLoader
        QStringList availableTypes = Model->getPluginLoader()->getAvailableTypes();
        for (const QString& type : availableTypes) {
            QAction* addTrackAction = addTrackMenu->addAction("Add " + type + " Track");
            connect(addTrackAction, &QAction::triggered, [this, type]() {
                Model->onAddTrack(type);
            });
        }

        // Add delete track option if a track is selected
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            contextMenu.addAction(m_deleteTrackAction);
        }

        contextMenu.exec(event->globalPos());
    }
    
/**
 * @brief 鼠标按下事件
 */
void BaseTracklistView::mousePressEvent(QMouseEvent *event) {
    /**
     * @brief 轨道列表鼠标按下
     * @details
     * - 这里只处理“轨道行”的选择。
     * - 选择状态由共享 selectionModel 承载；若 selectionModel 尚未注入，则直接走基类逻辑，避免空指针崩溃。
     */
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
        if (selectionModel()) {
            QModelIndex item = indexAt(event->pos());
            if (item.isValid()) {
                selectionModel()->select(item, QItemSelectionModel::ClearAndSelect);
                selectionModel()->setCurrentIndex(item, QItemSelectionModel::Current);
            }
        }
        viewport()->update();
    }

    QAbstractItemView::mousePressEvent(event);
}
/**
 * @brief 获取某一点的轨道索引
 */
QModelIndex BaseTracklistView::indexAt(const QPoint &point) const {
    const int header = rulerHeight + toolbarHeight;
    if (!model() || trackHeight <= 0 || point.y() < header)
        return {};

    const int contentY = point.y() + verticalScrollBar()->value() - header;
    if (contentY < 0)
        return {};
    const int row = contentY / trackHeight;
    if (row < 0 || row >= model()->rowCount())
        return {};

    const QModelIndex index = model()->index(row, 0);
    if (visualRect(index).contains(point))
        return index;
    return {};
}

BaseTimeLineModel* BaseTracklistView::getModel()
{
    return Model;
}
/**
 * @brief 选择改变
 */
void BaseTracklistView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QAbstractItemView::selectionChanged(selected, deselected);

    viewport()->update();
}

/**
 * @brief 鼠标移动事件
 */
void BaseTracklistView::mouseMoveEvent(QMouseEvent *event) {
    // 如果不是左键拖动，只更新悬停状态
    if (!(event->buttons() & Qt::LeftButton)) {
        const QModelIndex newHover = indexAt(event->pos());
        if (newHover != m_hoverIndex) {
            m_hoverIndex = newHover;
            viewport()->update();
        }
        return;
    }

    // 检查是否达到拖动启动距离，避免轻微抖动就触发拖拽
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    // 获取拖动起始位置的轨道索引
    const QModelIndex index = indexAt(m_dragStartPosition);
    if (!index.isValid()) {
        return;
    }

    // 创建拖动对象
    auto* drag = new QDrag(this);
    auto* mimeData = new QMimeData;

    // 存储源轨道索引
    mimeData->setData("application/x-track-index", QByteArray::number(index.row()));
    drag->setMimeData(mimeData);

    // 创建预览图
    const QRect itemRect = visualRect(index);
    const int previewW = qMax(1, viewport()->width());
    const int previewH = qMax(1, itemRect.height());
    QPixmap pixmap(previewW, previewH);

    // 拖拽预览底色使用视图成员颜色（QSS 可配置），不再直读全局变量
    QColor fillColor = m_trackSelectedColor;
    fillColor.setAlphaF(0.8);
    pixmap.fill(fillColor);

    // 将热点设置为鼠标相对于轨道的位置
    const QPoint relativePos = m_dragStartPosition - itemRect.topLeft();
    drag->setPixmap(pixmap);
    drag->setHotSpot(relativePos);

    // 开始拖动操作
    drag->exec(Qt::MoveAction);

    // 防止一次按下过程中重复触发拖拽
    m_dragStartPosition = QPoint();
}

/**
 * @brief 离开事件
 */
void BaseTracklistView::leaveEvent(QEvent *event) {
    // selectionModel()->clear();
    if (m_hoverIndex.isValid()) {
        m_hoverIndex = QModelIndex();
        viewport()->update();
    }
    QAbstractItemView::leaveEvent(event);
}
/**
 * @brief 大小改变事件
 */
void BaseTracklistView::resizeEvent(QResizeEvent *event) {
    onUpdateViewport();
    updateScrollBars();
    QAbstractItemView::resizeEvent(event);
}

/**
 * @brief 显示事件
 */
void BaseTracklistView::showEvent(QShowEvent *event) {
    /**
     * @brief 显示事件
     * @details
     * 兜底逻辑：
     * - 如果外部没有注入共享 selectionModel，则创建一个本地 selectionModel。
     * - 如果本视图未被 Controller 托管（独立使用），则在此处按需绑定“模型变化 -> 刷新”。
     */
    if (!selectionModel() && model()) {
        setSelectionModel(new QItemSelectionModel(model(), this));
    }

    // 独立使用场景：没有 Controller 时，TrackList 需要自己监听模型变化来刷新
    if (!property("qtimeline_managed").toBool() && !property("qtimeline_model_bound").toBool() && Model) {
        connect(Model, &QAbstractItemModel::modelReset,   this, &BaseTracklistView::onUpdateViewport);
        connect(Model, &QAbstractItemModel::rowsInserted, this, &BaseTracklistView::onUpdateViewport);
        connect(Model, &QAbstractItemModel::rowsRemoved,  this, &BaseTracklistView::onUpdateViewport);
        connect(Model, &QAbstractItemModel::rowsMoved,    this, &BaseTracklistView::onUpdateViewport);

        connect(Model, &QAbstractItemModel::dataChanged,
                this, [this](const QModelIndex& topLeft, const QModelIndex&, const QList<int>&) {
                    // Clip 层数据变化不影响 TrackList；Track 层变化轻量重绘即可
                    if (!topLeft.parent().isValid()) {
                        viewport()->update();
                    }
                });


        connect(Model, &BaseTimeLineModel::S_playheadMoved, this, [this](int) {
            updatePlayheadHeader();
        });

        setProperty("qtimeline_model_bound", true);
    }

    updateScrollBars();
    QAbstractItemView::showEvent(event);
}

/**
 * @brief 滚动事件
 */
void BaseTracklistView::scrollContentsBy(int dx, int dy) {
    /**
     * @brief QAbstractItemView 内部滚动回调
     * @details
     * 统一以滚动条 value 为唯一真值来源：
     * - 这里不再维护 m_scrollOffset。
     * - 通过 visualRect() 读取 scrollBar value 来计算偏移。
     */
    QAbstractItemView::scrollContentsBy(dx, dy);
    updateEditorGeometries();
    onUpdateViewport();
}

/**
 * @brief 滚动事件
 */


/**
 * @brief 更新视图
 */
void BaseTracklistView::onUpdateViewport(){
    /**
     * @brief 更新视图
     * @details
     * persistent editor（轨道行内的编辑控件）由本函数统一管理：
     * - 仅在可视范围（含少量缓冲行）内按需 openPersistentEditor。
     * - 对离开可视范围的行按需 closePersistentEditor，避免滚动过程中 editor 只增不减。
     */
    updateScrollBars();
    ensureVisiblePersistentEditors();
    viewport()->update();
}

void BaseTracklistView::ensureVisiblePersistentEditors()
{
    // 轨道名平时只画文字，不再为可见行打开常驻输入框。
    // 这里只关掉以前留下的 persistent editor。
    for (int i = m_openPersistentEditors.size() - 1; i >= 0; --i) {
        const QPersistentModelIndex& p = m_openPersistentEditors[i];
        if (p.isValid() && indexWidget(p))
            closePersistentEditor(p);
    }
    m_openPersistentEditors.clear();
}

/**
 * @brief 更新滚动条
 */
void BaseTracklistView::updatePlayheadHeader()
{
    const int header = rulerHeight + toolbarHeight;
    viewport()->update(QRect(0, 0, viewport()->width(), header));
}

void BaseTracklistView::updateScrollBars(){
    if(!model())
        return;
    int max =0;
    verticalScrollBar()->setRange(0,model()->rowCount() * trackHeight + rulerHeight+toolbarHeight - viewport()->height());
}

/**
 * @brief 获取视图矩形
 */
QRect BaseTracklistView::visualRect(const QModelIndex &index) const{
    /**
     * @brief 计算索引的可视矩形
     * @details
     * 以滚动条 value 为偏移来源，避免 m_scrollOffset 与滚动条不同步。
     */
    const QPoint offset(horizontalScrollBar()->value(), verticalScrollBar()->value());
    return itemRect(index).translated(-offset);
}
/**
 * @brief 获取项目矩形
 */
QRect BaseTracklistView::itemRect(const QModelIndex &index) const{
    if(!index.isValid())
        return QRect();
    return QRect(0, (index.row() * trackHeight) + rulerHeight+toolbarHeight, viewport()->width(), trackHeight);
};
/**
 * @brief 更新编辑几何
 */
void BaseTracklistView::updateEditorGeometries() {
    QAbstractItemView::updateEditorGeometries();

    for (int i = 0; i < model()->rowCount(); ++i){
        QModelIndex trackIndex = model()->index(i,0);

        QWidget *editor = indexWidget(trackIndex);
        if(!editor){
            continue;
        }

        QRect rect  = editor->rect();
        QPoint topInView = editor->mapToParent(rect.topLeft());
        if(topInView.y()<rulerHeight+toolbarHeight){
            int offset = rulerHeight+toolbarHeight-topInView.y();
            editor->setMask(QRegion(0,offset,editor->width(),rulerHeight+toolbarHeight));
        }

    }
}
/**
 * @brief 事件过滤
 */
bool BaseTracklistView::eventFilter(QObject *watched, QEvent *event)
{

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (watched == this) {
            // 处理 view1 的 keyPressEvent
            // 注意：可以添加其他逻辑来处理特定的键盘输入
            
            if(keyEvent->key()==Qt::Key_Delete){
                onDeleteTrack();
            }
            return true;  // 表示事件已处理，不再向其他控件传递
        }
    }
    return QObject::eventFilter(watched, event);
}
/**
 * @brief 设置选择
 */
void BaseTracklistView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) {
    if (!model() || !selectionModel())
    return;

    // 将视图坐标转换为模型索引围
    QModelIndex topLeft = indexAt(rect.topLeft());
    QModelIndex bottomRight = indexAt(rect.bottomRight());

    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    // 创建选择范围
    QItemSelection selection(topLeft, bottomRight);

    // 更新选择模型
    selectionModel()->select(selection, command);
}
/**
 * @brief 绘制事件
 */
void BaseTracklistView::paintEvent(QPaintEvent *event) {
    /**
     * @brief 绘制轨道列表
     * @details
     * 这里做一个关键的“绑定简化”：
     * - 当 TimelineView 选中 clip 时，selectionModel 的 currentIndex 是“clipIndex（parent 有效）”。
     * - TrackListView 的行索引是“trackIndex（parent 无效）”，两者不是同一个 index。
     * - 旧实现只用 isSelected(trackIndex) 判断，导致“选中 clip 时左侧轨道不高亮”。
     * - 新实现：根据 currentIndex 推导出“当前轨道行号”，让视觉反馈一致，但不强行改变 selectionModel 的内容。
     */
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setBrush(QBrush(m_trackBgColor));

    const QItemSelectionModel* sel = selectionModel();
    const QModelIndex current = sel ? sel->currentIndex() : QModelIndex();
    int currentTrackRow = -1;
    if (current.isValid()) {
        currentTrackRow = current.parent().isValid() ? current.parent().row() : current.row();
    }

    const int header = rulerHeight + toolbarHeight;
    const bool drawRows = event->rect().bottom() >= header;
    if (drawRows) {
    const int scrollY = verticalScrollBar()->value();
    const int bottomContent = scrollY + viewport()->height() - header - 1;
    const int count = model() ? model()->rowCount() : 0;
    int firstRow = 0;
    int lastRow = -1;
    if (trackHeight > 0 && count > 0 && bottomContent >= 0) {
        lastRow = qMin(count - 1, bottomContent / trackHeight);
        firstRow = scrollY <= header ? 0 : qMin(count - 1, (scrollY - header) / trackHeight);
        if (firstRow > lastRow)
            lastRow = -1;
    }

    for (int i = firstRow; i <= lastRow; ++i){
        QModelIndex trackIndex = model()->index(i,0);

        const bool isCurrentTrack = (i == currentTrackRow);
        const bool isTrackSelected = sel ? sel->isSelected(trackIndex) : false;

        if (isCurrentTrack || isTrackSelected) {
            painter.setBrush(QBrush(m_trackSelectedColor));
        } else if (m_hoverIndex==trackIndex) {
            painter.setBrush(QBrush(m_trackHoverColor));
        } else {
            painter.setBrush(QBrush(m_trackColor));
        }

        const QRect rowRect = visualRect(trackIndex);
        painter.drawRect(rowRect);

        painter.save();
        painter.setPen(m_trackTitleColor);
        const QRect textRect = rowRect.adjusted(8, 0, -28, 0);
        painter.drawText(textRect, Qt::AlignCenter,
                         trackIndex.data(TimelineRoles::TrackNameRole).toString());
        const QRect gripRect(rowRect.right() - 24, rowRect.center().y() - 10, 20, 20);
        painter.drawPixmap(gripRect, QPixmap(":/tlnode/icons/move.png").scaled(
                               gripRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        painter.restore();
    }
    }

    if (event->rect().top() < header)
        drawTitle(&painter);

    painter.restore();
}

void BaseTracklistView::drawTitle(QPainter *painter)
{
    if (!painter) {
        return;
    }

    painter->save();

    painter->setBrush(QBrush(m_trackBgColor));
    QRect rect(0,  0, viewport()->width(), rulerHeight+toolbarHeight);
    painter->drawRect(rect);

    if (Model) {
        painter->setPen(m_timelinePlayheadColor);
        QFont font;
        font.setPixelSize(fontSize);
        painter->setFont(font);
        if(Model->getTimeDisplayFormat()==TimedisplayFormat::TimeCodeFormat){
            painter->drawText(rect,Qt::AlignCenter,QString("%1").arg(Model->getPlayheadPos()));
        }else{
            painter->drawText(rect,Qt::AlignCenter,QString("00:%1").arg(Model->getPlayheadPos()));
        }
    }

    painter->restore();
}

void BaseTracklistView::dragEnterEvent(QDragEnterEvent *event) {
    // 检查是否是我们的自定义MIME类型

    if (event->mimeData()->hasFormat("application/x-track-index")) {
        // 设置移动鼠标形状

        event->acceptProposedAction();

        // unsetCursor();
    } else {
        unsetCursor();
    }
}

void BaseTracklistView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-track-index")) {
        event->acceptProposedAction();
        // 获取当前拖动位置的轨道索引
        QModelIndex dropIndex = indexAt(event->position().toPoint());
        if (dropIndex.isValid()) {
            // 只高亮显示目标位置，不进行选择
            m_hoverIndex = dropIndex;
            viewport()->update();
        } else {
            // 如果不在有效轨道上，设置禁止鼠标形状
            QAbstractItemView::dragMoveEvent(event);

        }
    }
}

void BaseTracklistView::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-track-index")) {
        // 获取源轨道和目标轨道的索引
        QByteArray data = event->mimeData()->data("application/x-track-index");
        int sourceRow = data.toInt();
        int targetRow = indexAt(event->position().toPoint()).row();

        // 只在源和目标不同且都有效时进行移动
        if (sourceRow != targetRow && targetRow >= 0 && sourceRow >= 0) {
            if (Model) {
                // 移动前关闭当前已打开的 persistent editor（数量应接近可视行数）
                for (const QPersistentModelIndex& p : m_openPersistentEditors) {
                    if (p.isValid() && indexWidget(p)) {
                        closePersistentEditor(p);
                    }
                }
                m_openPersistentEditors.clear();

                Model->onMoveTrack(sourceRow, targetRow);

                // 清除选择和悬停状态（selectionModel 可能由外部注入；兜底情况下也可能为空）
                if (selectionModel()) {
                    selectionModel()->clearSelection();
                }
                m_hoverIndex = QModelIndex();
                onUpdateViewport();
            }
        }

        event->acceptProposedAction();
    }else
    {
        QAbstractItemView::dropEvent(event);
    }
}

// ---------- BaseTimeLineToolBar.cpp ----------
#include <QStyle>
#include <QApplication>
#include <QClipboard>

BaseTimelineToolbar::BaseTimelineToolbar(QWidget* parent)
    : QToolBar(parent),
    _OscMapping(std::make_shared<std::unordered_map<QString, QAction*>>())
{

}

BaseTimelineToolbar::~BaseTimelineToolbar()
{

}


bool BaseTimelineToolbar::eventFilter(QObject *watched, QEvent *event)

{
    QToolButton* btn = qobject_cast<QToolButton*>(watched);
    if (!btn)
        return false;

    QAction* action = btn->defaultAction();
    if (!action)
        return false;

    // 判断 action 是否在 _OscMapping 里
    auto it = std::find_if(_OscMapping->begin(), _OscMapping->end(),
        [action](const auto& pair) { return pair.second == action; });
    if (it == _OscMapping->end())
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton && (mouseEvent->modifiers() & Qt::ControlModifier)) {
                dragStartPosition = mouseEvent->pos();
                isDragging = true;
            }
            break;
    }
    case QEvent::MouseMove: {
            if (!isDragging) break;
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if ((mouseEvent->pos() - dragStartPosition).manhattanLength() >= QApplication::startDragDistance()) {
                startDrag(action);
                isDragging = false;
                return true;
            }
            break;
    }
    case QEvent::MouseButtonRelease: {
            isDragging = false;
            break;
    }
    default:
        break;
    }
    return false;
}

void BaseTimelineToolbar::startDrag(QAction* widget)
{
    // 找到对应的OSC地址
    QString oscAddress;
    for (const auto& pair : *_OscMapping) {
        if (pair.second == widget) {
            oscAddress = pair.first;
            break;
        }
    }

    if (oscAddress.isEmpty()) return;

    OSCMessage message;
    message.address = "/timeline/default/toolbar" + oscAddress;
    message.host = "127.0.0.1";
    message.port = 8991;
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(message.address);
    // 获取控件的值

    message.value = widget->isChecked();
    message.type = "Int";


    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << message.host << message.port << message.address << message.value<<message.type;

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-osc-address", itemData);

    QDrag* drag = new QDrag(widget);
    drag->setMimeData(mimeData);
    QPixmap pixmap(200, 30);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景：使用调色板取色（跟随 QSS/主题），不做硬编码
    QColor bgColor = palette().color(QPalette::Window);
    bgColor.setAlpha(200);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect(), 5, 5);  // 圆角矩形
    // 绘制文本
    painter.setPen(palette().color(QPalette::WindowText));
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    QRect textRect = pixmap.rect().adjusted(30, 0, -8, 0);  // 图标右侧的文本区域
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, message.address);

    // 设置拖拽预览
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));  // 热点在中心

    drag->exec(Qt::CopyAction);
}
