/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (c) 2015 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/sys.h>
#include <metal/irq.h>
#include <metal/irq_controller.h>
#include "platform_info.h"

#include "xil_exception.h"
#include "xparameters.h"
#include "xscugic.h"

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID

static XScuGic xInterruptController;

int xlnx_hw_to_bsp_irq(int sys_irq)
{
	return sys_irq;
}

int system_interrupt_register(int intr_num, void (*intr_handler)(void *),
			      void *data)
{
	uint32_t status;
	XScuGic_Config *int_ctrl_config; /* interrupt controller configuration params */
	uint32_t int_id;
	uint32_t mask_cpu_id = ((u32)0x1 << XPAR_CPU_ID);
	uint32_t target_cpu;

	mask_cpu_id |= mask_cpu_id << 8U;
	mask_cpu_id |= mask_cpu_id << 16U;

	Xil_ExceptionDisable();

	/*
	 * Initialize the interrupt controller driver
	 */
	int_ctrl_config = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (int_ctrl_config == NULL)
		return XST_FAILURE;

	status = XScuGic_CfgInitialize(&xInterruptController, int_ctrl_config,
				       int_ctrl_config->CpuBaseAddress);
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	/* Only associate interrupt needed to this CPU */
	for (int_id = 32U; int_id < XSCUGIC_MAX_NUM_INTR_INPUTS;
	     int_id = int_id + 4U) {
		target_cpu = XScuGic_DistReadReg(&xInterruptController,
						 XSCUGIC_SPI_TARGET_OFFSET_CALC(int_id));
		/* Remove current CPU from interrupt target register */
		target_cpu &= ~mask_cpu_id;
		XScuGic_DistWriteReg(&xInterruptController,
				     XSCUGIC_SPI_TARGET_OFFSET_CALC(int_id), target_cpu);
	}
	XScuGic_InterruptMaptoCpu(&xInterruptController, XPAR_CPU_ID, intr_num);

	/*
	 * Register the interrupt handler to the hardware interrupt handling
	 * logic in the ARM processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				     (Xil_ExceptionHandler)XScuGic_InterruptHandler,
				     &xInterruptController);

	/*
	 * Disable the interrupt before enabling exception to avoid interrupts
	 * received before exception is enabled.
	 */
	XScuGic_Disable(&xInterruptController, intr_num);

	Xil_ExceptionEnable();
	/* Connect Interrupt ID with ISR */
	status = XScuGic_Connect(&xInterruptController, intr_num,
			(Xil_InterruptHandler)intr_handler,
			(void *)data);

	if (status != XST_SUCCESS)
		metal_err("failed to config interrupt %d\n", intr_num);

	metal_dbg("sys interrupt %d config success\n", intr_num);
	return status;
}
