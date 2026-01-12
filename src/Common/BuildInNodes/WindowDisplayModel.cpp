#include "WindowDisplayModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QLabel>
using namespace Nodes;
using namespace NodeDataTypes;
/**
 * @brief 构造函数，初始化图像显示标签
 */
WindowDisplayModel::WindowDisplayModel()
{
    InPortCount =2;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="Window Display";
    WidgetEmbeddable=true;
    Resizable=false;
    // 控制面板
    _panel = new QWidget();
    _layout = new QGridLayout();
    _panel->setLayout(_layout);
    _screenCombo = new QComboBox(_panel);
    _openBtn = new QPushButton("打开窗口", _panel);
    _layout->addWidget(new QLabel("显示器:"), 0, 0);
    _layout->addWidget(_screenCombo, 0, 1);
    _layout->addWidget(_openBtn, 1, 0, 1, 2);
    _panel->setMinimumSize(240, 80);
    // OpenGL窗口
    _glWindow = new ImageOpenGLWindow();
    _glWindow->onClosed = [this](){
        if (_openBtn) _openBtn->setText("打开窗口");
    };
    refreshScreens();
    QObject::connect(_screenCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [this](int idx){ onScreenChanged(idx); });
    QObject::connect(_openBtn, &QPushButton::clicked, [this](){ toggleWindow(); });
}

WindowDisplayModel::~WindowDisplayModel()
{
    if (_glWindow) {
        _glWindow->close();
        delete _glWindow;
        _glWindow = nullptr;
    }
}

/**
 * @brief 返回端口数据类型
 */
QtNodes::NodeDataType WindowDisplayModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {

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
            case PortType::None:
                break;
            default:
                break;
        }
        return VariableData().type();
}

/**
 * @brief 输出当前缓存的图像数据
 */
std::shared_ptr<QtNodes::NodeData> WindowDisplayModel::outData(QtNodes::PortIndex) {
    return m_outData;
}

/**
 * @brief 设置输入图像数据并刷新显示
 * 使用 QLabel 的缩放功能，避免每帧创建缩放后的 QPixmap 导致内存增长
 */
void WindowDisplayModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) {
    if (portIndex == 0) {
        m_outData = std::dynamic_pointer_cast<ImageData>(nodeData);
        if (m_outData) {
            const QImage img = m_outData->image();
            if (!img.isNull() && _glWindow) {
                _glWindow->setFrame(img);
            }
        } else {
            if (_glWindow) _glWindow->setFrame(QImage());
        }
    } else if (portIndex == 1) {
        auto varData = std::dynamic_pointer_cast<VariableData>(nodeData);
        if (varData) {
            bool show = varData->value().toBool();
            setWindowVisible(show);
        }
    }

    Q_EMIT dataUpdated(0);
}

/**
 * @brief 刷新系统显示器列表
 */
void WindowDisplayModel::refreshScreens(){
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

/**
 * @brief 当选择的显示器改变时移动窗口到目标屏幕
 */
void WindowDisplayModel::onScreenChanged(int index){
    _currentScreenIndex = index;
    if (!_glWindow) return;
    const auto screens = QGuiApplication::screens();
    if (index >= 0 && index < screens.size()) {
        QScreen* target = screens[index];
        _glWindow->setScreen(target);
        const QRect geo = target->geometry();
        _glWindow->setGeometry(geo);
    }
}

/**
 * @brief 打开或关闭OpenGL显示窗口
 */
void WindowDisplayModel::toggleWindow(){
    if (!_glWindow) return;
    setWindowVisible(!_glWindow->isVisible());
}

/**
 * @brief 设置窗口可见性
 */
void WindowDisplayModel::setWindowVisible(bool visible) {
    if (!_glWindow) return;
    if (visible) {
        if (!_glWindow->isVisible()) {
            onScreenChanged(_currentScreenIndex);
            _glWindow->showFullScreen();
            _glWindow->raise();
            _glWindow->requestActivate();
            _openBtn->setText("关闭窗口");
        }
    } else {
        if (_glWindow->isVisible()) {
            _glWindow->hide();
            _openBtn->setText("打开窗口");
        }
    }
}

QJsonObject WindowDisplayModel::save() const {
    QJsonObject modelJson = AbstractDelegateModel::save();
    modelJson["screenIndex"] = _currentScreenIndex;
    return modelJson;
}

void WindowDisplayModel::load(const QJsonObject &p) {
    AbstractDelegateModel::load(p);
    QJsonValue v = p["screenIndex"];
    if (!v.isUndefined()) {
        int idx = v.toInt();
        if (idx >= 0 && idx < _screenCombo->count()) {
            _screenCombo->setCurrentIndex(idx);
            onScreenChanged(idx);
        }
    }
}
