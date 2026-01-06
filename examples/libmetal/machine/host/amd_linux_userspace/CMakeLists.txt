# Copyright (C) 2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
include (CheckIncludeFiles)
include (CheckCSourceCompiles)
include (${APPS_ROOT_DIR}/../legacy_apps/cmake/options.cmake)
include (${APPS_ROOT_DIR}/../legacy_apps/cmake/collect.cmake)

get_property (DEMO_CFG_FILE GLOBAL PROPERTY DEMO_CFG_FILE)
if (EXISTS ${DEMO_CFG_FILE})
  include(${DEMO_CFG_FILE})
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h")
  message("WARNING: DEMO_CFG_FILE ${DEMO_CFG_FILE} passed in at configure time. Using this to configure demo.")
endif()

collect (APP_COMMON_SOURCES ipi-uio.c)
collect (APP_COMMON_SOURCES platform_init.c)
collect (APP_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")

set (_elf_name ${DEMO})

collector_list  (_list PROJECT_INC_DIRS)
include_directories (${_list} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_INCLUDE_PATH})

collector_list  (_list PROJECT_LIB_DIRS)
link_directories (${_list})

collect(PROJECT_LIB_DEPS metal)
collector_list (_deps PROJECT_LIB_DEPS)

get_property (_ec_flgs GLOBAL PROPERTY "PROJECT_EC_FLAGS")
set (_app ${DEMO})
collector_list (_src APP_COMMON_SOURCES)

if (WITH_STATIC_LIB)
  add_executable (${_app}-static ${_src})
  if (PROJECT_EC_FLAGS)
    string(REPLACE " " ";" _ec_flgs ${PROJECT_EC_FLAGS})
    target_compile_options (${_app}-static PUBLIC ${_ec_flgs})
  endif (PROJECT_EC_FLAGS)
  target_link_libraries (${_app}-static ${_deps} -L${CMAKE_LIBRARY_PATH})
  install (TARGETS ${_app}-static RUNTIME DESTINATION bin)
endif (WITH_STATIC_LIB)
