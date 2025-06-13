/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
#include "FreeRTOS.h"
#include <openamp/open_amp.h>
#include <openamp/rpmsg_retarget.h>
#include "platform_info.h"
#include "rpmsg-rpc-demo.h"
#include "rsc_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"
#include <unistd.h>

#define LPRINTF(format, ...) metal_info(format, ##__VA_ARGS__)
#define LPERROR(format, ...) metal_err("ERROR: " format, ##__VA_ARGS__)

TaskHandle_t rpmsg_task;

/* RPMsg Task */
static void rpmsg_listen_task(void *unused_arg)
{
	void *platform;
	struct rpmsg_device *rpdev;

	(void)(int *)unused_arg;

	LPRINTF("Starting application...\r\n");

	/* Initialize platform */
	if (platform_init(NULL, NULL, &platform)) {
		LPERROR("Failed to initialize platform.\r\n");
	} else {
		rpdev = platform_create_rpmsg_vdev(platform, 0,
						   VIRTIO_DEV_DEVICE,
						   NULL, NULL);
		if (!rpdev) {
			LPERROR("Failed to create rpmsg virtio device.\r\n");
		} else {
			rpc_remote_app(rpdev, platform);
			platform_release_rpmsg_vdev(rpdev, platform);
		}
	}

	LPRINTF("Stopping application...\r\n");
	platform_cleanup(platform);

	/* Terminate this task */
	vTaskDelete(NULL);
}

/* Application entry point */
int main(void)
{
	BaseType_t stat;

	/* Create the tasks */
	stat = xTaskCreate(rpmsg_listen_task, (const char *)"RPMsg task", 1024, NULL, 2,
			   &rpmsg_task);
	if (stat != pdPASS) {
		LPERROR("cannot create task\r\n");
		while(1);
	}

	/* Start running FreeRTOS tasks */
	vTaskStartScheduler();

	/* Will not get here, unless a call is made to vTaskEndScheduler() */
	while (1);

	/* suppress compilation warnings*/
	return 0;
}
