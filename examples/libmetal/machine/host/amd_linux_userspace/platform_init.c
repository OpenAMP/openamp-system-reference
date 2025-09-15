/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <unistd.h>
#include <metal/device.h>
#include <metal/sys.h>
#include <metal/time.h>
#include "common.h"

static struct metal_device *shm_dev, *ipi_dev, *ttc_dev;
/**
 * @brief close_metal_devices() - close libmetal devices
 *        This function closes all the libmetal devices which have
 *        been opened.
 *
 */
static void close_metal_devices(void)
{
	/* Close shared memory device */
	if (shm_dev)
		metal_device_close(shm_dev);

	/* Close IPI device */
	if (ipi_dev)
		metal_device_close(ipi_dev);

	/* Close TTC device */
	if (ttc_dev)
		metal_device_close(ttc_dev);
}

/**
 * @brief open_metal_devices() - Open registered libmetal devices.
 *        This function opens all the registered libmetal devices.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
int open_metal_devices(void)
{
	int ret;

	/* Open shared memory device */
	ret = metal_device_open(BUS_NAME, SHM_DEV_NAME, &shm_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", SHM_DEV_NAME);
		goto out;
	}

	/* Open IPI device */
	ret = metal_device_open(BUS_NAME, IPI_DEV_NAME, &ipi_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", IPI_DEV_NAME);
		goto out;
	}

	/* Open TTC device */
	ret = metal_device_open(BUS_NAME, TTC_DEV_NAME, &ttc_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", TTC_DEV_NAME);
		goto out;
	}

out:
	return ret;
}

static int irq_isr(int vect_id, void *priv)
{
	(void)vect_id;
	struct channel_s *ch = (struct channel_s *)priv;
	struct metal_io_region *ipi_io = ch->ipi_io;
	uint32_t ipi_mask = IPI_MASK;
	uint64_t val = 1;

	if (!ipi_io)
		return METAL_IRQ_NOT_HANDLED;
	val = metal_io_read32(ipi_io, IPI_ISR_OFFSET);
	if (val & ipi_mask) {
		metal_io_write32(ipi_io, IPI_ISR_OFFSET, ipi_mask);
		atomic_flag_clear(&ch->remote_nkicked);
		return METAL_IRQ_HANDLED;
	}
	return METAL_IRQ_NOT_HANDLED;
}

int platform_init(struct channel_s *ch)
{
	struct metal_init_params init_param = METAL_INIT_DEFAULTS;
	int ret;

	metal_assert(ch);

	ret = metal_init(&init_param);
	if (ret) {
		metal_err("HOST: Failed to initialize libmetal\n");
		return ret;
	}

	/* initialize remote_nkicked */
	ch->remote_nkicked = (atomic_flag)ATOMIC_FLAG_INIT;
	atomic_flag_test_and_set(&ch->remote_nkicked);

	ret = open_metal_devices();
	if (ret) {
		metal_err("HOST: Failed to open devices\n");
		return ret;
	}

	/* Get shared memory device IO region */
	ch->shm_io = metal_device_io_region(shm_dev, 0);
	if (!ch->shm_io) {
		metal_err("HOST: Failed to map io region for %s.\n", shm_dev->name);
		return -ENODEV;
	}

	/* Get IPI device IO region */
	ch->ipi_io = metal_device_io_region(ipi_dev, 0);
	if (!ch->ipi_io) {
		metal_err("HOST: Failed to map io region for %s.\n", ipi_dev->name);
		return -ENODEV;
	}

	/* Get the IPI IRQ from the opened IPI device */
	ch->ipi_mask = (intptr_t)ipi_dev->irq_info;

	/* Get TTC IO region */
	ch->ttc_io = metal_device_io_region(ttc_dev, 0);
	if (!ch->ttc_io) {
		metal_err("HOST: Failed to map io region for %s.\n", ttc_dev->name);
		return -ENODEV;
	}

	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, IPI_MASK);
	/* clear old IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_ISR_OFFSET, IPI_MASK);
	/* Register IPI irq handler */
	metal_irq_register(ch->irq_vector_id, irq_isr, ch);
	metal_irq_enable(ch->irq_vector_id);
	/* Enable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IER_OFFSET, IPI_MASK);

	return 0;
}

void platform_cleanup(struct channel_s *ch)
{
	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, IPI_MASK);
	/* unregister IPI irq handler by setting the handler to 0 */
	metal_irq_disable(ch->irq_vector_id);
	metal_irq_unregister(ch->irq_vector_id);

	metal_irq_unregister(ch->irq_vector_id);
	memset(&ch, 0, sizeof(ch));

	/* Close libmetal devices which have been opened */
	close_metal_devices();
	/* Finish libmetal environment */
	metal_finish();
}

unsigned long long platform_gettime(void)
{
	return metal_get_timestamp();
}

void wait_for_interrupt(void)
{
}
