cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The following two lines can be used to force using one specific root installation (comment out if not needed)
#set(ROOTSYS ~/alice/sw/osx_x86-64/ROOT/v6-18-04-alice1-7)
#set(ROOTSYS ~/alice/sw/osx_x86-64/ROOT/latest)
set(ROOTSYS ~/root/mybuild)
list(APPEND CMAKE_PREFIX_PATH ${ROOTSYS})

project(PlottingFramework CXX)
set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.68)
set(REQUIRED_FMT_VERSION 6.1.2)
set(Boost_USE_MULTITHREADED TRUE)
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif(APPLE)

# add user applications that should be linked to the framework here
# name of the executable:
set(APPLICATIONS
  plot
  config
  create
)
# corresponding source file:
set(APPLICATIONS_SRC
  "app/PlottingApp.cxx"
  "app/MultDepSpec/ConfigInput.cxx"
  "app/MultDepSpec/CreatePlots.cxx app/MultDepSpec/Templates.cxx app/MultDepSpec/Datasets.cxx app/MultDepSpec/Energies.cxx app/MultDepSpec/Systems.cxx app/MultDepSpec/Publications.cxx app/MultDepSpec/Test.cxx app/MultDepSpec/Publications.cxx app/MultDepSpec/Helpers.cxx app/MultDepSpec/StyleDefinitions.cxx  app/MultDepSpec/TrackQA.cxx"
)
# files that you want to associate to your app put "" if not required
set(APPLICATIONS_ADDITIONAL_FILES
  ""
  ""
  "app/multDepSpec/MultDepSpec.h"
)

set(MODULE PlottingFramework)
set(MODULE_HDR inc/${MODULE}.h)
set(ADDITIONAL_FILES README.md TODO.md)

# this variable contains the framework source code
set(SRCS
  src/Plot.cxx
  src/PlotManager.cxx
  src/PlotPainter.cxx
)
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
string(REPLACE "src" "inc" HDRS "${HDRS}")

# find dependencies
find_package(ROOT ${REQUIRED_ROOT_VERSION} CONFIG REQUIRED)
include(${ROOT_USE_FILE})
message(STATUS "root  version: ${ROOT_VERSION}")
find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED COMPONENTS program_options)
message(STATUS "boost version: ${Boost_VERSION}")
find_package(fmt)
find_package(fmt ${REQUIRED_FMT_VERSION} REQUIRED)
message(STATUS "fmt   version: ${fmt_VERSION}")

# create PlottingFramework library
add_library(${MODULE} SHARED ${SRCS} ${HDRS} ${MODULE_HDR} ${ADDITIONAL_FILES})
target_link_libraries(${MODULE} PUBLIC
  ROOT::Gpad
  Boost::program_options
  fmt::fmt
)
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${Boost_INCLUDE_DIRS}/boost/property_tree
  ${Boost_INCLUDE_DIRS}/boost/program_options
)

# create executables
list(LENGTH APPLICATIONS NUMBER_OF_APPS)
math(EXPR NUMBER_OF_APPS "${NUMBER_OF_APPS} - 1")
foreach(INDEX RANGE ${NUMBER_OF_APPS})
  list(GET APPLICATIONS ${INDEX} APP_NAME)
  list(GET APPLICATIONS_SRC ${INDEX} APP_CODE)
  list(GET APPLICATIONS_ADDITIONAL_FILES ${INDEX} APP_ADD_FILE)
  message(STATUS "Creating executable ${APP_NAME} from file(s) ${APP_CODE} including ${APP_ADD_FILE}")
  string(REPLACE " " ";" APP_ADD_FILE "${APP_ADD_FILE}")
  string(REPLACE " " ";" APP_CODE "${APP_CODE}")
  add_executable(${APP_NAME} ${APP_CODE} ${APP_ADD_FILE})
  target_link_libraries(${APP_NAME} ${MODULE})
endforeach()
