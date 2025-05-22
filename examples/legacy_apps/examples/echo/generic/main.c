/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/alloc.h>
#include <metal/log.h>
#include <metal/version.h>
#include <openamp/open_amp.h>
#include <openamp/version.h>
#include "platform_info.h"
#include "rpmsg-echo.h"
#include <stdio.h>

#define LPRINTF(format, ...) metal_info(format, ##__VA_ARGS__)
#define LPERROR(format, ...) metal_err("ERROR: " format, ##__VA_ARGS__)

/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	void *platform;
	struct rpmsg_device *rpdev;
	int ret;

	LPRINTF("openamp lib version: %s (", openamp_version());
	LPRINTF("Major: %d, ", openamp_version_major());
	LPRINTF("Minor: %d, ", openamp_version_minor());
	LPRINTF("Patch: %d)\r\n", openamp_version_patch());

	LPRINTF("libmetal lib version: %s (", metal_ver());
	LPRINTF("Major: %d, ", metal_ver_major());
	LPRINTF("Minor: %d, ", metal_ver_minor());
	LPRINTF("Patch: %d)\r\n", metal_ver_patch());

	LPRINTF("Starting application...\r\n");

	/* Initialize platform */
	ret = platform_init(argc, argv, &platform);
	if (ret) {
		LPERROR("Failed to initialize platform.\r\n");
		ret = -1;
	} else {
		rpdev = platform_create_rpmsg_vdev(platform, 0,
						   VIRTIO_DEV_DEVICE,
						   NULL, NULL);
		if (!rpdev) {
			LPERROR("Failed to create rpmsg virtio device.\r\n");
			ret = -1;
		} else {
			rpmsg_echo_app(rpdev, platform);
			platform_release_rpmsg_vdev(rpdev, platform);
			ret = 0;
		}
	}

	LPRINTF("Stopping application...\r\n");
	platform_cleanup(platform);

	return ret;
}
