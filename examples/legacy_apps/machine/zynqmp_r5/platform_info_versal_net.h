/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_VERSAL_NET_H_
#define PLATFORM_INFO_VERSAL_NET_H_

#include <metal/sys.h>

#define SOC_STR		"VERSAL-NET"

/* IPI agent IDs macros */
#define	IPI_ID_PMC		1U
#define	IPI_ID_0		2U
#define	IPI_ID_1		3U
#define	IPI_ID_2		4U
#define	IPI_ID_3		5U
#define	IPI_ID_4		6U
#define	IPI_ID_5		7U
#define	IPI_ID_PMC_NOBUF	8U
#define	IPI_ID_6_NOBUF_95	9U
#define	IPI_ID_1_NOBUF		10U
#define	IPI_ID_2_NOBUF		11U
#define	IPI_ID_3_NOBUF		12U
#define	IPI_ID_4_NOBUF		13U
#define	IPI_ID_5_NOBUF		14U
#define	IPI_ID_6_NOBUF_101	15U

#define	FIRST_IPI		IPI_ID_PMC
#define	LAST_IPI		IPI_ID_6_NOBUF_101

#define	IPI_ADDR_0			0xEB330000U
#define	IPI_ADDR_1			0xEB340000U
#define	IPI_ADDR_2			0xEB350000U
#define	IPI_ADDR_3			0xEB360000U
#define	IPI_ADDR_4			0xEB370000U
#define	IPI_ADDR_5			0xEB380000U
#define	IPI_ADDR_PMC_NOBUF		0xEB390000U
#define	IPI_ADDR_6_NOBUF_95		0xEB3A0000U
#define	IPI_ADDR_1_NOBUF		0xEB3B0000U
#define	IPI_ADDR_2_NOBUF		0xEB3B1000U
#define	IPI_ADDR_3_NOBUF		0xEB3B2000U
#define	IPI_ADDR_4_NOBUF		0xEB3B3000U
#define	IPI_ADDR_5_NOBUF		0xEB3B4000U
#define	IPI_ADDR_6_NOBUF_101		0xEB3B5000U

/* GIC Interrupt Vector ID */
#define	IPI_VECT_ID_IPI0		89U
#define	IPI_VECT_ID_IPI1		90U
#define	IPI_VECT_ID_IPI2		91U
#define	IPI_VECT_ID_IPI3		92U
#define	IPI_VECT_ID_IPI4		93U
#define	IPI_VECT_ID_IPI5		94U
#define	IPI_VECT_ID_IPI6_NOBUF_95	95U
#define	IPI_VECT_ID_IPI1_NOBUF		96U
#define	IPI_VECT_ID_IPI2_NOBUF		97U
#define	IPI_VECT_ID_IPI3_NOBUF		98U
#define	IPI_VECT_ID_IPI4_NOBUF		99U
#define	IPI_VECT_ID_IPI5_NOBUF		100U
#define	IPI_VECT_ID_IPI6_NOBUF_101	101U

#define	IPI_BUFFER_BASEADDR	0xEB3F0000U

#ifndef IPI_IRQ_VECT_ID
#ifdef USE_FREERTOS
#define IPI_IRQ_VECT_ID    (IPI_VECT_ID_IPI1 - 32U)
#else /* Default is baremetal */
#define	IPI_IRQ_VECT_ID     IPI_VECT_ID_IPI1
#endif /* USE_FREERTOS */
#endif /* !IPI_IRQ_VECT_ID */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR       IPI_ADDR_1 /* IPI base address*/
#endif /* !POLL_BASE_ADDR */

#ifndef IPI_CHN_BITMASK
#define	IPI_CHN_BITMASK     (0x1U << IPI_ID_3)
#endif /* !IPI_CHN_BITMASK */

#endif /* PLATFORM_INFO_VERSAL_NET_H_ */
