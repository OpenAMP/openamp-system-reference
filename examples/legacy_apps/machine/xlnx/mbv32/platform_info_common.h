/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_COMMON_H
#define PLATFORM_INFO_COMMON_H

#include <openamp/remoteproc.h>
#include <openamp/rpmsg.h>
#include <openamp/virtio.h>

int platform_init(int argc, char *argv[], void **platform);
struct rpmsg_device *platform_create_rpmsg_vdev(
	void *platform, unsigned int vdev_index, unsigned int role,
	void (*rst_cb)(struct virtio_device *vdev), rpmsg_ns_bind_cb ns_bind_cb);
int platform_poll(void *platform);
void platform_release_rpmsg_vdev(struct rpmsg_device *rpdev, void *platform);
void platform_cleanup(void *platform);

#endif
