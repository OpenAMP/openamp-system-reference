 /*
 * Copyright (c) 2017 - 2022, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2023 - 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AMP_DEMO_OS_H__
#define __AMP_DEMO_OS_H__

#include <metal/atomic.h>
#include <metal/irq.h>
#include <metal/sys.h>
#include <metal/cpu.h>

struct channel_s {
	struct metal_io_region *ipi_io; /* IPI metal i/o region */
	struct metal_io_region *shm_io; /* Shared memory metal i/o region */
	struct metal_io_region *ttc_io; /* TTC metal i/o region */
	uint32_t ipi_mask; /* RPU IPI mask */
	atomic_flag *remote_nkicked; /* Specific to OS. used for suspend/resume of task. */
	int ipi_irq; /* IRQ number. */
};

static inline int amp_os_init(struct channel_s *ch, void *arg)
{
	metal_assert(ch);
	atomic_flag *af = arg;

	metal_assert(ch);
	metal_assert(af);
	ch->remote_nkicked = af;

	return 0;
}

static inline void wait_for_interrupt()
{
	metal_asm volatile("wfi");
}
/**
 * @brief suspend() - Loop until notified bit
 *        in channel is set.
 *
 * @param[in] ch - channel with task to suspend
 */
static inline void suspend(struct channel_s *ch)
{
	unsigned int flags;

	do {
		flags = metal_irq_save_disable();
		if (!atomic_flag_test_and_set(ch->remote_nkicked)) {
			metal_irq_restore_enable(flags);
			break;
		}
		wait_for_interrupt();
		metal_irq_restore_enable(flags);
	} while(1);
}

/**
 * @brief resume - This routine will wake the demo task
 *
 * @param[in] ch - channel with thread to wake
 */
static inline void resume(struct channel_s *ch)
{
	metal_assert(ch);
	metal_assert(ch->remote_nkicked);

	/* Resume the suspended task. */
	atomic_flag_clear(&ch->remote_nkicked);
}

#endif /* __AMP_DEMO_OS_H__ */
