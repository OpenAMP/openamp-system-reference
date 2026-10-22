/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2015 Xilinx, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This file populates resource table for BM remote
 * for use by the Linux host
 */

#include <openamp/open_amp.h>

#ifdef _AMD_GENERATED_
#include "amd_platform_info.h"
#endif

#include "platform_info.h"
#include "rsc_table.h"

/* Place resource table in special ELF section */
#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)
#define __resource_metadata     __section_t(.resource_table_metadata)

#define RSC_TBL_XLNX_MAGIC	((uint32_t)'x' << 24 | (uint32_t)'a' << 16 | \
				 (uint32_t)'m' << 8 | (uint32_t)'p')

#define RPMSG_VDEV_DFEATURES        (1U << VIRTIO_RPMSG_F_NS | \
				     1U << VIRTIO_RPMSG_F_BUFSZ)

#define RPMSG_BUF_ALIGN		    64
#define RPMSG_RX_BUF_SIZE_UNALIGNED 4096
#define RPMSG_TX_BUF_SIZE_UNALIGNED 4096

/* Driver (remote) to device (local) Buf size */
#define RPMSG_RX_BUF_SIZE	metal_align_up(RPMSG_RX_BUF_SIZE_UNALIGNED, RPMSG_BUF_ALIGN)

/* Device (local) to driver (remote) Buf size */
#define RPMSG_TX_BUF_SIZE	metal_align_up(RPMSG_TX_BUF_SIZE_UNALIGNED, RPMSG_BUF_ALIGN)

/* VirtIO rpmsg device id */
#define VIRTIO_ID_RPMSG_             7

#define NUM_VRINGS                  0x02
#define VRING_ALIGN                 RPMSG_BUF_ALIGN
#ifndef RING_TX
#define RING_TX                     FW_RSC_U32_ADDR_ANY
#endif /* !RING_TX */
#ifndef RING_RX
#define RING_RX                     FW_RSC_U32_ADDR_ANY
#endif /* RING_RX */

#define RX_VRING_SIZE	32 /* number of rx vrings/rpmsg bufs */
#define TX_VRING_SIZE	32 /* number of tx vrings/rpmsg bufs */

#define SHMEM_SIZE_REQUIRED ((RPMSG_RX_BUF_SIZE * RX_VRING_SIZE) + \
			     (RPMSG_TX_BUF_SIZE * TX_VRING_SIZE))

#if (SHMEM_SIZE_REQUIRED > (SHARED_MEM_SIZE - SHARED_BUF_OFFSET))
#error "error: allocated vdev buf size is not enough for rpmsg buffers"
#endif

/* trace buffer total size */
#define RSC_TRACE_SZ		    4096

/* Number of resources provided by the resource table */
#define NUM_TABLE_ENTRIES           2

static char rsc_trace_buf[RSC_TRACE_SZ];

struct remote_resource_table __resource resources = {
	/* Version */
	1,

	/* NUmber of table entries */
	NUM_TABLE_ENTRIES,
	/* reserved fields */
	{0, 0,},

	/* Offsets of rsc entries */
	.offset[0] = offsetof(struct remote_resource_table, rpmsg_vdev),
	.offset[1] = offsetof(struct remote_resource_table, rsc_trace),

	/* Virtio device entry */
	.rpmsg_vdev = {
		RSC_VDEV, VIRTIO_ID_RPMSG_, 31, RPMSG_VDEV_DFEATURES, 0,
		/* vdev config space len */
		RPMSG_VIRTIO_CONFIG_SIZE, 0,
		NUM_VRINGS, {0, 0},
	},

	/* Vring rsc entry - part of vdev rsc entry */
	{RING_TX, VRING_ALIGN, TX_VRING_SIZE, 1, 0},
	{RING_RX, VRING_ALIGN, RX_VRING_SIZE, 2, 0},
	/* vdev config space */
	.vdev_config = {
		.version = 1,
		.size = RPMSG_VIRTIO_CONFIG_SIZE,
		.drv2dev_buf_size = RPMSG_RX_BUF_SIZE,
		.dev2drv_buf_size = RPMSG_TX_BUF_SIZE,
	},
	/* trace buffer for logs, accessible via debugfs */
	.rsc_trace = {
		.type =		RSC_TRACE,
		.da =		(uint32_t)rsc_trace_buf,
		.len =		sizeof(rsc_trace_buf),
		.reserved =	0,
		.name =		"r5_trace",
	},
};

struct remote_resource_table_metadata __resource_metadata resources_metadata = {
	.version = 1,
	.magic_num = RSC_TBL_XLNX_MAGIC,
	.comp_magic_num = (~RSC_TBL_XLNX_MAGIC),
	.rsc_tbl_size = sizeof(resources),
	.rsc_tbl = (uintptr_t)&resources
};

char *get_rsc_trace_info(uint32_t *len)
{
	*len = sizeof(rsc_trace_buf);
	return rsc_trace_buf;
}

void *get_resource_table (int rsc_id, int *len)
{
	(void) rsc_id;
	*len = sizeof(resources);
	return &resources;
}
