#include "application.h"
#include "platform/x11/opengl_x11.h"
#include <GL/glx.h>

Application::Application() {
  OpenGLX11 opengl;

  while (1) {
    opengl.test();
  }
}

void Application::run() {}
