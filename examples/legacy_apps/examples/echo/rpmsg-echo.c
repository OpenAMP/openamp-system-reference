/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This is a sample demonstration application that showcases usage of rpmsg
 * This application is meant to run on the remote CPU running baremetal code.
 * This application echoes back data that was sent to it by the host core.
 */

#include <stdio.h>
#include <openamp/open_amp.h>
#include <openamp/version.h>
#include <metal/alloc.h>
#include <metal/version.h>
#include "platform_info.h"
#include "rpmsg-echo.h"

#define SHUTDOWN_MSG	0xEF56A55A

#define LPRINTF(format, ...) printf(format, ##__VA_ARGS__)
#define LPERROR(format, ...) LPRINTF("ERROR: " format, ##__VA_ARGS__)

static struct rpmsg_endpoint lept;
static int shutdown_req = 0;

/*-----------------------------------------------------------------------------*
 *  RPMSG endpoint callbacks
 *-----------------------------------------------------------------------------*/
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			     uint32_t src, void *priv)
{
	(void)priv;
	(void)src;

	/* On reception of a shutdown we signal the application to terminate */
	if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
		LPRINTF("shutdown message is received.\r\n");
		shutdown_req = 1;
		return RPMSG_SUCCESS;
	}

	/* Send data back to host */
	if (rpmsg_send(ept, data, len) < 0) {
		LPERROR("rpmsg_send failed\r\n");
	}
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	LPRINTF("unexpected Remote endpoint destroy\r\n");
	shutdown_req = 1;
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
int rpmsg_echo_app(struct rpmsg_device *rdev, void *priv)
{
	int ret;

	/* Initialize RPMSG framework */
	LPRINTF("Try to create rpmsg endpoint.\r\n");

	ret = rpmsg_create_ept(&lept, rdev, RPMSG_SERVICE_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_endpoint_cb,
			       rpmsg_service_unbind);
	if (ret) {
		LPERROR("Failed to create endpoint.\r\n");
		return -1;
	}

	LPRINTF("Successfully created rpmsg endpoint.\r\n");

	LPRINTF("RPMsg device TX buffer size: %#x\r\n", rpmsg_get_tx_buffer_size(&lept));
	LPRINTF("RPMsg device RX buffer size: %#x\r\n", rpmsg_get_rx_buffer_size(&lept));

	while(1) {
		platform_poll(priv);
		/* we got a shutdown request, exit */
		if (shutdown_req) {
			break;
		}
	}
	rpmsg_destroy_ept(&lept);

	return 0;
}

/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
int __attribute__((weak)) main(int argc, char *argv[])
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
