#pragma once

/**
 * @file WindowDisplayModel.hpp
 * @brief Window Display — 独立窗口多屏图像输出（对齐 TD Window COMP 核心能力）
 *
 * ## 职责
 * - 在指定显示器上打开独立窗口，显示上游 ImageData 当前帧
 * - 透传同一 ImageData 到输出端口
 *
 * ## 刷新策略（与 ImageShowModel 对齐）
 * 1. TimestampGenerator::frameCountUpdated（QueuedConnection）
 * 2. ImageTimestampRingQueue::newFrameWritten
 * 3. setInData 后立即拉帧
 *
 * @see ImageTextureWindow
 * @see ImageShowModel
 */

#include <QtNodes/NodeDelegateModelRegistry>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Elements/ImageTextureWindow/ImageTextureWindow.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QWidget>

#include <memory>

using namespace NodeDataTypes;

namespace Nodes
{

class WindowDisplayModel final : public AbstractDelegateModel
{
    Q_OBJECT

public:
    enum class OpeningSize {
        FillDisplay = 0, ///< 填满目标显示器（TD Fill Location）
        Custom = 1       ///< 自定义宽高
    };
    Q_ENUM(OpeningSize)

    /** 相对所选显示器区域（忽略任务栏时用整屏 geometry） */
    enum class WindowPosition {
        Center = 0,  ///< 自定义尺寸居中；填满时铺满区域
        TopLeft = 1, ///< 对齐区域左上角
        Custom = 2   ///< 左上角 = 区域原点 + (X, Y)
    };
    Q_ENUM(WindowPosition)

    enum class FitMode {
        Contain = 0,
        Cover = 1,
        Stretch = 2
    };
    Q_ENUM(FitMode)

    WindowDisplayModel();
    ~WindowDisplayModel() override;

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;
    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

    QWidget* embeddedWidget() override { return _panel; }

    QJsonObject save() const override;
    void load(const QJsonObject& p) override;

private:
    void buildUi();
    void ensureQuitAnchor();
    void ensureWindow();
    void destroyWindow();
    void refreshScreens();
    void syncUiFromState();
    void syncCustomSizeEnabled();
    void syncPositionEnabled();
    void applyWindowSettings();
    void openWindow();
    void closeWindow();
    void setWindowOpen(bool open);
    bool isWindowOpen() const;

    void bindInputBuffer();
    void updateDisplayFromInput(qint64 frameCount);
    void onSystemFrameTick(qint64 frameCount);
    bool applyFrameIfChanged(const ImageFrame& frame);
    void clearDisplayIfNeeded();
    bool resolveDisplayFrame(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                             qint64 frameCount,
                             ImageFrame& frame);

    void onOpenClicked();
    void onScreenIndexChanged(int index);
    void onOpeningSizeChanged(int index);
    void onPositionChanged(int index);
    void onFitModeChanged(int index);

    // --- UI ---
    QWidget* _panel = nullptr;
    QGridLayout* _layout = nullptr;
    QLineEdit* _titleEdit = nullptr;
    QComboBox* _screenCombo = nullptr;
    QComboBox* _openingSizeCombo = nullptr;
    IntDragValueWidget* _widthSpin = nullptr;
    IntDragValueWidget* _heightSpin = nullptr;
    QComboBox* _positionCombo = nullptr;
    IntDragValueWidget* _posXSpin = nullptr;
    IntDragValueWidget* _posYSpin = nullptr;
    QComboBox* _fitCombo = nullptr;
    QCheckBox* _bordersCheck = nullptr;
    QCheckBox* _alwaysOnTopCheck = nullptr;
    QCheckBox* _closeOnEscCheck = nullptr;
    QCheckBox* _ignoreTaskbarCheck = nullptr;
    QPushButton* _openBtn = nullptr;

    ImageTextureWindow* _glWindow = nullptr;
    /**
     * 永不显示的锚点窗口。
     * QOpenGLWindow 默认是 primary window，会挡住 quitOnLastWindowClosed：
     * Flow 主窗口关掉后进程不退出，输出窗口就留在屏幕上。
     * 挂上 transient parent 后它变成 secondary window，不再阻止退出。
     * 锚点本身不 show()，避免成为 Win32 owner 后主窗口最小化时把输出窗口一起藏掉。
     */
    QWindow* _quitAnchor = nullptr;

    // --- 参数状态 ---
    QString _title = QStringLiteral("Window Display");
    int _screenIndex = 0;
    OpeningSize _openingSize = OpeningSize::FillDisplay;
    int _customWidth = 1280;
    int _customHeight = 720;
    WindowPosition _position = WindowPosition::Center;
    int _posX = 0; ///< 相对目标屏幕区域左上角
    int _posY = 0;
    FitMode _fitMode = FitMode::Contain;
    bool _borders = false;
    bool _alwaysOnTop = true;
    bool _closeOnEscape = true;
    bool _ignoreTaskbar = true;
    bool _wantOpen = false; ///< load 后自动打开 / 端口驱动期望态

    // --- 图像管线 ---
    std::shared_ptr<ImageData> m_inImage;
    std::shared_ptr<ImageData> m_outData;
    QMetaObject::Connection m_bufferFrameConnection;
    qint64 m_lastDisplayedTimestamp = -1;
    unsigned int m_lastDisplayedTextureId = 0;
};

} // namespace Nodes
