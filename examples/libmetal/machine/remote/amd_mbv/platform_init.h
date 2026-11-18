/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_INIT_H__
#define __PLATFORM_INIT_H__

struct channel_s;

/**
 * @brief platform_init() - register the libmetal devices and connect the
 *        AXI Mailbox receive interrupt.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
int platform_init(struct channel_s *ch);

/**
 * @brief platform_cleanup() - disconnect the mailbox interrupt and finish
 *        libmetal.
 */
void platform_cleanup(struct channel_s *ch);

/**
 * @brief mbv_mailbox_kick() - notify the host through the AXI Mailbox.
 */
void mbv_mailbox_kick(struct channel_s *ch);

#endif /* __PLATFORM_INIT_H__ */
