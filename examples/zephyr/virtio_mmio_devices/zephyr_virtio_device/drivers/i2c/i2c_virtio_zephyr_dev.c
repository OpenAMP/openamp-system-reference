/*
 * Copyright (c) 2023, STMICROLECTRONICS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT virtio_i2c_dev

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include "virtio_i2c_dev.h"

struct i2c_virtio_data {
	const struct device		*parent;
	struct i2c_target_config	*broadcast;
	sys_slist_t			targets;

	struct k_sem			sem;
	struct k_thread			thread;
	k_thread_stack_t		*thread_stack;

	struct virtio_i2c_dev		i2c_dev;
};

#define DEV_DATA(dev)	\
	((struct i2c_virtio_data * const)(dev)->data)

#define I2C_VIRTIO_STACK_SIZE	512

static struct i2c_target_config *find_address(struct i2c_virtio_data *data,
					      uint16_t address)
{
	struct i2c_target_config *cfg = NULL;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&data->targets, node) {
		cfg = CONTAINER_OF(node, struct i2c_target_config, node);

		if (cfg->address == address)
			return cfg;
	}

	return NULL;
}

static int i2c_virtio_bus_callback(const struct device *dev,
				   struct virtio_i2c_msg msg)
{
	struct i2c_virtio_data *data = DEV_DATA(dev);
	struct i2c_target_config *cfg;
	struct i2c_msg msg2 = {
		.buf = msg.buf,
		.flags = msg.flags,
		.len = msg.len
	};

	if (data->broadcast) {
		data->broadcast->address = msg.addr;
		cfg = data->broadcast;
	} else
		cfg = find_address(data, msg.addr);

	if (!cfg)
		return -1;

	if (cfg->callbacks->transfer)
		return cfg->callbacks->transfer(cfg, msg2);
	return -1;
}

static int i2c_virtio_runtime_configure(const struct device *dev,
					uint32_t config)
{
	return 0;
}

static int i2c_virtio_target_register(const struct device *dev,
				      struct i2c_target_config *config)
{
	struct i2c_virtio_data *data = DEV_DATA(dev);

	if (!config)
		return -EINVAL;

	/* if address is 0xff then it means that any address is allowed */
	if (config->address == 0xff) {
		if (data->broadcast)
			return -EINVAL;

		data->broadcast = config;
		return 0;
	}

	if (config->address == 0xff)
		data->broadcast = config;

	/* Check the address is unique */
	if (find_address(data, config->address))
		return -EINVAL;

	sys_slist_append(&data->targets, &config->node);

	return 0;
}

static int i2c_virtio_target_unregister(const struct device *dev,
					struct i2c_target_config *config)
{
	struct i2c_virtio_data *data = DEV_DATA(dev);

	if (!config)
		return -EINVAL;

	if (config == data->broadcast) {
		if (!data->broadcast)
			return -EINVAL;

		data->broadcast->address = 0xff;

		data->broadcast = NULL;
		return 0;
	}

	if (!sys_slist_find_and_remove(&data->targets, &config->node))
		return -EINVAL;

	return 0;
}

static void i2c_virtio_it_callback(struct virtqueue *vq)
{
	struct i2c_virtio_data *data = vq->priv;

	/* notify the thread that a buffer is available */
	k_sem_give(&data->sem);
}


static void i2c_virtio_task(struct i2c_virtio_data *data,
			    void *u2,
			    void *u3)
{
rst:
	/** wait for the virtio device to be fully initialized */
	while (!virtio_i2c_ready(&data->i2c_dev))
		k_msleep(500);

	/** We set a callback on the virtqueue to be notified when a buffer is available */
	if (virtio_i2c_configure_avail_callback(&data->i2c_dev, i2c_virtio_it_callback, data)) {
		virtio_set_status(data->i2c_dev.vdev, VIRTIO_CONFIG_STATUS_NEEDS_RESET);
		goto rst;
	}

	/** The thread will keep waiting for available buffers
	 * We need this thread to get out of the interruption context
	 */
	while (virtio_i2c_ready(&data->i2c_dev)) {

		/** when the semaphore is given we have a buffer available */
		k_sem_take(&data->sem, K_FOREVER);

		virtio_i2c_handle_avail(&data->i2c_dev);
	}

	goto rst;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_virtio_runtime_configure,
	.target_register = i2c_virtio_target_register,
	.target_unregister = i2c_virtio_target_unregister,
};

static int i2c_virtio_init(const struct device *dev)
{
	struct i2c_virtio_data *data = DEV_DATA(dev);

	/** list used for the I2C target api */
	sys_slist_init(&data->targets);

	/** Configure the i2c device */
	virtio_i2c_configure(&data->i2c_dev, data->parent->api, i2c_virtio_bus_callback, dev);

	/** once everything is ready we start the thread */
	k_sem_init(&data->sem, 0, 1);

	k_thread_create(&data->thread,
			data->thread_stack,
			I2C_VIRTIO_STACK_SIZE,
			i2c_virtio_task,
			data,
			NULL,
			NULL,
			K_PRIO_COOP(7),
			0,
			K_NO_WAIT);

	return 0;
}

#define I2C_DEVICE_INIT_VIRTIO(n)						\
	K_THREAD_STACK_DEFINE(i2c_virtio_stack_##id, I2C_VIRTIO_STACK_SIZE);	\
										\
	static struct i2c_virtio_data i2c_virtio_data_##n = {			\
		.parent = DEVICE_DT_GET(DT_PARENT(DT_INST(n, DT_DRV_COMPAT))),	\
		.thread_stack = i2c_virtio_stack_##id,				\
	};	\
		\
	I2C_DEVICE_DT_INST_DEFINE(n,		\
			i2c_virtio_init, NULL,	\
			&i2c_virtio_data_##n,	\
			NULL, POST_KERNEL,	\
			1,			\
			&api_funcs);

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_VIRTIO)
