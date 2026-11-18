/*
 * Copyright (C) 2025 - 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_INIT_H__
#define __PLATFORM_INIT_H__

#include <stddef.h>
#include <stdint.h>

struct channel_s;

int platform_init(struct channel_s *ch);
void platform_cleanup(struct channel_s *ch);

/**
 * @brief app_uio_find_path() - find the sysfs path of a UIO device
 *
 * @param[in] uio_name - linux,uio-name of the device
 * @param[out] uio_path - /sys/class/uio/uioX of the device
 * @param[in] uio_path_len - size of uio_path
 *
 * @return 0 when exactly one device has the name, -ENODEV when none has it,
 *         -EEXIST when several have it, other negative values on failure.
 */
int app_uio_find_path(const char *uio_name, char *uio_path, size_t uio_path_len);

/**
 * @brief app_uio_read_dt_u32() - read a 32-bit property of a UIO device's
 *        device-tree node
 *
 * @return 0 for success, negative value for failure.
 */
int app_uio_read_dt_u32(const char *uio_name, const char *property, uint32_t *value);

#endif /* __PLATFORM_INIT_H__ */
