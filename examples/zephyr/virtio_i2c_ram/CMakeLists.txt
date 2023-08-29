cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2023  STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(virtio_i2c_ram)

zephyr_compile_definitions(
  -DMETAL_MAX_DEVICE_REGIONS=2
  -DVIRTIO_SLAVE_ONLY
  )

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_include_directories(app PRIVATE ${LIBMETAL_INCLUDE_DIR} ${OPENAMP_INCLUDE_DIR} ${PLATFORM_DIR})

target_sources(app PRIVATE src/main_remote.c)
target_sources(app PRIVATE src/mmio_table.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
