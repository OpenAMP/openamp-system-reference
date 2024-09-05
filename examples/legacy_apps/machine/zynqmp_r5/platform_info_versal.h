/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_VERSAL_H_
#define PLATFORM_INFO_VERSAL_H_

#include <metal/sys.h>

#define SOC_STR		"VERSAL"

/* IPI agent IDs macros */
#define	IPI_ID_PMC		1U
#define	IPI_ID_0		2U
#define	IPI_ID_1		3U
#define	IPI_ID_2		4U
#define	IPI_ID_3		5U
#define	IPI_ID_4		6U
#define	IPI_ID_5		7U
#define	IPI_ID_PMCNOBUF		8U
#define	IPI_ID_6		9U

#define	FIRST_IPI			IPI_ID_PMC
#define	LAST_IPI			IPI_ID_6

#define	IPI_ADDR_PMC			0xFF320000U
#define	IPI_ADDR_0			0xFF330000U
#define	IPI_ADDR_1			0xFF340000U
#define	IPI_ADDR_2			0xFF350000U
#define	IPI_ADDR_3			0xFF360000U
#define	IPI_ADDR_4			0xFF370000u
#define	IPI_ADDR_5			0xFF380000U
#define	IPI_ADDR_PMCNOBUF		0xFF390000U
#define	IPI_ADDR_6			0xFF3A0000U

/* GIC Interrupt Vector ID */
#define	IPI_VECT_ID_PMC			59U
#define	IPI_VECT_ID_PMCNOBUF		60U
#define	IPI_VECT_ID_IPI0		62U
#define	IPI_VECT_ID_IPI1		63U
#define	IPI_VECT_ID_IPI2		64U
#define	IPI_VECT_ID_IPI3		65U
#define	IPI_VECT_ID_IPI4		66U
#define	IPI_VECT_ID_IPI5		67U
#define	IPI_VECT_ID_IPI6		68U

#define	IPI_BUFFER_BASEADDR		0xFF3F0000U

#ifndef IPI_IRQ_VECT_ID
#ifdef USE_FREERTOS
#define IPI_IRQ_VECT_ID     (IPI_VECT_ID_IPI1 - 32U)
#else /* Default is baremetal */
#define IPI_IRQ_VECT_ID     IPI_VECT_ID_IPI1
#endif /* USE_FREERTOS */
#endif /* !IPI_IRQ_VECT_ID */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR       IPI_ADDR_1 /* IPI base address*/
#endif /* !POLL_BASE_ADDR */

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     (0x1U << IPI_ID_3)
#endif /* !IPI_CHN_BITMASK */

#endif /* PLATFORM_INFO_VERSAL_H_ */
