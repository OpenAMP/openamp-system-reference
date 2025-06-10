/*
 * Copyright (c) 2025 STMicroelectronics.
 * Copyright (c) 2025 Advanced Micro Devices.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "task.h"

extern TaskHandle_t rpmsg_task;

void system_suspend(void)
{
         vTaskSuspend(rpmsg_task);
}

void system_resume(void)
{
         (void)data;
         xTaskResumeFromISR(rpmsg_task);
}
