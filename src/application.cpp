#include "application.h"
#include "log.h"
#include "platform/x11/opengl_x11.h"
#include <stdexcept>

namespace octo {
Application::Application() {}

void Application::run() try {
  while (1) {
    opengl_x11_.test();
  }
} catch (const std::runtime_error &e) {
  log(log_level::Error, e.what());
}
} // namespace octo
