/*
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <openamp/open_amp.h>
#include <openamp/version.h>
#include <metal/alloc.h>
#include <metal/log.h>
#include <metal/version.h>
#include "platform_info.h"
#include "matrix_multiply.h"
#include "FreeRTOS.h"
#include "task.h"

#define LPRINTF(format, ...) xil_printf(format, ##__VA_ARGS__)
#define LPERROR(fmt, ...) LPRINTF("ERROR: " fmt, ##__VA_ARGS__)

TaskHandle_t rpmsg_task;

/* RPMsg Task */
static void rpmsg_listen_task(void *unused_arg)
{
	void *platform;
	struct rpmsg_device *rpdev;
	int ret;

	(void)(int *)unused_arg;

	/* can't use metal_info, metal_log setup is in init_system */
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
	ret = platform_init(0, NULL, &platform);
	if (ret) {
		LPERROR("Failed to initialize platform.\r\n");
		metal_err("RPU reboot is required to recover\r\n");
		platform_cleanup(platform);
		/*
		 * If main function is returned in baremetal firmware,
		 * RPU behavior is undefined. It's better to wait in
		 * an infinite loop instead
		 */
		while (1)
			;
	}

	/*
	 * If host detach from remoteproc device, then destroy current rpmsg
	 * device and create new one.
	 */
	while (1) {
		rpdev = platform_create_rpmsg_vdev(platform, 0,
						   VIRTIO_DEV_DEVICE,
						   NULL, NULL);
		if (!rpdev) {
			metal_err("Failed to create rpmsg virtio device.\r\n");
			metal_err("RPU reboot is required to recover\r\n");
			platform_cleanup(platform);

			/*
			 * If main function is returned in baremetal firmware,
			 * RPU behavior is undefined. It's better to wait in
			 * an infinite loop instead
			 */
			while (1)
				;
		}

		app(rpdev, platform);
		platform_release_rpmsg_vdev(rpdev, platform);
	}

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
