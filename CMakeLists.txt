cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_BUILD_TYPE Debug)

project(PlottingFramework CXX)
set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.68)
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
  "apps/PlottingApp.cxx"
  "apps/MultDepSpec/ConfigInput.cxx"
  "apps/MultDepSpec/CreatePlots.cxx apps/MultDepSpec/Datasets.cxx apps/MultDepSpec/Energies.cxx apps/MultDepSpec/Systems.cxx apps/MultDepSpec/Publications.cxx apps/MultDepSpec/Test.cxx apps/MultDepSpec/Publications.cxx apps/MultDepSpec/Helpers.cxx"
)
# files that you want to associate to your app put "" if not required
set(APPLICATIONS_ADDITIONAL_FILES
  ""
  ""
  "apps/multDepSpec/MultDepSpec.h"
)

set(MODULE PlottingFramework)
set(MODULE_HDR include/${MODULE}.h)
set(ADDITIONAL_FILES README.md TODO.md)

# this variable contains the framework source code
set(SRCS
  src/PlotManager.cxx
  src/PlottingTools.cxx
  src/PlotStyle.cxx
  src/Plot.cxx
)
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")
string(REPLACE "src" "include" HDRS "${HDRS}")

# find ROOT
set(ROOTCOMPONENTS Core RIO Hist Matrix)
find_package(ROOT ${REQUIRED_ROOT_VERSION} REQUIRED COMPONENTS ${ROOTCOMPONENTS})
include(${ROOT_USE_FILE})
ROOT_GENERATE_DICTIONARY(G__${MODULE} ${HDR} LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/LinkDef.h)
# find BOOST
set(Boost_USE_MULTITHREADED TRUE) # this required for some reason
set(BOOSTCOMPONENTS program_options)
find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED COMPONENTS ${BOOSTCOMPONENTS})
#find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED)

add_library(${MODULE} SHARED ${SRCS} ${HDRS} ${MODULE_HDR} ${ADDITIONAL_FILES})
target_link_libraries(${MODULE} ${ROOT_LIBRARIES} ${Boost_LIBRARIES} Boost::boost)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}/boost/property_tree ${Boost_INCLUDE_DIRS}/boost/program_options)


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

# install ROOT pre-compiled headers and rootmap
ROOT_GET_LIBRARY_OUTPUT_DIR(library_output_dir)
ROOT_GET_INSTALL_DIR(shared_lib_install_dir)
install(FILES ${library_output_dir}/lib${MODULE}_rdict.pcm ${library_output_dir}/lib${MODULE}.rootmap
        DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
