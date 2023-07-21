#include "application.h"
#include "log.h"
#include "platform/x11/opengl_x11.h"

namespace octo {
Application::Application() {
  OpenGLX11 opengl;

  while (1) {
    opengl.test();
  }
}

void Application::run() {}
} // namespace octo
