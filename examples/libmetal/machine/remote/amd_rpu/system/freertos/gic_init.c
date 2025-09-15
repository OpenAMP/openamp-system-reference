/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include <metal/sys.h>
#include <metal/irq.h>
#include "FreeRTOS.h"
#include "common.h"

extern void xlnx_irq_isr(void *arg);

/* Interrupt Controller setup */
int init_irq(void)
{
	/*
	 * Register the ISR with the interrupt controller instance
	 * initialized by porting layer.
	 */
	xPortInstallInterruptHandler(IPI_IRQ_VECT_ID,
				     (Xil_ExceptionHandler)xlnx_irq_isr,
				     (void *)IPI_IRQ_VECT_ID);
	return 0;
}
