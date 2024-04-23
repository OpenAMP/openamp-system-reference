/*
 * Copyright (c) 2023, STMICROLECTRONICS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _VIRTIO_MMIO_H_
#define _VIRTIO_MMIO_H_

#include <openamp/open_amp.h>
#include <openamp/virtio_mmio.h>

typedef int (*interrupt_callback)();

struct virtio_mmio_data {
	const void		*table;
	struct metal_device	metal_dev;
	struct virtio_mmio_dev	dev;

	interrupt_callback	interrupt_callback;
};

/**
 * @brief virtio mmio interrupt
 *
 * This should be called when an interrupt is received for the virtio mmio device
 *
 * @param dev The device to interrupt
 */
void virtio_mmio_interrupt(const struct device *dev);


/**
 * @brief virtio mmio configure interrupt
 *
 * Configure the callback function to interrupt the driver
 *
 * @param dev The device to interrupt
 * @param callback The callback function to call when the device want to interrupt the driver
 */
void virtio_mmio_configure_interrupt(const struct device *dev, interrupt_callback callback);

#endif
