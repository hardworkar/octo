#include "application.h"
#include "log.h"
#include "platform/x11/opengl_x11.h"
#include <stdexcept>

namespace octo {
Application::Application() {
  try {
    OpenGLX11 opengl;
    while (1) {
      opengl.test();
    }
  } catch (const std::runtime_error &e) {
    log(log_level::Error, e.what());
  }
}

void Application::run() {}
} // namespace octo
