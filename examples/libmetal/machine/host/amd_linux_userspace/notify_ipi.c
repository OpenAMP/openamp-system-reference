/*
 * Copyright (C) 2025 - 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * PS IPI notification for RPU remotes. The host IPI agent is the UIO device
 * "libmetal-ipi"; its libmetal,uio-ipi-bitmask device-tree property selects
 * the remote's agent.
 */

#include <metal/irq.h>
#include <metal/log.h>
#include "common.h"
#include "notify.h"
#include "platform_init.h"

#define APP_IPI_DEV_NAME "libmetal-ipi"
#define APP_IPI_REMOTE_MASK_PROP "libmetal,uio-ipi-bitmask"

/*
 * IPI agent register offsets. These are fixed by the PS IPI block (see the
 * register reference of the SoC's TRM), the same for every agent, so they
 * are constants; the device tree gives only where the agent is.
 */
#define IPI_TRIG_OFFSET 0x0  /* IPI trigger reg offset */
#define IPI_OBS_OFFSET  0x4  /* IPI observation reg offset */
#define IPI_ISR_OFFSET  0x10 /* IPI interrupt status reg offset */
#define IPI_IMR_OFFSET  0x14 /* IPI interrupt mask reg offset */
#define IPI_IER_OFFSET  0x18 /* IPI interrupt enable reg offset */
#define IPI_IDR_OFFSET  0x1C /* IPI interrupt disable reg offset */

static int ipi_isr(int vect_id, void *priv)
{
	struct channel_s *ch = (struct channel_s *)priv;
	struct channel_machine_ctx_s *machine = channel_machine_ctx(ch);
	struct metal_io_region *ipi_io = ch->ipi_io;
	uint32_t ipi_mask = ch->ipi_mask;
	uint64_t val = 1;

	(void)vect_id;

	if (!ipi_io)
		return METAL_IRQ_NOT_HANDLED;
	val = metal_io_read32(ipi_io, IPI_ISR_OFFSET);
	if (val & ipi_mask) {
		metal_io_write32(ipi_io, IPI_ISR_OFFSET, ipi_mask);
		atomic_flag_clear(&machine->remote_nkicked);
		return METAL_IRQ_HANDLED;
	}
	return METAL_IRQ_NOT_HANDLED;
}

static int ipi_init(struct channel_s *ch)
{
	uint32_t ipi_mask;
	int ret;

	ret = app_uio_read_dt_u32(APP_IPI_DEV_NAME, APP_IPI_REMOTE_MASK_PROP, &ipi_mask);
	if (ret) {
		metal_err("HOST: Failed to read %s for %s.\n",
			  APP_IPI_REMOTE_MASK_PROP, APP_IPI_DEV_NAME);
		return ret;
	}
	ch->ipi_mask = ipi_mask;

	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, ch->ipi_mask);
	/* clear old IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_ISR_OFFSET, ch->ipi_mask);
	/* Register IPI irq handler */
	ret = metal_irq_register(ch->irq_vector_id, ipi_isr, ch);
	if (ret) {
		metal_err("HOST: Failed to register IRQ handler.\n");
		return ret;
	}
	metal_irq_enable(ch->irq_vector_id);
	/* Enable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IER_OFFSET, ch->ipi_mask);

	return 0;
}

static void ipi_kick(struct channel_s *ch)
{
	metal_assert(ch);
	metal_io_write32(ch->ipi_io, IPI_TRIG_OFFSET, ch->ipi_mask);
}

static void ipi_cleanup(struct channel_s *ch)
{
	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, ch->ipi_mask);
	/* unregister IPI irq handler by setting the handler to 0 */
	metal_irq_disable(ch->irq_vector_id);
	metal_irq_unregister(ch->irq_vector_id);
}

const struct notify_ops notify_ipi_ops = {
	.dev_name = APP_IPI_DEV_NAME,
	.init = ipi_init,
	.kick = ipi_kick,
	.cleanup = ipi_cleanup,
};
