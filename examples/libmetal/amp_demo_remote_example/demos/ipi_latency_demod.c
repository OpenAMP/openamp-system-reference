/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*****************************************************************************
 * ipi_latency_demod.c
 * This is the remote side of the IPI latency measurement demo.
 * This demo does the following steps:
 *
 *  1. Open the shared memory device.
 *  1. Open the TTC timer device.
 *  2. Open the IPI device.
 *  3. Register IPI interrupt handler.
 *  6. When it receives IPI interrupt, the IPI interrupt handler to stop
 *     the RPU to APU TTC counter.
 *  7. Check the shared memory to see if demo is on. If the demo is on,
 *     reest the RPU to APU TTC counter and kick IPI to notify the remote.
 *  8. If the shared memory indicates the demo is off, cleanup resource:
 *     disable IPI interrupt and deregister the IPI interrupt handler.
 */

#include "common.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

/* Shared memory offset */
#define SHM_DEMO_CNTRL_OFFSET    0x0

#define DEMO_STATUS_IDLE         0x0
#define DEMO_STATUS_START        0x1 /* Status value to indicate demo start */

/**
 * @brief measure_ipi_latencyd() - measure IPI latency with libmetal
 *        Loop until APU tells RPU to stop via shared memory.
 *        In loop, wait for interrupt (interrupt handler stops APU to
 *        RPU TTC counter). Then reset count on RPU to APU TTC counter
 *        and kick IPI to notify APU.
 *
 * @param[in] ch - channel information
 * @return - 0 on success, error code if failure.
 */
int ipi_latency_demod(struct channel_s *ch)
{
	print_demo("IPI latency");
	while(1) {
		suspend(ch);
		if (metal_io_read32(ch->shm_io, SHM_DEMO_CNTRL_OFFSET) ==
			DEMO_STATUS_START) {
			/* Reset RPU to APU TTC counter */
			reset_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);
			/* Kick IPI to notify the remote */
			metal_io_write32(ch->ipi_io, IPI_TRIG_OFFSET,
					ch->ipi_mask);
		} else {
			break;
		}
	}
	return 0;
}
