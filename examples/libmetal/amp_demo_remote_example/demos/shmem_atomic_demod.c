/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 /*****************************************************************************
 * atomic_shmem_demod.c - Shared memory atomic operation demo
 * This task will:
 *  1. Get the shared memory device I/O region.
 *  2. Get the IPI device I/O region.
 *  3. Register IPI interrupt handler.
 *  4. Wait for the APU to kick IPI to start the demo
 *  5. Once notification is received, start atomic add by
 *     1 for 5000 times over the shared memory
 *  6. Trigger IPI to notify the remote it has finished calculation.
 *  7. Clean up: Disable IPI interrupt, deregister the IPI interrupt handler.
 */
#include <metal/shmem.h>
#include <metal/atomic.h>
#include <metal/device.h>
#include <metal/io.h>
#include <sys/time.h>
#include <stdio.h>
#include "common.h"
#include "sys_init.h"

#define ATOMIC_INT_OFFSET 0x0 /* shared memory offset for atomic operation */
#define ITERATIONS 5000

#define SHM_DEMO_CNTRL_OFFSET	 0x500 /* Shared memory for the demo status */
#define DEMO_STATUS_IN_PROGRESS 0x0
#define DEMO_STATUS_DONE	 0x1 /* Status value to indicate demo start */

/**
 * @brief   atomic_add_shmemd() - Shared memory atomic operation demo
 *          This task will:
 *          * Wait for the remote to write to shared memory.
 *          * Once it receives the notification via polling, start atomic add by
 *            1 for 5000 times to first 32 bits of memory in the shared memory
 *            which is pointed to by shm_io.
 *          * Write to shared mem to notify the remote once it finishes
 *            calculation.
 *
 * @param[in] ipi_io - IPI metal i/o region
 * @param[in] shm_io - shared memory metal i/o region
 * @return - If setup failed, return the corresponding error number. Otherwise
 *          return 0 on success.
 */
int atomic_shmem_demod(struct channel_s *ch)
{
	atomic_int *shm_int;
	uint32_t ipi_mask = IPI_MASK;
	int i;

	print_demo("atomic operation over shared memory");

	/* clear demo status value */
	metal_io_write32(ch->shm_io, SHM_DEMO_CNTRL_OFFSET, DEMO_STATUS_IN_PROGRESS);

	LPRINTF("Starting atomic add on shared memory demo.\n");
	shm_int = (atomic_int *)metal_io_virt(ch->shm_io,
					ATOMIC_INT_OFFSET);

	/* Wait for notification from the remote to start the demo */
	suspend(ch);

	/* Do atomic add over the shared memory */
	for (i = 0; i < ITERATIONS; i++)
		atomic_fetch_add(shm_int, 1);

	/* Write to IPI trigger register to notify the remote it has finished
	 * the atomic operation. */
	metal_io_write32(ch->ipi_io, IPI_TRIG_OFFSET, ipi_mask);

	/* Wait for other side to mark test as done before proceeding. */
	while (metal_io_read32(ch->shm_io, SHM_DEMO_CNTRL_OFFSET) == DEMO_STATUS_IN_PROGRESS)
		continue;

	LPRINTF("Shared memory with atomics test finished\n");
	return 0;
}
