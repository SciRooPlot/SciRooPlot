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

################################################################################
# User Settings:
################################################################################
# Change this part according to your needs...
set(APP_NAME example)
set(SRCS
  Example.cxx
)
set(IS_FRAMEWORK_DEV OFF)
################################################################################


if (IS_FRAMEWORK_DEV)
  ################################################################################
  # Developer Mode:
  ################################################################################
  # This does not work if the folder lives in the framework hierarchy
  add_subdirectory(${PLOTTING_FRAMEWORK} ${PLOTTING_FRAMEWORK}/build)
  add_executable(${APP_NAME} ${SRCS})
  target_link_libraries(${APP_NAME} PlottingFramework)
else()
  ################################################################################
  # User Mode:
  ################################################################################
  # find dependencies
  find_package(ROOT ${REQUIRED_ROOT_VERSION} CONFIG REQUIRED)
  include(${ROOT_USE_FILE})
  message(STATUS "root  version: ${ROOT_VERSION}")
  find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED COMPONENTS program_options)
  message(STATUS "boost version: ${Boost_VERSION}")
  find_package(fmt)
  find_package(fmt ${REQUIRED_FMT_VERSION} REQUIRED)
  message(STATUS "fmt   version: ${fmt_VERSION}")

  # add include paths
  include_directories(
    ${PLOTTING_FRAMEWORK}/inc
    ${Boost_INCLUDE_DIRS}/boost/property_tree
    ${Boost_INCLUDE_DIRS}/boost/program_options
  )

  # add plotting framework lib
  set(PLOTTING_FRAMEWORK_LIB ${PLOTTING_FRAMEWORK}/build/libPlottingFramework.so)
  if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    string(REPLACE ".so" ".dylib" PLOTTING_FRAMEWORK_LIB "${PLOTTING_FRAMEWORK_LIB}")
  endif(APPLE)

  # create and link executable
  add_executable(${APP_NAME} ${SRCS})
  target_link_libraries(${APP_NAME} PUBLIC
    ${PLOTTING_FRAMEWORK_LIB}
    ROOT::Gui
    ROOT::Hist
    ROOT::Gpad
    Boost::program_options
    fmt::fmt
  )
endif()
