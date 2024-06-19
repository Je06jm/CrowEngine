#ifndef CROW_LOG_HPP
#define CROW_LOG_HPP

#include <format>
#include <iostream>

namespace crow {

#ifdef CROW_DEBUG
template <typename... Args>
void print(const std::format_string<Args...> fmt, Args&&... args) {
    auto str = std::vformat(fmt.get(), std::make_format_args(args...));
    std::cout << "[" << type << "] " << str;
}

template <typename... Args>
void println(const std::format_string<Args...> fmt, Args&&... args) {
    auto str = std::vformat(fmt.get(), std::make_format_args(args...));
    std::cout << "[" << type << "] " << str << std::endl;
}
#else

template <typename... Args>
void print(const std::format_string<Args...> fmt, Args&&... args) {}

template <typename... Args>
void println(const std::format_string<Args...> fmt, Args&&... args) {}

#endif

} // namespace crow

#endif