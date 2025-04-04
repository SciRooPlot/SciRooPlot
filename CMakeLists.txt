cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_BUILD_TYPE Debug)

project(SciRooPlot CXX)
set(SciRooPlot_VERSION 1.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  message(STATUS "activating filesystem workaround for old gcc versions")
  set(CXX_FILESYSTEM_LIBRARIES "stdc++fs")
endif()

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif(APPLE)

set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.65)
set(REQUIRED_FMT_VERSION 6.1.2)
set(Boost_USE_MULTITHREADED TRUE)

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

set(SciRooPlot_COMFLAGS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompileFlags.cmake)
include(${SciRooPlot_COMFLAGS})

set(SciRooPlot_INC ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${SciRooPlot_INC})

set(SciRooPlot_DEP
  ROOT::Gui
  ROOT::Hist
  ROOT::Gpad
  Boost::program_options
  fmt::fmt
  ${CXX_FILESYSTEM_LIBRARIES}
)

# define plotting framework lib
set(SciRooPlot_LIB ${CMAKE_CURRENT_BINARY_DIR}/libSciRooPlot.so)
if(APPLE)
  string(REPLACE ".so" ".dylib" SciRooPlot_LIB "${SciRooPlot_LIB}")
endif(APPLE)


set(MODULE SciRooPlot)
set(SRCS
  src/Plot.cxx
  src/PlotManager.cxx
  src/PlotPainter.cxx
  src/Helpers.cxx
)
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
string(REPLACE "src" "include" HDRS "${HDRS}")
set(MODULE_HDR include/${MODULE}.h)
set(ADDITIONAL_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Logging.h
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SciRooPlotConfig.cmake
  README.md
  docs/README.md
  docs/DEPENDENCIES_INSTALL.md
)

# create SciRooPlot library
add_library(${MODULE} SHARED ${SRCS} ${HDRS} ${MODULE_HDR} ${ADDITIONAL_FILES})
target_link_libraries(${MODULE} PUBLIC ${SciRooPlot_DEP})

# helper function to add executables that are linked to the SciRooPlot library and its dependencies
function(add_plotting_executable APP_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 APP "" "" "SOURCES;ADD_FILES")
  if(APP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Got trailing arguments ${APP_UNPARSED_ARGUMENTS}")
  endif()
  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_ADD_FILES})
  target_link_libraries(${APP_NAME} SciRooPlot ${SciRooPlot_DEP})
  message(STATUS "Creating executable ${APP_NAME} from file(s) ${APP_SOURCES}")
endfunction()

# add the command-line plotting app
add_plotting_executable(plot
  SOURCES app/PlottingApp.cxx
)
add_plotting_executable(plot-config
  SOURCES app/PlottingAppConfig.cxx
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/SciRooPlotConfig.cmake" ${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfig.cmake @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfigVersion.cmake"
  VERSION ${SciRooPlot_VERSION}
  COMPATIBILITY AnyNewerVersion
)
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
export(PACKAGE SciRooPlot)

# generate some root files to play with
execute_process (COMMAND ${ROOT_EXECUTABLE} -l -q ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generateTestFiles.C)
