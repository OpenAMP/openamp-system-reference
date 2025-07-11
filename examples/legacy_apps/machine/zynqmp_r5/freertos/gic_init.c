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

/* Interrupt Controller setup */
int app_gic_initialize(void)
{
	/*
	 * Register the ISR with the interrupt controller instance
	 * initialized by porting layer.
	 */
	xPortInstallInterruptHandler(IPI_IRQ_VECT_ID,
				     (Xil_ExceptionHandler)metal_xlnx_irq_isr,
				     (void *)IPI_IRQ_VECT_ID);
	return 0;
}
