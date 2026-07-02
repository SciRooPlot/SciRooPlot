cmake_minimum_required(VERSION 3.15)

project(
  SciRooPlot
  VERSION 2.0.0
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

# get compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# needed for when fetching and building fmt on the fly
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


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

include(cmake/CompileFlags.cmake)

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
# Python bindings
# -----------------------
if(BUILD_PYTHON_BINDINGS)
  find_package(Python3 COMPONENTS Interpreter Development)
  if(NOT Python3_FOUND)
    message(WARNING "Python not found -- disabling bindings")
    set(BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)
  else()
    message(STATUS "Python version: ${Python3_VERSION}")
    message(STATUS "${Python3_EXECUTABLE}")
    set(PYBIND11_FINDPYTHON NEW)
    find_package(pybind11 CONFIG QUIET)
    if(NOT pybind11_FOUND)
      message(STATUS "pybind11 not found -- fetching it now")
      include(FetchContent)
      FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v3.0.1
      )
      FetchContent_MakeAvailable(pybind11)
      if(NOT TARGET pybind11::module)
        message(WARNING "pybind11 not fetched -- disabling bindings")
        set(BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)
      endif()
    endif()
  endif()
endif()

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
add_library(SciRooPlot SHARED
  src/Plot.cpp
  src/PlotManager.cpp
  src/PlotPainter.cpp
  src/Helpers.cpp
  include/Plot.h
  include/PlotManager.h
  include/PlotPainter.h
  include/PlotPainter.h
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
    ROOT::ROOTDataFrame
    Boost::program_options
    fmt::fmt
    ${FILESYSTEM_LIB}
)

set_target_properties(SciRooPlot PROPERTIES
  INSTALL_RPATH "${SCIROOPLOT_RPATH}"
  BUILD_RPATH "${SCIROOPLOT_RPATH}"
)
set(ALL_TARGETS SciRooPlot)

if(BUILD_PYTHON_BINDINGS)
  pybind11_add_module(SciRooPlotPython
    src/PythonBindings.cpp
  )
  target_link_libraries(SciRooPlotPython PRIVATE SciRooPlot)
  target_compile_definitions(SciRooPlotPython PRIVATE
    SCIROOPLOT_ROOT_PYTHON_DIR="${ROOT_LIBRARY_DIR}"
  )
  set_target_properties(SciRooPlotPython PROPERTIES
    BUILD_RPATH "${SCIROOPLOT_RPATH}"
    INSTALL_RPATH "${SCIROOPLOT_RPATH}"
    OUTPUT_NAME "SciRooPlot"
  )
  list(APPEND ALL_TARGETS SciRooPlotPython)
endif()

# -----------------------
# Executables
# -----------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SciRooPlotHelpers.cmake)

add_plotting_executable(srp
  SOURCES app/SciRooPlotApp.cpp
)
add_plotting_executable(plot
  SOURCES app/PlotApp.cpp
)
list(APPEND ALL_TARGETS srp plot)

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
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/user
            DESTINATION ${SCIROOPLOT_BUILD_DATA_DIR}
            PATTERN ".*" EXCLUDE)

add_custom_command(
  OUTPUT ${SCIROOPLOT_BUILD_DATA_DIR}/TestFiles/.generated
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SCIROOPLOT_BUILD_DATA_DIR}
  COMMAND ${ROOT_EXECUTABLE} -l -q -b
          ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generateTestFiles.C
  COMMAND ${CMAKE_COMMAND} -E touch
          ${SCIROOPLOT_BUILD_DATA_DIR}/TestFiles/.generated
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generateTestFiles.C
  WORKING_DIRECTORY ${SCIROOPLOT_BUILD_DATA_DIR}
  COMMENT "Generating ROOT test files"
)
add_custom_target(generate_testfiles ALL
  DEPENDS ${SCIROOPLOT_BUILD_DATA_DIR}/TestFiles/.generated
)
add_dependencies(srp generate_testfiles)

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
  TARGETS ${ALL_TARGETS}
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
