/*
********************************************************************************
* --------------------------------- SciRooPlot ---------------------------------
* Copyright (c) 2019-2025 Mario Krüger
* Contact: mario.kruger@cern.ch
* For a full list of contributors please see doc/CONTRIBUTORS.md.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation in version 3 (or later) of the License.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
* The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
*******************************************************************************
*/

#ifndef Logging_h
#define Logging_h

#include <fmt/core.h>

#define _BRIGHTNESS_ "9"
#ifdef DARK_COLORS
#undef _BRIGHTNESS_
#define _BRIGHTNESS_ "3"
#endif

#define BLACK_ "\033[" _BRIGHTNESS_ "0m"
#define WHITE_ "\033[" _BRIGHTNESS_ "7m"
#define BLUE_ "\033[" _BRIGHTNESS_ "4m"
#define GREEN_ "\033[" _BRIGHTNESS_ "2m"
#define MAGENTA_ "\033[" _BRIGHTNESS_ "5m"
#define CYAN_ "\033[" _BRIGHTNESS_ "6m"
#define YELLOW_ "\033[" _BRIGHTNESS_ "3m"
#define RED_ "\033[" _BRIGHTNESS_ "1m"
#define _END "\033[0m"

#ifdef DISABLE_COLORS
#undef BLACK_
#define BLACK_ ""
#undef WHITE_
#define WHITE_ ""
#undef BLUE_
#define BLUE_ ""
#undef GREEN_
#define GREEN_ ""
#undef MAGENTA_
#define MAGENTA_ ""
#undef CYAN_
#define CYAN_ ""
#undef YELLOW_
#define YELLOW_ ""
#undef RED_
#define RED_ ""
#undef _END
#define _END ""
#endif

// some preprocessor macros for logging, printing and debugging
#define DEBUG(s, ...)                              \
  {                                                \
    fmt::print(stderr, CYAN_ "[ DEBU ]" _END " "); \
    fmt::print(stderr, s, ##__VA_ARGS__);          \
    fmt::print(stderr, "\n");                      \
  }
#define WARNING(s, ...)                              \
  {                                                  \
    fmt::print(stderr, YELLOW_ "[ WARN ]" _END " "); \
    fmt::print(stderr, s, ##__VA_ARGS__);            \
    fmt::print(stderr, "\n");                        \
  }
#define ERROR(s, ...)                             \
  {                                               \
    fmt::print(stderr, RED_ "[ ERR  ]" _END " "); \
    fmt::print(stderr, s, ##__VA_ARGS__);         \
    fmt::print(stderr, "\n");                     \
  }
#define LOG(s, ...)                         \
  {                                         \
    fmt::print(GREEN_ "[ LOG  ]" _END " "); \
    fmt::print(s, ##__VA_ARGS__);           \
    fmt::print("\n");                       \
  }
#define INFO(s, ...)              \
  {                               \
    fmt::print("[ INFO ] ");      \
    fmt::print(s, ##__VA_ARGS__); \
    fmt::print("\n");             \
  }
#define PRINT(s, ...)                \
  {                                  \
    fmt::print(stderr, "       | "); \
    fmt::print(s, ##__VA_ARGS__);    \
    fmt::print("\n");                \
  }
#define PRINT_INLINE(s, ...)      \
  {                               \
    fmt::print(s, ##__VA_ARGS__); \
  }
#define PRINT_SEPARATOR                     \
  {                                         \
    PRINT(fmt::format("{:-<{}}", "-", 60)); \
  }
#define HERE                                                                              \
  {                                                                                       \
    fmt::print("[ ---> ] Line {} in function {} ({})", __LINE__, __FUNCTION__, __FILE__); \
    fmt::print("\n");                                                                     \
  }

#ifdef DISABLE_DEBUG
#undef DEBUG
#define DEBUG(s, ...) ;
#endif
#ifdef DISABLE_WARNING
#undef WARNING
#define WARNING(s, ...) ;
#endif
#ifdef DISABLE_ERROR
#undef ERROR
#define ERROR(s, ...) ;
#endif

#ifdef DISABLE_LOG
#undef LOG
#define LOG(s, ...) ;
#endif
#ifdef DISABLE_INFO
#undef INFO
#define INFO(s, ...) ;
#endif
#ifdef DISABLE_PRINT
#undef PRINT
#define PRINT(s, ...) ;
#undef PRINT_INLINE
#define PRINT_INLINE(s, ...) ;
#endif

// helper macro to quickly introspect deduced type of a variable
#define CHECK_TYPE(var)        \
  {                            \
    typename decltype(var)::_; \
  }

#endif /* Logging_h */
