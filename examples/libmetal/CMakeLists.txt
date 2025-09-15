# Copyright (C) 2025 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project (libmetal_apps C)

set (APPS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set (APPS_SHARE_DIR "${CMAKE_CURRENT_BINARY_DIR}/share")

list (APPEND CMAKE_MODULE_PATH
  "${APPS_ROOT_DIR}../legacy_apps/cmake/"
  "${APPS_ROOT_DIR}../legacy_apps/cmake/modules"
  "${APPS_ROOT_DIR}/../legacy_apps/cmake/platforms")
include ("${APPS_ROOT_DIR}/../legacy_apps/cmake/collect.cmake")

add_subdirectory(demos)
add_subdirectory(machine)
