project(PlottingFramework CXX)
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  message(STATUS "activating filesystem workaround for old gcc versions")
  set(CXX_FILESYSTEM_LIBRARIES "stdc++fs")
endif()

option(DISABLE_COLORS "no ANSI colors in console" OFF)
if(DISABLE_COLORS)
  message(STATUS "disabling ANSI colors")
add_compile_definitions(DISABLE_COLORS) # no ANSI colors in console
endif()
option(DARK_COLORS "darker color scheme" OFF)
if(DARK_COLORS)
  add_compile_definitions(DARK_COLORS)
endif()

# The following two lines can be used to force using one specific root installation
set(ROOTSYS data/mhemmer/rootbuild)
list(APPEND CMAKE_PREFIX_PATH ${ROOTSYS})

set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.65)
set(REQUIRED_FMT_VERSION 6.1.2)
set(Boost_USE_MULTITHREADED TRUE)
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif(APPLE)

set(MODULE PlottingFramework)
set(SRCS
  src/Plot.cxx
  src/PlotManager.cxx
  src/PlotPainter.cxx
  src/Helpers.cxx
)
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
string(REPLACE "src" "inc" HDRS "${HDRS}")
set(MODULE_HDR inc/${MODULE}.h)
set(ADDITIONAL_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/Logging.h
  README.md
  TODO.md
)

# find dependencies
find_package(ROOT ${REQUIRED_ROOT_VERSION} CONFIG REQUIRED)
include(${ROOT_USE_FILE})
message(STATUS "root  version: ${ROOT_VERSION}")
find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED COMPONENTS program_options)
message(STATUS "boost version: ${Boost_VERSION}")
find_package(fmt ${REQUIRED_FMT_VERSION} REQUIRED)
message(STATUS "fmt   version: ${fmt_VERSION}")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${Boost_INCLUDE_DIRS}/boost/property_tree
  ${Boost_INCLUDE_DIRS}/boost/program_options
)

# create PlottingFramework library
add_library(${MODULE} SHARED ${SRCS} ${HDRS} ${MODULE_HDR} ${ADDITIONAL_FILES})
target_link_libraries(${MODULE} PUBLIC
  ROOT::Gui
  ROOT::Hist
  ROOT::Gpad
  Boost::program_options
  fmt::fmt
  ${CXX_FILESYSTEM_LIBRARIES}
)

# helper function to add executables that are linked to the PlottingFramework
function(add_plotting_executable APP_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 APP "" "" "SOURCES;ADD_FILES")
  if(APP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Got trailing arguments ${APP_UNPARSED_ARGUMENTS}")
  endif()
  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_ADD_FILES})
  target_link_libraries(${APP_NAME} PlottingFramework)
  message(STATUS "Creating executable ${APP_NAME} from file(s) ${APP_SOURCES}")
endfunction()

# add the command-line plotting app
add_plotting_executable(plot
  SOURCES app/PlottingApp.cxx
)
add_plotting_executable(plot-config
  SOURCES app/PlottingAppConfig.cxx
)
