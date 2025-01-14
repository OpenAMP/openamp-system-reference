/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_VERSAL_NET_H_
#define PLATFORM_INFO_VERSAL_NET_H_

#include <metal/sys.h>

#ifndef IPI_IRQ_VECT_ID
#ifdef USE_FREERTOS
#define IPI_IRQ_VECT_ID    (IPI_VECT_ID_IPI1 - 32U)
#else /* Default is baremetal */
#define IPI_IRQ_VECT_ID     90
#endif /* USE_FREERTOS */
#endif /* !IPI_IRQ_VECT_ID */

#ifndef POLL_BASE_ADDR
#define POLL_BASE_ADDR       0xEB340000 /* IPI base address*/
#endif /* !POLL_BASE_ADDR */

#ifndef IPI_CHN_BITMASK
#define IPI_CHN_BITMASK     0x0000020
#endif /* !IPI_CHN_BITMASK */

#endif /* PLATFORM_INFO_VERSAL_NET_H_ */
