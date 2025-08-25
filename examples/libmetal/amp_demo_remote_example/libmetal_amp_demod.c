/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 /***************************************************************************
  * libmetal_amp_demo.c
  *
  * This application shows how to use IPI to trigger interrupt and how to
  * setup shared memory with libmetal API for communication between processors.
  *
  * This application does the following:
  * 1.  Initialize the platform hardware such as UART, GIC.
  * 2.  Connect the IPI interrupt.
  * 3.  Register IPI device, shared memory descriptor device and shared memory
  *     device with libmetal in the initialization.
  * 4.  In the main application it does the following,
  *     * open the registered libmetal devices: IPI device, shared memory
  *       descriptor device and shared memory device.
  *     * Map the shared memory descriptor as non-cached memory.
  *     * Map the shared memory as non-cached memory. If you do not map the
  *       shared memory as non-cached memory, make sure you flush the cache,
  *       before you notify the remote.
  * 7.  Register the IPI interrupt handler with libmetal.
  * 8.  Run the atomic demo task ipi_task_shm_atomicd():
  *     * Wait for the IPI interrupt from the remote.
  *     * Once it receives the interrupt, it does atomic add by 1 to the
  *       first 32bit of the shared memory descriptor location by 1000 times.
  *     * It will then notify the remote after the calculation.
  *     * As the remote side also does 1000 times add after it has notified
  *       this end. The remote side will check if the result is 2000, if not,
  *       it will error.
  * 9.  Run the shared memory echo demo task ipi_task_echod()
  *     * Wait for the IPI interrupt from the other end.
  *     * If an IPI interrupt is received, copy the message to the current
  *       available RPU to APU buffer, increase the available buffer indicator,
  *       and trigger IPI to notify the remote.
  *     * If "shutdown" message is received, cleanup the libmetal source.
  */

#include "common.h"

/* Used in below list of routines */
typedef int (*demo_routine)(struct channel_s *ch);

/**
 * @brief    demo application main processing task
 *           Here are the steps for the main function:
 *           * Setup libmetal resources
 *           * Run the IPI with shared memory demo.
 *           * Run the shared memory demo.
 *           * Run the atomic across shared memory demo.
 *           * Run the ipi latency demo.
 *           * Run the shared memory latency demo.
 *           * Run the shared memory throughput demo.
 *           * Cleanup libmetal resources.
 *           Report if any of the above demos failed.
 * @return   0 - succeeded, non-zero for failures.
 */ 
int libmetal_amp_demo_remote(void *arg)
{
	struct channel_s ch;
	unsigned int i;
	int ret;

	/* sys_init will set the OS agnostic channel information */
	ret = sys_init(&ch);
	if (ret) {
		LPERROR("Failed to initialize system.\n");
		goto out;
	}

	/* OS specific setup for suspend/resume done here. */	
	ret = amp_os_init(&ch, arg);
	if (ret) {
		LPERROR("Failed to set task to demo system.\n");
		goto out;
	}

	/* Below is list of the routines to use. This will make the
	 * main demo routine much easier to read and extend.
	 */
	demo_routine demo_list[] = {
	   shmem_demod,
	   atomic_shmem_demod,
	   ipi_shmem_demod,
	   ipi_latency_demod,
	   shmem_latency_demod,
	   shmem_throughput_demod
	};	

	for (i = 0; i < sizeof(demo_list) / sizeof(demo_list[0]); i++) {
		ret = demo_prepare(&ch);
		if (ret) {
			LPERROR("Demo prepare failed.\n");
			break;
		}
		ret = demo_list[i](&ch);
		if (ret) {
			LPERROR("Demo failed.\n");
			break;
		}
		ret = demo_unprepare(&ch);
		if (ret) {
			LPERROR("Demo unprepare failed.\n");
			break;
		}
	}

out:
	sys_cleanup(ch);
	return ret;
}
