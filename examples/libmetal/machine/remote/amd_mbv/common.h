/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <string.h>

#include <metal/alloc.h>
#include <metal/device.h>
#include <metal/errno.h>
#include <metal/io.h>
#include <metal/irq.h>
#include <metal/sys.h>

#include "amd_mbv_hw.h"
#include "amp_demo_os.h"
#include "config.h"
#include "platform_init.h"

struct msg_hdr_s {
	uint32_t index;
	uint32_t len;
};

static inline void irq_kick(struct channel_s *ch)
{
	mbv_mailbox_kick(ch);
}

int demo(void *arg);

#endif /* __COMMON_H__ */
