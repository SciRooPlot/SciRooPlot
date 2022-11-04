cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(MyPlots CXX)

set(SCIROOPLOT_ROOT CACHE STRING "path to SciRooPlot base directory")
if(SCIROOPLOT_ROOT)
  message(STATUS "looking for SciRooPlot in ${SCIROOPLOT_ROOT}.")
elseif (NOT "$ENV{__SCIROOPLOT_DIR}" STREQUAL "")
  set(SCIROOPLOT_ROOT $ENV{__SCIROOPLOT_DIR})
  message(STATUS "Found SciRooPlot: ${SCIROOPLOT_ROOT}")
else()
  message(FATAL_ERROR "Did not find SciRooPlot. Please source the .plotrc file or specify path via -DSCIROOPLOT_ROOT=/path/to/SciRooPlot.")
endif()

list(PREPEND CMAKE_PREFIX_PATH ${SCIROOPLOT_ROOT})
find_package(SciRooPlot CONFIG REQUIRED)

if(NOT EXISTS "${SCIROOPLOT_LIB}")
  message(FATAL_ERROR "Did not find SciRooPlot in ${SCIROOPLOT_ROOT}. Please source the .plotrc file or specify the correct path via -DSCIROOPLOT_ROOT=/path/to/SciRooPlot.")
endif()
message(STATUS "SciRooPlot version: ${SCIROOPLOT_VERSION}")

####################################################################################################
# Function to add an executable that is linked to the SciRooPlot and all its dependencies
####################################################################################################
function(add_plotting_executable APP_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 APP "" "" "SOURCES;ADD_FILES")
  if(APP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Got trailing arguments ${APP_UNPARSED_ARGUMENTS}")
  endif()
  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_ADD_FILES})
  target_link_libraries(${APP_NAME} PUBLIC
    ${SCIROOPLOT_LIB}
    ${SCIROOPLOT_DEPENDENCIES}
  )
  message(STATUS "Creating executable ${APP_NAME} from file(s) ${APP_SOURCES} including ${APP_ADD_FILE}")
endfunction()

####################################################################################################
# Add plotting executables:
####################################################################################################
# Here you can add as many plotting programs as you need
add_plotting_executable(create
  SOURCES CreatePlots.cxx
)
# and add your include directories / files
#include_directories(include)
####################################################################################################
