#ifndef INCLUDE_LOGGING_H_
#define INCLUDE_LOGGING_H_

#include <fmt/core.h>
#include <fmt/format.h>
#include <string_view>
#include <memory>

#if defined(__clang__) || defined(__GNUC__)
#include <cxxabi.h>
#endif

namespace logger
{
enum class Color {
  Black,
  Red,
  Green,
  Yellow,
  Blue,
  Magenta,
  Cyan,
  White
};

inline std::string_view begin_color(Color c)
{
#ifdef DISABLE_COLORS
  return "";
#else
#ifdef DARK_COLORS
  constexpr bool bright = false;
#else
  constexpr bool bright = true;
#endif
  switch (c) {
    case Color::Black:
      return bright ? "\033[90m" : "\033[30m";
    case Color::Red:
      return bright ? "\033[91m" : "\033[31m";
    case Color::Green:
      return bright ? "\033[92m" : "\033[32m";
    case Color::Yellow:
      return bright ? "\033[93m" : "\033[33m";
    case Color::Blue:
      return bright ? "\033[94m" : "\033[34m";
    case Color::Magenta:
      return bright ? "\033[95m" : "\033[35m";
    case Color::Cyan:
      return bright ? "\033[96m" : "\033[36m";
    case Color::White:
      return bright ? "\033[97m" : "\033[37m";
    default:
      return "\033[0m";
  }
#endif
}

inline constexpr std::string_view end_color()
{
#ifdef DISABLE_COLORS
  return "";
#else
  return "\033[0m";
#endif
}

template <typename... Args>
inline void print(std::string_view fmt_str, Args&&... args)
{
#ifndef DISABLE_PRINT
  fmt::print(stderr, "       | ");
  fmt::print(fmt::runtime(fmt_str), std::forward<Args>(args)...);
  fmt::print("\n");
#endif
}

template <typename... Args>
inline void print_inline(std::string_view fmt_str, Args&&... args)
{
#ifndef DISABLE_PRINT
  fmt::print(fmt::runtime(fmt_str), std::forward<Args>(args)...);
#endif
}

inline void print_separator()
{
  logger::print("{:-<60}", "-");
}

template <typename... Args>
inline void info(std::string_view fmt_str, Args&&... args)
{
#ifndef DISABLE_INFO
  fmt::print("[ INFO ] ");
  fmt::print(fmt::runtime(fmt_str), std::forward<Args>(args)...);
  fmt::print("\n");
#endif
}

template <typename... Args>
inline void log(std::string_view fmt_str, Args&&... args)
{
#ifndef DISABLE_LOG
  fmt::print("{}[ LOG  ]{} ", begin_color(Color::Green), end_color());
  fmt::print(fmt::runtime(fmt_str), std::forward<Args>(args)...);
  fmt::print("\n");
#endif
}

template <typename... Args>
inline void debug(std::string_view fmt_str, Args&&... args)
{
#ifndef DISABLE_DEBUG
  fmt::print(stderr, "{}[ DEBU ]{} ", begin_color(Color::Cyan), end_color());
  fmt::print(stderr, fmt::runtime(fmt_str), std::forward<Args>(args)...);
  fmt::print(stderr, "\n");
#endif
}

template <typename... Args>
inline void warning(std::string_view fmt_str, Args&&... args)
{
#ifndef DISABLE_WARNING
  fmt::print(stderr, "{}[ WARN ]{} ", begin_color(Color::Yellow), end_color());
  fmt::print(stderr, fmt::runtime(fmt_str), std::forward<Args>(args)...);
  fmt::print(stderr, "\n");
#endif
}

template <typename... Args>
inline void error(std::string_view fmt_str, Args&&... args)
{
#ifndef DISABLE_ERROR
  fmt::print(stderr, "{}[ ERR  ]{} ", begin_color(Color::Red), end_color());
  fmt::print(stderr, fmt::runtime(fmt_str), std::forward<Args>(args)...);
  fmt::print(stderr, "\n");
#endif
}

template <typename T>
inline std::string type_name()
{
#if defined(__clang__) || defined(__GNUC__)
  int status = 0;
  std::unique_ptr<char[], void (*)(void*)> res{
    abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status),
    std::free};
  return status == 0 ? res.get() : typeid(T).name();
#else
  return typeid(T).name();
#endif
}

template <typename T>
inline void check_type(const T&, std::string_view name = "")
{
  fmt::print(stderr, "{}[ TYPE ]{} ", begin_color(Color::Cyan), end_color());
  fmt::print("Variable {}{}has type {}", name, name.empty() ? "" : " ", type_name<T>());
  fmt::print("\n");
}

inline void here(const char* file, const char* function, int line)
{
  fmt::print(stderr, "{}[ HERE ]{} ", begin_color(Color::Cyan), end_color());
  fmt::print("Line {} in function {} ({})\n", line, function, file);
  fmt::print("\n");
}

}  // namespace logger

// shorthand macros for logging features
#define DEBUG(...) logger::debug(__VA_ARGS__)
#define WARNING(...) logger::warning(__VA_ARGS__)
#define ERROR(...) logger::error(__VA_ARGS__)
#define LOG(...) logger::log(__VA_ARGS__)
#define INFO(...) logger::info(__VA_ARGS__)
#define PRINT(...) logger::print(__VA_ARGS__)
#define PRINT_INLINE(...) logger::print_inline(__VA_ARGS__)
#define PRINT_SEPARATOR() logger::print_separator()

// debug helper macros
#define HERE() logger::here(__FILE__, __FUNCTION__, __LINE__);
#define CHECK_TYPE(var) logger::check_type(var, #var);

#endif  // INCLUDE_LOGGING_H_
