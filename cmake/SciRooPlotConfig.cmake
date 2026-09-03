@PACKAGE_INIT@

# Policy for modern Boost usage
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

include(CMakeFindDependencyMacro)

# Dependencies
find_dependency(ROOT @REQUIRED_ROOT_VERSION@)
find_dependency(Boost @REQUIRED_BOOST_VERSION@ COMPONENTS program_options)
find_dependency(fmt @REQUIRED_FMT_VERSION@)

# Include the exported targets (installed by install(EXPORT ...))
include("${CMAKE_CURRENT_LIST_DIR}/SciRooPlotTargets.cmake")

# Include the helper for downstream projects
include("${CMAKE_CURRENT_LIST_DIR}/SciRooPlotHelpers.cmake")

# Link against SciRooPlot using the helper function add_plotting_executable:
#
#  add_plotting_executable(definePlots
#    SOURCES
#      src/DefinePlots.cxx
#    INCLUDES
#      include/
#  )
