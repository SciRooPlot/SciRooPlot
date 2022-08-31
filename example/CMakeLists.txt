cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(MyPlots CXX)

set(PLOTTING_FRAMEWORK_ROOT CACHE STRING "path to PlottingFramework base directory")
if(PLOTTING_FRAMEWORK_ROOT)
  message(STATUS "looking for PlottingFramework in ${PLOTTING_FRAMEWORK_ROOT}.")
elseif (NOT "$ENV{__PLOTTING_FRAMEWORK_DIR}" STREQUAL "")
  set(PLOTTING_FRAMEWORK_ROOT $ENV{__PLOTTING_FRAMEWORK_DIR})
  message(STATUS "Found PlottingFramework: ${PLOTTING_FRAMEWORK_ROOT}")
else()
  message(FATAL_ERROR "Did not find PlottingFramework. Please source the .plotrc file or specify path via -DPLOTTING_FRAMEWORK_ROOT=/path/to/PlottingFramework.")
endif()

list(PREPEND CMAKE_PREFIX_PATH ${PLOTTING_FRAMEWORK_ROOT})
find_package(PlottingFramework CONFIG REQUIRED)

if(NOT EXISTS "${PLOTTING_FRAMEWORK_LIB}")
  message(FATAL_ERROR "Did not find PlottingFramework in ${PLOTTING_FRAMEWORK_ROOT}. Please source the .plotrc file or specify the correct path via -DPLOTTING_FRAMEWORK_ROOT=/path/to/PlottingFramework.")
endif()
message(STATUS "PlottingFramework version: ${PLOTTING_FRAMEWORK_VERSION}")

####################################################################################################
# Function to add an executable that is linked to the PlottingFramework and all its dependencies
####################################################################################################
function(add_plotting_executable APP_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 APP "" "" "SOURCES;ADD_FILES")
  if(APP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Got trailing arguments ${APP_UNPARSED_ARGUMENTS}")
  endif()
  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_ADD_FILES})
  target_link_libraries(${APP_NAME} PUBLIC
    ${PLOTTING_FRAMEWORK_LIB}
    ${PLOTTING_FRAMEWORK_DEPENDENCIES}
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
#include_directories(inc)
####################################################################################################
