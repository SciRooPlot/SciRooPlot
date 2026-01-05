cmake_minimum_required(VERSION 3.15)

project(
  SciRooPlot
  VERSION 1.2.0
  LANGUAGES CXX
)

# -----------------------
# Global project settings
# -----------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

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
  COMMAND ${ROOT_EXECUTABLE} -l -q
          ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generateTestFiles.C
)

# -----------------------
# Extract ROOT library directory
# -----------------------
get_target_property(_root_core_location ROOT::Core LOCATION)
get_filename_component(ROOT_LIBDIR "${_root_core_location}" DIRECTORY)

# -----------------------
# GCC < 9 filesystem workaround
# -----------------------
if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  set(FILESYSTEM_LIB stdc++fs)
endif()

# -----------------------
# Common install RPATH
# -----------------------
if(APPLE)
  set(SCIROOPLOT_INSTALL_RPATH
    "@loader_path/../lib"
    "${ROOT_LIBDIR}"
  )
else()
  set(SCIROOPLOT_INSTALL_RPATH
    "$ORIGIN/../lib"
    "${ROOT_LIBDIR}"
  )
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
  INSTALL_RPATH "${SCIROOPLOT_INSTALL_RPATH}"
)

# -----------------------
# Executables
# -----------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SciRooPlotHelpers.cmake)

function(add_plotting_executable name)
  cmake_parse_arguments(APP "" "" "SOURCES" ${ARGN})

  add_executable(${name} ${APP_SOURCES})
  target_link_libraries(${name} PRIVATE SciRooPlot::SciRooPlot)
  
  set_target_properties(${name} PROPERTIES
    INSTALL_RPATH "${SCIROOPLOT_INSTALL_RPATH}"
  )  
endfunction()

add_plotting_executable(plot
  SOURCES app/PlottingApp.cxx
)

add_plotting_executable(plot-config
  SOURCES app/PlottingAppConfig.cxx
)

# -----------------------
# Install
# -----------------------
include(GNUInstallDirs)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${CMAKE_CURRENT_BINARY_DIR}/install")
  message(STATUS "No install location specified. Defaulting to 'install' folder in build directory.")
endif()
message(STATUS "Install location prefix: ${CMAKE_INSTALL_PREFIX}")

install(
  TARGETS SciRooPlot plot plot-config
  EXPORT SciRooPlotTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PATTERN ".*" EXCLUDE
)

# -----------------------
# Package config
# -----------------------
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
  cmake/SciRooPlotConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SciRooPlot
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfig.cmake"
    "cmake/SciRooPlotHelpers.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SciRooPlotConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SciRooPlot
)

install(
  EXPORT SciRooPlotTargets
  NAMESPACE SciRooPlot::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SciRooPlot
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/user
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/scirooplot
  PATTERN ".*" EXCLUDE
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TestFiles
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/scirooplot
  PATTERN ".*" EXCLUDE
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/scirooplot-env.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/scirooplot-env.sh
  @ONLY
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/scirooplot-env.sh
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/scirooplot
)

message(STATUS "-----------------------------------------------------------------------------------------")
message(STATUS "IMPORTANT: run the following line to use SciRooPlot (e.g. add it to .bashrc)")
message(STATUS "source ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/scirooplot/scirooplot-env.sh")
message(STATUS "----------------------------------------------------------------------------------------")
