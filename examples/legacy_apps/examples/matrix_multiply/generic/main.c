/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "matrix_multiply.h"
#include <metal/alloc.h>
#include <metal/log.h>
#include <metal/version.h>
#include <openamp/open_amp.h>
#include <openamp/version.h>
#include "platform_info.h"
#include <stdio.h>

#define LPRINTF(format, ...) metal_info(format, ##__VA_ARGS__)
#define LPERROR(format, ...) metal_err(format, ##__VA_ARGS__)

/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	void *platform;
	struct rpmsg_device *rpdev;
	int ret;

	/* Initialize platform */
	ret = platform_init(argc, argv, &platform);
	if (ret) {
		LPERROR("Failed to initialize platform.\r\n");
		return ret;
	}

	LPRINTF("openamp lib version: %s\n", openamp_version());
	LPRINTF("libmetal lib version: %s\n", metal_ver());
	LPRINTF("Starting application...\r\n");

	rpdev = platform_create_rpmsg_vdev(platform, 0,
					   VIRTIO_DEV_DEVICE,
					   NULL, NULL);
	if (!rpdev) {
		LPERROR("Failed to create rpmsg virtio device.\r\n");
		ret = -1;
	} else {
		rpmsg_matrix_app(rpdev, platform);
		platform_release_rpmsg_vdev(rpdev, platform);
		ret = 0;
	}

	LPRINTF("Stopping application...\r\n");
	platform_cleanup(platform);

	return ret;
}
