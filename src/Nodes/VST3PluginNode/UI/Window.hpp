#pragma once
//#include <Media/Effect/Settings/Model.hpp>
//#include <Vst3/EffectModel.hpp>
#include "WindowContainer.hpp"

#include <QResizeEvent>
#include <QWindow>

#include <pluginterfaces/gui/iplugview.h>

namespace vst3
{

WindowContainer createVstWindowContainer(
    Window& parentWindow, const Steinberg::IPlugView& view);

class Window : public QDialog
{
    const Steinberg::IPlugView& m_view;
    WindowContainer container;

public:
  Window(const Steinberg::IPlugView& view, QWidget* parent);
  ~Window();

  void resizeEvent(QResizeEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
};
}
