# Copyright (C) 2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_SYSTEM}/CMakeLists.txt")
  add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_SYSTEM})
endif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_SYSTEM}/CMakeLists.txt")
