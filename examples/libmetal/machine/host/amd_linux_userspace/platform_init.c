/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <metal/device.h>
#include <metal/sys.h>
#include <metal/time.h>
#include "common.h"

static struct metal_device *rpu_to_apu_desc_dev, *apu_to_rpu_desc_dev;
static struct metal_device *shm_dev, *ipi_dev, *ttc_dev;

#define DEV_NAME_MAX 64
#define CFG_PATH_DEFAULT "./platform.conf"
#define BUS_NAME "platform"

struct platform_cfg {
	char shm_dev_name[DEV_NAME_MAX];
	char shm0_desc_dev_name[DEV_NAME_MAX];
	char shm1_desc_dev_name[DEV_NAME_MAX];
	char ipi_dev_name[DEV_NAME_MAX];
	char ttc_dev_name[DEV_NAME_MAX];
	uint32_t ipi_mask;
	uint32_t desc0_size;
	uint32_t desc1_size;
	uint32_t shm_payload_size;
};

static int parse_config_file(const char *path, struct platform_cfg *cfg)
{
	FILE *fp;
	char line[256];
	int line_no = 0;
	int ret = 0;
	bool have_shm = false;
	bool have_desc0 = false;
	bool have_desc1 = false;
	bool have_ipi = false;
	bool have_ttc = false;
	bool have_mask = false;
	bool have_desc0_size = false;
	bool have_desc1_size = false;
	bool have_payload_size = false;

	memset(cfg, 0, sizeof(*cfg));
	fp = fopen(path, "r");
	if (!fp) {
		metal_err("Cannot open config file '%s'.\n", path);
		return -ENOENT;
	}

	while (fgets(line, sizeof(line), fp)) {
		char *key;
		char *val;
		char *eq;

		line_no++;
		key = line;
		while (*key == ' ' || *key == '\t')
			key++;
		if (*key == '\0' || *key == '\n' || *key == '#')
			continue;

		eq = strchr(key, '=');
		if (!eq) {
			metal_err("Malformed config line %d: '%s'\n", line_no, key);
			ret = -EINVAL;
			break;
		}
		*eq = '\0';
		val = eq + 1;
		while (*val == ' ' || *val == '\t')
			val++;

		if (!strcmp(key, "SHM_DEV_NAME")) {
			if (strlen(val) >= DEV_NAME_MAX) {
				metal_err("Value for 'SHM_DEV_NAME' exceeds %d characters\n",
					  DEV_NAME_MAX - 1);
				ret = -EINVAL;
				break;
			}
			if (sscanf(val, "%63s", cfg->shm_dev_name) != 1) {
				ret = -EINVAL;
				break;
			}
			have_shm = true;
		} else if (!strcmp(key, "SHM0_DESC_DEV_NAME")) {
			if (strlen(val) >= DEV_NAME_MAX) {
				metal_err("Value for 'SHM0_DESC_DEV_NAME' exceeds %d characters\n",
					  DEV_NAME_MAX - 1);
				ret = -EINVAL;
				break;
			}
			if (sscanf(val, "%63s", cfg->shm0_desc_dev_name) != 1) {
				ret = -EINVAL;
				break;
			}
			have_desc0 = true;
		} else if (!strcmp(key, "SHM1_DESC_DEV_NAME")) {
			if (strlen(val) >= DEV_NAME_MAX) {
				metal_err("Value for 'SHM1_DESC_DEV_NAME' exceeds %d characters\n",
					  DEV_NAME_MAX - 1);
				ret = -EINVAL;
				break;
			}
			if (sscanf(val, "%63s", cfg->shm1_desc_dev_name) != 1) {
				ret = -EINVAL;
				break;
			}
			have_desc1 = true;
		} else if (!strcmp(key, "IPI_DEV_NAME")) {
			if (strlen(val) >= DEV_NAME_MAX) {
				metal_err("Value for 'IPI_DEV_NAME' exceeds %d characters\n",
					  DEV_NAME_MAX - 1);
				ret = -EINVAL;
				break;
			}
			if (sscanf(val, "%63s", cfg->ipi_dev_name) != 1) {
				ret = -EINVAL;
				break;
			}
			have_ipi = true;
		} else if (!strcmp(key, "TTC_DEV_NAME")) {
			if (strlen(val) >= DEV_NAME_MAX) {
				metal_err("Value for 'TTC_DEV_NAME' exceeds %d characters\n",
					  DEV_NAME_MAX - 1);
				ret = -EINVAL;
				break;
			}
			if (sscanf(val, "%63s", cfg->ttc_dev_name) != 1) {
				ret = -EINVAL;
				break;
			}
			have_ttc = true;
		} else if (!strcmp(key, "IPI_MASK")) {
			cfg->ipi_mask = (uint32_t)strtoul(val, NULL, 0);
			have_mask = true;
		} else if (!strcmp(key, "DESC0_SIZE")) {
			cfg->desc0_size = (uint32_t)strtoul(val, NULL, 0);
			have_desc0_size = true;
		} else if (!strcmp(key, "DESC1_SIZE")) {
			cfg->desc1_size = (uint32_t)strtoul(val, NULL, 0);
			have_desc1_size = true;
		} else if (!strcmp(key, "SHM_PAYLOAD_SIZE")) {
			cfg->shm_payload_size = (uint32_t)strtoul(val, NULL, 0);
			have_payload_size = true;
		}
	}

	if (!ret) {
		if (!have_shm || !have_desc0 || !have_desc1 || !have_ipi ||
		    !have_ttc || !have_mask || !have_desc0_size ||
		    !have_desc1_size || !have_payload_size) {
			if (!have_shm)
				metal_err("Config key 'SHM_DEV_NAME' not found in '%s'\n", path);
			if (!have_desc0)
				metal_err("Config key 'SHM0_DESC_DEV_NAME' not found in '%s'\n", path);
			if (!have_desc1)
				metal_err("Config key 'SHM1_DESC_DEV_NAME' not found in '%s'\n", path);
			if (!have_ipi)
				metal_err("Config key 'IPI_DEV_NAME' not found in '%s'\n", path);
			if (!have_ttc)
				metal_err("Config key 'TTC_DEV_NAME' not found in '%s'\n", path);
			if (!have_mask)
				metal_err("Config key 'IPI_MASK' not found in '%s'\n", path);
			if (!have_desc0_size)
				metal_err("Config key 'DESC0_SIZE' not found in '%s'\n", path);
			if (!have_desc1_size)
				metal_err("Config key 'DESC1_SIZE' not found in '%s'\n", path);
			if (!have_payload_size)
				metal_err("Config key 'SHM_PAYLOAD_SIZE' not found in '%s'\n", path);
			ret = -EINVAL;
		}
	}

	fclose(fp);
	return ret;
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
	if (shm_dev)
		metal_device_close(shm_dev);

	/* Close IPI device */
	if (ipi_dev)
		metal_device_close(ipi_dev);

	/* Close TTC device */
	if (ttc_dev)
		metal_device_close(ttc_dev);

	/* Close descriptor devices */
	if (rpu_to_apu_desc_dev)
		metal_device_close(rpu_to_apu_desc_dev);

	if (apu_to_rpu_desc_dev)
		metal_device_close(apu_to_rpu_desc_dev);
}

/**
 * @brief open_metal_devices() - Open registered libmetal devices.
 *        This function opens all the registered libmetal devices.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
static int open_metal_devices(struct platform_cfg *cfg)
{
	int ret;

	/* Open shared memory device */
	ret = metal_device_open(BUS_NAME, cfg->shm_dev_name, &shm_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n",
			  cfg->shm_dev_name);
		goto out;
	}

	/* Open descriptor devices */
	ret = metal_device_open(BUS_NAME, cfg->shm0_desc_dev_name,
				&apu_to_rpu_desc_dev);
	if (ret) {
		metal_err("Failed to open device %s.\n", cfg->shm0_desc_dev_name);
		goto out;
	}

	ret = metal_device_open(BUS_NAME, cfg->shm1_desc_dev_name,
				&rpu_to_apu_desc_dev);
	if (ret) {
		metal_err("Failed to open device %s.\n", cfg->shm1_desc_dev_name);
		goto out;
	}

	/* Open IPI device */
	ret = metal_device_open(BUS_NAME, cfg->ipi_dev_name, &ipi_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", cfg->ipi_dev_name);
		goto out;
	}

	/* Open TTC device */
	ret = metal_device_open(BUS_NAME, cfg->ttc_dev_name, &ttc_dev);
	if (ret) {
		metal_err("HOST: Failed to open device %s.\n", cfg->ttc_dev_name);
		goto out;
	}

out:
	return ret;
}

static int irq_isr(int vect_id, void *priv)
{
	struct channel_s *ch = (struct channel_s *)priv;
	struct metal_io_region *ipi_io = ch->ipi_io;
	uint32_t ipi_mask = ch->ipi_mask;
	uint64_t val = 1;

	(void)vect_id;

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

int platform_init(struct channel_s *ch, const char *config_path)
{
	struct metal_init_params init_param = METAL_INIT_DEFAULTS;
	struct platform_cfg cfg;
	bool using_default_path = false;
	int ret;

	metal_assert(ch);

	if (!config_path) {
		config_path = CFG_PATH_DEFAULT;
		using_default_path = true;
	}

	ret = metal_init(&init_param);
	if (ret) {
		metal_err("HOST: Failed to initialize libmetal\n");
		return ret;
	}

	/* initialize remote_nkicked */
	ch->remote_nkicked = (atomic_flag)ATOMIC_FLAG_INIT;
	atomic_flag_test_and_set(&ch->remote_nkicked);

	ret = parse_config_file(config_path, &cfg);
	if (ret) {
		if (ret == -ENOENT && using_default_path) {
			metal_err("HOST: No config file found at '%s'. "
				  "Run discover_platform.sh to generate one.\n",
				  config_path);
		}
		return ret;
	}

	ret = open_metal_devices(&cfg);
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

	/* Get descriptor IO Regions */
	ch->host_to_remote_desc_io = metal_device_io_region(apu_to_rpu_desc_dev, 0);
	if (!ch->host_to_remote_desc_io) {
		metal_err("Failed to map io region for %s.\n",
			  apu_to_rpu_desc_dev->name);
		return -ENODEV;
	}
	ch->remote_to_host_desc_io = metal_device_io_region(rpu_to_apu_desc_dev, 0);
	if (!ch->remote_to_host_desc_io) {
		metal_err("Failed to map io region for %s.\n",
			  rpu_to_apu_desc_dev->name);
		return -ENODEV;
	}

	/* Get IPI device IO region */
	ch->ipi_io = metal_device_io_region(ipi_dev, 0);
	if (!ch->ipi_io) {
		metal_err("HOST: Failed to map io region for %s.\n", ipi_dev->name);
		return -ENODEV;
	}

	/* Get the IPI IRQ from the opened IPI device */
	ch->ipi_mask = cfg.ipi_mask;
	ch->desc0_size = cfg.desc0_size;
	ch->desc1_size = cfg.desc1_size;
	ch->shm_payload_size = cfg.shm_payload_size;

	/* Get TTC IO region */
	ch->ttc_io = metal_device_io_region(ttc_dev, 0);
	if (!ch->ttc_io) {
		metal_err("HOST: Failed to map io region for %s.\n", ttc_dev->name);
		return -ENODEV;
	}

	/* Get the IPI IRQ from the opened IPI device */
	ch->irq_vector_id = (intptr_t)ipi_dev->irq_info;

	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, ch->ipi_mask);
	/* clear old IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_ISR_OFFSET, ch->ipi_mask);
	/* Register IPI irq handler */
	metal_irq_register(ch->irq_vector_id, irq_isr, ch);
	metal_irq_enable(ch->irq_vector_id);
	/* Enable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IER_OFFSET, ch->ipi_mask);

	return 0;
}

void platform_cleanup(struct channel_s *ch)
{
	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, ch->ipi_mask);
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
