# Copyright (c) 2023  STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.13.1)

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../zephyr_virtio_device)
set(SHIELD x_nucleo_iks01a2)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(openamp_virtio_i2c)

target_sources(app PRIVATE src/main_remote.c)
target_sources(app PRIVATE src/mmio_table.c)
target_include_directories(app PRIVATE ${LIBMETAL_INCLUDE_DIR} ${OPENAMP_INCLUDE_DIR} ${PLATFORM_DIR})
