/*
 * Copyright (C) 2017-2022 Xilinx, Inc.  All rights reserved.
 * Copyright (c) 2022-2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_INIT_H__
#define __PLATFORM_INIT_H__

struct channel_s;

/**
 * @brief platform_init() - Register libmetal devices.
 *        This function register the libmetal generic bus, and then
 *        register the IPI, shared memory descriptor and shared memory
 *        devices to the libmetal generic bus.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
int platform_init(struct channel_s *ch);

/**
 * @brief platform_cleanup() - system cleanup
 *        This function finish the libmetal environment
 *        and disable caches.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
void platform_cleanup(struct channel_s *ch);

/**
 * @brief init_irq() - Initialize GIC and connect IPI interrupt
 *        This function will initialize the GIC and connect the IPI
 *        interrupt.
 *
 * @return 0 - succeeded, non-0 for failures
 */
int init_irq(void);

#endif /* __PLATFORM_INIT_H__ */
