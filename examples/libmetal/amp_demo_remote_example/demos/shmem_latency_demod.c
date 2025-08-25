/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*****************************************************************************
 * shmem_latency_demod.c
 * This is the remote side of the IPI latency measurement demo.
 * This demo does the following steps:
 *
 *  1. Get the shared memory device libmetal I/O region.
 *  1. Get the TTC timer device libemtal I/O region.
 *  2. Get IPI device libmetal I/O region and the IPI interrupt vector.
 *  3. Register IPI interrupt handler.
 *  6. When it receives IPI interrupt, the IPI interrupt handler marked the
 *     remote has kicked.
 *  7. Check the shared memory to see if demo is on. If the demo is on,
 *     copy data from the shared memory to local memory, stop the APU to RPU
 *     timer. Reset the RPU to APU TTC counter, copy data from local memory
 *     to shared memory, kick IPI to notify the remote.
 *  8. If the shared memory indicates the demo is off, cleanup resource:
 *     disable IPI interrupt and deregister the IPI interrupt handler.
 */

#include <unistd.h>
#include <metal/atomic.h>
#include <metal/io.h>
#include <metal/device.h>
#include <metal/irq.h>
#include "common.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

#define TTC_CLK_FREQ_HZ	100000000

/* Shared memory offset */
#define SHM_DEMO_CNTRL_OFFSET 0x0 /* Shared memory for the demo status */
#define SHM_BUFF_OFFSET_RX 0x1000 /* Shared memory RX buffer start offset */
#define SHM_BUFF_OFFSET_TX 0x2000 /* Shared memory TX buffer start offset */

#define DEMO_STATUS_IDLE         0x0
#define DEMO_STATUS_START        0x1 /* Status value to indicate demo start */

#define BUF_SIZE_MAX 4096

/**
 * @brief measure_shmem_latencyd() - measure shmem latency with libmetal
 *        Loop until APU tells RPU to stop via shared memory.
 *        In loop, wait for interrupt (interrupt handler stops APU to
 *        RPU TTC counter). Then reset count on RPU to APU TTC counter
 *        and kick IPI to notify APU.
 *
 * @param[in] ch - channel information
 * @return - 0 on success, error code if failure.
 */
int shmem_latency_demod(struct channel_s *ch)
{
	void *lbuf = NULL;
	struct msg_hdr_s *msg_hdr;
	int ret = 0;

	/* allocate memory for receiving data */
	lbuf = metal_allocate_memory(BUF_SIZE_MAX);
	if (!lbuf) {
		LPERROR("Failed to allocate memory.\r\n");
		return -1;
	}

	LPRINTF("Starting IPI latency demo\r\n");
	while(1) {
		suspend(ch);
		if (metal_io_read32(ch->shm_io, SHM_DEMO_CNTRL_OFFSET) ==
			DEMO_STATUS_START) {
			/* Read message header from shared memory */
			metal_io_block_read(ch->shm_io, SHM_BUFF_OFFSET_RX,
				lbuf, sizeof(struct msg_hdr_s));
			msg_hdr = (struct msg_hdr_s *)lbuf;

			/* Check if the message header is valid */
			if (msg_hdr->len > (BUF_SIZE_MAX - sizeof(*msg_hdr))) {
				LPERROR("wrong msg: length invalid: %u, %u.\n",
					BUF_SIZE_MAX - sizeof(*msg_hdr),
					msg_hdr->len);
				ret = -EINVAL;
				goto out;
			}
			/* Read message */
			metal_io_block_read(ch->shm_io,
					SHM_BUFF_OFFSET_RX + sizeof(*msg_hdr),
					lbuf + sizeof(*msg_hdr), msg_hdr->len);
			/* Stop APU to RPU TTC counter */
			stop_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);

			/* Reset RPU to APU TTC counter */
			reset_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);
			/* Copy the message back to the other end */
			metal_io_block_write(ch->shm_io, SHM_BUFF_OFFSET_TX,
					msg_hdr,
					sizeof(*msg_hdr) + msg_hdr->len);

			/* Kick IPI to notify the remote */
			metal_io_write32(ch->ipi_io, IPI_TRIG_OFFSET,
					ch->ipi_mask);
		} else {
			break;
		}
	}

out:
	metal_free_memory(lbuf);
	return ret;
}
