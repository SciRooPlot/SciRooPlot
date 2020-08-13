// Plotting Framework
//
// Copyright (C) 2019-2020  Mario Krüger
// Contact: mario.kruger@cern.ch
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef Logging_h
#define Logging_h

#include <fmt/core.h>

#define DEBUG_LVL 2 // < 2: no debug, < 1: no warnings, < 0 no errors
#define COUT_LVL 2  // < 2: no log,   < 1: no info,     < 0 no print

// some preprocessor macros for logging, printing and debugging
#define DEBUG(s, ...)                                 \
  {                                                   \
    fmt::print(stderr, "\033[1;36m[ DEBU ]\033[0m "); \
    fmt::print(stderr, s, ##__VA_ARGS__);             \
    fmt::print(stderr, "\n");                         \
  }
#define WARNING(s, ...)                               \
  {                                                   \
    fmt::print(stderr, "\033[1;33m[ WARN ]\033[0m "); \
    fmt::print(stderr, s, ##__VA_ARGS__);             \
    fmt::print(stderr, "\n");                         \
  }
#define ERROR(s, ...)                                 \
  {                                                   \
    fmt::print(stderr, "\033[1;31m[ ERR  ]\033[0m "); \
    fmt::print(stderr, s, ##__VA_ARGS__);             \
    fmt::print(stderr, "\n");                         \
  }

#define LOG(s, ...)                           \
  {                                           \
    fmt::print("\033[1;32m[ LOG  ]\033[0m "); \
    fmt::print(s, ##__VA_ARGS__);             \
    fmt::print("\n");                         \
  }
#define INFO(s, ...)                          \
  {                                           \
    fmt::print("\033[1;37m[ INFO ]\033[0m "); \
    fmt::print(s, ##__VA_ARGS__);             \
    fmt::print("\n");                         \
  }

#define PRINT(s, ...)             \
  {                               \
    fmt::print("         ");      \
    fmt::print(s, ##__VA_ARGS__); \
    fmt::print("\n");             \
  }
#define PRINT_INLINE(s, ...)      \
  {                               \
    fmt::print(s, ##__VA_ARGS__); \
  }
#define PRINT_SEPARATOR                     \
  {                                         \
    PRINT(fmt::format("{:-<{}}", "-", 80)); \
  }
#define HERE                                                                                    \
  {                                                                                             \
    fmt::print("\033[1;33m[ ---> ]\033[0m Line {} in function {} ({})", __LINE__, __FUNCTION__, \
               __FILE__);                                                                       \
    fmt::print("\n");                                                                           \
  }

// Debug suppression levels
#if DEBUG_LVL < 2
#undef DEBUG
#define DEBUG(s, ...) ;
#endif
#if DEBUG_LVL < 1
#undef WARNING
#define WARNING(s, ...) ;
#endif
#if DEBUG_LVL < 0
#undef ERROR
#define ERROR(s, ...) ;
#endif

// Output stream suppression levels
#if COUT_LVL < 2
#undef LOG
#define LOG(s, ...) ;
#endif
#if COUT_LVL < 1
#undef INFO
#define INFO(s, ...) ;
#endif
#if COUT_LVL < 0
#undef PRINT
#define PRINT(s, ...) ;
#undef PRINT_INLINE
#define PRINT_INLINE(s, ...) ;
#endif

#endif /* Logging_h */
