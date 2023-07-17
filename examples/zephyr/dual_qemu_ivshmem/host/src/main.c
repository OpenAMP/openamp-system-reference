/*
 * Copyright (c) 2023, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>

#include <zephyr/kernel.h>
#include <openamp/open_amp.h>
#include <zephyr/shell/shell.h>
#include "rpmsg_ivshmem_backend.h"

static struct rpmsg_endpoint ept;
struct rpmsg_device *rpmsg_dev;
static int number_of_completed_msgs;
K_SEM_DEFINE(rx_sem, 0, 1);

int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	number_of_completed_msgs++;

	printf("Remote side echoed the string back:\n");
	printf("[ %s ]\n", (const char *)data);
	printf("at message number %d\n\n", number_of_completed_msgs);

	k_sem_give(&rx_sem);
	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
}

void main(void)
{
	rpmsg_dev = get_rpmsg_ivshmem_device();

	if (!rpmsg_dev) {
		printf("Could not get the RPMsg device for IVSHMEM backend!\n");
		return;
	}

	/* Creates the RPMSg endpoint to communicate with the remote side.
	 */
	int status = rpmsg_create_ept(&ept, rpmsg_dev, "k", RPMSG_ADDR_ANY,
			get_rpmsg_ivshmem_ept_dest_addr(),
			endpoint_cb, rpmsg_service_unbind);
	if (status != 0) {
		printf("rpmsg_create_ept failed %d\n", status);
		return;
	}

	printf("Host Side, the communication over RPMsg is ready to use!\n");
}

static int cmd_rpmsg_ivshmem_send(const struct shell *sh, size_t argc, char **argv)
{
	char *str = argv[1];
	int noof_messages = strtol(argv[2], NULL, 10);

	if (!rpmsg_dev) {
		printf("RPMsg over IVSHMEM backend is not ready yet!\n");
		return -ENODEV;
	}

	number_of_completed_msgs = 0;

	do {
		int status = rpmsg_send(&ept, str, strlen(str) + 1);

		if (status < 0) {
			printf("failed to send message to the Remote side: %d\n", status);
			return status;
		}

		status = k_sem_take(&rx_sem, K_MSEC(5000));
		if (status) {
			printf("Remote side response timed out!\n");
			return status;
		}

		noof_messages--;

	} while (noof_messages);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_rpmsg_ivshmem,
			       SHELL_CMD_ARG(send, NULL,
					     "Usage: rpmsg_ivshmem send <string>"
					     "<number of messages>",
					     cmd_rpmsg_ivshmem_send, 3, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(rpmsg_ivshmem, &sub_rpmsg_ivshmem,
		       "Commands to use RPMsg over IVSHMEM",
		       cmd_rpmsg_ivshmem_send, 3, 0);
