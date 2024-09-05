/*
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
#include <metal/log.h>
#include <metal/version.h>
#include "platform_info.h"
#include "rpmsg-echo.h"

#define LPRINTF(fmt, ...) printf("%s():%u " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LPERROR(fmt, ...) LPRINTF("ERROR: " fmt, ##__VA_ARGS__)

#define U_BOOT_EPT 1035
#define U_BOOT_RPMSG_CHANNEL "U-Boot-rpmsg"
#define RPMSG_CLIENT_SAMPLE_CH "rpmsg-client-sample"

static struct rpmsg_endpoint lept, uboot_ept, rpcs_ept;

#ifdef USE_FREERTOS
extern TaskHandle_t rpmsg_task;
#endif /* USE_FREERTOS */

/*-----------------------------------------------------------------------------*
 *  RPMSG endpoint callbacks
 *-----------------------------------------------------------------------------*/
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			     uint32_t src, void *priv)
{
	(void)priv;
	(void)src;

	/* Send data back to host */
	if (rpmsg_send(ept, data, len) < 0) {
		metal_err("rpmsg_send failed\r\n");
	}
#ifdef USE_FREERTOS
	xTaskResumeFromISR(rpmsg_task);
#endif /* USE_FREERTOS */
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	metal_info("ept %s, src=0x%x, dst=0x%x, destroyed by host\r\n",
		ept->name, ept->addr, ept->dest_addr);
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
int app(struct rpmsg_device *rdev, void *priv)
{
	int ret;

	/* Initialize RPMSG framework */
	metal_info("Try to create rpmsg endpoints.\r\n");

	/*
	 * Initially U-Boot tx/rx over fix endpoint.
	 * Initial implementation of rpmsg in U-boot is expecting fix endpoint
	 * This will be changed in future when name-service support will be
	 * added in U-Boot. Till that time maintain this fix endpoint channel.
	 */
	ret = rpmsg_create_ept(&uboot_ept, rdev, U_BOOT_RPMSG_CHANNEL,
			       U_BOOT_EPT, U_BOOT_EPT,
			       rpmsg_endpoint_cb,
			       rpmsg_service_unbind);
	if (ret) {
		metal_err("Failed to create %s endpoint.\r\n", U_BOOT_RPMSG_CHANNEL);
		return -1;
	}

	/*
	 * create endpoint to communicate with rpmsg-client-sample driver
	 * This channel communicate with rpmsg-client-sample driver in
	 * linux kernel.
	 */
	ret = rpmsg_create_ept(&rpcs_ept, rdev, RPMSG_CLIENT_SAMPLE_CH,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_endpoint_cb,
			       rpmsg_service_unbind);
	if (ret) {
		metal_err("Failed to create %s endpoint.\r\n", RPMSG_CLIENT_SAMPLE_CH);
		return -1;
	}

	/* create endpoint to communicate with echo_test app */
	ret = rpmsg_create_ept(&lept, rdev, RPMSG_SERVICE_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_endpoint_cb,
			       rpmsg_service_unbind);
	if (ret) {
		metal_err("Failed to create endpoint.\r\n");
		return -1;
	}

	metal_info("Successfully created rpmsg endpoint.\r\n");
	ret = platform_poll_on_vdev_reset(rdev, priv);

	return ret;
}
