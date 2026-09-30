@PACKAGE_INIT@

# Policy for modern Boost usage
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Policy for upper-cased <PackageName>_ROOT variables (e.g. ROOT_ROOT, BOOST_ROOT, FMT_ROOT)
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

include(CMakeFindDependencyMacro)

# Dependencies (the locations used to build SciRooPlot serve as hints, so they need not be on CMAKE_PREFIX_PATH)
find_dependency(ROOT @REQUIRED_ROOT_VERSION@ HINTS "@ROOT_DIR@")
find_dependency(Boost @REQUIRED_BOOST_VERSION@ COMPONENTS program_options HINTS "@Boost_DIR@")
find_dependency(fmt @REQUIRED_FMT_VERSION@ HINTS "@fmt_DIR@")

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
