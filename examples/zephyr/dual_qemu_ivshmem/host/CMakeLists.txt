# Copyright (c) 2023 Linaro
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openamp_rpmsg_over_ivshmem_host)

target_include_directories(app PRIVATE ../rpmsg_ivshmem_backend)

target_sources(app PRIVATE
        src/main.c
        ../rpmsg_ivshmem_backend/rpmsg_ivshmem_backend.c)
