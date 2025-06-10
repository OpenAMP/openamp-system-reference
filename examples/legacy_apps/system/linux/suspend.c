/*
 * Copyright (c) 2025 STMicroelectronics.
 * Copyright (c) 2025 Advanced Micro Devices.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/cpu.h>

void __attribute__((weak)) rpmsg_linux_suspend(void)
{
        metal_cpu_yield();
}

void __attribute__((weak)) rpmsg_linux_resume(void)
{
}

void system_suspend(void)
{
        rpmsg_linux_suspend();
}

void system_resume(void)
{
        rpmsg_linux_resume();
}