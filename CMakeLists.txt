cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(PlottingProject CXX)
set(CMAKE_BUILD_TYPE Debug)

#set(ROOTSYS /usr/local/Cellar/root/6.14.06/)
set(ROOTSYS /usr/local/Cellar/root/6.16.00/)
list(APPEND CMAKE_PREFIX_PATH ${ROOTSYS})
set(ROOTCOMPONENTS Core RIO Hist Matrix)

set(MODULE PlottingFramework)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# add source files of classes
set(SRCS
  PlottingFramework.cxx
  Plot.cxx
)
string(REPLACE ".cxx" ".h" HDRS "${SRCS}")

# do ROOT magic
find_package(ROOT REQUIRED COMPONENTS ${ROOTCOMPONENTS})
include(${ROOT_USE_FILE})
ROOT_GENERATE_DICTIONARY(G__${MODULE} ${HDRS} LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/LinkDef.h)
add_library(${MODULE} SHARED ${SRCS} G__${MODULE}.cxx)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,${ROOTSYS}/lib")


# include ROOT libraries
target_include_directories(${MODULE} PUBLIC ${ROOT_INCLUDE_DIRS})
message(STATUS "Adding ROOT include directories: ${ROOT_INCLUDE_DIRS}")
target_link_libraries(${MODULE} ${ROOT_LIBRARIES})
message(STATUS "Loding ROOT libraries: ${ROOT_LIBRARIES}")


find_package(Boost REQUIRED)
target_link_libraries(${MODULE} Boost::boost)


# create executable
add_executable(plot CreatePlots.cxx)
target_link_libraries(plot PlottingFramework)
