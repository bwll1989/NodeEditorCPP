#pragma once

#include <QtNodes/NodeDelegateModelRegistry>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Elements/ImageTextureWindow/ImageTextureWindow.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QComboBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QPushButton>
#include <QScreen>

#include <memory>

using namespace NodeDataTypes;

namespace Nodes
{
// 全屏窗口显示：刷新策略同 ImageShowModel，见 WindowDisplayModel.cpp。
class WindowDisplayModel final : public AbstractDelegateModel
{
    Q_OBJECT

public:
    WindowDisplayModel();
    ~WindowDisplayModel() override;

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;
    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

    QWidget* embeddedWidget() override { return _panel; }

    QJsonObject save() const override;
    void load(const QJsonObject& p) override;

private:
    void refreshScreens();
    void onScreenChanged(int index);
    void toggleWindow();
    void setWindowVisible(bool visible);
    void updateDisplayFromInput(qint64 frameCount);
    void onSystemFrameTick(qint64 frameCount);
    bool applyFrameIfChanged(const ImageFrame& frame);
    void clearDisplayIfNeeded();
    void closeOutputWindow();

    QWidget* _panel = nullptr;
    QGridLayout* _layout = nullptr;
    QComboBox* _screenCombo = nullptr;
    QPushButton* _openBtn = nullptr;
    ImageTextureWindow* _glWindow = nullptr;
    int _currentScreenIndex = 0;

    std::shared_ptr<ImageData> m_inImage;
    std::shared_ptr<ImageData> m_outData;
    qint64 m_lastDisplayedTimestamp = -1;  // 已显示帧的时间戳，-1 表示换源后待首帧 tick 刷新
    unsigned int m_lastDisplayedTextureId = 0;
};
} // namespace Nodes
