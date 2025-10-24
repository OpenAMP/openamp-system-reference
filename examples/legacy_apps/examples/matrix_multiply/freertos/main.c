/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "matrix_multiply.h"
#include <metal/alloc.h>
#include <metal/log.h>
#include <metal/version.h>
#include <openamp/open_amp.h>
#include <openamp/version.h>
#include "platform_info.h"
#include <stdio.h>
#include "task.h"

#define LPRINTF(format, ...) metal_info(format, ##__VA_ARGS__)
#define LPERROR(fmt, ...) metal_err(fmt, ##__VA_ARGS__)

TaskHandle_t rpmsg_task;

/* RPMsg Task */
static void rpmsg_listen_task(__attribute__((unused)) void *arg)
{
	void *platform;
	struct rpmsg_device *rpdev;
	int ret;

	/* Initialize platform */
	ret = platform_init(0, NULL, &platform);
	if (ret) {
		LPERROR("Failed to initialize platform.\r\n");
		vTaskDelete(NULL);
		return;
	}

	metal_info("\n");
	metal_info("FreeRTOS Version: %u.%u.%u\n",
		   tskKERNEL_VERSION_MAJOR,
		   tskKERNEL_VERSION_MINOR,
		   tskKERNEL_VERSION_BUILD);

	LPRINTF("openamp lib version: %s\n", openamp_version());
	LPRINTF("libmetal lib version: %s\n", metal_ver());

	LPRINTF("Starting application...\r\n");

	while (1) {
		rpdev = platform_create_rpmsg_vdev(platform, 0,
						   VIRTIO_DEV_DEVICE,
						   NULL, NULL);
		if (!rpdev) {
			LPERROR("Failed to create rpmsg virtio device.\r\n");
			break;
		}

		rpmsg_matrix_app(rpdev, platform);
		platform_release_rpmsg_vdev(rpdev, platform);
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
		while (1);
	}

	/* Start running FreeRTOS tasks */
	vTaskStartScheduler();

	/* Will not get here, unless a call is made to vTaskEndScheduler() */
	while (1);

	/* suppress compilation warnings*/
	return 0;
}
