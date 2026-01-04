cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(PROJECT_NAME CXX)

find_package(SciRooPlot CONFIG REQUIRED)
message(STATUS "SciRooPlot version: ${SciRooPlot_VERSION}")

add_plotting_executable(definePlots
  SOURCES DefinePlots.cxx
)

# Link against SciRooPlot using the helper function add_plotting_executable:
#
#  add_plotting_executable(definePlots
#    SOURCES
#      src/DefinePlots.cxx
#      src/SomeOtherFile.cxx
#    INCLUDES
#      include/
#      include2/
#  )