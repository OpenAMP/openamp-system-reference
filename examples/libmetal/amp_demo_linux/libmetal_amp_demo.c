/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 /*****************************************************************************
  * libmetal_amp_demo.c
  *
  * This application shows how to use IPI to trigger interrupt and how to
  * setup shared memory with libmetal API for communication between processors.
  *
  * This app does the following:
  * 1.  Run the shared memory echo demo task ipi_shmem_task()
  *     * Write message to the APU to RPU shared buffer.
  *     * Update the APU to RPU shared memory available index.
  *     * Trigger IPI to the remote.
  *     * Repeat the above 3 sub steps until it sends all the packages.
  *     * Wait for IPI to receive all the packages
  *     * If "shutdown" message is received, cleanup the libmetal source.
  * 2.  Run shared memory demo with shmem_task().
  *     * Open shared memory device.
  *     * For 1000 times, communicate between local and remote processes
  *       using shared memory and polling via shared memory.
  *     * Cleanup shared memory device.
  * 3.  Run the atomic demo task atomic_shmem_task():
  *     * Trigger the IPI to the remote, the remote will then start doing atomic
  *       add calculation.
  *     * Start atomic add by 1 for 1000 times to the first 32bit of the shared
  *       memory descriptor location.
  *     * Once it receives the IPI interrupt, it will check if the value stored
  *       in the shared memory descriptor location is 2000. If yes, the atomic
  *       across the shared memory passed, otherwise, it failed.
  * 4.  Demonstrate IPI latency with ipi_latency_demo_task()
  *     * Open IPI and timer devices.
  *     * For 1000 times, record APU to RPU IPI latency and RPU to APU
  *     latency. Then report average time for each direction.
  *     * Cleanup libmetal resources
  * 5.  Demonstrate shared memory latency with shmem_latency_demo_task()
  *     * Open shared memory and timer devices.
  *     * For 1000 times, record APU to RPU shared memory latency and RPU to APU
  *       latency for 8 bytes, 1/2K and 1K. Then report average time for each
  *       direction.
  *     * Cleanup libmetal resources
  * 6.  Demonstrate shared memory throughput with shmem_throughput_demo_task()
  *     * Open shared memory, IPI and timer devices.
  *     * For 1000 times, record APU block read and write times. Notify remote
  *       to run test, then similarly record RPU block read and write times for
  *       1/2KB, 1KB and 2KB. Then report average throughput for each data size
  *       and operation.
  *     * Cleanup libmetal resources
  */

#include <stdio.h>
#include <unistd.h>
#include <metal/io.h>
#include <metal/device.h>
#include "common.h"
#include "sys_init.h"

/* Used in below list of routines */
typedef int (*demo_routine)(void);

/**
 * @brief main function of the demo application.
 *        Here are the steps for the main function:
 *        * initialize libmetal environment
 *        * Run the IPI with shared memory demo.
 *        * Run the shared memory demo.
 *        * Run the atomic across shared memory demo.
 *        * Run the ipi latency demo.
 *        * Run the shared memory latency demo.
 *        * Run the shared memory throughput demo.
 *        * Cleanup libmetal environment
 *        Report if any of the above tasks failed.
 * @return   0 - succeeded, non-zero for failures.
 */
int main(int ac, char **av)
{
	unsigned int i;
	int ret, opt;

	ret = sys_init();
	if (ret) {
		LPERROR("Failed to initialize system.\n");
		return ret;
	}

	while ((opt = getopt(ac, av, "d")) != -1) {
		if (opt == 'd')
			metal_set_log_level(METAL_LOG_DEBUG);
	}
	struct metal_device *dev;
	struct metal_io_region *io;

	/* Open shared memory device */
	ret = metal_device_open(BUS_NAME, IPI_DEV_NAME, &dev);
	if (ret) {
		LPERROR("Failed to open device %s.\n", SHM_DEV_NAME);
		goto out;
	}

	io = metal_device_io_region(dev, 0);
	if (!io) {
		LPERROR("Failed to map io region for %s.\n", dev->name);
		ret = -ENODEV;
		goto out;
	}

	/* Below is list of the routines to use. This will make the
	 * main demo routine much easier to read and extend.
	 */
	demo_routine demo_list[] = {
		shmem_demo,
		atomic_shmem_demo,
		ipi_shmem_demo,
		ipi_latency_demo,
		shmem_latency_demo,
		shmem_throughput_demo
	};

	for (i = 0; i < sizeof(demo_list) / sizeof(demo_list[0]); i++) {
		sleep(1);
		ret = demo_list[i]();
		if (ret) {
			LPERROR("Demo failed.\n");
			break;
			goto out;
		}
	}

out:
	sys_cleanup();

	return ret;
}
