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

  set_target_properties(${NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

  message(STATUS "Created executable ${NAME} with sources: ${APP_SOURCES}")
endfunction()
