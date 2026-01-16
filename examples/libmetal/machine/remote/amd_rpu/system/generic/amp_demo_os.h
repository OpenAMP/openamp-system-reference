/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AMP_DEMO_OS_H__
#define __AMP_DEMO_OS_H__

#include <stdint.h>

#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/cpu.h>
#include <metal/sys.h>

#include "xil_printf.h"

struct channel_s {
	struct metal_io_region *ipi_io; /* IPI metal i/o region */
	struct metal_io_region *shm_io; /* Shared memory metal i/o region */
	struct metal_io_region *ttc_io; /* TTC metal i/o region */
	uint32_t ipi_mask;              /* RPU IPI mask */
	int irq_vector_id;              /* IRQ number. */
	atomic_flag irq_pending;        /* Lightweight wait primitive. */
};

/**
 * @brief amp_os_init() - initialize bare-metal rendezvous primitives
 *
 * Baremetal builds do not rely on an RTOS scheduler, so we track IPI wakeups
 * with a simple flag that the interrupt handler toggles.
 *
 * @param[in] ch - remote communication channel
 * @param[in] arg - unused for bare-metal builds
 */
static inline int amp_os_init(struct channel_s *ch, void *arg)
{
	(void)arg;

	metal_assert(ch);

	ch->irq_pending = (atomic_flag)ATOMIC_FLAG_INIT;

	/* Start in the "waiting" state until the first interrupt arrives. */
	(void)atomic_flag_test_and_set(&ch->irq_pending);

	return 0;
}

/**
 * @brief system_suspend() - park the demo loop until an IPI fires
 *
 * Baremetal implementations spin-wait on the `irq_pending` flag and yield the
 * CPU so other interrupt handlers can progress.
 *
 * @param[in] ch - remote communication channel
 */
static inline void system_suspend(struct channel_s *ch)
{
	metal_assert(ch);

	while (atomic_flag_test_and_set(&ch->irq_pending)) {
		metal_asm volatile("wfi");
	}
}

/**
 * @brief system_resume() - resume the demo loop after an IPI
 *
 * @param[in] ch - remote communication channel
 */
static inline void system_resume(struct channel_s *ch)
{
	metal_assert(ch);
	atomic_flag_clear(&ch->irq_pending);
}

#endif /* __AMP_DEMO_OS_H__ */
