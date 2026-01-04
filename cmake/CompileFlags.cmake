option(DISABLE_COLORS   "no ANSI colors in console" OFF)
option(DARK_COLORS      "darker color scheme" OFF)
option(DISABLE_DEBUG    "disable DEBUG output" OFF)
option(DISABLE_WARNING  "disable WARNING output" OFF)
option(DISABLE_ERROR    "disable ERROR output" OFF)
option(DISABLE_LOG      "disable LOG output" OFF)
option(DISABLE_INFO     "disable INFO output" OFF)
option(DISABLE_PRINT    "disable PRINT output" OFF)

function(scirooplot_apply_compile_flags TARGET)
  foreach(opt
      DISABLE_COLORS
      DARK_COLORS
      DISABLE_DEBUG
      DISABLE_WARNING
      DISABLE_ERROR
      DISABLE_LOG
      DISABLE_INFO
      DISABLE_PRINT
    )
    if(${opt})
      target_compile_definitions(${TARGET} PUBLIC ${opt})
      message(STATUS "Option ${opt} = ON")
    endif()
  endforeach()
endfunction()