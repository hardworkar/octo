#pragma once

#include "platform/x11/opengl_x11.h"
namespace octo {
class Application {
public:
  Application();
  void run();

private:
  OpenGLX11 opengl_x11_;
};
} // namespace octo
