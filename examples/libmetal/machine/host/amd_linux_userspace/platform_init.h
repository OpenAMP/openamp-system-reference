/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_INIT_H__
#define __PLATFORM_INIT_H__

struct channel_s;

int platform_init(struct channel_s *ch);
void platform_cleanup(struct channel_s *ch);

#endif /* __PLATFORM_INIT_H__ */
