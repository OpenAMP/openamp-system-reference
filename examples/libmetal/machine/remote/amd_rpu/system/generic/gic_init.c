/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/errno.h>
#include <metal/log.h>

#include <stdint.h>

#include "xparameters.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xipipsu.h"
#include "xinterrupt_wrap.h"
#include "xil_cache.h"

#include "common.h"

static XIpiPsu ipi_inst;

extern void xlnx_irq_isr(void *arg);

/* Handler for AMD specific baremetal BSP IPI driver */
void IpiIntrHandler(void *arg)
{

	XIpiPsu *ipi = (XIpiPsu *)arg;
	u32 src_mask, idx;

	metal_assert(ipi);

	src_mask = XIpiPsu_GetInterruptStatus(ipi);

	for (idx = 0U; idx < ipi->Config.TargetCount; idx++) {
		if (src_mask & ipi->Config.TargetList[idx].Mask) {
			/* Raise to Generic Libmetal ISR */
			xlnx_irq_isr((void*)IPI_IRQ_VECT_ID);
		}
	}
}

/* Interrupt Controller setup */
int init_irq(void)
{
	XIpiPsu_Config *cfg_ptr;
	int status = XST_FAILURE;

	/* Look Up the config data */
	cfg_ptr = XIpiPsu_LookupConfig(IPI_BASE_ADDR);
	if (!cfg_ptr) {
		return XST_FAILURE;
	}

	/* Init with the Cfg Data */
	status = XIpiPsu_CfgInitialize(&ipi_inst, cfg_ptr, cfg_ptr->BaseAddress);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Setup the GIC */
	status = XSetupInterruptSystem(&ipi_inst, &IpiIntrHandler,
				       ipi_inst.Config.IntId,
				       ipi_inst.Config.IntrParent,
				       XINTERRUPT_DEFAULT_PRIORITY);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Clear Any existing Interrupts */
	XIpiPsu_ClearInterruptStatus(&ipi_inst, XIPIPSU_ALL_MASK);

	return 0;
}
