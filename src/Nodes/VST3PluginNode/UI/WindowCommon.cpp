

#include "PlugFrame.hpp"
#include "Window.hpp"

namespace vst3
{
Window::Window(const Steinberg::IPlugView& view, QWidget* parent)
    : PluginWindow{ctx.app.settings<Media::Settings::Model>().getVstAlwaysOnTop(), parent}
    , m_view{view}
{
//  if(view)
//    throw std::runtime_error("Cannot create UI");

  container = createVstWindowContainer(*this, view);
  show();
}

Window::~Window()
{
  if(container.frame)
  {
    delete container.frame;
    container.frame = nullptr;
  }
}

void Window::resizeEvent(QResizeEvent* event)
{
  Steinberg::IPlugView& view = *m_model.fx.view;
  container.setSizeFromUser(view, event->size(), *this);
  event->accept();
}

void Window::closeEvent(QCloseEvent* event)
{
  QPointer<Window> p(this);
  if(auto view = m_model.fx.view)
    view->removed();
  delete container.frame;
  container.frame = nullptr;

  const_cast<QWidget*&>(m_model.externalUI) = nullptr;
  m_model.externalUIVisible(false);
  if(p)
    QDialog::closeEvent(event);
}
}
