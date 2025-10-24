/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This sample code demonstrates how to use file system of host processor
 * using proxy mechanism. Proxy service is implemented on host processor.
 * This application can print to the host console, take input from host console
 * and perform regular file I/O such as open, read, write and close.
 */

#include <fcntl.h>
#include <metal/version.h>
#include <openamp/open_amp.h>
#include <openamp/rpmsg_retarget.h>
#include <openamp/version.h>
#include "platform_info.h"
#include "rpmsg-rpc-demo.h"
#include "rsc_table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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
		rpmsg_rpc_app(rpdev, platform);
		platform_release_rpmsg_vdev(rpdev, platform);
		ret = 0;
	}

	LPRINTF("Stopping application...\r\n");
	platform_cleanup(platform);

	return ret;
}
