/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Resource table of the MicroBlaze-V OpenAMP remote: one RPMsg virtio
 * device with two vrings. The layout and the constants below are the
 * OpenAMP/virtio protocol values Linux remoteproc expects: resource table
 * version 1, the RPMsg device (VIRTIO_ID_RPMSG) with name-service
 * announcements, and vrings of 256 buffers aligned to 4 KB. Only the vring
 * addresses (RING_TX, RING_RX) come from the Lopper-generated
 * amd_platform_info.h.
 */

#include <stddef.h>
#include "platform_config.h"
#include "rsc_table.h"

#define RSC_SECTION __attribute__((section(".resource_table"), used))
#define RPMSG_FEATURES (1U << VIRTIO_RPMSG_F_NS)
#define VRING_ALIGN 0x1000U
#define VRING_NUM 256U

struct remote_resource_table resources RSC_SECTION = {
	.version = 1,
	.num = 1,
	.reserved = { 0, 0 },
	.offset = { offsetof(struct remote_resource_table, rpmsg_vdev) },
	.rpmsg_vdev = {
		.type = RSC_VDEV,
		.id = VIRTIO_ID_RPMSG,
		/* Notification ID of the vdev itself; the vrings use 1 and 2. */
		.notifyid = 31,
		.dfeatures = RPMSG_FEATURES,
		.gfeatures = 0,
		.config_len = 0,
		.status = 0,
		.num_of_vrings = 2,
		.reserved = { 0, 0 },
	},
	/* Linux binds the named DT carveouts and writes the negotiated device
	 * addresses and notification IDs back into this loaded BRAM table before
	 * releasing MB-V reset.
	 */
	.rpmsg_vring0 = { RING_TX, VRING_ALIGN, VRING_NUM, 1, 0 },
	.rpmsg_vring1 = { RING_RX, VRING_ALIGN, VRING_NUM, 2, 0 },
};

void *get_resource_table(int rsc_id, int *len)
{
	(void)rsc_id;
	*len = sizeof(resources);
	return &resources;
}
