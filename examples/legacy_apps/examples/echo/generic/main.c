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
#include "rpmsg-echo.h"

#define LPRINTF(fmt, ...) printf("%s():%u " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LPERROR(fmt, ...) LPRINTF("ERROR: " fmt, ##__VA_ARGS__)

/* Application entry point */
int main(int argc, char *argv[])
{
	void *platform;
	struct rpmsg_device *rpdev;
	int ret;

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
	ret = platform_init(argc, argv, &platform);
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

	/* Never reach here. */
	metal_info("Stopping application...\r\n");

	return -EINVAL;
}
