/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include <metal/sys.h>
#include <metal/irq.h>
#include "platform_info.h"
#include <stdio.h>
#include <stdarg.h>
#include "xil_exception.h"
#include "xparameters.h"
#include "xscugic.h"

#define FREERTOS_BSP_IRQ_OFFSET 32

int xlnx_hw_to_bsp_irq(int sys_irq)
{
	return sys_irq - FREERTOS_BSP_IRQ_OFFSET;
}

/* Interrupt Controller setup */
int system_interrupt_register(int int_num, void (*intr_handler)(void *),
			      void *data)
{
	long ret;

	/*
	 * Register the ISR with the interrupt controller instance
	 * initialized by porting layer.
	 * BSP returns success of this call with ret = 1.
	 * This is unconventional so return 0 for success else -EINVAL.
	 */
	ret = xPortInstallInterruptHandler(int_num,
					   intr_handler,
					   data);
	if (!ret) {
		metal_err("failed to register interrupt %d\n", int_num);
		return -EINVAL;
	}

	metal_dbg("sys interrupt %d config success\n", int_num);

	return 0;
}
