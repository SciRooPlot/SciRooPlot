# every .py file in a feature folder is a test, labelled with its folder: ctest -L frames runs the frame tests
# (extended/ only with SCIROOPLOT_EXTENDED_TESTS, tools/ never); SRP_TESTS_FULL=1 makes the tests take their full case lists
file(GLOB entries LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(folder IN LISTS entries)
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${folder} OR folder MATCHES "^(tools|__pycache__)$" OR (folder STREQUAL "extended" AND NOT SCIROOPLOT_EXTENDED_TESTS))
    continue()
  endif()
  file(GLOB tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${folder}/*.py)
  foreach(test IN LISTS tests)
    get_filename_component(name ${test} NAME_WE)
    add_test(NAME ${folder}.${name} COMMAND ${Python3_EXECUTABLE} ${test})
    set_tests_properties(${folder}.${name} PROPERTIES
      LABELS ${folder}
      SKIP_RETURN_CODE 77
      ENVIRONMENT "SRP_PREFIX=${CMAKE_BINARY_DIR};SRP_TESTS_WORK=${CMAKE_CURRENT_BINARY_DIR}/work/${folder}.${name}"
      ENVIRONMENT_MODIFICATION "PYTHONPATH=path_list_prepend:${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endforeach()
endforeach()
