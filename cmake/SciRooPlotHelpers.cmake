function(add_plotting_executable NAME)
  cmake_parse_arguments(APP "" "" "SOURCES;INCLUDES" ${ARGN})

  if(NOT APP_SOURCES)
    message(FATAL_ERROR "add_plotting_executable: SOURCES must be provided")
  endif()

  add_executable(${NAME} ${APP_SOURCES})
  target_link_libraries(${NAME} PRIVATE SciRooPlot::SciRooPlot)

  if(APP_INCLUDES)
    target_include_directories(${NAME} PRIVATE ${APP_INCLUDES})
  endif()

  # Set RPATH and includes like the library
  if(TARGET SciRooPlot)
    get_target_property(_rpath SciRooPlot INSTALL_RPATH)
    if(_rpath)
      set_target_properties(${NAME} PROPERTIES 
        INSTALL_RPATH "${_rpath}"
        BUILD_RPATH "${_rpath}"
      )
    endif()
    get_target_property(_incs SciRooPlot INTERFACE_INCLUDE_DIRECTORIES)
    if(_incs)
      target_include_directories(${NAME} PRIVATE ${_incs})
    endif()
  endif()
  
  message(STATUS "Created executable ${NAME} with sources: ${APP_SOURCES}")
endfunction()
