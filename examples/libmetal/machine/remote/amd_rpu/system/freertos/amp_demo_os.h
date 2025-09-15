 /*
  * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#include "portmacro.h"

#include "xil_printf.h"

struct channel_s {
	struct metal_io_region *ipi_io; /* IPI metal i/o region */
	struct metal_io_region *shm_io; /* Shared memory metal i/o region */
	struct metal_io_region *ttc_io; /* TTC metal i/o region */
	uint32_t ipi_mask; /* RPU IPI mask */
	TaskHandle_t task; /* Demo task handle used for suspend/resume. */
	int irq_vector_id; /* IRQ number. */
};

/**
 * @brief amp_os_init() - set OS specific information in the channel
 *
 * @param[in] ch - channel to store task
 * @param[in] arg - task to use in demos
 */
static inline int amp_os_init(struct channel_s *ch, void *arg)
{
	TaskHandle_t task = (TaskHandle_t)arg;

	metal_assert(ch);
	metal_assert(task);

	ch->task = task;

	return 0;
}

/**
 * @brief system_suspend() - suspend the channel's execution
 *
 * @param[in] ch - channel with task to suspend
 */
static inline void system_suspend(struct channel_s *ch)
{
	metal_assert(ch);

	/* Suspend this task until the ISR resumes it. */
	vTaskSuspend(NULL);
}

/**
 * @brief system_resume - This routine will wake the demo task
 *
 * For AMD Implementation, the suspended task needs to be resumed
 * and the scheduler needs an explicit signal for context switch. Call
 * that in this routine.
 * @param[in] ch - channel with task to wake
 */
static inline void system_resume(struct channel_s *ch)
{
	BaseType_t yield_required;

	metal_assert(ch);
	metal_assert(ch->task);

	yield_required = xTaskResumeFromISR(ch->task);
	portYIELD_FROM_ISR(yield_required);
}

#endif /* __AMP_DEMO_OS_H__ */
