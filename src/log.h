#pragma once
#include <format>
#include <iostream>
#include <string>

namespace octo {
class Log {
public:
  static void info(std::string_view description) {
    std::cout << std::format("INFO {}\n", description);
  }
  static void warn(std::string_view description) {
    std::cerr << std::format("WARN {}\n", description);
  }

private:
};
} // namespace octo
