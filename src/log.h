#pragma once
#include <algorithm>
#include <format>
#include <iostream>
#include <iterator>
#include <random>
#include <source_location>
#include <string_view>

namespace octo {
enum class log_level : char { Info = 'I', Warning = 'W', Error = 'E' };

inline std::string get_random_face(bool happy) {
  static std::vector<std::string> happies{"😸", "😺", "😹", "😻"};
  static std::vector<std::string> saddies{"😓", "🤬", "🤢", "🤡"};
  auto sample = [](const std::vector<std::string> &data) {
    std::vector<std::string> out;
    std::sample(data.begin(), data.end(), std::back_inserter(out), 1,
                std::mt19937{std::random_device{}()});
    return out[0];
  };
  return sample(happy ? happies : saddies);
}

inline void
log(const log_level level, const std::string_view message,
    const std::source_location location = std::source_location::current()) {
  std::clog << get_random_face(level == log_level::Info) << " "
            << location.file_name() << '(' << location.line() << ':'
            << location.column() << ") `" << location.function_name()
            << "`: " << message << "\n";
}

} // namespace octo
