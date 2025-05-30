/*
 * Copyright (C) 2022, Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * This is a sample demonstration application that showcases usage of proxy from the remote core.
 * This application is meant to run on the remote CPU running baremetal.
 * This applicationr can print to to host console and perform file I/O using proxy mechanism.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <openamp/open_amp.h>
#include <openamp/rpmsg_retarget.h>
#include "platform_info.h"
#include "rpmsg-rpc-demo.h"

#define RPC_BUFF_SIZE 1024
#define REDEF_O_CREAT 100
#define REDEF_O_EXCL 200
#define REDEF_O_RDONLY 0
#define REDEF_O_WRONLY 1
#define REDEF_O_RDWR 2
#define REDEF_O_APPEND 2000
#define REDEF_O_ACCMODE 3

#define raw_printf(format, ...) printf(format, ##__VA_ARGS__)
#define LPRINTF(format, ...) raw_printf("Host> " format, ##__VA_ARGS__)
#define LPERROR(format, ...) LPRINTF("ERROR: " format, ##__VA_ARGS__)

static void *platform;
static struct rpmsg_device *rpdev;
static struct rpmsg_endpoint app_ept;
static struct metal_io_region *shbuf_io;
static int request_termination = 0;
static int ept_deleted = 0;
static int err_cnt = 0;

static int copy_from_shbuf(void *dst, void *shbuf, int len)
{
	int ret;
	unsigned long offset = metal_io_virt_to_offset(shbuf_io, shbuf);

	if (offset == METAL_BAD_OFFSET) {
		LPERROR("no offset within IO region for data ptr: %p\r\n",
			shbuf);
		return -EINVAL;
	}

	ret = metal_io_block_read(shbuf_io, offset, dst, len);
	if (ret < 0)
		LPERROR("metal_io_block_read failed with err: %d\r\n", ret);

	return ret;
}

static int handle_open(struct rpmsg_rpc_syscall *syscall,
		       struct rpmsg_endpoint *ept)
{
	char *buf;
	struct rpmsg_rpc_syscall resp;
	int fd, ret;

	if (!syscall || !ept)
		return -EINVAL;
	buf = (char *)syscall;
	buf += sizeof(*syscall);

	/* Open remote fd */
	fd = open(buf, syscall->args.int_field1, syscall->args.int_field2);

	/* Construct rpc response */
	resp.id = OPEN_SYSCALL_ID;
	resp.args.int_field1 = fd;
	resp.args.int_field2 = 0;	/*not used */
	resp.args.data_len = 0;	/*not used */

	/* Transmit rpc response */
	ret = rpmsg_send(ept, (void *)&resp, sizeof(resp));

	return ret > 0 ?  0 : ret;
}

static int handle_close(struct rpmsg_rpc_syscall *syscall,
			struct rpmsg_endpoint *ept)
{
	struct rpmsg_rpc_syscall resp;
	int ret;

	if (!syscall || !ept)
		return -EINVAL;
	/* Close remote fd */
	ret = close(syscall->args.int_field1);

	/* Construct rpc response */
	resp.id = CLOSE_SYSCALL_ID;
	resp.args.int_field1 = ret;
	resp.args.int_field2 = 0;	/*not used */
	resp.args.data_len = 0;	/*not used */

	/* Transmit rpc response */
	ret = rpmsg_send(ept, &resp, sizeof(resp));

	return ret > 0 ?  0 : ret;
}

static int handle_read(struct rpmsg_rpc_syscall *syscall,
		       struct rpmsg_endpoint *ept)
{
	struct rpmsg_rpc_syscall *resp;
	unsigned char buf[RPC_BUFF_SIZE];
	unsigned char *payload;
	int bytes_read, payload_size;
	int ret;

	if (!syscall || !ept)
		return -EINVAL;
	payload = buf + sizeof(*resp);

	/*
	 * For STD_IN read up to the buf size. Otherwise read
	 * only the size requested in in syscall->rgs.int_field2
	 */
	bytes_read = sizeof(buf) - sizeof(*resp);
	if (!syscall->args.int_field1 && syscall->args.int_field2 < bytes_read)
		bytes_read = syscall->args.int_field2;

	bytes_read = read(syscall->args.int_field1, payload, bytes_read);

	/* Construct rpc response */
	resp = (struct rpmsg_rpc_syscall *)buf;
	resp->id = READ_SYSCALL_ID;
	resp->args.int_field1 = bytes_read;
	resp->args.int_field2 = 0;	/* not used */
	resp->args.data_len = bytes_read;

	payload_size = sizeof(*resp) +
		       ((bytes_read > 0) ? bytes_read : 0);

	/* Transmit rpc response */
	ret = rpmsg_send(ept, buf, payload_size);

	return ret > 0 ?  0 : ret;
}

static int handle_write(struct rpmsg_rpc_syscall *syscall,
			struct rpmsg_endpoint *ept)
{
	struct rpmsg_rpc_syscall resp;
	unsigned char *buf;
	int bytes_written;
	int ret;

	if (!syscall || !ept)
		return -EINVAL;
	buf = (unsigned char *)syscall;
	buf += sizeof(*syscall);
	/* Write to remote fd */
	bytes_written = write(syscall->args.int_field1, buf,
			      syscall->args.int_field2);

	/* Construct rpc response */
	resp.id = WRITE_SYSCALL_ID;
	resp.args.int_field1 = bytes_written;
	resp.args.int_field2 = 0;	/*not used */
	resp.args.data_len = 0;	/*not used */

	/* Transmit rpc response */
	ret = rpmsg_send(ept, (void *)&resp, sizeof(resp));

	return ret > 0 ?  0 : ret;
}

static int handle_rpc(struct rpmsg_rpc_syscall *syscall,
		      struct rpmsg_endpoint *ept)
{
	int retval;

	/* Handle RPC */
	switch (syscall->id) {
	case OPEN_SYSCALL_ID:
		{
			retval = handle_open(syscall, ept);
			break;
		}
	case CLOSE_SYSCALL_ID:
		{
			retval = handle_close(syscall, ept);
			break;
		}
	case READ_SYSCALL_ID:
		{
			retval = handle_read(syscall, ept);
			break;
		}
	case WRITE_SYSCALL_ID:
		{
			retval = handle_write(syscall, ept);
			break;
		}
	case TERM_SYSCALL_ID:
		{
			LPRINTF("Received termination request\r\n");
			request_termination = 1;
			retval = 0;
			break;
		}
	default:
		{
			LPERROR
			    ("Invalid RPC sys call ID: %d:%d!\r\n",
			     (int)syscall->id, (int)WRITE_SYSCALL_ID);
			retval = -1;
			break;
		}
	}

	return retval;
}
static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	(void)ept;
	rpmsg_destroy_ept(&app_ept);
	LPRINTF("Endpoint is destroyed\r\n");
	ept_deleted = 1;
}

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			      uint32_t src, void *priv)
{
	unsigned char buf[RPC_BUFF_SIZE];
	struct rpmsg_rpc_syscall *syscall;
	int ret;

	(void)priv;
	(void)src;

	if (len < (int)sizeof(*syscall)) {
		LPERROR("Received data is less than the rpc structure: %zd\r\n",
			len);
		err_cnt++;
		return RPMSG_SUCCESS;
	}

	/*
	 * In case the shared memory is device memory
	 * E.g. For now, we only use UIO device memory in Linux.
	 */
	if (len > RPC_BUFF_SIZE)
		len = RPC_BUFF_SIZE;
	ret = copy_from_shbuf(buf, data, len);
	if (ret < 0)
		return ret;

	syscall = (struct rpmsg_rpc_syscall *)buf;
	if (handle_rpc(syscall, ept)) {
		LPRINTF("\nHandling remote procedure call errors:\r\n");
		raw_printf("rpc id %d\r\n", syscall->id);
		raw_printf("rpc int field1 %d\r\n",
		       syscall->args.int_field1);
		raw_printf("\nrpc int field2 %d\r\n",
		       syscall->args.int_field2);
		err_cnt++;
	}
	return RPMSG_SUCCESS;
}


void terminate_rpc_app()
{
	LPRINTF("Destroying endpoint.\r\n");
	if (!ept_deleted)
		rpmsg_destroy_ept(&app_ept);
}

void exit_action_handler(int signum)
{
	(void)signum;
	terminate_rpc_app();
}

void kill_action_handler(int signum)
{
	(void)signum;
	LPRINTF("RPC service killed !!\r\n");

	terminate_rpc_app();

	if (rpdev)
		platform_release_rpmsg_vdev(rpdev, platform);
	if (platform)
		platform_cleanup(platform);
}

/* Application entry point */
int app(struct rpmsg_device *rdev, void *priv)
{
	int ret = 0;
	struct sigaction exit_action;
	struct sigaction kill_action;
	struct rpmsg_virtio_device *rvdev;

	/* Initialize signalling infrastructure */
	memset(&exit_action, 0, sizeof(struct sigaction));
	memset(&kill_action, 0, sizeof(struct sigaction));
	exit_action.sa_handler = exit_action_handler;
	kill_action.sa_handler = kill_action_handler;
	sigaction(SIGTERM, &exit_action, NULL);
	sigaction(SIGINT, &exit_action, NULL);
	sigaction(SIGKILL, &kill_action, NULL);
	sigaction(SIGHUP, &kill_action, NULL);

	/* Initialize RPMSG framework */
	LPRINTF("Try to create rpmsg endpoint.\r\n");

	ret = rpmsg_create_ept(&app_ept, rdev, RPMSG_SERVICE_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_endpoint_cb,
			       rpmsg_service_unbind);
	if (ret) {
		LPERROR("Failed to create endpoint.\r\n");
		return -EINVAL;
	}

	LPRINTF("Successfully created rpmsg endpoint.\r\n");
	rvdev = metal_container_of(rdev, struct rpmsg_virtio_device, rdev);
	shbuf_io = rvdev->shbuf_io;
	if (!shbuf_io) {
		LPERROR("no IO region for the application.\r\n");
		return -ENOMEM;
	}

	while(1) {
		platform_poll(priv);
		if (err_cnt) {
			LPERROR("Got error!\r\n");
			ret = -EINVAL;
			break;
		}
		/* we got a shutdown request, exit */
		if (ept_deleted || request_termination) {
			break;
		}
	}
	LPRINTF("\nRPC service exiting !!\r\n");

	terminate_rpc_app();
	return ret;
}

int main(int argc, char *argv[])
{
	int ret;

	LPRINTF("Starting application...\r\n");

	/* Initialize platform */
	ret = platform_init(argc, argv, &platform);
	if (ret) {
		LPERROR("Failed to initialize platform.\r\n");
		ret = -1;
	} else {
		rpdev = platform_create_rpmsg_vdev(platform, 0,
						   VIRTIO_DEV_DRIVER,
						   NULL, NULL);
		if (!rpdev) {
			LPERROR("Failed to create rpmsg virtio device.\r\n");
			ret = -1;
		} else {
			app(rpdev, platform);
			platform_release_rpmsg_vdev(rpdev, platform);
			ret = 0;
		}
	}

	LPRINTF("Stopping application...\r\n");
	platform_cleanup(platform);

	return ret;
}

