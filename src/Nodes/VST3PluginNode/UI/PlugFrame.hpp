#pragma once
#include "WindowContainer.hpp"

#include <QTimer>
#include <QWindow>

#include <pluginterfaces/gui/iplugview.h>

namespace vst3
{

#if defined(_WIN32)
class PlugFrame final : public Steinberg::IPlugFrame
{
public:
  Steinberg::tresult queryInterface(const Steinberg::TUID _iid, void** obj) override
  {
    *obj = nullptr;
    return Steinberg::kResultFalse;
  }

  Steinberg::uint32 addRef() override { return 1; }
  Steinberg::uint32 release() override { return 1; }

  QDialog& w;
  WindowContainer wc;
  PlugFrame(QDialog& w, WindowContainer& wc)
      : w{w}
      , wc{wc}
  {
  }

  Steinberg::tresult
  resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override
  {
    wc.setSizeFromVst(*view, *newSize, w);
    return Steinberg::kResultOk;
  }
};
#endif

#if defined(__APPLE__)
class PlugFrame final : public Steinberg::IPlugFrame
{
public:
  Steinberg::tresult queryInterface(const Steinberg::TUID _iid, void** obj) override
  {
    *obj = nullptr;
    return Steinberg::kResultFalse;
  }

  Steinberg::uint32 addRef() override { return 1; }
  Steinberg::uint32 release() override { return 1; }

  QDialog& w;
  WindowContainer wc;
  PlugFrame(QDialog& w, WindowContainer& wc)
      : w{w}
      , wc{wc}
  {
  }

  Steinberg::tresult
  resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override
  {
    wc.setSizeFromVst(*view, *newSize, w);
    return Steinberg::kResultOk;
  }
};
#endif
}
