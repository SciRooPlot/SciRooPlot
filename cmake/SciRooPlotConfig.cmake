include(CMakeFindDependencyMacro)
find_dependency(ROOT)
find_dependency(Boost COMPONENTS program_options)
find_dependency(fmt)

include(@SciRooPlot_COMFLAGS@)

set(SciRooPlot_DEP @SciRooPlot_DEP@)
set(SciRooPlot_INC @SciRooPlot_INC@)
set(SciRooPlot_LIB @SciRooPlot_LIB@)
# find_package also automatically defines SciRooPlot_VERSION and SciRooPlot_DIR

include_directories(${SciRooPlot_INC})

####################################################################################################
# Function to add an executable that is linked to the SciRooPlot and all its dependencies
####################################################################################################
function(add_plotting_executable APP_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 APP "" "" "SOURCES;ADD_FILES")
  if(APP_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Got trailing arguments ${APP_UNPARSED_ARGUMENTS}")
  endif()
  add_executable(${APP_NAME} ${APP_SOURCES} ${APP_ADD_FILES})
  target_link_libraries(${APP_NAME} PUBLIC
    ${SciRooPlot_LIB}
    ${SciRooPlot_DEP}
  )
  message(STATUS "Creating executable ${APP_NAME} from file(s) ${APP_SOURCES} including ${APP_ADD_FILE}")
endfunction()
####################################################################################################
