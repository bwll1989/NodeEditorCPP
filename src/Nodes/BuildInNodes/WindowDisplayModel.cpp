#include "WindowDisplayModel.hpp"

#include <QGuiApplication>
#include <QLabel>
#include <QScreen>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QTimer>

#include <QtCore/QCoreApplication>
#include <QtNodes/NodeDelegateModelRegistry>

using namespace Nodes;
using namespace NodeDataTypes;

// =============================================================================
// WindowDisplayModel — 独立窗口多屏图像输出
// =============================================================================
//
// 定位对齐 TouchDesigner Window COMP 的常用子集：
//   Display / Opening Size / Borders / Always on Top / Close on Escape / Open·Close
//
// 刷新策略与 ImageShowModel 同源，避免 GPU 算子同 tick 竞态误清屏。
// =============================================================================

namespace {

ImageTextureWindow::FitMode toWindowFit(WindowDisplayModel::FitMode mode)
{
    switch (mode) {
    case WindowDisplayModel::FitMode::Cover:
        return ImageTextureWindow::FitMode::Cover;
    case WindowDisplayModel::FitMode::Stretch:
        return ImageTextureWindow::FitMode::Stretch;
    case WindowDisplayModel::FitMode::Contain:
    default:
        return ImageTextureWindow::FitMode::Contain;
    }
}

} // namespace

WindowDisplayModel::WindowDisplayModel()
{
    InPortCount = 2;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("Window Display");
    WidgetEmbeddable = false;
    Resizable = false;

    buildUi();

    if (QCoreApplication* app = QCoreApplication::instance()) {
        // 主窗口关闭后由 quitOnLastWindowClosed 触发；输出窗口已是 secondary，不会挡住这次退出
        QObject::connect(app, &QCoreApplication::aboutToQuit, this, [this]() {
            destroyWindow();
        }, Qt::DirectConnection);
    }

    if (auto* gui = qobject_cast<QGuiApplication*>(QGuiApplication::instance())) {
        QObject::connect(gui, &QGuiApplication::screenAdded, this, [this](QScreen*) {
            refreshScreens();
        });
        QObject::connect(gui, &QGuiApplication::screenRemoved, this, [this](QScreen*) {
            refreshScreens();
            if (isWindowOpen()) {
                applyWindowSettings();
                openWindow();
            }
        });
    }

    connect(TimestampGenerator::getInstance(),
            &TimestampGenerator::frameCountUpdated,
            this,
            &WindowDisplayModel::onSystemFrameTick,
            Qt::QueuedConnection);
}

WindowDisplayModel::~WindowDisplayModel()
{
    disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
    QObject::disconnect(m_bufferFrameConnection);
    m_bufferFrameConnection = {};
    destroyWindow();
}

void WindowDisplayModel::buildUi()
{
    _panel = new QWidget();
    _layout = new QGridLayout(_panel);
    _layout->setContentsMargins(4, 4, 4, 4);
    _layout->setHorizontalSpacing(6);
    _layout->setVerticalSpacing(4);

    int row = 0;

    _titleEdit = new QLineEdit(_title, _panel);
    _layout->addWidget(new QLabel(QStringLiteral("标题:"), _panel), row, 0);
    _layout->addWidget(_titleEdit, row++, 1, 1, 2);

    _screenCombo = new QComboBox(_panel);
    _layout->addWidget(new QLabel(QStringLiteral("显示器:"), _panel), row, 0);
    _layout->addWidget(_screenCombo, row++, 1, 1, 2);

    _openingSizeCombo = new QComboBox(_panel);
    _openingSizeCombo->addItem(QStringLiteral("填满显示器"), static_cast<int>(OpeningSize::FillDisplay));
    _openingSizeCombo->addItem(QStringLiteral("自定义尺寸"), static_cast<int>(OpeningSize::Custom));
    _layout->addWidget(new QLabel(QStringLiteral("尺寸:"), _panel), row, 0);
    _layout->addWidget(_openingSizeCombo, row++, 1, 1, 2);

    _widthSpin = new IntDragValueWidget(_panel);
    _widthSpin->setRange(16, 16384);
    _widthSpin->setValue(_customWidth);
    _widthSpin->setSuffix(QStringLiteral(" px"));
    _heightSpin = new IntDragValueWidget(_panel);
    _heightSpin->setRange(16, 16384);
    _heightSpin->setValue(_customHeight);
    _heightSpin->setSuffix(QStringLiteral(" px"));
    _layout->addWidget(new QLabel(QStringLiteral("宽高:"), _panel), row, 0);
    _layout->addWidget(_widthSpin, row, 1);
    _layout->addWidget(_heightSpin, row++, 2);

    _positionCombo = new QComboBox(_panel);
    _positionCombo->addItem(QStringLiteral("居中"), static_cast<int>(WindowPosition::Center));
    _positionCombo->addItem(QStringLiteral("左上"), static_cast<int>(WindowPosition::TopLeft));
    _positionCombo->addItem(QStringLiteral("自定义"), static_cast<int>(WindowPosition::Custom));
    _layout->addWidget(new QLabel(QStringLiteral("位置:"), _panel), row, 0);
    _layout->addWidget(_positionCombo, row++, 1, 1, 2);

    _posXSpin = new IntDragValueWidget(_panel);
    _posXSpin->setRange(-32768, 32768);
    _posXSpin->setValue(_posX);
    _posXSpin->setSuffix(QStringLiteral(" X"));
    _posYSpin = new IntDragValueWidget(_panel);
    _posYSpin->setRange(-32768, 32768);
    _posYSpin->setValue(_posY);
    _posYSpin->setSuffix(QStringLiteral(" Y"));
    _layout->addWidget(new QLabel(QStringLiteral("坐标:"), _panel), row, 0);
    _layout->addWidget(_posXSpin, row, 1);
    _layout->addWidget(_posYSpin, row++, 2);

    _fitCombo = new QComboBox(_panel);
    _fitCombo->addItem(QStringLiteral("Contain"), static_cast<int>(FitMode::Contain));
    _fitCombo->addItem(QStringLiteral("Cover"), static_cast<int>(FitMode::Cover));
    _fitCombo->addItem(QStringLiteral("Stretch"), static_cast<int>(FitMode::Stretch));
    _layout->addWidget(new QLabel(QStringLiteral("适配:"), _panel), row, 0);
    _layout->addWidget(_fitCombo, row++, 1, 1, 2);

    _bordersCheck = new QCheckBox(QStringLiteral("边框"), _panel);
    _bordersCheck->setChecked(_borders);
    _alwaysOnTopCheck = new QCheckBox(QStringLiteral("置顶"), _panel);
    _alwaysOnTopCheck->setChecked(_alwaysOnTop);
    _layout->addWidget(_bordersCheck, row, 0);
    _layout->addWidget(_alwaysOnTopCheck, row++, 1);

    _closeOnEscCheck = new QCheckBox(QStringLiteral("Esc 关闭"), _panel);
    _closeOnEscCheck->setChecked(_closeOnEscape);
    _ignoreTaskbarCheck = new QCheckBox(QStringLiteral("忽略任务栏"), _panel);
    _ignoreTaskbarCheck->setChecked(_ignoreTaskbar);
    _layout->addWidget(_closeOnEscCheck, row, 0);
    _layout->addWidget(_ignoreTaskbarCheck, row++, 1);

    _openBtn = new QPushButton(QStringLiteral("打开窗口"), _panel);
    _layout->addWidget(_openBtn, row++, 0, 1, 3);

    _layout->addItem(new QSpacerItem(20, 8, QSizePolicy::Minimum, QSizePolicy::Expanding),
                     row, 0, 1, 3);
    _panel->setMinimumSize(260, 280);

    refreshScreens();
    syncCustomSizeEnabled();
    syncPositionEnabled();

    QObject::connect(_titleEdit, &QLineEdit::editingFinished, this, [this]() {
        _title = _titleEdit->text().trimmed();
        if (_title.isEmpty()) {
            _title = QStringLiteral("Window Display");
            _titleEdit->setText(_title);
        }
        if (_glWindow) {
            _glWindow->setTitle(_title);
        }
    });
    QObject::connect(_screenCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &WindowDisplayModel::onScreenIndexChanged);
    QObject::connect(_openingSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &WindowDisplayModel::onOpeningSizeChanged);
    QObject::connect(_positionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &WindowDisplayModel::onPositionChanged);
    QObject::connect(_widthSpin, &IntDragValueWidget::valueChanged, this, [this](int v) {
        _customWidth = v;
        if (isWindowOpen() && _openingSize == OpeningSize::Custom) {
            openWindow();
        }
    });
    QObject::connect(_heightSpin, &IntDragValueWidget::valueChanged, this, [this](int v) {
        _customHeight = v;
        if (isWindowOpen() && _openingSize == OpeningSize::Custom) {
            openWindow();
        }
    });
    QObject::connect(_posXSpin, &IntDragValueWidget::valueChanged, this, [this](int v) {
        _posX = v;
        if (isWindowOpen() && _position == WindowPosition::Custom) {
            openWindow();
        }
    });
    QObject::connect(_posYSpin, &IntDragValueWidget::valueChanged, this, [this](int v) {
        _posY = v;
        if (isWindowOpen() && _position == WindowPosition::Custom) {
            openWindow();
        }
    });
    QObject::connect(_fitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &WindowDisplayModel::onFitModeChanged);
    QObject::connect(_bordersCheck, &QCheckBox::toggled, this, [this](bool on) {
        _borders = on;
        if (_glWindow) {
            _glWindow->setBordersEnabled(on);
        }
    });
    QObject::connect(_alwaysOnTopCheck, &QCheckBox::toggled, this, [this](bool on) {
        _alwaysOnTop = on;
        if (_glWindow) {
            _glWindow->setAlwaysOnTop(on);
        }
    });
    QObject::connect(_closeOnEscCheck, &QCheckBox::toggled, this, [this](bool on) {
        _closeOnEscape = on;
        if (_glWindow) {
            _glWindow->setCloseOnEscape(on);
        }
    });
    QObject::connect(_ignoreTaskbarCheck, &QCheckBox::toggled, this, [this](bool on) {
        _ignoreTaskbar = on;
        if (isWindowOpen()) {
            openWindow();
        }
    });
    QObject::connect(_openBtn, &QPushButton::clicked, this, &WindowDisplayModel::onOpenClicked);
}

void WindowDisplayModel::ensureQuitAnchor()
{
    if (_quitAnchor) {
        return;
    }

    _quitAnchor = new QWindow();
    // ToolTip 即使被意外显示也不计入 lastWindowClosed
    _quitAnchor->setFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    _quitAnchor->setObjectName(QStringLiteral("WindowDisplayQuitAnchor"));
    // QWindow::setParent(QWindow*) 会盖住 QObject::setParent，必须显式调用
    _quitAnchor->QObject::setParent(this);
}

void WindowDisplayModel::ensureWindow()
{
    ensureQuitAnchor();
    if (_glWindow) {
        _glWindow->setTransientParent(_quitAnchor);
        return;
    }
    // 仅构造 QObject/QWindow 外壳；GL context 在首次 show() 时创建
    _glWindow = new ImageTextureWindow();
    _glWindow->setTransientParent(_quitAnchor);
    _glWindow->onClosed = [this]() {
        // Esc / Alt+F4：立刻摘掉指针，deleteLater 销毁 GL，避免事件栈内同步析构
        // 也避免「未销毁完又 open」复用到半死窗口
        _wantOpen = false;
        ImageTextureWindow* window = _glWindow;
        _glWindow = nullptr;
        if (window) {
            window->onClosed = nullptr;
            window->deleteLater();
        }
        m_lastDisplayedTimestamp = -1;
        m_lastDisplayedTextureId = 0;
        if (_openBtn) {
            _openBtn->setText(QStringLiteral("打开窗口"));
        }
    };
    applyWindowSettings();
}

void WindowDisplayModel::destroyWindow()
{
    if (!_glWindow) {
        return;
    }
    ImageTextureWindow* window = _glWindow;
    _glWindow = nullptr;
    window->onClosed = nullptr;
    window->setTransientParent(nullptr);
    // 同步拆掉 HWND。Flow 退出时事件循环马上停，不能只靠 deleteLater 把窗口留到下一轮。
    window->hide();
    window->destroy();
    window->deleteLater();
    m_lastDisplayedTimestamp = -1;
    m_lastDisplayedTextureId = 0;
    if (_openBtn) {
        _openBtn->setText(QStringLiteral("打开窗口"));
    }
}

void WindowDisplayModel::applyWindowSettings()
{
    if (!_glWindow) {
        return;
    }
    _glWindow->setTitle(_title);
    _glWindow->setBordersEnabled(_borders);
    _glWindow->setAlwaysOnTop(_alwaysOnTop);
    _glWindow->setCloseOnEscape(_closeOnEscape);
    _glWindow->setFitMode(toWindowFit(_fitMode));
}

void WindowDisplayModel::refreshScreens()
{
    if (!_screenCombo) {
        return;
    }

    const QSignalBlocker blocker(_screenCombo);
    _screenCombo->clear();

    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        const QScreen* s = screens[i];
        const QString name = s->name().isEmpty() ? QStringLiteral("Screen %1").arg(i) : s->name();
        const QSize sz = s->size();
        const bool primary = (s == QGuiApplication::primaryScreen());
        _screenCombo->addItem(
            QStringLiteral("%1%2 (%3x%4)")
                .arg(name)
                .arg(primary ? QStringLiteral(" [主]") : QString())
                .arg(sz.width())
                .arg(sz.height()),
            i);
    }

    if (screens.isEmpty()) {
        _screenIndex = 0;
        return;
    }

    if (_screenIndex < 0 || _screenIndex >= screens.size()) {
        _screenIndex = 0;
    }
    _screenCombo->setCurrentIndex(_screenIndex);
}

void WindowDisplayModel::syncCustomSizeEnabled()
{
    const bool custom = (_openingSize == OpeningSize::Custom);
    if (_widthSpin) {
        _widthSpin->setEnabled(custom);
    }
    if (_heightSpin) {
        _heightSpin->setEnabled(custom);
    }
}

void WindowDisplayModel::syncPositionEnabled()
{
    const bool custom = (_position == WindowPosition::Custom);
    if (_posXSpin) {
        _posXSpin->setEnabled(custom);
    }
    if (_posYSpin) {
        _posYSpin->setEnabled(custom);
    }
}

void WindowDisplayModel::syncUiFromState()
{
    if (_titleEdit) {
        const QSignalBlocker b(_titleEdit);
        _titleEdit->setText(_title);
    }
    if (_screenCombo && _screenIndex >= 0 && _screenIndex < _screenCombo->count()) {
        const QSignalBlocker b(_screenCombo);
        _screenCombo->setCurrentIndex(_screenIndex);
    }
    if (_openingSizeCombo) {
        const QSignalBlocker b(_openingSizeCombo);
        const int idx = _openingSizeCombo->findData(static_cast<int>(_openingSize));
        if (idx >= 0) {
            _openingSizeCombo->setCurrentIndex(idx);
        }
    }
    if (_widthSpin) {
        const QSignalBlocker b(_widthSpin);
        _widthSpin->setValue(_customWidth);
    }
    if (_heightSpin) {
        const QSignalBlocker b(_heightSpin);
        _heightSpin->setValue(_customHeight);
    }
    if (_positionCombo) {
        const QSignalBlocker b(_positionCombo);
        const int idx = _positionCombo->findData(static_cast<int>(_position));
        if (idx >= 0) {
            _positionCombo->setCurrentIndex(idx);
        }
    }
    if (_posXSpin) {
        const QSignalBlocker b(_posXSpin);
        _posXSpin->setValue(_posX);
    }
    if (_posYSpin) {
        const QSignalBlocker b(_posYSpin);
        _posYSpin->setValue(_posY);
    }
    if (_fitCombo) {
        const QSignalBlocker b(_fitCombo);
        const int idx = _fitCombo->findData(static_cast<int>(_fitMode));
        if (idx >= 0) {
            _fitCombo->setCurrentIndex(idx);
        }
    }
    if (_bordersCheck) {
        const QSignalBlocker b(_bordersCheck);
        _bordersCheck->setChecked(_borders);
    }
    if (_alwaysOnTopCheck) {
        const QSignalBlocker b(_alwaysOnTopCheck);
        _alwaysOnTopCheck->setChecked(_alwaysOnTop);
    }
    if (_closeOnEscCheck) {
        const QSignalBlocker b(_closeOnEscCheck);
        _closeOnEscCheck->setChecked(_closeOnEscape);
    }
    if (_ignoreTaskbarCheck) {
        const QSignalBlocker b(_ignoreTaskbarCheck);
        _ignoreTaskbarCheck->setChecked(_ignoreTaskbar);
    }
    syncCustomSizeEnabled();
    syncPositionEnabled();
    if (_openBtn) {
        _openBtn->setText(isWindowOpen() ? QStringLiteral("关闭窗口") : QStringLiteral("打开窗口"));
    }
}

bool WindowDisplayModel::isWindowOpen() const
{
    return _glWindow && _glWindow->isVisible();
}

void WindowDisplayModel::openWindow()
{
    // 每次打开新建窗口：关闭时销毁 GL，避免残留 context 再开崩溃
    if (!_glWindow) {
        ensureWindow();
    }
    ensureQuitAnchor();
    if (_glWindow && _quitAnchor) {
        _glWindow->setTransientParent(_quitAnchor);
    }
    applyWindowSettings();

    const auto screens = QGuiApplication::screens();
    QScreen* target = nullptr;
    if (_screenIndex >= 0 && _screenIndex < screens.size()) {
        target = screens[_screenIndex];
    } else if (!screens.isEmpty()) {
        target = screens.first();
        _screenIndex = 0;
    }

    const QSize custom = (_openingSize == OpeningSize::Custom)
        ? QSize(_customWidth, _customHeight)
        : QSize();

    const ImageTextureWindow::Anchor anchor = (_position == WindowPosition::Center)
        ? ImageTextureWindow::Anchor::Center
        : ImageTextureWindow::Anchor::TopLeft;
    const QPoint offset = (_position == WindowPosition::Custom)
        ? QPoint(_posX, _posY)
        : QPoint();

    _wantOpen = true;
    // show() 时才创建 native window + initializeGL
    _glWindow->openOnScreen(target, custom, _ignoreTaskbar, offset, anchor);
    if (_openBtn) {
        _openBtn->setText(QStringLiteral("关闭窗口"));
    }

    // GL 就绪后再推帧；若本帧尚未 initializeGL，等下一 tick / newFrameWritten
    if (_glWindow->isGlReady()) {
        updateDisplayFromInput(TimestampGenerator::getInstance()->getCurrentFrameCount());
    }
}

void WindowDisplayModel::closeWindow()
{
    _wantOpen = false;
    if (!_glWindow) {
        if (_openBtn) {
            _openBtn->setText(QStringLiteral("打开窗口"));
        }
        return;
    }
    // 主动关闭：直接销毁，不再走 onClosed 二次 destroy
    ImageTextureWindow* window = _glWindow;
    _glWindow = nullptr;
    window->onClosed = nullptr;
    window->closeWindow();
    window->deleteLater();
    m_lastDisplayedTimestamp = -1;
    m_lastDisplayedTextureId = 0;
    if (_openBtn) {
        _openBtn->setText(QStringLiteral("打开窗口"));
    }
}

void WindowDisplayModel::setWindowOpen(bool open)
{
    if (open) {
        if (!isWindowOpen()) {
            openWindow();
        }
    } else if (isWindowOpen()) {
        closeWindow();
    }
}

void WindowDisplayModel::onOpenClicked()
{
    setWindowOpen(!isWindowOpen());
}

void WindowDisplayModel::onScreenIndexChanged(int index)
{
    _screenIndex = index;
    if (isWindowOpen()) {
        openWindow();
    }
}

void WindowDisplayModel::onOpeningSizeChanged(int index)
{
    if (!_openingSizeCombo || index < 0) {
        return;
    }
    _openingSize = static_cast<OpeningSize>(_openingSizeCombo->itemData(index).toInt());
    syncCustomSizeEnabled();
    if (isWindowOpen()) {
        openWindow();
    }
}

void WindowDisplayModel::onPositionChanged(int index)
{
    if (!_positionCombo || index < 0) {
        return;
    }
    _position = static_cast<WindowPosition>(_positionCombo->itemData(index).toInt());
    syncPositionEnabled();
    if (isWindowOpen()) {
        openWindow();
    }
}

void WindowDisplayModel::onFitModeChanged(int index)
{
    if (!_fitCombo || index < 0) {
        return;
    }
    _fitMode = static_cast<FitMode>(_fitCombo->itemData(index).toInt());
    if (_glWindow) {
        _glWindow->setFitMode(toWindowFit(_fitMode));
    }
}

QtNodes::NodeDataType WindowDisplayModel::dataType(QtNodes::PortType portType,
                                                     QtNodes::PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        switch (portIndex) {
        case 0:
            return ImageData().type();
        default:
            return VariableData().type();
        }
    case PortType::Out:
        return ImageData().type();
    default:
        break;
    }
    return VariableData().type();
}

QString WindowDisplayModel::portCaption(QtNodes::PortType portType,
                                          QtNodes::PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        switch (portIndex) {
        case 0:
            return QStringLiteral("IMAGE");
        case 1:
            return QStringLiteral("OPEN");
        default:
            return {};
        }
    case PortType::Out:
        return portIndex == 0 ? QStringLiteral("IMAGE") : QString();
    default:
        return {};
    }
}

std::shared_ptr<QtNodes::NodeData> WindowDisplayModel::outData(QtNodes::PortIndex)
{
    return m_outData;
}

void WindowDisplayModel::bindInputBuffer()
{
    QObject::disconnect(m_bufferFrameConnection);
    m_bufferFrameConnection = {};

    if (!m_inImage || !m_inImage->isConnectedToSharedBuffer()) {
        return;
    }

    const auto buffer = m_inImage->getSharedImageBuffer();
    if (!buffer) {
        return;
    }

    m_bufferFrameConnection = connect(
        buffer.get(),
        &ImageTimestampRingQueue::newFrameWritten,
        this,
        [this](const ImageFrame&) {
            updateDisplayFromInput(TimestampGenerator::getInstance()->getCurrentFrameCount());
        },
        Qt::QueuedConnection);
}

void WindowDisplayModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData,
                                   QtNodes::PortIndex const portIndex)
{
    if (portIndex == 0) {
        m_inImage = std::dynamic_pointer_cast<ImageData>(nodeData);
        m_outData = m_inImage;

        if (m_inImage) {
            m_lastDisplayedTimestamp = -1;
            m_lastDisplayedTextureId = 0;
        }

        bindInputBuffer();
        updateDisplayFromInput(TimestampGenerator::getInstance()->getCurrentFrameCount());
        Q_EMIT dataUpdated(0);
        return;
    }

    if (portIndex == 1) {
        if (const auto varData = std::dynamic_pointer_cast<VariableData>(nodeData)) {
            setWindowOpen(varData->asBool());
        } else {
            // 断开 OPEN 端口时不强制关窗，保留手动状态
        }
    }
}

void WindowDisplayModel::clearDisplayIfNeeded()
{
    if (!_glWindow || m_lastDisplayedTimestamp < 0) {
        return;
    }
    m_lastDisplayedTimestamp = -1;
    m_lastDisplayedTextureId = 0;
    _glWindow->clearTexture();
}

bool WindowDisplayModel::applyFrameIfChanged(const ImageFrame& frame)
{
    if (!frame.texture.valid() || !_glWindow) {
        return false;
    }

    if (m_lastDisplayedTimestamp >= 0
        && frame.timestamp == m_lastDisplayedTimestamp
        && frame.texture.textureId == m_lastDisplayedTextureId) {
        return true;
    }

    _glWindow->setTexture(frame.texture);
    m_lastDisplayedTimestamp = frame.timestamp;
    m_lastDisplayedTextureId = frame.texture.textureId;
    return true;
}

bool WindowDisplayModel::resolveDisplayFrame(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                               qint64 frameCount,
                                               ImageFrame& frame)
{
    if (!buffer) {
        return false;
    }

    auto ensureTex = [](ImageFrame& f) -> bool {
        if (!f.texture.valid() && f.hasCpuCache()) {
            ImageFrame uploadFrame = f;
            if (uploadFrame.ensureGpuTexture()) {
                f = std::move(uploadFrame);
            }
        }
        return f.texture.valid();
    };

    // 音画对齐：按 clock 取（生产者 stamp=clock+5）；勿用未来 latest 抢跑
    if (buffer->getFrameByTimestamp(frameCount, frame) && ensureTex(frame)) {
        return true;
    }

    if (buffer->getLatestFrame(frame) && frame.timestamp <= frameCount && ensureTex(frame)) {
        return true;
    }

    return false;
}

void WindowDisplayModel::updateDisplayFromInput(qint64 frameCount)
{
    // 窗口未显示或 GL 未就绪时不驱动，避免提前 makeCurrent
    if (!_glWindow || !_glWindow->isVisible() || !_glWindow->isGlReady()) {
        return;
    }

    if (!m_inImage) {
        clearDisplayIfNeeded();
        return;
    }

    if (!m_inImage->isConnectedToSharedBuffer()) {
        clearDisplayIfNeeded();
        return;
    }

    const auto buffer = m_inImage->getSharedImageBuffer();
    if (!buffer) {
        clearDisplayIfNeeded();
        return;
    }

    if (m_inImage->isEmpty()) {
        _glWindow->clearTexture();
        m_lastDisplayedTimestamp = -1;
        m_lastDisplayedTextureId = 0;
        return;
    }

    ImageFrame frame;
    if (resolveDisplayFrame(buffer, frameCount, frame)) {
        applyFrameIfChanged(frame);
        return;
    }

    // 竞态保护：曾显示过则保留上一帧，避免闪黑
    if (m_lastDisplayedTimestamp >= 0) {
        return;
    }

    clearDisplayIfNeeded();
}

void WindowDisplayModel::onSystemFrameTick(qint64 frameCount)
{
    updateDisplayFromInput(frameCount);
}

QJsonObject WindowDisplayModel::save() const
{
    QJsonObject modelJson = AbstractDelegateModel::save();
    modelJson[QStringLiteral("title")] = _title;
    modelJson[QStringLiteral("screenIndex")] = _screenIndex;
    modelJson[QStringLiteral("openingSize")] = static_cast<int>(_openingSize);
    modelJson[QStringLiteral("customWidth")] = _customWidth;
    modelJson[QStringLiteral("customHeight")] = _customHeight;
    modelJson[QStringLiteral("position")] = static_cast<int>(_position);
    modelJson[QStringLiteral("posX")] = _posX;
    modelJson[QStringLiteral("posY")] = _posY;
    modelJson[QStringLiteral("fitMode")] = static_cast<int>(_fitMode);
    modelJson[QStringLiteral("borders")] = _borders;
    modelJson[QStringLiteral("alwaysOnTop")] = _alwaysOnTop;
    modelJson[QStringLiteral("closeOnEscape")] = _closeOnEscape;
    modelJson[QStringLiteral("ignoreTaskbar")] = _ignoreTaskbar;
    modelJson[QStringLiteral("windowOpen")] = _wantOpen || isWindowOpen();
    return modelJson;
}

void WindowDisplayModel::load(const QJsonObject& p)
{
    AbstractDelegateModel::load(p);

    if (p.contains(QStringLiteral("title"))) {
        _title = p.value(QStringLiteral("title")).toString(_title);
    }
    if (p.contains(QStringLiteral("screenIndex"))) {
        _screenIndex = p.value(QStringLiteral("screenIndex")).toInt(_screenIndex);
    }
    if (p.contains(QStringLiteral("openingSize"))) {
        _openingSize = static_cast<OpeningSize>(p.value(QStringLiteral("openingSize")).toInt(
            static_cast<int>(_openingSize)));
    }
    if (p.contains(QStringLiteral("customWidth"))) {
        _customWidth = p.value(QStringLiteral("customWidth")).toInt(_customWidth);
    }
    if (p.contains(QStringLiteral("customHeight"))) {
        _customHeight = p.value(QStringLiteral("customHeight")).toInt(_customHeight);
    }
    if (p.contains(QStringLiteral("position"))) {
        _position = static_cast<WindowPosition>(p.value(QStringLiteral("position")).toInt(
            static_cast<int>(_position)));
    }
    if (p.contains(QStringLiteral("posX"))) {
        _posX = p.value(QStringLiteral("posX")).toInt(_posX);
    }
    if (p.contains(QStringLiteral("posY"))) {
        _posY = p.value(QStringLiteral("posY")).toInt(_posY);
    }
    if (p.contains(QStringLiteral("fitMode"))) {
        _fitMode = static_cast<FitMode>(p.value(QStringLiteral("fitMode")).toInt(
            static_cast<int>(_fitMode)));
    }
    if (p.contains(QStringLiteral("borders"))) {
        _borders = p.value(QStringLiteral("borders")).toBool(_borders);
    }
    if (p.contains(QStringLiteral("alwaysOnTop"))) {
        _alwaysOnTop = p.value(QStringLiteral("alwaysOnTop")).toBool(_alwaysOnTop);
    }
    if (p.contains(QStringLiteral("closeOnEscape"))) {
        _closeOnEscape = p.value(QStringLiteral("closeOnEscape")).toBool(_closeOnEscape);
    }
    if (p.contains(QStringLiteral("ignoreTaskbar"))) {
        _ignoreTaskbar = p.value(QStringLiteral("ignoreTaskbar")).toBool(_ignoreTaskbar);
    }

    refreshScreens();
    syncUiFromState();

    // 工程加载阶段延后开窗，避免与其它节点 GL 初始化抢同一 context
    const bool shouldOpen = p.value(QStringLiteral("windowOpen")).toBool(false);
    _wantOpen = shouldOpen;
    if (shouldOpen) {
        QTimer::singleShot(0, this, [this]() {
            if (_wantOpen && !isWindowOpen()) {
                openWindow();
            }
        });
    }
}
