/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Bare-metal MicroBlaze-V platform of the irq_shmem_demo.
 *
 * The shared regions and the MicroBlaze-V interface of the AXI Mailbox are
 * identity-mapped libmetal devices at the addresses the Lopper-generated
 * configuration gives, as the MicroBlaze-V addresses them. The mailbox
 * interrupt comes from the BSP's configuration of that mailbox.
 *
 * With MBV_PROFILE, the remote timestamps each host word when it reads it
 * (t1) and each reply when it sends it (t2), from the shared AXI Timer at
 * TIMER_BASE_ADDR, and returns both with the reply; see amd_mbv_hw.h and
 * the host's pl_mbox_profile.h.
 */

#include <stdarg.h>
#include <stdio.h>
#include "common.h"
#include "xil_printf.h"
#include "xinterrupt_wrap.h"
#include "xmbox.h"
#include "xparameters.h"

/* Single-entry physmaps on RV32 need an all-ones page shift and mask. */
#define DEFAULT_PAGE_SHIFT (-1UL)
#define DEFAULT_PAGE_MASK  (-1UL)

#ifdef MBV_PROFILE
#define TIMER_NOW() (*(volatile uint32_t *)(TIMER_BASE_ADDR + TIMER_TCR0))
/* t1 of the last host words, by count; the host waits for each reply. */
#define PROF_RX_RING 64U
static uint32_t rx_count;
static uint32_t rx_t1[PROF_RX_RING];
#endif

static const metal_phys_addr_t metal_phys[] = {
	SHM0_DESC_BASE,
	SHM1_DESC_BASE,
	SHM_PAYLOAD_BASE,
	MBOX_BASE_ADDR,
};

#define MBV_REGION(base, phys, sz) { \
	.virt = (void *)(base), \
	.physmap = (phys), \
	.size = (sz), \
	.page_shift = DEFAULT_PAGE_SHIFT, \
	.page_mask = DEFAULT_PAGE_MASK, \
	.mem_flags = 0, \
	.ops = {NULL}, \
}

static struct metal_device metal_dev_table[] = {
	{
		.name = SHM0_DESC_DEV_NAME,
		.num_regions = 1,
		.regions = { MBV_REGION(SHM0_DESC_BASE, &metal_phys[0],
					SHM0_DESC_SIZE) },
	},
	{
		.name = SHM1_DESC_DEV_NAME,
		.num_regions = 1,
		.regions = { MBV_REGION(SHM1_DESC_BASE, &metal_phys[1],
					SHM1_DESC_SIZE) },
	},
	{
		.name = SHM_DEV_NAME,
		.num_regions = 1,
		.regions = { MBV_REGION(SHM_PAYLOAD_BASE, &metal_phys[2],
					SHM_PAYLOAD_SIZE) },
	},
	{
		.name = MBOX_DEV_NAME,
		.num_regions = 1,
		.regions = { MBV_REGION(MBOX_BASE_ADDR, &metal_phys[3],
					MBOX_SIZE) },
	},
};

static struct metal_device *devs[sizeof(metal_dev_table) /
				 sizeof(metal_dev_table[0])];
static XMbox_Config *mbox_cfg;
static uint32_t tx_seq;

static void mbv_log(enum metal_log_level level, const char *fmt, ...)
{
	char line[160];
	va_list ap;

	(void)level;
	va_start(ap, fmt);
	vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);
	xil_printf("%s", line);
}

static void mailbox_isr(void *arg)
{
	struct channel_s *ch = arg;
	struct metal_io_region *mbox_io = ch->ipi_io;
	uint32_t pending;

	pending = metal_io_read32(mbox_io, MBOX_IS) &
		  metal_io_read32(mbox_io, MBOX_IE);

	if (pending & MBOX_IRQ_RTI) {
		/* Drain so the receive threshold can fire for the next word. */
		while (!(metal_io_read32(mbox_io, MBOX_STATUS) &
			 MBOX_STATUS_EMPTY)) {
			(void)metal_io_read32(mbox_io, MBOX_RDDATA);
#ifdef MBV_PROFILE
			/*
			 * Timestamp each word when it is read: words written by
			 * the host while this loop drains the FIFO arrive after
			 * the interrupt entry.
			 */
			rx_t1[++rx_count % PROF_RX_RING] = TIMER_NOW();
#endif
		}
		system_resume(ch);
	}

	if (pending & MBOX_IRQ_ERR)
		metal_err("REMOTE: mailbox error 0x%x\n",
			  metal_io_read32(mbox_io, MBOX_ERROR));

	metal_io_write32(mbox_io, MBOX_IS, pending);
}

/* Write one word to the send FIFO, waiting for the host to make room. */
static void mbox_put(struct metal_io_region *mbox_io, uint32_t word)
{
	while (metal_io_read32(mbox_io, MBOX_STATUS) & MBOX_STATUS_FULL)
		;
	metal_io_write32(mbox_io, MBOX_WRDATA, word);
}

void mbv_mailbox_kick(struct channel_s *ch)
{
	uint32_t seq = ++tx_seq;

#ifdef MBV_PROFILE
	/* The seq-th reply answers the host's seq-th word. */
	uint32_t t2 = TIMER_NOW();

	mbox_put(ch->ipi_io, (seq & MBV_PROF_SEQ_MASK) | MBV_PROF_REPLY);
	mbox_put(ch->ipi_io, rx_t1[seq % PROF_RX_RING]);
	mbox_put(ch->ipi_io, t2);
#else
	mbox_put(ch->ipi_io, seq);
#endif
}

int platform_init(struct channel_s *ch)
{
	struct metal_init_params params = METAL_INIT_DEFAULTS;
	struct metal_io_region *mbox_io;
	unsigned int i;
	int ret;

	tx_seq = 0;
#ifdef MBV_PROFILE
	rx_count = 0;
#endif

	params.log_handler = mbv_log;
	params.log_level = METAL_LOG_INFO;
	ret = metal_init(&params);
	if (ret)
		return ret;

	for (i = 0; i < sizeof(metal_dev_table) / sizeof(metal_dev_table[0]);
	     i++) {
		ret = metal_register_generic_device(&metal_dev_table[i]);
		if (!ret)
			ret = metal_device_open(BUS_NAME,
						metal_dev_table[i].name,
						&devs[i]);
		if (ret) {
			metal_err("REMOTE: device %s: %d\n",
				  metal_dev_table[i].name, ret);
			return ret;
		}
	}

	ch->host_to_remote_desc_io = metal_device_io_region(devs[0], 0);
	ch->remote_to_host_desc_io = metal_device_io_region(devs[1], 0);
	ch->shm_io = metal_device_io_region(devs[2], 0);
	ch->ipi_io = metal_device_io_region(devs[3], 0);
	if (!ch->host_to_remote_desc_io || !ch->remote_to_host_desc_io ||
	    !ch->shm_io || !ch->ipi_io)
		return -ENODEV;

	mbox_cfg = XMbox_LookupConfig(MBOX_BASE_ADDR);
	if (!mbox_cfg) {
		metal_err("REMOTE: no AXI Mailbox at 0x%lx in the BSP\n",
			  (unsigned long)MBOX_BASE_ADDR);
		return -ENODEV;
	}

	mbox_io = ch->ipi_io;
	metal_io_write32(mbox_io, MBOX_IE, 0);
	metal_io_write32(mbox_io, MBOX_CTRL,
			 MBOX_CTRL_RESET_SEND | MBOX_CTRL_RESET_RECV);
	metal_io_write32(mbox_io, MBOX_RIT, 0);
	metal_io_write32(mbox_io, MBOX_IS,
			 MBOX_IRQ_STI | MBOX_IRQ_RTI | MBOX_IRQ_ERR);

	ret = XSetupInterruptSystem(ch, (void *)mailbox_isr, mbox_cfg->IntrId,
				    mbox_cfg->IntrParent,
				    XINTERRUPT_DEFAULT_PRIORITY);
	if (ret != XST_SUCCESS) {
		metal_err("REMOTE: cannot connect the mailbox interrupt\n");
		return -EIO;
	}
	metal_io_write32(mbox_io, MBOX_IE, MBOX_IRQ_RTI | MBOX_IRQ_ERR);

	return 0;
}

void platform_cleanup(struct channel_s *ch)
{
	unsigned int i;

	if (ch->ipi_io)
		metal_io_write32(ch->ipi_io, MBOX_IE, 0);
	if (mbox_cfg)
		XDisconnectInterruptCntrl(mbox_cfg->IntrId,
					  mbox_cfg->IntrParent);

	for (i = 0; i < sizeof(devs) / sizeof(devs[0]); i++) {
		if (devs[i])
			metal_device_close(devs[i]);
		devs[i] = NULL;
	}

	metal_finish();
}
