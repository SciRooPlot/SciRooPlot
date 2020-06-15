project(MyPlots CXX)
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif(APPLE)


if (NOT "$ENV{__PLOTTING_FRAMEWORK_DIR}" STREQUAL "")
  set(PLOTTING_FRAMEWORK $ENV{__PLOTTING_FRAMEWORK_DIR})
  message(STATUS "Found PlottingFramework: ${PLOTTING_FRAMEWORK}")
else()
  message(FATAL_ERROR "Did not find PlottingFramework. Please source the .plotrc file!")
endif()
include_directories(
  ${PLOTTING_FRAMEWORK}/inc
)

# change this part according to your needs
set(APP_NAME ex)
set(SRCS
  Example.cxx
)

# use the following line when using this from somewhere else
add_subdirectory(${PLOTTING_FRAMEWORK} ${PLOTTING_FRAMEWORK}/build)


add_executable(${APP_NAME} ${SRCS})
target_link_libraries(${APP_NAME} PlottingFramework)
