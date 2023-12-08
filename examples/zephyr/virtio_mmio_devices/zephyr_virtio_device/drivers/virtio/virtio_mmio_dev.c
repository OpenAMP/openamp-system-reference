/*
 * Copyright (c) 2023, STMICROLECTRONICS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/device.h>

#include <drivers/virtio/virtio_mmio_dev.h>

#define DT_DRV_COMPAT virtio_mmio_dev

#define DEV_DATA(dev)	\
	((struct virtio_mmio_data * const)(dev)->data)

void virtio_mmio_interrupt(const struct device *dev)
{
	struct virtio_mmio_data *data = DEV_DATA(dev);

	/** pass the interrupt to the virtio mmio device driver */
	virtio_mmio_dev_interrupt(&data->dev);
}

void virtio_mmio_configure_interrupt(const struct device *dev,
				     interrupt_callback callback)
{
	struct virtio_mmio_data *data = DEV_DATA(dev);

	data->interrupt_callback = callback;
}


static void virtio_mmio_notify_other_side(struct virtio_device *vdev)
{
	struct virtio_mmio_data *data = vdev->priv;

	/** send an interrupt to the guest driver */
	if (data->interrupt_callback)
		data->interrupt_callback();
}

static int virtio_mmio_init(const struct device *dev)
{
	struct virtio_mmio_data *data = DEV_DATA(dev);

	/** configure metal io region */
	data->metal_dev.regions[0].physmap = ((metal_phys_addr_t *)(&data->table));
	data->metal_dev.regions[0].virt = data->table;
	data->metal_dev.regions[0].size = 0x8000;
	data->metal_dev.regions[0].page_mask = -1;
	data->metal_dev.regions[0].page_shift = -1;

	data->metal_dev.num_regions = 1;
	data->metal_dev.name = "MMIO_DEV";

	struct metal_device *device;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	int err = metal_init(&metal_params);

	if (err) {
		metal_log(METAL_LOG_ERROR, "metal_init: failed - error code %d\n", err);
		return err;
	}

	err = metal_register_generic_device(&data->metal_dev);
	if (err) {
		metal_log(METAL_LOG_ERROR, "Couldn't register shared memory device: %d\n", err);
		return err;
	}

	err = metal_device_open("generic", data->metal_dev.name, &device);
	if (err) {
		metal_log(METAL_LOG_ERROR, "metal_device_open failed: %d", err);
		return err;
	}

	struct metal_io_region *io = metal_device_io_region(device, 0);

	if (!io) {
		metal_log(METAL_LOG_ERROR, "metal_device_io_region failed to get region");
		return err;
	}

	/** this driver data is use to identify this instance inside the notify callback */
	data->dev.vdev.priv = data;

	virtio_mmio_dev_init(&data->dev, io, virtio_mmio_notify_other_side);
}

#define VIRTIO_MMIO_DEVICE_INIT(id)					\
	static struct virtio_mmio_data virtio_mmio_data_##id = {	\
		.table = DT_REG_ADDR(DT_INST(id, DT_DRV_COMPAT)),	\
	};	\
		\
		\
	DEVICE_DT_DEFINE(DT_INST(id, DT_DRV_COMPAT),	\
			virtio_mmio_init, NULL,		\
			&virtio_mmio_data_##id,		\
			NULL,				\
			POST_KERNEL,			\
			0,				\
			&(virtio_mmio_data_##id).dev.vdev);
/** we use priority of 0 so that this driver is init before higher level virtio drivers,
 * that may use this one at init
 */

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_MMIO_DEVICE_INIT)
