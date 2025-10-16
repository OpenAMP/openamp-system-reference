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
 *       platform_info.c
 *
 * DESCRIPTION
 *
 *       This file define platform specific data and implements APIs to set
 *       platform specific information for OpenAMP.
 *
 **************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/irq_controller.h>
#include <metal/sys.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>

#include "platform_info.h"
#include "rsc_table.h"
#include "suspend.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_cache.h"

#define KICK_DEV_NAME         "poll_dev"
#define KICK_BUS_NAME         "generic"

/* Cortex R5 memory attributes */
#define DEVICE_SHARED		0x00000001U /* device, shareable */
#define DEVICE_NONSHARED	0x00000010U /* device, non shareable */
#define NORM_NSHARED_NCACHE	0x00000008U /* Non cacheable  non shareable */
#define NORM_SHARED_NCACHE	0x0000000CU /* Non cacheable shareable */

#ifdef _AMD_GENERATED_
/*
 * @file   amd_platform_info.h
 * @file   platform_info.c
 * @brief  Generated header that contains OpenAMP IPC information.
 *
 *         Namely interrupt and shared memory information. If values are
 *	   provided via generated header, then include thus. These values
 *	   are to describe interrupt and shared memory information that
 *	   describes one end of an OpenAMP IPC connection. This application
 *	   is for target AMD RPUs. The file 'amd_platform_info.h'
 *	   is generated via Vitis NG or Yocto-SHEL flow for
 *	   AMD RPU targets. The channel information is defined in the
 *	   OpenAMP YAML channel description. The generated symbols can be
 *	   changed by editing the OpenAMP YAML channel description.
 */
#include "amd_platform_info.h"
#else

/* Interrupt vectors */
#ifdef versal

#ifndef IPI_IRQ_VECT_ID
#define IPI_IRQ_VECT_ID     63
#endif /* !IPI_IRQ_VECT_ID */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR       0xFF340000 /* IPI base address*/
#endif /* !POLL_BASE_ADDR */

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     0x0000020 /* IPI channel bit mask for IPI from/to
					   APU */
#endif /* !IPI_CHN_BITMASK */

#else

#include "xreg_cortexr5.h"
#ifndef IPI_IRQ_VECT_ID
#define IPI_IRQ_VECT_ID     65
#endif /* !IPI_IRQ_VECT_ID */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR      0xFF310000
#endif /* !POLL_BASE_ADDR */

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     0x01000000
#endif /* !IPI_CHN_BITMASK */

#endif /* versal */
#ifndef SHARED_MEM_PA
#if XPAR_CPU_ID == 0
#define SHARED_MEM_PA  0x3ED40000UL
#else
#define SHARED_MEM_PA  0x3EF40000UL
#endif /* XPAR_CPU_ID */
#endif /* !SHARED_MEM_PA */

#ifndef SHARED_MEM_SIZE
#define SHARED_MEM_SIZE 0x100000UL
#endif /* !SHARED_MEM_SIZE */

#ifndef SHARED_BUF_OFFSET
#define SHARED_BUF_OFFSET 0x8000UL
#endif /* !SHARED_BUF_OFFSET */

#endif /* _AMD_GENERATED_ */

/* Possible to control metal log build time */
#ifndef XLNX_METAL_LOG_LEVEL
#define XLNX_METAL_LOG_LEVEL METAL_LOG_INFO
#endif

void xlnx_log_handler(enum metal_log_level level, const char *format, ...);

#define XLNX_PLATFORM_METAL_INIT_PARAMS \
{ \
	.log_handler = xlnx_log_handler, \
	.log_level = XLNX_METAL_LOG_LEVEL, \
}

/* Polling information used by remoteproc operations. */
static metal_phys_addr_t poll_phys_addr = POLL_BASE_ADDR;
struct metal_device kick_device = {
	.name = "poll_dev",
	.bus = NULL,
	.num_regions = 1,
	.regions = {
		{
			.virt = (void *)POLL_BASE_ADDR,
			.physmap = &poll_phys_addr,
			.size = 0x1000,
			.page_shift = -1UL,
			.page_mask = -1UL,
			.mem_flags = DEVICE_NONSHARED | PRIV_RW_USER_RW,
			.ops = {NULL},
		}
	},
	.node = {NULL},
#ifndef RPMSG_NO_IPI
	.irq_num = 1,
	.irq_info = (void *)IPI_IRQ_VECT_ID,
#endif /* !RPMSG_NO_IPI */
};

static struct remoteproc_priv rproc_priv = {
	.kick_dev_name = KICK_DEV_NAME,
	.kick_dev_bus_name = KICK_BUS_NAME,
#ifndef RPMSG_NO_IPI
	.ipi_chn_mask = IPI_CHN_BITMASK,
#endif /* !RPMSG_NO_IPI */
};

static struct remoteproc rproc_inst;

/*
 * processor operations from r5 to a53. It defines
 * notification operation and remote processor management operations.
 */
extern const struct remoteproc_ops zynqmp_r5_a53_proc_ops;

/**
 * Definition of the interrupt controller will be provided by
 * libmetal_xlnx_extension.a module
 **/
extern struct metal_irq_controller xlnx_irq_cntr;

int system_interrupt_register(int int_num, void (*intr_handler)(void *),
			      void *data);

int xlnx_hw_to_bsp_irq(int sys_irq);

/* RPMsg virtio shared buffer pool */
static struct rpmsg_virtio_shm_pool shpool;

static void xlnx_irq_isr(void *arg)
{
	int vector;

	vector = (int)arg;

	if (vector >= xlnx_irq_cntr.irq_num || vector < 0) {
		metal_err("%s, irq %d out of range, max = %d\n",
			  __func__, vector, xlnx_irq_cntr.irq_num);
		return;
	}

	(void)metal_irq_handle(&xlnx_irq_cntr.irqs[vector], (int)vector);
}

static struct remoteproc *
platform_create_proc(int proc_index, int rsc_index)
{
	void *rsc_table;
	int rsc_size;
	int ret;
	metal_phys_addr_t pa;

	(void) proc_index;
	rsc_table = get_resource_table(rsc_index, &rsc_size);

	/* Register IPI device */
	if (metal_register_generic_device(&kick_device))
		return NULL;

	/* Initialize remoteproc instance */
	if (!remoteproc_init(&rproc_inst, &zynqmp_r5_a53_proc_ops, &rproc_priv))
		return NULL;

	/*
	 * Mmap shared memories
	 * Or shall we constraint that they will be set as carved out
	 * in the resource table?
	 */
	/* mmap resource table */
	pa = (metal_phys_addr_t)rsc_table;
	(void *)remoteproc_mmap(&rproc_inst, &pa,
				NULL, rsc_size,
				NORM_NSHARED_NCACHE|PRIV_RW_USER_RW,
				&rproc_inst.rsc_io);
	/* mmap shared memory */
	pa = SHARED_MEM_PA;
	(void *)remoteproc_mmap(&rproc_inst, &pa,
				NULL, SHARED_MEM_SIZE,
				NORM_NSHARED_NCACHE|PRIV_RW_USER_RW,
				NULL);

	/* parse resource table to remoteproc */
	ret = remoteproc_set_rsc_table(&rproc_inst, rsc_table, rsc_size);
	if (ret) {
		metal_err("Failed to initialize remoteproc\n");
		remoteproc_remove(&rproc_inst);
		return NULL;
	}
	metal_dbg("Initialize remoteproc successfully.\r\n");

	return &rproc_inst;
}

static int xlnx_machine_init(void)
{

	struct metal_init_params metal_param = XLNX_PLATFORM_METAL_INIT_PARAMS;
	int ret;

	metal_init(&metal_param);

	if (!kick_device.irq_info) {
		metal_err("invalid kick dev, irq info not available\n");
		return -EINVAL;
	}

	metal_dbg("kick device irq id = %d\n", (int)kick_device.irq_info);

	/* convert from HW irq number to bsp specific irq */
	kick_device.irq_info =
		(void *)xlnx_hw_to_bsp_irq((int)kick_device.irq_info);

	ret = system_interrupt_register((int)kick_device.irq_info,
					xlnx_irq_isr,
					(void *)kick_device.irq_info);
	if (ret) {
		metal_err("sys intr %d registration failed %d\n",
			  (int)kick_device.irq_info, ret);
		return ret;
	}

	/* Register Xilinx IRQ controller with libmetal */
	ret = metal_irq_register_controller(&xlnx_irq_cntr);
	if (ret < 0) {
		metal_err("irq controller registration with libmetal failed.\n");
		return ret;
	}

	return ret;
}

static void xlnx_machine_cleanup(void)
{
	metal_finish();
	free_resource_table();

	Xil_DCacheDisable();
	Xil_ICacheDisable();
	Xil_DCacheInvalidate();
	Xil_ICacheInvalidate();
}

void xlnx_log_handler(enum metal_log_level level, const char *format, ...)
{
	char msg[1024];
	va_list args;

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);

	if (level > metal_get_log_level())
		return;

	xil_printf("RPU%d: %s", XPAR_CPU_ID, msg);
}

int platform_init(int argc, char *argv[], void **platform)
{
	unsigned long proc_id = 0;
	unsigned long rsc_id = 0;
	struct remoteproc *rproc;
	int len = 0;
	int ret;

	/*
	 * Ensure resource table resource is set up before any attempts
	 * are made to cache the table.
	 */
	get_resource_table(0, &len);

	if (!platform)
		return -EINVAL;

	if (argc >= 2) {
		proc_id = strtoul(argv[1], NULL, 0);
	}

	if (argc >= 3) {
		rsc_id = strtoul(argv[2], NULL, 0);
	}

	/* Initialize HW machine components */
	ret = xlnx_machine_init();
	if (ret < 0) {
		metal_err("failed to init machine err %d\n", ret);
		return ret;
	}

	rproc = platform_create_proc(proc_id, rsc_id);
	if (!rproc) {
		metal_err("Failed to create remoteproc device.\r\n");
		return -EINVAL;
	}

	*platform = rproc;

	metal_dbg("platform init success\n");
	return 0;
}

struct  rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_ns_bind_cb ns_bind_cb)
{
	struct remoteproc *rproc = platform;
	struct rpmsg_virtio_device *rpmsg_vdev;
	struct virtio_device *vdev;
	void *shbuf;
	struct metal_io_region *shbuf_io;
	int ret;

	restore_initial_rsc_table();

	rpmsg_vdev = metal_allocate_memory(sizeof(*rpmsg_vdev));
	if (!rpmsg_vdev)
		return NULL;
	shbuf_io = remoteproc_get_io_with_pa(rproc, SHARED_MEM_PA);
	if (!shbuf_io)
		goto err1;
	shbuf = metal_io_phys_to_virt(shbuf_io,
				      SHARED_MEM_PA + SHARED_BUF_OFFSET);

	metal_dbg("creating remoteproc virtio\r\n");
	/* TODO: can we have a wrapper for the following two functions? */
	vdev = remoteproc_create_virtio(rproc, vdev_index, role, rst_cb);
	if (!vdev) {
		metal_err("failed remoteproc_create_virtio\r\n");
		goto err1;
	}

	metal_dbg("initializing rpmsg shared buffer pool\r\n");
	/* Only RPMsg virtio driver needs to initialize the shared buffers pool */
	rpmsg_virtio_init_shm_pool(&shpool, shbuf, SHARED_MEM_SIZE);

	metal_dbg("initializing rpmsg vdev\r\n");
	/* RPMsg virtio device can set shared buffers pool argument to NULL */
	ret =  rpmsg_init_vdev(rpmsg_vdev, vdev, ns_bind_cb,
			       shbuf_io,
			       &shpool);
	if (ret) {
		metal_err("failed rpmsg_init_vdev\r\n");
		goto err2;
	}

	metal_dbg("initializing rpmsg vdev\r\n");

	return rpmsg_virtio_get_rpmsg_device(rpmsg_vdev);
err2:
	remoteproc_remove_virtio(rproc, vdev);
err1:
	metal_free_memory(rpmsg_vdev);
	return NULL;
}

int platform_poll(void *priv)
{
	struct remoteproc *rproc = priv;
	struct remoteproc_priv *prproc;
	unsigned int flags;
	int ret;

	prproc = rproc->priv;
	while(1) {
#ifdef RPMSG_NO_IPI
		if (metal_io_read32(prproc->kick_io, 0)) {
			ret = remoteproc_get_notification(rproc,
							  RSC_NOTIFY_ID_ANY);
			if (ret)
				return ret;
			break;
		}
		(void)flags;
#else /* !RPMSG_NO_IPI */
		flags = metal_irq_save_disable();
		if (!(atomic_flag_test_and_set(&prproc->ipi_nokick))) {
			metal_irq_restore_enable(flags);
			ret = remoteproc_get_notification(rproc,
							  RSC_NOTIFY_ID_ANY);
			if (ret)
				return ret;
			break;
		}
		system_suspend();
		metal_irq_restore_enable(flags);
#endif /* RPMSG_NO_IPI */
	}
	return 0;
}

void platform_release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform)
{
	struct rpmsg_virtio_device *rpvdev;
	struct virtio_device *vdev;
	struct remoteproc *rproc;

	rpvdev = metal_container_of(rpdev, struct rpmsg_virtio_device, rdev);
	rproc = platform;
	vdev = rpvdev->vdev;

	rpmsg_deinit_vdev(rpvdev);
	remoteproc_remove_virtio(rproc, vdev);
	metal_free_memory(rpvdev);

}

void platform_cleanup(void *platform)
{
	struct remoteproc *rproc = platform;

	if (rproc)
		remoteproc_remove(rproc);

	xlnx_machine_cleanup();
}
