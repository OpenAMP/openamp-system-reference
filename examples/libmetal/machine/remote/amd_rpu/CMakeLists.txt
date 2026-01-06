# Copyright (C) 2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
include(${CMAKE_CURRENT_SOURCE_DIR}/build_deps.cmake)

include_directories(${CMAKE_BINARY_DIR}/include)

get_property (DEMO_CFG_FILE GLOBAL PROPERTY DEMO_CFG_FILE)
if (EXISTS ${DEMO_CFG_FILE})
  include(${DEMO_CFG_FILE})
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h")
  message("WARNING: DEMO_CFG_FILE ${DEMO_CFG_FILE} passed in at configure time. Using this to configure demo.")
endif()

add_subdirectory (system)

set (_elf_name ${DEMO})

set (source_dirs ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/system/${PROJECT_SYSTEM})
link_directories(${source_dirs})
include_directories(${source_dirs})

collector_list (_deps PROJECT_LIB_DEPS)

collector_list (_sources APP_COMMON_SOURCES)
list(APPEND _sources ${CMAKE_CURRENT_SOURCE_DIR}/platform_init.c)

get_property (LINKER_METADATA_FILE GLOBAL PROPERTY LINKER_METADATA_FILE)
if (EXISTS ${LINKER_METADATA_FILE})
  include(${LINKER_METADATA_FILE})

  # map SOC property to right linker .in file
  get_property (SOC GLOBAL PROPERTY SOC)
  set (SOCS VERSAL2 ZYNQMP VERSAL_NET VERSAL)
  set (LOPPER_CFG_LINKERS lscript_r52_generic.ld.in  lscript_r5.ld.in  lscript_versal_net_r5.ld.in  lscript_versal_r5.ld.in)
  list(FIND SOCS "${SOC}" soc_index)
  list (GET LOPPER_CFG_LINKERS ${soc_index} linker_in)

  # generate linker script
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/linker_files/${linker_in} "${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld")
  list(APPEND LINKER_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${linker_in})
  list(APPEND LINKER_FILE ${linker_in})
  set (_linker_script "${CMAKE_CURRENT_SOURCE_DIR}/lscript.ld")
  message("Using LINKER_METADATA_FILE: ${LINKER_METADATA_FILE}")
else()
  set (_linker_script ${CMAKE_CURRENT_SOURCE_DIR}/lscript_sdt.ld)
endif(EXISTS ${LINKER_METADATA_FILE})

add_executable(${_elf_name}.elf ${_sources})

# Add library search paths properly
target_link_directories(${_elf_name}.elf PRIVATE
  ${CMAKE_LIBRARY_PATH}
  ${USER_LINK_DIRECTORIES}
)

target_link_libraries(${_elf_name}.elf PRIVATE
  -Wl,-Map=${_elf_name}.map
  -Wl,--gc-sections -T\"${_linker_script}\"
  -Wl,--start-group ${_deps} -Wl,--end-group)

target_compile_definitions(${_elf_name}.elf PUBLIC ${USER_COMPILE_DEFINITIONS})
target_include_directories(${_elf_name}.elf PUBLIC ${USER_INCLUDE_DIRECTORIES})
install (TARGETS ${_elf_name}.elf RUNTIME DESTINATION bin)
