cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_BUILD_TYPE Debug)

project(PlottingFramework CXX)
include(cmake/PlottingFrameworkConfig.cmake)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlottingFrameworkConfig.cmake
  README.md
  TODO.md
)

# create PlottingFramework library
add_library(${MODULE} SHARED ${SRCS} ${HDRS} ${MODULE_HDR} ${ADDITIONAL_FILES})
target_link_libraries(${MODULE} PUBLIC ${PLOTTING_FRAMEWORK_DEPENDENCIES})

# helper function to add executables that are linked to the PlottingFramework
function(add_plotting_executable APP_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 APP "" "" "SOURCES;ADD_FILES")
  if(APP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Got trailing arguments ${APP_UNPARSED_ARGUMENTS}")
  endif()
  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_ADD_FILES})
  target_link_libraries(${APP_NAME} PlottingFramework ${PLOTTING_FRAMEWORK_DEPENDENCIES})
  message(STATUS "Creating executable ${APP_NAME} from file(s) ${APP_SOURCES}")
endfunction()

# add the command-line plotting app
add_plotting_executable(plot
  SOURCES app/PlottingApp.cxx
)
add_plotting_executable(plot-config
  SOURCES app/PlottingAppConfig.cxx
)
