/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * IPC configuration of the MicroBlaze V OpenAMP remote.
 *
 * All values come from the Lopper-generated amd_platform_info.h; see
 * README.md for how to generate it. It must define:
 *
 *   SHARED_MEM_PA      start of the shared region: vrings and RPMsg buffers
 *   SHARED_BUF_OFFSET  offset of the RPMsg buffers within the region
 *   SHARED_MEM_SIZE    size of the RPMsg buffers, as for the RPU machines;
 *                      the region ends at SHARED_BUF_OFFSET + SHARED_MEM_SIZE
 *   RING_TX, RING_RX   vring device addresses in the resource table, or
 *                      FW_RSC_U32_ADDR_ANY to let Linux assign them
 *   MBOX_BASE_ADDR     base of the MicroBlaze V interface of the AXI Mailbox
 */

#ifndef MBV32_PLATFORM_CONFIG_H
#define MBV32_PLATFORM_CONFIG_H

#include "xparameters.h"
#include "amd_platform_info.h"

#if !defined(SHARED_MEM_PA) || !defined(SHARED_MEM_SIZE) || \
	!defined(SHARED_BUF_OFFSET) || !defined(RING_TX) || \
	!defined(RING_RX) || !defined(MBOX_BASE_ADDR)
#error "amd_platform_info.h must define SHARED_MEM_PA, SHARED_MEM_SIZE, " \
	"SHARED_BUF_OFFSET, RING_TX, RING_RX and MBOX_BASE_ADDR"
#endif

/* The vrings and the RPMsg buffers. */
#define SHARED_REGION_SIZE (SHARED_BUF_OFFSET + SHARED_MEM_SIZE)

#endif /* MBV32_PLATFORM_CONFIG_H */
