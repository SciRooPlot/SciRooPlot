cmake_minimum_required(VERSION 3.15)

project(
  SciRooPlot
  VERSION 1.2.3
  LANGUAGES CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Please run cmake in a separate build directory.")
endif()

# -----------------------
# Global project settings
# -----------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

include(GNUInstallDirs)
# Match build tree structure to install tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

# -----------------------
# Policies
# -----------------------
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)   # uppercase _ROOT variables
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)   # modern Boost usage
endif()

# -----------------------
# Dependencies
# -----------------------
set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.65)
set(REQUIRED_FMT_VERSION 6.1.2)

find_package(ROOT ${REQUIRED_ROOT_VERSION} CONFIG REQUIRED)
find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED COMPONENTS program_options)
find_package(fmt CONFIG QUIET)
if(NOT (fmt_FOUND AND fmt_VERSION VERSION_GREATER_EQUAL REQUIRED_FMT_VERSION))
  message(STATUS "fmt not found or too old -- fetching it now")
  include(FetchContent)
  FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/releases/download/11.2.0/fmt-11.2.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
  )
  FetchContent_MakeAvailable(fmt)
  set(fmt_VERSION 11.2.0)
endif()

message(STATUS "ROOT  version: ${ROOT_VERSION}")
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "fmt   version: ${fmt_VERSION}")

# -----------------------
# ROOT test file generation
# -----------------------
execute_process(
  COMMAND ${ROOT_EXECUTABLE} -l -q ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generateTestFiles.C
  OUTPUT_VARIABLE root_output # avoids printout
)

# -----------------------
# Common install RPATH
# -----------------------
get_target_property(_root_core_location ROOT::Core LOCATION)
get_filename_component(ROOT_LIBDIR "${_root_core_location}" DIRECTORY)
if(APPLE)
  set(SCIROOPLOT_RPATH
    "@loader_path/../lib"
    "${ROOT_LIBDIR}"
  )
else()
  set(SCIROOPLOT_RPATH
    "$ORIGIN/../lib"
    "${ROOT_LIBDIR}"
  )
endif()

# -----------------------
# GCC < 9 filesystem workaround
# -----------------------
if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  set(FILESYSTEM_LIB stdc++fs)
endif()

# -----------------------
# Library
# -----------------------
include(cmake/CompileFlags.cmake)

add_library(SciRooPlot SHARED
  src/Plot.cxx
  src/PlotManager.cxx
  src/PlotPainter.cxx
  src/Helpers.cxx
)
add_library(SciRooPlot::SciRooPlot ALIAS SciRooPlot)
scirooplot_apply_compile_flags(SciRooPlot)

target_include_directories(SciRooPlot
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(SciRooPlot
  PUBLIC
    ROOT::Gui
    ROOT::Hist
    ROOT::Gpad
    Boost::program_options
    fmt::fmt
    ${FILESYSTEM_LIB}
)

set_target_properties(SciRooPlot PROPERTIES
  INSTALL_RPATH "${SCIROOPLOT_RPATH}"
  BUILD_RPATH "${SCIROOPLOT_RPATH}"
)

# -----------------------
# Executables
# -----------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SciRooPlotHelpers.cmake)

add_plotting_executable(plot
  SOURCES app/PlottingApp.cxx
)

add_plotting_executable(plot-config
  SOURCES app/PlottingAppConfig.cxx
)

# -----------------------
# Install
# -----------------------
include(CMakePackageConfigHelpers)

if(NOT DEFINED NO_INSTALL)
  set(NO_INSTALL ${CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT} CACHE BOOL "Build-only mode (no install)")
endif()
if(NO_INSTALL)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "No install location specified. Defaulting to build directory.")
endif()

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# --- Build-tree config ---
configure_package_config_file(
  cmake/SciRooPlotConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfig.cmake"
  INSTALL_DESTINATION "unused"
)
file(
  COPY "cmake/SciRooPlotHelpers.cmake"
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
export(
  TARGETS SciRooPlot
  NAMESPACE SciRooPlot::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotTargets.cmake"
)
set(SCIROOPLOT_BUILD_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/share/scirooplot)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/user ${CMAKE_CURRENT_BINARY_DIR}/TestFiles
            DESTINATION ${SCIROOPLOT_BUILD_DATA_DIR}
            PATTERN ".*" EXCLUDE)
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/TestFiles)
set(SCIROOPLOT_ROOT "${CMAKE_CURRENT_BINARY_DIR}")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/scirooplot-env.sh.in
  ${SCIROOPLOT_BUILD_DATA_DIR}/scirooplot-env.sh
  @ONLY
)

# --- Install config ---
set(INSTALL_STAGE ${CMAKE_CURRENT_BINARY_DIR}/install_stage)
configure_package_config_file(
  cmake/SciRooPlotConfig.cmake
  "${INSTALL_STAGE}/SciRooPlotConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SciRooPlot
)
install(
  FILES
    "${INSTALL_STAGE}/SciRooPlotConfig.cmake"
    "cmake/SciRooPlotHelpers.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SciRooPlot
)
install(
  EXPORT SciRooPlotTargets
  NAMESPACE SciRooPlot::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SciRooPlot
)
set(SCIROOPLOT_INSTALL_DATA_DIR ${CMAKE_INSTALL_DATAROOTDIR}/scirooplot)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/user ${SCIROOPLOT_BUILD_DATA_DIR}/TestFiles
  DESTINATION ${SCIROOPLOT_INSTALL_DATA_DIR}
  PATTERN ".*" EXCLUDE
)
set(SCIROOPLOT_ROOT "${CMAKE_INSTALL_PREFIX}")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/scirooplot-env.sh.in
  ${INSTALL_STAGE}/scirooplot-env.sh
  @ONLY
)
install(
  FILES ${INSTALL_STAGE}/scirooplot-env.sh
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/scirooplot
)
install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PATTERN ".*" EXCLUDE
)
install(
  TARGETS SciRooPlot plot plot-config
  EXPORT SciRooPlotTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

message(STATUS "---------------------------------------------------------------------------------------------")
message(STATUS "IMPORTANT: To use SciRooPlot you have to put the following line into your .bashrc or .zshrc")
if(NO_INSTALL)
  message(STATUS "source ${SCIROOPLOT_BUILD_DATA_DIR}/scirooplot-env.sh")
else()
  message(STATUS "source ${CMAKE_INSTALL_PREFIX}/${SCIROOPLOT_INSTALL_DATA_DIR}/scirooplot-env.sh")
endif()
message(STATUS "---------------------------------------------------------------------------------------------")
