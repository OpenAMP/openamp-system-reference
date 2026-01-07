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
#include "rsc_table.h"

/* Place resource table in special ELF section */
#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)
#define __resource_metadata     __section_t(.resource_table_metadata)

#define RSC_TBL_XLNX_MAGIC	((uint32_t)'x' << 24 | (uint32_t)'a' << 16 | \
				 (uint32_t)'m' << 8 | (uint32_t)'p')
 
#define RPMSG_VDEV_DFEATURES        (1 << VIRTIO_RPMSG_F_NS)

/* VirtIO rpmsg device id */
#define VIRTIO_ID_RPMSG_             7

#define NUM_VRINGS                  0x02
#define VRING_ALIGN                 0x1000
#ifndef RING_TX
#define RING_TX                     FW_RSC_U32_ADDR_ANY
#endif /* !RING_TX */
#ifndef RING_RX
#define RING_RX                     FW_RSC_U32_ADDR_ANY
#endif /* RING_RX */
#define VRING_SIZE                  256

#define NUM_TABLE_ENTRIES           1

static struct remote_resource_table *initial_resources;

struct remote_resource_table __resource resources = {
	/* Version */
	1,

	/* NUmber of table entries */
	NUM_TABLE_ENTRIES,
	/* reserved fields */
	{0, 0,},

	/* Offsets of rsc entries */
	{
	 offsetof(struct remote_resource_table, rpmsg_vdev),
	 },

	/* Virtio device entry */
	{
	 RSC_VDEV, VIRTIO_ID_RPMSG_, 31, RPMSG_VDEV_DFEATURES, 0, 0, 0,
	 NUM_VRINGS, {0, 0},
	 },

	/* Vring rsc entry - part of vdev rsc entry */
	{RING_TX, VRING_ALIGN, VRING_SIZE, 1, 0},
	{RING_RX, VRING_ALIGN, VRING_SIZE, 2, 0},
};

struct remote_resource_table_metadata __resource_metadata resources_metadata = {
	.version = 1,
	.magic_num = RSC_TBL_XLNX_MAGIC,
	.comp_magic_num = (~RSC_TBL_XLNX_MAGIC),
	.rsc_tbl_size = sizeof(resources),
	.rsc_tbl = (uintptr_t)&resources
};

void *get_resource_table (int rsc_id, int *len)
{
	(void) rsc_id;
	*len = sizeof(resources);
	/* make copy of resources to restore later */
	if (!initial_resources) {
		initial_resources = (struct remote_resource_table *)malloc(*len);
		memcpy(initial_resources, &resources, *len);
	}

	return &resources;
}

void free_resource_table (void)
{
	if (initial_resources)
		free(initial_resources);
}

void restore_initial_rsc_table (void)
{
	if (initial_resources)
		memcpy(&resources, initial_resources, sizeof(resources));
}
