cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(PlottingFramework CXX)
set(CMAKE_BUILD_TYPE Debug)

set(REQUIRED_ROOT_VERSION 6.16)
set(REQUIRED_BOOST_VERSION 1.68)

set(MODULE PlottingFramework)

# add source files of classes here
set(SRCS
  src/PlotManager.cxx
  src/PlotGenerator.cxx
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
find_package(Boost ${REQUIRED_BOOST_VERSION} REQUIRED)

add_library(${MODULE} SHARED ${SRCS} ${HDRS} G__${MODULE}.cxx)
target_link_libraries(${MODULE} ${ROOT_LIBRARIES} Boost::boost)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}/boost/property_tree)

# create executables
add_executable(plot src/PlottingApp.cxx ${SRCS} ${HDRS} include/PlottingFramework.h)
target_link_libraries(plot ${MODULE})

add_executable(create src/CreatePlots.cxx ${SRCS} ${HDRS} include/PlottingFramework.h)
target_link_libraries(create ${MODULE})

# install ROOT pre-compiled headers and rootmap
ROOT_GET_LIBRARY_OUTPUT_DIR(library_output_dir)
ROOT_GET_INSTALL_DIR(shared_lib_install_dir)
install(FILES ${library_output_dir}/lib${MODULE}_rdict.pcm ${library_output_dir}/lib${MODULE}.rootmap
        DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
