#pragma once
#include <format>
#include <iostream>
#include <source_location>
#include <string_view>

namespace octo {
enum class log_level : char { Info = 'I', Warning = 'W', Error = 'E' };
inline void
log(const log_level level, const std::string_view message,
    const std::source_location location = std::source_location::current()) {
  std::clog << "file: " << location.file_name() << '(' << location.line() << ':'
            << location.column() << ") `" << location.function_name()
            << "`: " << message << '\n';
}
} // namespace octo
