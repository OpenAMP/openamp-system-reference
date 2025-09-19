/*
 * Copyright (c) 2025 STMicroelectronics.
 * Copyright (c) 2025 Advanced Micro Devices.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "task.h"

void system_suspend(void)
{
	TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
	vTaskSuspend(current_task);
}

void system_resume(void)
{
	TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
	xTaskResumeFromISR(current_task);
}
