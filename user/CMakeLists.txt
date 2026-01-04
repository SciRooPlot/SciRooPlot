cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(PROJECT_NAME CXX)

if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

find_package(SciRooPlot CONFIG REQUIRED)
message(STATUS "SciRooPlot version: ${SciRooPlot_VERSION}")

add_executable(definePlots
  src/DefinePlots.cxx
)
target_link_libraries(definePlots PRIVATE SciRooPlot::SciRooPlot)

# if needed add include dirs
#target_include_directories(definePlots
#    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc
#)