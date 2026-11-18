/*
 * Copyright (C) 2025 - 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <metal/device.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <metal/time.h>
#include "common.h"
#include "platform_init.h"

static struct metal_device *rpu_to_apu_desc_dev, *apu_to_rpu_desc_dev;
static struct metal_device *shm_dev, *notify_dev, *ttc_dev;

#define APP_UIO_BUS_NAME "uio"
#define APP_UIO_CLASS_PATH "/sys/class/uio"

#define APP_SHM_DEV_NAME "libmetal-data"
#define APP_SHM0_DESC_DEV_NAME "libmetal-desc0"
#define APP_SHM1_DESC_DEV_NAME "libmetal-desc1"
#define APP_TTC_DEV_NAME "libmetal-timer"

/* Notification backends, in the order platform_init() looks for them. */
static const struct notify_ops *const notify_backends[] = {
	&notify_ipi_ops,
};

static int app_read_first_line(const char *path, char *output, size_t output_len)
{
	FILE *fp;
	char *newline;

	if (!path || !output || output_len < 2)
		return -EINVAL;

	fp = fopen(path, "r");
	if (!fp)
		return -errno;

	if (!fgets(output, output_len, fp)) {
		int err = ferror(fp) ? -errno : -ENODATA;

		fclose(fp);
		return err;
	}

	fclose(fp);

	newline = strchr(output, '\n');
	if (newline)
		*newline = '\0';

	return 0;
}

int app_uio_find_path(const char *uio_name, char *uio_path, size_t uio_path_len)
{
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];
	char value[PATH_MAX];
	bool found = false;
	int ret = -ENODEV;

	if (!uio_name || !strlen(uio_name) || !uio_path || !uio_path_len)
		return -EINVAL;

	dir = opendir(APP_UIO_CLASS_PATH);
	if (!dir)
		return -errno;

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "uio", 3) != 0)
			continue;

		ret = snprintf(path, sizeof(path), "%s/%s/name", APP_UIO_CLASS_PATH, entry->d_name);
		if (ret < 0 || ret >= (int)sizeof(path)) {
			ret = -EOVERFLOW;
			goto out;
		}

		ret = app_read_first_line(path, value, sizeof(value));
		if (ret)
			continue;

		if (strcmp(value, uio_name) != 0)
			continue;

		if (found) {
			ret = -EEXIST;
			goto out;
		}
		found = true;

		ret = snprintf(uio_path, uio_path_len, "%s/%s", APP_UIO_CLASS_PATH, entry->d_name);
		if (ret < 0 || ret >= (int)uio_path_len) {
			ret = -EOVERFLOW;
			goto out;
		}
	}

	ret = found ? 0 : -ENODEV;

out:
	closedir(dir);
	return ret;
}

int app_uio_read_dt_u32(const char *uio_name, const char *property, uint32_t *value)
{
	unsigned char raw[4];
	char uio_path[PATH_MAX];
	char path[PATH_MAX];
	FILE *fp;
	size_t len;
	int ret;

	if (!uio_name || !property || !value)
		return -EINVAL;

	ret = app_uio_find_path(uio_name, uio_path, sizeof(uio_path));
	if (ret)
		return ret;

	ret = snprintf(path, sizeof(path), "%s/device/of_node/%s", uio_path, property);
	if (ret < 0 || ret >= (int)sizeof(path))
		return -EOVERFLOW;

	fp = fopen(path, "rb");
	if (!fp)
		return -errno;

	len = fread(raw, 1, sizeof(raw), fp);
	fclose(fp);
	if (len != sizeof(raw))
		return -ENODATA;

	*value = ((uint32_t)raw[0] << 24) | ((uint32_t)raw[1] << 16) | ((uint32_t)raw[2] << 8) |
		 (uint32_t)raw[3];

	return 0;
}

/**
 * @brief find_notify_backend() - select the notification backend
 *        The first backend whose UIO device exists is used.
 *
 * @return the backend, or NULL if no notification device exists.
 */
static const struct notify_ops *find_notify_backend(void)
{
	char uio_path[PATH_MAX];
	size_t i;
	int ret;

	for (i = 0; i < sizeof(notify_backends) / sizeof(notify_backends[0]); i++) {
		ret = app_uio_find_path(notify_backends[i]->dev_name, uio_path,
					sizeof(uio_path));
		if (!ret)
			return notify_backends[i];
		if (ret != -ENODEV) {
			metal_err("HOST: Failed to look up UIO device %s: %d.\n",
				  notify_backends[i]->dev_name, ret);
			return NULL;
		}
	}

	metal_err("HOST: No notification UIO device found.\n");
	return NULL;
}

/**
 * @brief close_metal_devices() - close libmetal devices
 *        This function closes all the libmetal devices which have
 *        been opened.
 *
 */
static void close_metal_devices(void)
{
	/* Close shared memory device */
	if (shm_dev) {
		metal_device_close(shm_dev);
		shm_dev = NULL;
	}

	/* Close notification device */
	if (notify_dev) {
		metal_device_close(notify_dev);
		notify_dev = NULL;
	}

	/* Close TTC device */
	if (ttc_dev) {
		metal_device_close(ttc_dev);
		ttc_dev = NULL;
	}

	/* Close descriptor devices */
	if (rpu_to_apu_desc_dev) {
		metal_device_close(rpu_to_apu_desc_dev);
		rpu_to_apu_desc_dev = NULL;
	}

	if (apu_to_rpu_desc_dev) {
		metal_device_close(apu_to_rpu_desc_dev);
		apu_to_rpu_desc_dev = NULL;
	}
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
	ret = metal_device_open(APP_UIO_BUS_NAME, APP_SHM_DEV_NAME, &shm_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", APP_SHM_DEV_NAME);
		goto out;
	}

	/* Open descriptor devices */
	ret = metal_device_open(APP_UIO_BUS_NAME, APP_SHM0_DESC_DEV_NAME, &apu_to_rpu_desc_dev);
	if (ret) {
		metal_err("Failed to open device %s.\n", APP_SHM0_DESC_DEV_NAME);
		goto out;
	}

	ret = metal_device_open(APP_UIO_BUS_NAME, APP_SHM1_DESC_DEV_NAME, &rpu_to_apu_desc_dev);
	if (ret) {
		metal_err("Failed to open device %s.\n", APP_SHM1_DESC_DEV_NAME);
		goto out;
	}

	/* Open TTC device */
	ret = metal_device_open(APP_UIO_BUS_NAME, APP_TTC_DEV_NAME, &ttc_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", APP_TTC_DEV_NAME);
		goto out;
	}

out:
	return ret;
}

/**
 * @brief open_notify_device() - open and initialize the notification device
 *
 * @return 0 - succeeded, non-zero for failures.
 */
static int open_notify_device(struct channel_s *ch)
{
	struct channel_machine_ctx_s *machine = channel_machine_ctx(ch);
	const struct notify_ops *notify;
	int ret;

	notify = find_notify_backend();
	if (!notify)
		return -ENODEV;

	ret = metal_device_open(APP_UIO_BUS_NAME, notify->dev_name, &notify_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", notify->dev_name);
		return ret;
	}

	ch->ipi_io = metal_device_io_region(notify_dev, 0);
	if (!ch->ipi_io) {
		metal_err("HOST: Failed to map io region for %s.\n", notify_dev->name);
		return -ENODEV;
	}

	/* Get the IRQ from the opened notification device */
	ch->irq_vector_id = (intptr_t)notify_dev->irq_info;

	ret = notify->init(ch);
	if (ret) {
		metal_err("HOST: Failed to initialize notification device %s.\n",
			  notify->dev_name);
		return ret;
	}
	machine->notify = notify;

	return 0;
}

int platform_init(struct channel_s *ch)
{
	struct metal_init_params init_param = METAL_INIT_DEFAULTS;
	struct channel_machine_ctx_s *machine = channel_machine_ctx(ch);
	int ret;

	ret = metal_init(&init_param);
	if (ret) {
		metal_err("HOST: Failed to initialize libmetal\n");
		return ret;
	}

	/* initialize remote_nkicked */
	machine->remote_nkicked = (atomic_flag)ATOMIC_FLAG_INIT;
	atomic_flag_test_and_set(&machine->remote_nkicked);

	ret = open_metal_devices();
	if (ret) {
		metal_err("HOST: Failed to open devices\n");
		goto out_close;
	}

	/* Get shared memory device IO region */
	ch->shm_io = metal_device_io_region(shm_dev, 0);
	if (!ch->shm_io) {
		metal_err("HOST: Failed to map io region for %s.\n", shm_dev->name);
		ret = -ENODEV;
		goto out_close;
	}

	/* Get descriptor IO Regions */
	ch->host_to_remote_desc_io = metal_device_io_region(apu_to_rpu_desc_dev, 0);
	if (!ch->host_to_remote_desc_io) {
		metal_err("Failed to map io region for %s.\n",
			  apu_to_rpu_desc_dev->name);
		ret = -ENODEV;
		goto out_close;
	}
	ch->remote_to_host_desc_io = metal_device_io_region(rpu_to_apu_desc_dev, 0);
	if (!ch->remote_to_host_desc_io) {
		metal_err("Failed to map io region for %s.\n",
			  rpu_to_apu_desc_dev->name);
		ret = -ENODEV;
		goto out_close;
	}

	/* Check before narrowing the runtime mapping sizes to channel fields. */
	if (metal_io_region_size(ch->host_to_remote_desc_io) > UINT32_MAX ||
	    metal_io_region_size(ch->remote_to_host_desc_io) > UINT32_MAX ||
	    metal_io_region_size(ch->shm_io) > UINT32_MAX) {
		metal_err("HOST: Shared memory region size exceeds 32-bit capacity.\n");
		ret = -EINVAL;
		goto out_close;
	}

	ch->desc0_size = (uint32_t)metal_io_region_size(ch->host_to_remote_desc_io);
	ch->desc1_size = (uint32_t)metal_io_region_size(ch->remote_to_host_desc_io);
	ch->shm_payload_size = (uint32_t)metal_io_region_size(ch->shm_io);
	if (!ch->desc0_size || !ch->desc1_size || !ch->shm_payload_size) {
		metal_err("HOST: Invalid descriptor or payload region size.\n");
		ret = -EINVAL;
		goto out_close;
	}

	/* Get TTC IO region */
	ch->ttc_io = metal_device_io_region(ttc_dev, 0);
	if (!ch->ttc_io) {
		metal_err("HOST: Failed to map io region for %s.\n", ttc_dev->name);
		ret = -ENODEV;
		goto out_close;
	}

	/* Open the notification device and enable its interrupt */
	ret = open_notify_device(ch);
	if (ret)
		goto out_close;

	return 0;

out_close:
	close_metal_devices();
	metal_finish();
	return ret;
}

void platform_cleanup(struct channel_s *ch)
{
	struct channel_machine_ctx_s *machine = channel_machine_ctx(ch);

	/* Disable and unregister the notification interrupt */
	if (machine->notify)
		machine->notify->cleanup(ch);
	machine->notify = NULL;
	memset(ch, 0, sizeof(*ch));

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
