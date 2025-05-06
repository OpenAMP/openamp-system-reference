/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_ZYNQMP_H_
#define PLATFORM_INFO_ZYNQMP_H_

#define SOC_STR			"ZYNQMP"

/* IPI agent IDs macros */
#define IPI_ID_APU		0U
#define IPI_ID_RPU0		1U
#define IPI_ID_RPU1		2U
#define IPI_ID_PMU0		3U
#define IPI_ID_PMU1		4U
#define IPI_ID_PMU2		5U
#define IPI_ID_PMU3		6U
#define	IPI_ID_PL0		7U
#define	IPI_ID_PL1		8U
#define	IPI_ID_PL2		9U
#define	IPI_ID_PL3		10U

#define FIRST_IPI		IPI_ID_APU
#define LAST_IPI		IPI_ID_PL3

#define AGENT(ipi_tbl, ipi_idx) \
	(((ipi_tbl[ipi_idx].base_addr >> 16U) & 0xFU) - 1U)

#define	IPI_ADDR_RPU0		0xFF310000U
#define	IPI_ADDR_RPU1		0xFF320000U
#define	IPI_ADDR_PL0		0xFF340000U
#define	IPI_ADDR_PL1		0xFF350000U
#define	IPI_ADDR_PL2		0xFF360000U
#define	IPI_ADDR_PL3		0xFF370000U

/* GIC Interrupt Vector ID */
#define	IPI_VECT_ID_CH1		65U
#define	IPI_VECT_ID_CH2		66U
#define	IPI_VECT_ID_CH7		61U
#define	IPI_VECT_ID_CH8		62U
#define	IPI_VECT_ID_CH9		63U
#define	IPI_VECT_ID_CH10	64U

/* IPI Channel ID bits */
#define IPI_CH1_BIT		8U
#define IPI_CH2_BIT		9U
#define IPI_CH7_BIT		24U
#define IPI_CH8_BIT		25U
#define IPI_CH9_BIT		26U
#define IPI_CH10_BIT		27U

#define IPI_BUFFER_BASEADDR	0xFF990000U

#ifdef SDT

/* In case BSP does not provide symbol */
#ifndef XPAR_XIPIPSU_0_BASEADDR

#if XPAR_CPU_ID == 0
#define XPAR_XIPIPSU_0_BASEADDR IPI_ADDR_RPU0
#else /* RPU 1 case */
#define XPAR_XIPIPSU_0_BASEADDR IPI_ADDR_RPU1
#endif /* XPAR_CPU_ID == 0 */

#endif /* !XPAR_XIPIPSU_0_BASEADDR */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR XPAR_XIPIPSU_0_BASEADDR
#endif /* !POLL_BASE_ADDR */

/* In case BSP does not provide symbol */
#ifndef XPAR_XIPIPSU_0_INTR

#if XPAR_CPU_ID == 0
#define XPAR_XIPIPSU_0_INTR IPI_VECT_ID_CH1
#else /* RPU 1 case */
#define XPAR_XIPIPSU_0_INTR IPI_VECT_ID_CH2
#endif /* XPAR_CPU_ID == 0 */

#endif /* !XPAR_XIPIPSU_0_INTR */

#ifndef IPI_IRQ_VECT_ID
#define IPI_IRQ_VECT_ID	(XPAR_XIPIPSU_0_INTR)
#endif /* !IPI_IRQ_VECT_ID */

#ifndef IPI_CHN_BITMASK

#if XPAR_CPU_ID == 0

#ifndef XPAR_IPI1_7_IPI_BITMASK
#define XPAR_IPI1_7_IPI_BITMASK	(0x1U << IPI_CH7_BIT)
#endif /* !XPAR_IPI1_7_IPI_BITMASK */

#define IPI_CHN_BITMASK     XPAR_IPI1_7_IPI_BITMASK

#else /* RPU 1 case */

#ifndef XPAR_IPI2_8_IPI_BITMASK
#define XPAR_IPI2_8_IPI_BITMASK	(0x1U << IPI_CH8_BIT)
#endif /* !XPAR_IPI2_8_IPI_BITMASK */

#define IPI_CHN_BITMASK     XPAR_IPI2_8_IPI_BITMASK

#endif /* XPAR_CPU_ID == 0 */

#endif /* !IPI_CHN_BITMASK */

#else /* Vitis Classic Case */

#ifndef IPI_IRQ_VECT_ID
#define IPI_IRQ_VECT_ID     XPAR_XIPIPSU_0_INT_ID
#endif /* !IPI_IRQ_VECT_ID */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR      XPAR_XIPIPSU_0_BASE_ADDRESS
#endif /* !POLL_BASE_ADDR */

#if XPAR_CPU_ID == 0

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     (0x1U << IPI_CH7_BIT)
#endif /* !IPI_CHN_BITMASK */

#else /* RPU1 */

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     (0x1U << IPI_CH8_BIT)
#endif /* !IPI_CHN_BITMASK */

#endif /* XPAR_CPU_ID == 0 */

#endif /* SDT */

#endif /* PLATFORM_INFO_ZYNQMP_H_ */
