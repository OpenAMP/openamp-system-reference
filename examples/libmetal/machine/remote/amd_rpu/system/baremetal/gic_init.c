/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"

#include <metal/errno.h>

#include "common.h"

extern void xlnx_irq_isr(void *arg);

static XScuGic GicInstance;

/* Interrupt Controller setup */
int init_irq(void)
{
	XScuGic_Config *cfg;
	int status;

	Xil_ExceptionDisable();

	cfg = XScuGic_LookupConfig(XPAR_XSCUGIC_0_BASEADDR);
	if (!cfg) {
		metal_err("REMOTE: XScuGic lookup failed\n");
		return -ENODEV;
	}

	status = XScuGic_CfgInitialize(&GicInstance, cfg, cfg->CpuBaseAddress);
	if (status != XST_SUCCESS) {
		metal_err("REMOTE: XScuGic init failed: %d\n", status);
		return -EIO;
	}

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				     (Xil_ExceptionHandler)XScuGic_InterruptHandler,
				     &GicInstance);

	status = XScuGic_Connect(&GicInstance, IPI_IRQ_VECT_ID,
				 (Xil_ExceptionHandler)xlnx_irq_isr,
				 (void *)(uintptr_t)IPI_IRQ_VECT_ID);
	if (status != XST_SUCCESS) {
		metal_err("REMOTE: XScuGic connect failed: %d\n", status);
		return -EIO;
	}

	XScuGic_Enable(&GicInstance, IPI_IRQ_VECT_ID);
	Xil_ExceptionEnable();

	return 0;
}
