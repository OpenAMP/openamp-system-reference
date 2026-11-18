# Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause

collect(APP_COMMON_SOURCES platform_info.c)
collect(APP_COMMON_SOURCES rsc_table.c)
collect(APP_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")

find_path(XIL_INCLUDE_DIR NAMES xparameters.h PATHS ${CMAKE_FIND_ROOT_PATH})
collect(PROJECT_INC_DIRS "${XIL_INCLUDE_DIR}")
collect(PROJECT_LIB_DIRS ${CMAKE_LIBRARY_PATH})

collect(PROJECT_LIB_DEPS metal_xlnx_extension)
collect(PROJECT_LIB_DEPS xiltimer)
collect(PROJECT_LIB_DEPS xil)
collect(PROJECT_LIB_DEPS xilstandalone)
collect(PROJECT_LIB_DEPS c)
collect(PROJECT_LIB_DEPS m)

# The IPC layout comes only from the Lopper-generated amd_platform_info.h,
# here or at MBV32_PLATFORM_INFO. See README.md.
set(MBV32_PLATFORM_INFO "${CMAKE_CURRENT_SOURCE_DIR}/amd_platform_info.h"
    CACHE FILEPATH "Lopper-generated amd_platform_info.h")
if(NOT EXISTS "${MBV32_PLATFORM_INFO}")
  message(FATAL_ERROR
    "mbv32: ${MBV32_PLATFORM_INFO} does not exist. Generate it with Lopper "
    "from the OpenAMP domain YAML (see ${CMAKE_CURRENT_SOURCE_DIR}/README.md) "
    "or set MBV32_PLATFORM_INFO.")
endif()
get_filename_component(_mbv32_info_dir "${MBV32_PLATFORM_INFO}" DIRECTORY)
collect(APP_INC_DIRS "${_mbv32_info_dir}")
