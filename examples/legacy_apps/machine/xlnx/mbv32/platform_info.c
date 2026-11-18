/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * OpenAMP platform of a MicroBlaze-V remote. Linux loads the firmware into
 * the MicroBlaze-V BRAM and is the remoteproc and RPMsg host. Virtqueue
 * notifications are 32-bit words through an AXI Mailbox: the MicroBlaze-V
 * interface named by MBOX_BASE_ADDR, with the interrupt the BSP's mailbox
 * configuration gives. The shared region comes from amd_platform_info.h
 * (see platform_config.h).
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <metal/alloc.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/log.h>
#include <metal/sys.h>
#include <openamp/rpmsg_virtio.h>
#include "platform_config.h"
#include "platform_info_common.h"
#include "rsc_table.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xinterrupt_wrap.h"
#include "xmbox.h"
#include "xmbox_hw.h"

/*
 * Memory attributes passed to remoteproc_mmap(), in the encoding of the
 * libmetal and OpenAMP Xilinx ports (the RPU zynqmp_r5 machine uses the
 * same values): normal, non-shared, non-cacheable memory with read/write
 * access. The MicroBlaze-V has no MMU or MPU, so they only describe the
 * mapping to libmetal.
 */
#define NORM_NSHARED_NCACHE 0x00000008U
#define PRIV_RW_USER_RW (0x00000003U << 8U)

struct mbv_remoteproc_priv {
	XMbox mbox;
	volatile u32 pending;
	volatile u32 notify_id;
	struct remoteproc_mem mem[2];
	struct metal_io_region io[2];
	unsigned int mem_count;
};

static struct mbv_remoteproc_priv mbv_priv;
static struct remoteproc rproc_inst;
static struct rpmsg_virtio_shm_pool shpool;

static void mbv_log(enum metal_log_level level, const char *fmt, ...)
{
	char line[192];
	va_list ap;
	(void)level;
	va_start(ap, fmt);
	vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);
	xil_printf("%s", line);
}

static void mailbox_isr(void *arg)
{
	struct mbv_remoteproc_priv *priv = arg;
	u32 status = XMbox_GetInterruptStatus(&priv->mbox);
	u32 bytes;

	while ((status & XMB_IX_RTA) && !XMbox_IsEmpty(&priv->mbox)) {
		bytes = 0;
		if (XMbox_Read(&priv->mbox, (u32 *)&priv->notify_id,
			       sizeof(priv->notify_id), &bytes) == XST_SUCCESS &&
		    bytes == sizeof(priv->notify_id))
			priv->pending = 1U;
		status = XMbox_GetInterruptStatus(&priv->mbox);
	}
	XMbox_ClearInterrupt(&priv->mbox, status);
}

static struct remoteproc *mbv_rproc_init(struct remoteproc *rproc,
					 const struct remoteproc_ops *ops, void *arg)
{
	(void)ops;
	if (!rproc || !arg)
		return NULL;
	return rproc;
}

static void mbv_rproc_remove(struct remoteproc *rproc)
{
	(void)rproc;
	XMbox_SetInterruptEnable(&mbv_priv.mbox, 0U);
}

static void *mbv_rproc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
			    metal_phys_addr_t *da, size_t size,
			    unsigned int attribute, struct metal_io_region **io)
{
	struct remoteproc_mem *mem;
	struct metal_io_region *region;
	metal_phys_addr_t lpa = *pa;
	metal_phys_addr_t lda = *da;

	if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
		return NULL;
	if (lpa == METAL_BAD_PHYS)
		lpa = lda;
	if (lda == METAL_BAD_PHYS)
		lda = lpa;
	/* The resource table and the shared region. */
	if (mbv_priv.mem_count >= 2U) {
		metal_err("MB-V: no free remoteproc memory slot\r\n");
		return NULL;
	}
	mem = &mbv_priv.mem[mbv_priv.mem_count];
	region = &mbv_priv.io[mbv_priv.mem_count++];
	remoteproc_init_mem(mem, NULL, lpa, lda, size, region);
	metal_io_init(region, (void *)(uintptr_t)lpa, &mem->pa, size,
		      (sizeof(size_t) << 3) - 1U, attribute, NULL);
	/* Generic libmetal uses an all-ones mask for a single-entry physmap. */
	region->page_mask = (metal_phys_addr_t)-1;
	remoteproc_add_mem(rproc, mem);
	*pa = lpa;
	*da = lda;
	if (io)
		*io = region;
	return metal_io_phys_to_virt(region, mem->pa);
}

static int mbv_rproc_notify(struct remoteproc *rproc, u32 id)
{
	struct mbv_remoteproc_priv *priv;

	if (!rproc)
		return -EINVAL;
	priv = rproc->priv;
	/* OpenAMP treats notify as fire-and-forget and will not retry a transient
	 * full FIFO.  RPC generates short notification bursts, so wait until the
	 * single 32-bit virtqueue ID is accepted instead of losing a kick.
	 */
	XMbox_WriteBlocking(&priv->mbox, &id, sizeof(id));
	return 0;
}

static const struct remoteproc_ops mbv_rproc_ops = {
	.init = mbv_rproc_init,
	.remove = mbv_rproc_remove,
	.mmap = mbv_rproc_mmap,
	.notify = mbv_rproc_notify,
};

int platform_init(int argc, char *argv[], void **platform)
{
	struct metal_init_params params = METAL_INIT_DEFAULTS;
	XMbox_Config *cfg;
	void *table;
	metal_phys_addr_t pa;
	int table_size;
	int ret;
	(void)argc;
	(void)argv;

	if (!platform)
		return -EINVAL;
	params.log_handler = mbv_log;
	params.log_level = METAL_LOG_INFO;
	metal_init(&params);

	cfg = XMbox_LookupConfig(MBOX_BASE_ADDR);
	if (!cfg || XMbox_CfgInitialize(&mbv_priv.mbox, cfg, cfg->BaseAddress)) {
		metal_err("MB-V: no AXI Mailbox at 0x%lx\r\n",
			  (unsigned long)MBOX_BASE_ADDR);
		return -ENODEV;
	}
	XMbox_ResetFifos(&mbv_priv.mbox);
	XMbox_SetReceiveThreshold(&mbv_priv.mbox, 0U);
	XMbox_ClearInterrupt(&mbv_priv.mbox, XMB_IX_STA | XMB_IX_RTA | XMB_IX_ERR);
	ret = XSetupInterruptSystem(&mbv_priv.mbox, mailbox_isr, cfg->IntrId,
				    cfg->IntrParent, XINTERRUPT_DEFAULT_PRIORITY);
	if (ret != XST_SUCCESS) {
		metal_err("MB-V: cannot connect the mailbox interrupt\r\n");
		return -EIO;
	}
	XMbox_SetInterruptEnable(&mbv_priv.mbox, XMB_IX_RTA | XMB_IX_ERR);

	if (!remoteproc_init(&rproc_inst, &mbv_rproc_ops, &mbv_priv))
		return -EINVAL;
	table = get_resource_table(0, &table_size);
	pa = (metal_phys_addr_t)(uintptr_t)table;
	if (!remoteproc_mmap(&rproc_inst, &pa, NULL, table_size,
			     NORM_NSHARED_NCACHE | PRIV_RW_USER_RW,
			     &rproc_inst.rsc_io))
		return -ENOMEM;
	pa = SHARED_MEM_PA;
	if (!remoteproc_mmap(&rproc_inst, &pa, NULL, SHARED_REGION_SIZE,
			     NORM_NSHARED_NCACHE | PRIV_RW_USER_RW, NULL))
		return -ENOMEM;
	/*
	 * The host loads and updates this statically linked resource table in
	 * executable BRAM.  Keep the mapping returned above directly: asking
	 * remoteproc_set_rsc_table() to rediscover it by virtual address fails on
	 * this identity-mapped RV32 platform even though the mapping is valid.
	 */
	rproc_inst.rsc_table = table;
	rproc_inst.rsc_len = table_size;
	*platform = &rproc_inst;
	return 0;
}

struct rpmsg_device *platform_create_rpmsg_vdev(
	void *platform, unsigned int vdev_index, unsigned int role,
	void (*rst_cb)(struct virtio_device *vdev), rpmsg_ns_bind_cb ns_bind_cb)
{
	struct remoteproc *rproc = platform;
	struct rpmsg_virtio_device *rpvdev;
	struct metal_io_region *io;
	struct virtio_device *vdev;
	void *shbuf;

	rpvdev = metal_allocate_memory(sizeof(*rpvdev));
	if (!rpvdev)
		return NULL;
	io = remoteproc_get_io_with_pa(rproc, SHARED_MEM_PA);
	if (!io) {
		metal_err("MB-V: shared region 0x%lx is not mapped\r\n",
			  (unsigned long)SHARED_MEM_PA);
		goto fail;
	}
	shbuf = metal_io_phys_to_virt(io, SHARED_MEM_PA + SHARED_BUF_OFFSET);
	/* Wait until Linux has set up the vdev in the resource table. */
	while (!(((struct remote_resource_table *)rproc->rsc_table)->
		 rpmsg_vdev.status & VIRTIO_CONFIG_STATUS_DRIVER_OK))
		Xil_DCacheInvalidateRange((INTPTR)rproc->rsc_table,
					  rproc->rsc_len);
	vdev = remoteproc_create_virtio(rproc, vdev_index, role, rst_cb);
	if (!vdev) {
		metal_err("MB-V: cannot create the virtio device\r\n");
		goto fail;
	}
	rpmsg_virtio_init_shm_pool(&shpool, shbuf, SHARED_MEM_SIZE);
	if (rpmsg_init_vdev(rpvdev, vdev, ns_bind_cb, io, &shpool)) {
		metal_err("MB-V: cannot initialize the RPMsg device\r\n");
		remoteproc_remove_virtio(rproc, vdev);
		goto fail;
	}
	return rpmsg_virtio_get_rpmsg_device(rpvdev);
fail:
	metal_free_memory(rpvdev);
	return NULL;
}

int platform_poll(void *platform)
{
	struct remoteproc *rproc = platform;
	u32 id;
	unsigned int flags;

	for (;;) {
		flags = metal_irq_save_disable();
		if (mbv_priv.pending) {
			id = mbv_priv.notify_id;
			mbv_priv.pending = 0U;
			metal_irq_restore_enable(flags);
			Xil_DCacheInvalidateRange((INTPTR)SHARED_MEM_PA,
						  SHARED_REGION_SIZE);
			return remoteproc_get_notification(rproc, id);
		}
		metal_irq_restore_enable(flags);
		/*
		 * Do not WFI after re-enabling interrupts: an interrupt can arrive
		 * between the enable and WFI, set pending, and then leave the CPU
		 * asleep with no further edge.  Poll until the mailbox flag changes.
		 */
		__asm__ volatile("nop");
	}
}

void platform_release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform)
{
	struct rpmsg_virtio_device *rpvdev =
		metal_container_of(rpdev, struct rpmsg_virtio_device, rdev);
	struct virtio_device *vdev = rpvdev->vdev;

	rpmsg_deinit_vdev(rpvdev);
	remoteproc_remove_virtio(platform, vdev);
	metal_free_memory(rpvdev);
}

void platform_cleanup(void *platform)
{
	if (platform)
		remoteproc_remove(platform);
	metal_finish();
}
