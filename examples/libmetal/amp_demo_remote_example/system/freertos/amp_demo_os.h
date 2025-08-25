 /*
  * Copyright (c) 2017 - 2022, Xilinx Inc. and Contributors. All rights reserved.
  * Copyright (C) 2023 - 2025, Advanced Micro Devices, Inc.
  *
  * SPDX-License-Identifier: BSD-3-Clause
  */

#ifndef __AMP_DEMO_OS_H__
#define __AMP_DEMO_OS_H__

#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/sys.h>
#include <metal/cpu.h>

#include <FreeRTOS.h>
#include <task.h>
#include "semphr.h"
#include "timers.h"
#include "portmacro.h"

struct channel_s {
	struct metal_io_region *ipi_io; /* IPI metal i/o region */
	struct metal_io_region *shm_io; /* Shared memory metal i/o region */
	struct metal_io_region *ttc_io; /* TTC metal i/o region */
	uint32_t ipi_mask; /* RPU IPI mask */
	TaskHandle_t *task; /* Specific to OS. used for suspend/resume of task. */
	int ipi_irq; /* IRQ number. */
	SemaphoreHandle_t xSemaphore;
};

#define DELAY_100_MILISEC		100UL

/**
 * @brief amp_os_init() - set OS specific information in the channel
 *
 * @param[in] ch - channel to store task
 * @param[in] arg - task to use in demos
 */
static inline int amp_os_init(struct channel_s * ch, void *arg)
{
	SemaphoreHandle_t *xSemaphore = arg;

	metal_assert(ch);
	metal_assert(xSemaphore);

	ch->xSemaphore = *xSemaphore;

	return 0;
}

/**
 * @brief suspend() - suspend the channel's execution 
 *
 * @param[in] ch - channel with task to suspend
 */
static inline void suspend(struct channel_s * ch)
{
	if (xSemaphoreTake(ch->xSemaphore,
			   pdMS_TO_TICKS(DELAY_100_MILISEC)) != pdTRUE) {
		xil_printf("FreeRTOS suspend failed\n");
	}
}

/**
 * @brief resume - This routine will wake the demo task
 *
 * For AMD Implementation, the suspended task needs to be resumed
 * and the scheduler needs an explicit signal for context switch. Call
 * that in this routine.
 * @param[in] ch - channel with task to wake
 */
static inline void resume(struct channel_s * ch)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	metal_assert(ch);

	if (xSemaphoreGiveFromISR(ch->xSemaphore, &xHigherPriorityTaskWoken) != pdFALSE) {
	   /*
	    * AMD Implementation requires this call so that
	    * sheduler knows to do context switch. Otherwise
	    * the scheduler does not know that a context switch
	    * should occur.
	    */
	   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

#endif /* __AMP_DEMO_OS_H__ */
 
