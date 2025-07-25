/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2017 Xilinx, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       zynqmp_r5_a53_rproc.c
 *
 * DESCRIPTION
 *
 *       This file define Xilinx ZynqMP R5 to A53 platform specific
 *       remoteproc implementation.
 *
 **************************************************************************/

#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include "platform_info.h"
#include "suspend.h"

#ifndef RPMSG_NO_IPI
/* IPI REGs OFFSET */
#define IPI_TRIG_OFFSET          0x00000000    /* IPI trigger register offset */
#define IPI_OBS_OFFSET           0x00000004    /* IPI observation register offset */
#define IPI_ISR_OFFSET           0x00000010    /* IPI interrupt status register offset */
#define IPI_IMR_OFFSET           0x00000014    /* IPI interrupt mask register offset */
#define IPI_IER_OFFSET           0x00000018    /* IPI interrupt enable register offset */
#define IPI_IDR_OFFSET           0x0000001C    /* IPI interrupt disable register offset */

static int zynqmp_r5_a53_proc_irq_handler(int vect_id, void *data)
{
	struct remoteproc *rproc = data;
	struct remoteproc_priv *prproc;
	unsigned int ipi_intr_status;

	(void)vect_id;
	if (!rproc)
		return METAL_IRQ_NOT_HANDLED;
	prproc = rproc->priv;
	ipi_intr_status = (unsigned int)metal_io_read32(prproc->kick_io,
							IPI_ISR_OFFSET);
	if (ipi_intr_status & prproc->ipi_chn_mask) {
		atomic_flag_clear(&prproc->ipi_nokick);
		metal_io_write32(prproc->kick_io, IPI_ISR_OFFSET,
				 prproc->ipi_chn_mask);
		system_resume(NULL);
		return METAL_IRQ_HANDLED;
	}
	return METAL_IRQ_NOT_HANDLED;
}
#endif /* !RPMSG_NO_IPI */

static struct remoteproc *
zynqmp_r5_a53_proc_init(struct remoteproc *rproc,
			const struct remoteproc_ops *ops, void *arg)
{
	struct remoteproc_priv *prproc = arg;
	struct metal_device *kick_dev;
	unsigned int irq_vect;
	int ret;

	(void)ops;
	if (!rproc || !prproc)
		return NULL;
	ret = metal_device_open(prproc->kick_dev_bus_name,
				prproc->kick_dev_name,
				&kick_dev);
	if (ret) {
		xil_printf("failed to open polling device: %d.\r\n", ret);
		return NULL;
	}
	prproc->kick_dev = kick_dev;
	prproc->kick_io = metal_device_io_region(kick_dev, 0);
	if (!prproc->kick_io)
		goto err1;
#ifndef RPMSG_NO_IPI
	atomic_store(&prproc->ipi_nokick, 1);
	/* Register interrupt handler and enable interrupt */
	irq_vect = (uintptr_t)kick_dev->irq_info;
	metal_irq_register(irq_vect, zynqmp_r5_a53_proc_irq_handler, rproc);
	metal_irq_enable(irq_vect);
	metal_io_write32(prproc->kick_io, IPI_IER_OFFSET,
			 prproc->ipi_chn_mask);
#else
	(void)irq_vect;
	metal_io_write32(prproc->kick_io, 0, !POLL_STOP);
#endif /* !RPMSG_NO_IPI */

	return rproc;
err1:
	metal_device_close(kick_dev);
	return NULL;
}

static void zynqmp_r5_a53_proc_remove(struct remoteproc *rproc)
{
	struct remoteproc_priv *prproc;
	struct metal_device *dev;

	if (!rproc)
		return;
	prproc = rproc->priv;
#ifndef RPMSG_NO_IPI
	metal_io_write32(prproc->kick_io, IPI_IDR_OFFSET,
			 prproc->ipi_chn_mask);
	dev = prproc->kick_dev;
	if (dev) {
		metal_irq_disable((uintptr_t)dev->irq_info);
		metal_irq_unregister((uintptr_t)dev->irq_info);
	}
#else /* RPMSG_NO_IPI */
	(void)dev;
#endif /* !RPMSG_NO_IPI */
	metal_device_close(prproc->kick_dev);
}

static void *
zynqmp_r5_a53_proc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
			metal_phys_addr_t *da, size_t size,
			unsigned int attribute, struct metal_io_region **io)
{
	struct remoteproc_mem *mem;
	metal_phys_addr_t lpa, lda;
	struct metal_io_region *tmpio;

	lpa = *pa;
	lda = *da;

	if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
		return NULL;
	if (lpa == METAL_BAD_PHYS)
		lpa = lda;
	if (lda == METAL_BAD_PHYS)
		lda = lpa;

	if (!attribute)
		attribute = NORM_SHARED_NCACHE | PRIV_RW_USER_RW;
	mem = metal_allocate_memory(sizeof(*mem));
	if (!mem)
		return NULL;
	tmpio = metal_allocate_memory(sizeof(*tmpio));
	if (!tmpio) {
		metal_free_memory(mem);
		return NULL;
	}
	remoteproc_init_mem(mem, NULL, lpa, lda, size, tmpio);
	/* va is the same as pa in this platform */
	metal_io_init(tmpio, (void *)lpa, &mem->pa, size,
		      sizeof(metal_phys_addr_t) << 3, attribute, NULL);
	remoteproc_add_mem(rproc, mem);
	*pa = lpa;
	*da = lda;
	if (io)
		*io = tmpio;
	return metal_io_phys_to_virt(tmpio, mem->pa);
}

static int zynqmp_r5_a53_proc_notify(struct remoteproc *rproc, uint32_t id)
{
	struct remoteproc_priv *prproc;

	(void)id;
	if (!rproc)
		return -1;
	prproc = rproc->priv;

#ifdef RPMSG_NO_IPI
	metal_io_write32(prproc->kick_io, 0, POLL_STOP);
#else
	metal_io_write32(prproc->kick_io, IPI_TRIG_OFFSET,
			 prproc->ipi_chn_mask);
#endif /* RPMSG_NO_IPI */
	return 0;
}

/*
 * processor operations from r5 to a53. It defines
 * notification operation and remote processor management operations.
 */
const struct remoteproc_ops zynqmp_r5_a53_proc_ops = {
	.init = zynqmp_r5_a53_proc_init,
	.remove = zynqmp_r5_a53_proc_remove,
	.mmap = zynqmp_r5_a53_proc_mmap,
	.notify = zynqmp_r5_a53_proc_notify,
	.start = NULL,
	.stop = NULL,
	.shutdown = NULL,
};
