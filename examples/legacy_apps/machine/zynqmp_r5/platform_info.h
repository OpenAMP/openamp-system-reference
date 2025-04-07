/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2017 - 2021 Xilinx, Inc.
 * Copyright (c) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_H_
#define PLATFORM_INFO_H_

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>

#include "platform_info_common.h"

#include "xparameters.h"

#ifdef SDT                                                                     
#include "bspconfig.h" /* This provides SOC symbols. */                        
#endif

#if defined __cplusplus
extern "C" {
#endif

/* Cortex R5 memory attributes */
#define DEVICE_SHARED       0x00000001U /* device, shareable */
#define DEVICE_NONSHARED    0x00000010U /* device, non shareable */
#define NORM_NSHARED_NCACHE 0x00000008U /* Non cacheable  non shareable */
#define NORM_SHARED_NCACHE  0x0000000CU /* Non cacheable shareable */
#define PRIV_RW_USER_RW     (0x00000003U<<8U) /* Full Access */

#ifdef _AMD_GENERATED_
/*
 * @file   amd_platform_info.h
 * @brief  Generated header that contains OpenAMP IPC information.
 *
 *        Namely interrupt and shared memory information. If values are
 *        provided via generated header, then include thus. These values
 *        are to describe interrupt and shared memory information that
 *        describes one end of an OpenAMP IPC connection. This application
 *        is for target AMD RPUs. The file 'amd_platform_info.h'
 *        is generated via Vitis NG or Yocto-Decoupling flow for
 *        AMD RPU targets. The channel information is defined in the
 *        OpenAMP YAML channel description. The generated symbols can be
 *        changed by editing the OpenAMP YAML channel description.
 */
#include "amd_platform_info.h"
#else
#ifdef VERSAL_NET                                                              
#include "platform_info_versal_net.h"                                          
#elif defined(versal)                                                          
#include "platform_info_versal.h"                                              
#elif defined(PLATFORM_ZYNQMP)                                                 
#include "platform_info_zynqmp.h"
#error "unknown platform" 
#endif

#ifdef RPMSG_NO_IPI
#undef POLL_BASE_ADDR
#define POLL_BASE_ADDR 0x3EE40000
#define POLL_STOP 0x1U
#endif /* RPMSG_NO_IPI */

#ifndef RING_TX
#define RING_TX                     FW_RSC_U32_ADDR_ANY
#endif /* !RING_TX */

#ifndef RING_RX
#define RING_RX                     FW_RSC_U32_ADDR_ANY
#endif /* RING_RX */

#define NUM_VRINGS                  0x02
#define VRING_ALIGN                 0x1000
#define VRING_SIZE                  256

#ifndef SHARED_MEM_PA
#if XPAR_CPU_ID == 0
#define SHARED_MEM_PA  0x3ED40000UL
#else

#define SHARED_MEM_PA  0x3EF40000UL
#endif /* XPAR_CPU_ID */
#endif /* !SHARED_MEM_PA */

#ifndef SHARED_MEM_SIZE
#define SHARED_MEM_SIZE 0x100000UL
#endif /* !SHARED_MEM_SIZE */

#ifndef SHARED_BUF_OFFSET
#define SHARED_BUF_OFFSET 0x8000UL
#endif /* !SHARED_BUF_OFFSET */

#endif /* _AMD_GENERATED_ */

struct remoteproc_priv {
	const char *kick_dev_name;
	const char *kick_dev_bus_name;
	struct metal_device *kick_dev;
	struct metal_io_region *kick_io;
#ifndef RPMSG_NO_IPI
	unsigned int ipi_chn_mask; /**< IPI channel mask */
	atomic_int ipi_nokick;
#endif /* !RPMSG_NO_IPI */
};

#if defined __cplusplus
}
#endif

#endif /* PLATFORM_INFO_H_ */
