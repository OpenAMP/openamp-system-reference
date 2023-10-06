/*
 * Copyright (c) 2023, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/sys/printk.h>

#define LOG_MODULE_NAME	rpmsg_ivshmem_backend
#define LOG_LEVEL CONFIG_IVSHMEM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "rpmsg_ivshmem_backend.h"

#define VDEV_STATUS_SIZE 0x400
#define VRING_COUNT 2
#define VRING_ALIGNMENT 4
#define VRING_SIZE 16
#define IVSHMEM_EV_LOOP_STACK_SIZE 8192

#if CONFIG_OPENAMP_MASTER
#define VIRTQUEUE_ID 0
#else
#define VIRTQUEUE_ID 1
#endif

#define GREEN \x1b[32m
#define RESET \x1b[0m

K_THREAD_STACK_DEFINE(ivshmem_ev_loop_stack, IVSHMEM_EV_LOOP_STACK_SIZE);
static struct k_thread ivshmem_ev_loop_thread;

static const struct device *ivshmem_dev =
		DEVICE_DT_GET(DT_NODELABEL(ivshmem0));

static metal_phys_addr_t shm_physmap[1];
static struct metal_device shm_device = {
	.name = "ivshmem0",
	.bus = NULL,
	.num_regions = 1,
	{
		{
			.virt       = (void *)0,
			.physmap    = shm_physmap,
			.size       = 0,
			.page_shift = (metal_phys_addr_t)(-1),
			.page_mask  = (metal_phys_addr_t)(-1),
			.mem_flags  = 0,
			.ops        = { NULL },
		},
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

static struct virtio_vring_info rvrings[2] = {
	[0] = {
		.info.align = VRING_ALIGNMENT,
	},
	[1] = {
		.info.align = VRING_ALIGNMENT,
	},
};

static struct virtio_device vdev;
static struct rpmsg_virtio_device rvdev;
static struct metal_io_region *io;
static struct virtqueue *vq[2];

#ifdef CONFIG_OPENAMP_MASTER
K_SEM_DEFINE(ept_sem, 0, 1);
static struct rpmsg_virtio_shm_pool shpool;
struct rpmsg_device *rpmsg_ivshmem_rdev;
#endif

static uintptr_t shmem_base;
static uintptr_t vdev_status_base;
static uintptr_t rx_vring_base;
static uintptr_t tx_vring_base;
static size_t shmem_size;
static int remote_endpoint_dst_addr = -1;

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
#ifdef CONFIG_OPENAMP_MASTER
	return VIRTIO_CONFIG_STATUS_DRIVER_OK;
#else
	return sys_read8(vdev_status_base);
#endif
}

static void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, vdev_status_base);
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return 1 << VIRTIO_RPMSG_F_NS;
}

static void virtio_set_features(struct virtio_device *vdev,
				uint32_t features)
{
}

static void virtio_notify(struct virtqueue *vq)
{
	uint16_t peer_dest_id = ivshmem_get_id(ivshmem_dev);
#ifdef CONFIG_OPENAMP_MASTER
	peer_dest_id += 1;
#else
	peer_dest_id -= 1;
#endif
	LOG_DBG("sending notification to the peer id 0x%x\n", peer_dest_id);
	ivshmem_int_peer(ivshmem_dev, peer_dest_id, 0);
}

struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.set_features = virtio_set_features,
	.notify = virtio_notify,
};

#ifdef CONFIG_OPENAMP_MASTER

void ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
	rpmsg_ivshmem_rdev = rdev;
	remote_endpoint_dst_addr = dest;
	k_sem_give(&ept_sem);
}

#endif

/* IVSHMEM receives DOORBELL notifications in form of a event loop based in k_poll*/
static void ivshmem_event_loop_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* k_poll was signaled or not */
	unsigned int poll_signaled;
	/* vector received */
	int ivshmem_vector_rx;
	int ret;

	struct k_poll_signal sig;

	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &sig),
	};

	k_poll_signal_init(&sig);

	ret = ivshmem_register_handler(ivshmem_dev, &sig, 0);

	if (ret < 0) {
		LOG_ERR("registering handlers must be supported: %d\n", ret);
		k_panic();
	}

	while (1) {
		LOG_DBG("%s: waiting interrupt from client...\n", __func__);
		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);

		k_poll_signal_check(&sig, &poll_signaled, &ivshmem_vector_rx);
		/* get ready for next signal */
		k_poll_signal_reset(&sig);

		/* notify receive Vqueue once cross interrupt is received */
		virtqueue_notification(vq[VIRTQUEUE_ID]);
	}
}

int init_ivshmem_backend(void)
{
	int status = 0;
	struct metal_device *device;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	if (!IS_ENABLED(CONFIG_IVSHMEM_DOORBELL)) {
		LOG_ERR("CONFIG_IVSHMEM_DOORBELL is not enabled\n");
		k_panic();
	}

	shmem_size = ivshmem_get_mem(ivshmem_dev, &shmem_base);
	printk(STRINGIFY(GREEN)
	       "shmem mapped at %#lx, %ld byte(s).\n" STRINGIFY(RESET), shmem_base, shmem_size);

	shmem_size -= VDEV_STATUS_SIZE;
	vdev_status_base = shmem_base;
	shmem_base += VDEV_STATUS_SIZE;

	LOG_DBG("Memory got from ivshmem: %p, size: %ld\n", (void *)shmem_base, shmem_size);

	shm_device.regions[0].virt = (void *)(shmem_base);
	shm_device.regions[0].size = shmem_size;
	shm_physmap[0] = (metal_phys_addr_t)(shmem_base);

#ifdef CONFIG_OPENAMP_MASTER
	virtio_set_status(NULL, 0);
#endif

	k_thread_create(&ivshmem_ev_loop_thread,
			ivshmem_ev_loop_stack,
			IVSHMEM_EV_LOOP_STACK_SIZE,
			(k_thread_entry_t)ivshmem_event_loop_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	status = metal_init(&metal_params);
	if (status != 0) {
		LOG_ERR("metal_init: failed - error code %d\n", status);
		return status;
	}

	status = metal_register_generic_device(&shm_device);
	if (status != 0) {
		LOG_ERR("Couldn't register shared memory device: %d\n", status);
		return status;
	}

	status = metal_device_open("generic", "ivshmem0", &device);
	if (status != 0) {
		LOG_ERR("metal_device_open failed: %d\n", status);
		return status;
	}

	io = metal_device_io_region(device, 0);
	if (!io) {
		LOG_ERR("metal_device_io_region failed to get region\n");
		return status;
	}

	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (!vq[0]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[0]\n");
		return status;
	}
	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (!vq[1]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[1]\n");
		return status;
	}

	rx_vring_base = vdev_status_base + shmem_size - VDEV_STATUS_SIZE;
	tx_vring_base = vdev_status_base + shmem_size;

	LOG_DBG("rx_vring_address: %p - tx_vring_address: %p\n",
		(void *)rx_vring_base,
		(void *)tx_vring_base);

	rvrings[0].io = io;
	rvrings[0].info.vaddr = (void *)(rx_vring_base);
	rvrings[0].info.num_descs = VRING_SIZE;
	rvrings[0].info.align = VRING_ALIGNMENT;
	rvrings[0].vq = vq[0];

	rvrings[1].io = io;
	rvrings[1].info.vaddr = (void *)(tx_vring_base);
	rvrings[1].info.num_descs = VRING_SIZE;
	rvrings[1].info.align = VRING_ALIGNMENT;
	rvrings[1].vq = vq[1];

	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	vdev.vrings_info = &rvrings[0];

#ifdef CONFIG_OPENAMP_MASTER
	vdev.role = RPMSG_HOST;

	rpmsg_virtio_init_shm_pool(&shpool, (void *)(shmem_base), shmem_size);
	status = rpmsg_init_vdev(&rvdev, &vdev, ns_bind_cb, io, &shpool);
	if (status != 0) {
		LOG_ERR("rpmsg_init_vdev failed %d\n", status);
		return status;
	}
	k_sem_take(&ept_sem, K_FOREVER);

#else
	vdev.role = RPMSG_REMOTE;
	status = rpmsg_init_vdev(&rvdev, &vdev, NULL, io, NULL);
	if (status != 0) {
		LOG_ERR("rpmsg_init_vdev failed %d\n", status);
		return status;
	}
#endif

	return 0;
}

SYS_INIT(init_ivshmem_backend, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

struct rpmsg_device *get_rpmsg_ivshmem_device(void)
{
#ifdef CONFIG_OPENAMP_MASTER
	return rpmsg_ivshmem_rdev;
#else
	return rpmsg_virtio_get_rpmsg_device(&rvdev);
#endif
}

int get_rpmsg_ivshmem_ept_dest_addr(void)
{
	return remote_endpoint_dst_addr;
}
