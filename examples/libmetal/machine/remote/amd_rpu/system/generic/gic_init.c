/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/errno.h>
#include <metal/log.h>

#include <stdint.h>

#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xipipsu.h"
#include "xinterrupt_wrap.h"
#include "xil_cache.h"

#include "common.h"

static XIpiPsu ipi_inst;

extern void xlnx_irq_isr(void *arg);

/* Interrupt Controller setup */
int init_irq(void)
{
	XIpiPsu_Config *cfg_ptr;
	int status = XST_FAILURE;

	/* Look Up the config data */
	cfg_ptr = XIpiPsu_LookupConfig(XPAR_XIPIPSU_0_BASEADDR);
	if (!cfg_ptr) {
		return XST_FAILURE;
	}

	/* Init with the Cfg Data */
	status = XIpiPsu_CfgInitialize(&ipi_inst, cfg_ptr, cfg_ptr->BaseAddress);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Setup the GIC */
	status = XSetupInterruptSystem(&ipi_inst, &xlnx_irq_isr,
				       ipi_inst.Config.IntId,
				       ipi_inst.Config.IntrParent,
				       XINTERRUPT_DEFAULT_PRIORITY);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Enable reception of IPIs from all CPUs */
	XIpiPsu_InterruptEnable(&ipi_inst, XIPIPSU_ALL_MASK);

	/* Clear Any existing Interrupts */
	XIpiPsu_ClearInterruptStatus(&ipi_inst, XIPIPSU_ALL_MASK);

	Xil_ExceptionEnable();

	return 0;
}
