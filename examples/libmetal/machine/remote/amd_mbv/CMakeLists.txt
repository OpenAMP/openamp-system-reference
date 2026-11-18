# Copyright (C) 2026 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
# Bare-metal MicroBlaze-V remote of the irq_shmem_demo. Build against a
# standalone MicroBlaze-V BSP (libxil, libmetal, libmetal_xlnx_extension)
# with the BSP's microblaze_riscv_toolchain.cmake. Linux loads the ELF into
# the BRAM through the BRAM remoteproc driver; it has no resource table.
#
# Everything design-specific comes from two Lopper-generated files, with no
# built-in defaults (see README.md):
#   DEMO_CFG_FILE          shared-memory layout, device names and mailbox
#   LINKER_METADATA_FILE   memories, CODE and DATA regions, stack and heap
# Each is taken from the global property of that name, as the Vitis flow
# sets them, or from a -D variable.

include (${CMAKE_CURRENT_SOURCE_DIR}/build_deps.cmake)

function (amd_mbv_input var)
  get_property (_file GLOBAL PROPERTY ${var})
  if (NOT _file)
    set (_file "${${var}}")
  endif ()
  if (NOT _file OR NOT EXISTS "${_file}")
    message (FATAL_ERROR
      "amd_mbv: ${var} '${_file}' does not exist. Generate it with Lopper "
      "(see ${CMAKE_CURRENT_SOURCE_DIR}/README.md).")
  endif ()
  set (${var} "${_file}" PARENT_SCOPE)
endfunction ()

amd_mbv_input (DEMO_CFG_FILE)
include ("${DEMO_CFG_FILE}")
message (STATUS "amd_mbv: configured from ${DEMO_CFG_FILE}")
foreach (_var BUS_NAME SHM0_DESC_DEV_NAME SHM1_DESC_DEV_NAME SHM_DEV_NAME
	      MBOX_DEV_NAME SHM0_DESC_BASE SHM0_DESC_SIZE SHM1_DESC_BASE
	      SHM1_DESC_SIZE SHM_PAYLOAD_BASE SHM_PAYLOAD_SIZE
	      SHM_PAYLOAD_HALF_SIZE SHM_PAYLOAD_RX_OFFSET SHM_PAYLOAD_TX_OFFSET
	      MBOX_BASE_ADDR MBOX_SIZE)
  if (NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
    message (FATAL_ERROR "amd_mbv: ${DEMO_CFG_FILE} does not set ${_var}")
  endif ()
endforeach ()

# Optional notification latency profiling with the shared AXI Timer the
# configuration names (see README.md). Build the host with it too.
option (MBV_PROFILE "Timestamp mailbox notifications with the shared timer" OFF)
if (MBV_PROFILE AND (NOT DEFINED TIMER_BASE_ADDR OR TIMER_BASE_ADDR STREQUAL ""))
  message (FATAL_ERROR
    "amd_mbv: MBV_PROFILE needs TIMER_BASE_ADDR; name the shared timer in "
    "the libmetal relation (see README.md)")
endif ()

amd_mbv_input (LINKER_METADATA_FILE)
include ("${LINKER_METADATA_FILE}")
set (_linker_script "${CMAKE_CURRENT_BINARY_DIR}/lscript.ld")
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/linker_files/lscript_mb_riscv.ld.in"
		"${_linker_script}")

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
		"${CMAKE_CURRENT_BINARY_DIR}/config.h")

add_subdirectory (system)

collect (APP_COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/platform_init.c)
collector_list (_sources APP_COMMON_SOURCES)
collector_list (_inc_dirs PROJECT_INC_DIRS)
collector_list (_deps PROJECT_LIB_DEPS)

set (_elf_name ${DEMO}.elf)

add_executable (${_elf_name} ${_sources})
# ../.. holds amd_mbv_hw.h, the mailbox register map shared with the Linux
# host.
target_include_directories (${_elf_name} PRIVATE
	${_inc_dirs}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_INCLUDE_PATH})
if (MBV_PROFILE)
  target_compile_definitions (${_elf_name} PRIVATE MBV_PROFILE=1)
endif ()
target_link_directories (${_elf_name} PRIVATE ${CMAKE_LIBRARY_PATH})
target_link_libraries (${_elf_name} PRIVATE
	-Wl,-Map=${DEMO}.map
	-Wl,--gc-sections -T\"${_linker_script}\"
	-Wl,--start-group ${_deps} -Wl,--end-group)
set_target_properties (${_elf_name} PROPERTIES LINK_DEPENDS ${_linker_script})
install (TARGETS ${_elf_name} RUNTIME DESTINATION bin)
