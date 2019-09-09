cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(PlottingFramework CXX)
set(CMAKE_BUILD_TYPE Debug)

set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.68)

set(MODULE PlottingFramework)
# add source files of classes
set(SRCS
  PlotManager.cxx
  PlotGenerator.cxx
  PlotStyle.cxx
  Plot.cxx
)
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")

# do ROOT magic
set(ROOTCOMPONENTS Core RIO Hist Matrix)
find_package(ROOT ${REQUIRED_ROOT_VERSION} REQUIRED COMPONENTS ${ROOTCOMPONENTS})
include(${ROOT_USE_FILE})
ROOT_GENERATE_DICTIONARY(G__${MODULE} ${HDR} LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/LinkDef.h)
add_library(${MODULE} SHARED ${SRCS} G__${MODULE}.cxx)
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,${ROOTSYS}/lib")
#target_include_directories(${MODULE} PUBLIC ${ROOT_INCLUDE_DIRS})
target_link_libraries(${MODULE} ${ROOT_LIBRARIES})

find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED)
target_link_libraries(${MODULE} Boost::boost)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}/boost/property_tree)

# create executable
add_executable(plot CreatePlots.cxx)
target_link_libraries(plot ${MODULE})

#link_directories(${CMAKE_SOURCE_DIR}/)
#install(TARGETS plot ${MODULE}
#  CONFIGURATIONS Debug Release RelWithDebInfo MinSizeRel
#  LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/BUILD/
#  RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/BUILD/
#)
#install(FILES lib${MODULE}_rdict.pcm lib${MODULE}.rootmap
#  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/BUILD/
#)
