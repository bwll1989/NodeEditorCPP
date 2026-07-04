#include "WindowDisplayModel.hpp"

#include <QLabel>
#include <QSpacerItem>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtNodes/NodeDelegateModelRegistry>

using namespace Nodes;
using namespace NodeDataTypes;

// ---------------------------------------------------------------------------
// WindowDisplayModel — 全屏独立窗口图像显示节点
//
// 显示刷新仅由 TimestampGenerator tick 驱动；数据仅来自 ImageTimestampRingQueue。
// ---------------------------------------------------------------------------

WindowDisplayModel::WindowDisplayModel()
{
    InPortCount = 2;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = "Window Display";
    WidgetEmbeddable = false;
    Resizable = false;

    _panel = new QWidget();
    _layout = new QGridLayout();
    _panel->setLayout(_layout);
    _screenCombo = new QComboBox(_panel);
    _openBtn = new QPushButton("打开窗口", _panel);
    _layout->addWidget(new QLabel("显示器:"), 0, 0);
    _layout->addWidget(_screenCombo, 0, 1);
    _layout->addWidget(_openBtn, 1, 0, 1, 2);
    _layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::MinimumExpanding), 2, 0, 1, 2);
    _panel->setMinimumSize(240, 80);

    _glWindow = new ImageTextureWindow();
    _glWindow->onClosed = [this]() {
        if (_openBtn) {
            _openBtn->setText("打开窗口");
        }
    };

    if (QCoreApplication* app = QCoreApplication::instance()) {
        QObject::connect(app, &QCoreApplication::aboutToQuit, this, [this]() {
            closeOutputWindow();
        }, Qt::DirectConnection);
    }

    refreshScreens();
    QObject::connect(_screenCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        onScreenChanged(idx);
    });
    QObject::connect(_openBtn, &QPushButton::clicked, this, [this]() {
        toggleWindow();
    });

    connect(TimestampGenerator::getInstance(),
            &TimestampGenerator::frameCountUpdated,
            this,
            &WindowDisplayModel::onSystemFrameTick,
            Qt::QueuedConnection);
}

WindowDisplayModel::~WindowDisplayModel()
{
    disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
    closeOutputWindow();
}

void WindowDisplayModel::closeOutputWindow()
{
    if (!_glWindow) {
        return;
    }
    _glWindow->onClosed = nullptr;
    if (_glWindow->isVisible()) {
        _glWindow->hide();
    }
    _glWindow->close();
    _glWindow->destroy();
    delete _glWindow;
    _glWindow = nullptr;
}

QtNodes::NodeDataType WindowDisplayModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

std::shared_ptr<QtNodes::NodeData> WindowDisplayModel::outData(QtNodes::PortIndex)
{
    return m_outData;
}

void WindowDisplayModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex)
{
    if (portIndex == 0) {
        m_inImage = std::dynamic_pointer_cast<ImageData>(nodeData);
        m_outData = m_inImage;
        if (m_inImage) {
            m_lastDisplayedTimestamp = -1;
            m_lastDisplayedTextureId = 0;
        }
    } else if (portIndex == 1) {
        if (const auto varData = std::dynamic_pointer_cast<VariableData>(nodeData)) {
            setWindowVisible(varData->value().toBool());
        }
    }

    Q_EMIT dataUpdated(0);
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

void WindowDisplayModel::updateDisplayFromInput(qint64 frameCount)
{
    if (!_glWindow) {
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

    ImageFrame frame;
    const auto buffer = m_inImage->getSharedImageBuffer();
    if (!buffer) {
        clearDisplayIfNeeded();
        return;
    }

    if (m_inImage->isEmpty()) {
        if (_glWindow) {
            _glWindow->clearTexture();
        }
        m_lastDisplayedTimestamp = -1;
        m_lastDisplayedTextureId = 0;
        return;
    }

    if (buffer->getLatestFrame(frame) && frame.texture.valid()) {
        applyFrameIfChanged(frame);
        return;
    }

    if (m_lastDisplayedTimestamp < 0 &&
        buffer->getFrameByTimestamp(frameCount, frame) &&
        frame.texture.valid()) {
        applyFrameIfChanged(frame);
        return;
    }

    clearDisplayIfNeeded();
}

void WindowDisplayModel::onSystemFrameTick(qint64 frameCount)
{
    updateDisplayFromInput(frameCount);
}

void WindowDisplayModel::refreshScreens()
{
    _screenCombo->clear();
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        const QScreen* s = screens[i];
        const QString name = s->name().isEmpty() ? QString("Screen %1").arg(i) : s->name();
        const QSize sz = s->size();
        _screenCombo->addItem(QString("%1 (%2x%3)").arg(name).arg(sz.width()).arg(sz.height()), i);
    }
    _currentScreenIndex = _screenCombo->currentIndex();
}

void WindowDisplayModel::onScreenChanged(int index)
{
    _currentScreenIndex = index;
    if (!_glWindow) {
        return;
    }

    const auto screens = QGuiApplication::screens();
    if (index >= 0 && index < screens.size()) {
        QScreen* target = screens[index];
        _glWindow->setScreen(target);
        _glWindow->setGeometry(target->geometry());
    }
}

void WindowDisplayModel::toggleWindow()
{
    if (!_glWindow) {
        return;
    }
    setWindowVisible(!_glWindow->isVisible());
}

void WindowDisplayModel::setWindowVisible(bool visible)
{
    if (!_glWindow) {
        return;
    }

    if (visible) {
        if (!_glWindow->isVisible()) {
            onScreenChanged(_currentScreenIndex);
            _glWindow->showFullScreen();
            _glWindow->raise();
            _glWindow->requestActivate();
            _openBtn->setText("关闭窗口");
        }
    } else if (_glWindow->isVisible()) {
        _glWindow->hide();
        _openBtn->setText("打开窗口");
    }
}

QJsonObject WindowDisplayModel::save() const
{
    QJsonObject modelJson = AbstractDelegateModel::save();
    modelJson["screenIndex"] = _currentScreenIndex;
    return modelJson;
}

void WindowDisplayModel::load(const QJsonObject& p)
{
    AbstractDelegateModel::load(p);
    const QJsonValue v = p["screenIndex"];
    if (!v.isUndefined()) {
        const int idx = v.toInt();
        if (idx >= 0 && idx < _screenCombo->count()) {
            _screenCombo->setCurrentIndex(idx);
            onScreenChanged(idx);
        }
    }
}
