if ("${PROJECT_VENDOR}" STREQUAL "xlnx")
  # today in xilinx-amd vitis unified ide there is limitation on setting
  # the applications target marchine. set this explicitly here instead.
  GET_PROPERTY (MACHINE GLOBAL PROPERTY MACHINE)
  set (PROJECT_MACHINE ${MACHINE})
endif("${PROJECT_VENDOR}" STREQUAL "xlnx")


if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_MACHINE}/CMakeLists.txt")
  add_subdirectory (${PROJECT_MACHINE})
endif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_MACHINE}/CMakeLists.txt")
