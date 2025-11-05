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
#include "xil_printf.h"
#include "xparameters.h"
#include "xscugic.h"

/*
 * If BSP is generated using Vitis or Yocto then header file
 * already generates interupt id with the offset.
 * If either of above tools are not used then have to use correct offset to
 * configure interrupts correctly with freertos BSP.
 */
#ifndef _AMD_GENERATED_
#define FREERTOS_IRQ_OFFSET 32
#else
#define FREERTOS_IRQ_OFFSET 0
#endif

/* Interrupt Controller setup */
int system_interrupt_register(int int_num, void (*intr_handler)(void *),
			      void *data)
{
	long ret;

	int_num = int_num - FREERTOS_IRQ_OFFSET;
	/*
	 * Register the ISR with the interrupt controller instance
	 * initialized by porting layer.
	 */
	ret = xPortInstallInterruptHandler(int_num,
					   intr_handler,
					   data);
	return (int)ret;
}
