/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * AXI Mailbox notification, for remotes in the PL such as a MicroBlaze-V.
 * The host's interface of the mailbox is the UIO device "libmetal-mbox". A
 * kick writes one word, a sequence number, into the host send FIFO; the
 * remote's reply raises the host receive-threshold interrupt, delivered
 * through uio_pdrv_genirq.
 */

#include <errno.h>
#include <metal/cpu.h>
#include <metal/irq.h>
#include <metal/log.h>
#include "amd_mbv_hw.h"
#include "common.h"
#include "notify.h"

#define APP_MBOX_DEV_NAME "libmetal-mbox"

static uint32_t kick_seq;

static int mbox_isr(int vect_id, void *priv)
{
	struct channel_s *ch = (struct channel_s *)priv;
	struct channel_machine_ctx_s *machine = channel_machine_ctx(ch);
	struct metal_io_region *mbox_io = ch->ipi_io;
	uint32_t pending;

	(void)vect_id;

	pending = metal_io_read32(mbox_io, MBOX_IS) &
		  metal_io_read32(mbox_io, MBOX_IE);
	if (!pending)
		return METAL_IRQ_NOT_HANDLED;

	if (pending & MBOX_IRQ_RTI) {
		/*
		 * Drain every notification word so the receive-threshold
		 * condition clears and the next word raises a new interrupt.
		 */
		while (!(metal_io_read32(mbox_io, MBOX_STATUS) &
			 MBOX_STATUS_EMPTY))
			(void)metal_io_read32(mbox_io, MBOX_RDDATA);
	}

	if (pending & MBOX_IRQ_ERR)
		metal_err("HOST: mailbox error 0x%x\n",
			  metal_io_read32(mbox_io, MBOX_ERROR));

	metal_io_write32(mbox_io, MBOX_IS, pending);
	atomic_flag_clear(&machine->remote_nkicked);

	return METAL_IRQ_HANDLED;
}

static int mbox_init(struct channel_s *ch)
{
	struct metal_io_region *mbox_io = ch->ipi_io;
	int ret;

	if (ch->irq_vector_id < 0) {
		metal_err("HOST: %s has no interrupt\n", APP_MBOX_DEV_NAME);
		return -ENODEV;
	}
	kick_seq = 0;

	/*
	 * Program only this interface. Loading the PL clears the mailbox
	 * interrupt enables, so always initialize after the PDI is loaded.
	 */
	metal_io_write32(mbox_io, MBOX_IE, 0);
	metal_io_write32(mbox_io, MBOX_CTRL,
			 MBOX_CTRL_RESET_SEND | MBOX_CTRL_RESET_RECV);
	metal_io_write32(mbox_io, MBOX_RIT, 0);
	metal_io_write32(mbox_io, MBOX_IS,
			 MBOX_IRQ_STI | MBOX_IRQ_RTI | MBOX_IRQ_ERR);

	ret = metal_irq_register(ch->irq_vector_id, mbox_isr, ch);
	if (ret) {
		metal_err("HOST: Failed to register mailbox irq: %d\n", ret);
		return ret;
	}
	metal_irq_enable(ch->irq_vector_id);
	metal_io_write32(mbox_io, MBOX_IE, MBOX_IRQ_RTI | MBOX_IRQ_ERR);

	return 0;
}

static void mbox_kick(struct channel_s *ch)
{
	metal_assert(ch);
	/* The send FIFO may be full; wait for the remote to drain it. */
	while (metal_io_read32(ch->ipi_io, MBOX_STATUS) & MBOX_STATUS_FULL)
		metal_cpu_yield();
	metal_io_write32(ch->ipi_io, MBOX_WRDATA, ++kick_seq);
}

static void mbox_cleanup(struct channel_s *ch)
{
	metal_io_write32(ch->ipi_io, MBOX_IE, 0);
	metal_io_write32(ch->ipi_io, MBOX_IS,
			 MBOX_IRQ_STI | MBOX_IRQ_RTI | MBOX_IRQ_ERR);
	metal_irq_disable(ch->irq_vector_id);
	metal_irq_unregister(ch->irq_vector_id);
}

const struct notify_ops notify_pl_mbox_ops = {
	.dev_name = APP_MBOX_DEV_NAME,
	.init = mbox_init,
	.kick = mbox_kick,
	.cleanup = mbox_cleanup,
};
