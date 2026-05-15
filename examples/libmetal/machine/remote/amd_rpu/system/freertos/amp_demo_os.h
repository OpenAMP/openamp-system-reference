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
#include "irq_shmem_demo.h"
#include "portmacro.h"

#include "xil_printf.h"

struct channel_machine_ctx_s {
	TaskHandle_t task; /* Demo task handle used for suspend/resume. */
};

static inline struct channel_machine_ctx_s *channel_machine_ctx(struct channel_s *ch)
{
	metal_assert(ch);
	metal_assert(ch->machine_ctx);

	return (struct channel_machine_ctx_s *)ch->machine_ctx;
}

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

	channel_machine_ctx(ch)->task = task;

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
	struct channel_machine_ctx_s *machine = channel_machine_ctx(ch);

	metal_assert(ch);
	metal_assert(machine->task);

	yield_required = xTaskResumeFromISR(machine->task);
	portYIELD_FROM_ISR(yield_required);
}

#endif /* __AMP_DEMO_OS_H__ */
