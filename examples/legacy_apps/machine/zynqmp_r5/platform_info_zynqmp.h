/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_ZYNQMP_H_
#define PLATFORM_INFO_ZYNQMP_H_

#ifdef SDT

/* In case BSP does not provide symbol */
#ifndef XPAR_XIPIPSU_0_BASEADDR

#if XPAR_CPU_ID == 0
#define XPAR_XIPIPSU_0_BASEADDR 0xff310000
#else
#define XPAR_XIPIPSU_0_BASEADDR 0xff320000
#endif /* XPAR_CPU_ID == 0 */

#endif /* !XPAR_XIPIPSU_0_BASEADDR */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR XPAR_XIPIPSU_0_BASEADDR
#endif /* !POLL_BASE_ADDR */

/* In case BSP does not provide symbol */
#ifndef XPAR_XIPIPSU_0_INTR

#if XPAR_CPU_ID == 0
#define XPAR_XIPIPSU_0_INTR 0x41
#else
#define XPAR_XIPIPSU_0_INTR 0x42
#endif /* XPAR_CPU_ID == 0 */

#endif /* !XPAR_XIPIPSU_0_INTR */

#ifndef IPI_IRQ_VECT_ID
#define IPI_IRQ_VECT_ID        XPAR_XIPIPSU_0_INTR
#endif /* !IPI_IRQ_VECT_ID */

#if XPAR_CPU_ID == 0

#ifndef XPAR_IPI1_7_IPI_BITMASK
#define XPAR_IPI1_7_IPI_BITMASK 0x01000000
#endif /* !XPAR_IPI1_7_IPI_BITMASK */

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     XPAR_IPI1_7_IPI_BITMASK
#endif /* !IPI_CHN_BITMASK */

#else

#ifndef XPAR_IPI2_8_IPI_BITMASK
#define XPAR_IPI2_8_IPI_BITMASK 0x02000000
#endif /* !XPAR_IPI2_8_IPI_BITMASK */

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     XPAR_IPI2_8_IPI_BITMASK
#endif /* !IPI_CHN_BITMASK */

#endif

#else /* Vitis Classic Case */

#ifndef IPI_IRQ_VECT_ID
#define IPI_IRQ_VECT_ID     XPAR_XIPIPSU_0_INT_ID
#endif /* !IPI_IRQ_VECT_ID */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR      XPAR_XIPIPSU_0_BASE_ADDRESS
#endif /* !POLL_BASE_ADDR */

#if XPAR_CPU_ID == 0
#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     0x01000000
#endif /* !IPI_CHN_BITMASK */
#else /* RPU1 */
#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     0x02000000
#endif /* !IPI_CHN_BITMASK */
#endif /* XPAR_CPU_ID == 0 */

#endif /* SDT */

#endif /* PLATFORM_INFO_ZYNQMP_H_ */
