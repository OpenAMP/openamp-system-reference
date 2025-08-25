/*
 * Copyright (c) 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 /***************************************************************************
  * libmetal_amp_demo.c
  * main.c
  * 
  * This file simply calls Libmetal Demo routine
  *
  */

#include <metal/atomic.h>
#include "common.h"

atomic_flag remote_nkicked; /* 0 - kicked from remote */

/**
 * @brief    main function of the demo application.
 *           Here are the steps for the main function:
 *           * Setup libmetal resources
 *           * Run the IPI with shared memory demo.
 *           * Run the shared memory demo.
 *           * Run the atomic across shared memory demo.
 *           * Run the ipi latency demo.
 *           * Run the shared memory latency demo.
 *           * Run the shared memory throughput demo.
 *           * Cleanup libmetal resources
 *           Report if any of the above demos failed.
 * @return   0 - succeeded, non-zero for failures.
 */
int main(void)
{
	/* 0 - kicked from remote */
	atomic_flag remote_nkicked = (atomic_flag)ATOMIC_FLAG_INIT;
	/* libmetal_amp_demo_remote takes in OS specific arg for channel */
	return libmetal_amp_demo_remote(&remote_nkicked);
}

