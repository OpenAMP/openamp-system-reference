/*
 * Copyright (c) 2023, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>

#include <zephyr/kernel.h>
#include <openamp/open_amp.h>
#include "rpmsg_ivshmem_backend.h"

static struct rpmsg_endpoint remote_ept;
static struct rpmsg_device *rpmsg_dev;

int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	printf("Host side sent a string:\n");
	printf("[ %s ]\n", (const char *)data);
	printf("Now echoing it back!\n\n");

	int status = rpmsg_send(&remote_ept, data, len);

	if (status < 0)
		printf("failed to send message to the Host side: %d\n", status);

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
}

int main(void)
{
	rpmsg_dev = get_rpmsg_ivshmem_device();

	if (!rpmsg_dev) {
		printf("Could not get the RPMsg device for IVSHMEM backend!\n");
		return -1;
	}

	/* Setup the endpoint, this will notify the host side and allow it
	 * to finish the RPMsg communication estabilishement.
	 */
	int status = rpmsg_create_ept(&remote_ept, rpmsg_dev, "k", RPMSG_ADDR_ANY,
			RPMSG_ADDR_ANY, endpoint_cb, rpmsg_service_unbind);
	if (status != 0) {
		printf("rpmsg_create_ept failed %d\n", status);
		return status;
	}

	printf("Remote Side, the communication over RPMsg is ready to use!\n");

	return 0;
}
