/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AMP_DEMO_OS_H__
#define __AMP_DEMO_OS_H__

#include <stdint.h>

#include <metal/assert.h>
#include <metal/irq.h>
#include <metal/sys.h>

#include "irq_shmem_demo.h"

/*
 * The MB-V core has no atomic extension, so the wakeup flag is a volatile
 * word updated by the mailbox interrupt and consumed with interrupts masked.
 */
struct channel_machine_ctx_s {
	volatile uint32_t irq_pending;
};

static inline struct channel_machine_ctx_s *channel_machine_ctx(struct channel_s *ch)
{
	metal_assert(ch);
	metal_assert(ch->machine_ctx);

	return (struct channel_machine_ctx_s *)ch->machine_ctx;
}

static inline int amp_os_init(struct channel_s *ch, void *arg)
{
	(void)arg;
	channel_machine_ctx(ch)->irq_pending = 0;

	return 0;
}

/**
 * @brief system_suspend() - wait until the mailbox interrupt fires
 *
 * Polls instead of using wfi: an interrupt arriving between re-enabling
 * interrupts and wfi would otherwise leave the core asleep.
 */
static inline void system_suspend(struct channel_s *ch)
{
	struct channel_machine_ctx_s *machine = channel_machine_ctx(ch);
	unsigned int flags;

	for (;;) {
		flags = metal_irq_save_disable();
		if (machine->irq_pending) {
			machine->irq_pending = 0;
			metal_irq_restore_enable(flags);
			return;
		}
		metal_irq_restore_enable(flags);
	}
}

static inline void system_resume(struct channel_s *ch)
{
	channel_machine_ctx(ch)->irq_pending = 1;
}

#endif /* __AMP_DEMO_OS_H__ */
