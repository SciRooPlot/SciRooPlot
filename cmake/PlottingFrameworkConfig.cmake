set(PLOTTING_FRAMEWORK_VERSION 1.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  message(STATUS "activating filesystem workaround for old gcc versions")
  set(CXX_FILESYSTEM_LIBRARIES "stdc++fs")
endif()

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif(APPLE)

option(DISABLE_COLORS "no ANSI colors in console" OFF)
if(DISABLE_COLORS)
  message(STATUS "disabling ANSI colors")
  add_compile_definitions(DISABLE_COLORS)
endif()
option(DARK_COLORS "darker color scheme" OFF)
if(DARK_COLORS)
  add_compile_definitions(DARK_COLORS)
endif()

set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.65)
set(REQUIRED_FMT_VERSION 6.1.2)
set(Boost_USE_MULTITHREADED TRUE)

if(NOT PLOTTING_FRAMEWORK_ROOT)
  set(PLOTTING_FRAMEWORK_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# find dependencies
set(ROOTSYS CACHE STRING "ROOT installation folder")
if(ROOTSYS)
  message(STATUS "looking for ROOT installation in ${ROOTSYS}.")
  list(PREPEND CMAKE_PREFIX_PATH ${ROOTSYS})
endif()
find_package(ROOT ${REQUIRED_ROOT_VERSION} CONFIG REQUIRED)
include(${ROOT_USE_FILE})
message(STATUS "root  version: ${ROOT_VERSION}")

set(BOOST_ROOT CACHE STRING "boost installation folder")
if(BOOST_ROOT)
  message(STATUS "looking for boost installation in ${BOOST_ROOT}.")
  list(PREPEND CMAKE_PREFIX_PATH ${BOOST_ROOT})
endif()
find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED COMPONENTS program_options)
message(STATUS "boost version: ${Boost_VERSION}")

set(FMT_ROOT CACHE STRING "fmt installation folder")
if(FMT_ROOT)
  message(STATUS "looking for fmt installation in ${FMT_ROOT}.")
  list(PREPEND CMAKE_PREFIX_PATH ${FMT_ROOT})
endif()
find_package(fmt ${REQUIRED_FMT_VERSION} REQUIRED)
message(STATUS "fmt   version: ${fmt_VERSION}")

include_directories(
  ${PLOTTING_FRAMEWORK_ROOT}/inc
)

set(PLOTTING_FRAMEWORK_DEPENDENCIES
  ROOT::Gui
  ROOT::Hist
  ROOT::Gpad
  Boost::program_options
  fmt::fmt
  ${CXX_FILESYSTEM_LIBRARIES}
)

# define plotting framework lib
set(PLOTTING_FRAMEWORK_LIB ${PLOTTING_FRAMEWORK_ROOT}/build/libPlottingFramework.so)
if(APPLE)
  string(REPLACE ".so" ".dylib" PLOTTING_FRAMEWORK_LIB "${PLOTTING_FRAMEWORK_LIB}")
endif(APPLE)
