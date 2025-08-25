/******************************************************************************
 *
 * Copyright (c) 2022-2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include <xparameters.h>
#include <xil_cache.h>
#include <xil_exception.h>
#include <xstatus.h>
#include <xscugic.h>
#include <xreg_cortexr5.h>
#include <xipipsu.h>
#include <pm_api_sys.h>

#include <metal/io.h>
#include <metal/device.h>
#include <metal/sys.h>
#include <metal/irq.h>

#include "common.h"

#ifdef STDOUT_IS_16550
 #include <xuartns550_l.h>

 #define UART_BAUD 9600
#endif

/* Default generic I/O region page shift */
/* Each I/O region can contain multiple pages.
 * In FreeRTOS and Baremetal system, the memory mapping is
 * flat, there is no virtual memory.
 * We can assume there is only one page in the FreeRTOS system.
 */
#define DEFAULT_PAGE_SHIFT (-1UL)
#define DEFAULT_PAGE_MASK  (-1UL)

#if defined(SDT)
/* System Device Tree (SDT) flow does not provide this symbol. */
static XScuGic xInterruptController;
#else
extern XScuGic xInterruptController;
#endif

/**
 * Definition of the interrupt controller will be provided by
 * libmetal_xlnx_extention module
 **/
extern struct metal_irq_controller xlnx_irq_cntr;

const metal_phys_addr_t metal_phys[] = {
	IPI_BASE_ADDR, /**< base IPI address */
	SHM_BASE_ADDR, /**< shared memory base address */
	TTC0_BASE_ADDR, /**< base TTC0 address */
};

/* Define metal devices table for IPI, shared memory and TTC devices.
 * Linux system uses device tree to describe devices. Unlike Linux,
 * there is no standard device abstraction for FreeRTOS system, we
 * uses libmetal devices structure to describe the devices we used in
 * the example.
 * The IPI, shared memory and TTC devices are memory mapped
 * devices. For this type of devices, it is required to provide
 * accessible memory mapped regions, and interrupt information.
 * In FreeRTOS system, the memory mapping is flat. As you can see
 * in the table before, we set the virtual address "virt" the same
 * as the physical address.
 */
static struct metal_device metal_dev_table[] = {
	{
		/* IPI device */
		.name = IPI_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = {
			{
				.virt = (void *)IPI_BASE_ADDR,
				.physmap = &metal_phys[0],
				.size = 0x1000,
				.page_shift = DEFAULT_PAGE_SHIFT,
				.page_mask = DEFAULT_PAGE_MASK,
				.mem_flags = DEVICE_NONSHARED | PRIV_RW_USER_RW,
				.ops = {NULL},
			}
		},
		.node = {NULL},
		.irq_num = 1,
		.irq_info = (void *)IPI_IRQ_VECT_ID,
	},
	{
		/* Shared memory management device */
		.name = SHM_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = {
			{
				.virt = (void *)SHM_BASE_ADDR,
				.physmap = &metal_phys[1],
				.size = 0x1000000,
				.page_shift = DEFAULT_PAGE_SHIFT,
				.page_mask = DEFAULT_PAGE_MASK,
				.mem_flags = NORM_SHARED_NCACHE |
						PRIV_RW_USER_RW,
				.ops = {NULL},
			}
		},
		.node = {NULL},
		.irq_num = 0,
		.irq_info = NULL,
	},
	{
		/* ttc0 */
		.name = TTC_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = {
			{
				.virt = (void *)TTC0_BASE_ADDR ,
				.physmap = &metal_phys[2],
				.size = 0x1000,
				.page_shift = DEFAULT_PAGE_SHIFT,
				.page_mask = DEFAULT_PAGE_MASK,
				.mem_flags = DEVICE_NONSHARED | PRIV_RW_USER_RW,
				.ops = {NULL},
			}
		},
		.node = {NULL},
		.irq_num = 0,
		.irq_info = NULL,
	},
};

/**
 * Extern global variables
 */
struct metal_device *ipi_dev = NULL;
struct metal_device *shm_dev = NULL;
struct metal_device *ttc_dev = NULL;

/**
 * @brief enable_caches() - Enable caches
 */
void enable_caches()
{
#ifdef __MICROBLAZE__
#ifdef XPAR_MICROBLAZE_USE_ICACHE
	Xil_ICacheEnable();
#endif
#ifdef XPAR_MICROBLAZE_USE_DCACHE
	Xil_DCacheEnable();
#endif
#endif
}

/**
 * @brief disable_caches() - Disable caches
 */
void disable_caches()
{
	Xil_DCacheDisable();
	Xil_ICacheDisable();
}

/**
 * @brief init_uart() - Initialize UARTs
 */
void init_uart()
{
#ifdef STDOUT_IS_16550
	XUartNs550_SetBaud(STDOUT_BASEADDR, XPAR_XUARTNS550_CLOCK_HZ,
			   UART_BAUD);
	XUartNs550_SetLineControlReg(STDOUT_BASEADDR, XUN_LCR_8_DATA_BITS);
#endif
	/* Bootrom/BSP configures PS7/PSU UART to 115200 bps */
}

/**
 * @brief default handler
 */
void xlnx_irq_isr(void *arg)
{
       unsigned int vector;

       vector = (uintptr_t)arg;
       if (vector >= (unsigned int)xlnx_irq_cntr.irq_num) {
               return;
       }
       (void)metal_irq_handle(xlnx_irq_cntr.irqs, (int)vector);
}

int metal_xlnx_irq_init(void)
{
	int ret;

	ret =  metal_irq_register_controller(&xlnx_irq_cntr);
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s: register irq controller failed.\n",
			  __func__);
		return ret;
	}
	return 0;
}

/**
 * @brief platform_register_metal_device() - Statically Register libmetal
 *        devices.
 *        This function registers the IPI, shared memory and
 *        TTC devices to the libmetal generic bus.
 *        Libmetal uses bus structure to group the devices. Before you can
 *        access the device with libmetal device operation, you will need to
 *        register the device to a libmetal supported bus.
 *        For non-Linux system, libmetal only supports "generic" bus, which is
 *        used to manage the memory mapped devices.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
int platform_register_metal_device(void)
{
	unsigned int i;
	int ret;
	struct metal_device *dev;

	for (i = 0; i < sizeof(metal_dev_table)/sizeof(struct metal_device);
	     i++) {
		dev = &metal_dev_table[i];
		xil_printf("registering: %d, name=%s\n", i, dev->name);
		ret = metal_register_generic_device(dev);
		if (ret)
			return ret;
	}
	return 0;
}

/**
 * @brief open_metal_devices() - Open registered libmetal devices.
 *        This function opens all the registered libmetal devices.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
int open_metal_devices(void)
{
	int ret;

	/* Open shared memory device */
	ret = metal_device_open(BUS_NAME, SHM_DEV_NAME, &shm_dev);
	if (ret) {
		LPERROR("Failed to open device %s.\n", SHM_DEV_NAME);
		goto out;
	}

	/* Open IPI device */
	ret = metal_device_open(BUS_NAME, IPI_DEV_NAME, &ipi_dev);
	if (ret) {
		LPERROR("Failed to open device %s.\n", IPI_DEV_NAME);
		goto out;
	}

	/* Open TTC device */
	ret = metal_device_open(BUS_NAME, TTC_DEV_NAME, &ttc_dev);
	if (ret) {
		LPERROR("Failed to open device %s.\n", TTC_DEV_NAME);
		goto out;
	}

out:
	return ret;
}

/**
 * @brief close_metal_devices() - close libmetal devices
 *        This function closes all the libmetal devices which have
 *        been opened.
 *
 */
void close_metal_devices(void)
{
	/* Close shared memory device */
	if (shm_dev)
		metal_device_close(shm_dev);

	/* Close IPI device */
	if (ipi_dev)
		metal_device_close(ipi_dev);

	/* Close TTC device */
	if (ttc_dev)
		metal_device_close(ttc_dev);
}

static XStatus init_ttc(void)
{
	XPm_NodeStatus NodeStatus = {0};
	XIpiPsu_Config *IpiCfgPtr;
	XIpiPsu IpiInst;

	/* Look Up the config data */
	IpiCfgPtr = XIpiPsu_LookupConfig(XPAR_XIPIPSU_0_BASEADDR);
	if (!IpiCfgPtr) {
		LPERROR("%s ERROR in getting CfgPtr\n", __func__);
		return -EINVAL;
	}

	/* Init with the Cfg Data */
	if (XIpiPsu_CfgInitialize(&IpiInst, IpiCfgPtr, IpiCfgPtr->BaseAddress) != XST_SUCCESS) {
		LPERROR("Unable to configure IPI Instance for xilpm\n");
		return -EINVAL;
	}

	if (XPm_InitXilpm(&IpiInst) != XST_SUCCESS) {
		LPERROR("Failed to init xilpm\n");
		return -EINVAL;
	}

	if (XPm_GetNodeStatus(TTC_NODEID, &NodeStatus) != XST_SUCCESS) {
		LPERROR("XPm_GetNodeStatus failed\n");
		return -EINVAL;
	}

	/*
	 * If node status is 1, then TTC is powered on.
	 * Else attempt to power on TTC.
	 */
	if (!NodeStatus.status == 0 &&
	    XPm_RequestNode(TTC_NODEID, PM_CAP_ACCESS, 100, 0) != XST_SUCCESS) {
		LPERROR("TTC device was powered off. Attempt to power on failed.\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief sys_init() - Register libmetal devices.
 *        This function register the libmetal generic bus, and then
 *        register the IPI, shared memory descriptor and shared memory
 *        devices to the libmetal generic bus.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
int sys_init(struct channel_s *ch)
{
	struct metal_init_params metal_param = METAL_INIT_DEFAULTS;
	struct metal_io_region *io = NULL;
	int ret;

	enable_caches();
	init_uart();

	if (init_irq()) {
		LPERROR("Failed to initialize interrupt\n");
		return XST_FAILURE;
	}

	if (init_ttc() != XST_SUCCESS) {
		LPERROR("Failed to init IPI for xilpm\n");
		return XST_FAILURE;
	}

	/* Initialize libmetal environment */
	metal_init(&metal_param);

	/* Initialize metal Xilinx IRQ controller */
	ret = metal_xlnx_irq_init();
	if (ret) {
		LPERROR("%s: Xilinx metal IRQ controller init failed.\n",
			__func__);
		return ret;
	}

	/* Register libmetal devices */
	ret = platform_register_metal_device();
	if (ret) {
		LPERROR("%s: failed to register devices: %d\n", __func__, ret);
		return ret;
	}

	/* Open libmetal devices which have been registered */
	ret = open_metal_devices();
	if (ret) {
		LPERROR("%s: failed to open devices: %d\n", __func__, ret);
		return ret;
	}

	/* wipe pending interrupts */
	io = metal_device_io_region(ipi_dev, 0);
	if (!io) {
		LPERROR("Failed to map io region for %s.\n", ipi_dev->name);
	} else {
		/* disable IPI interrupt */
		metal_io_write32(io, IPI_IDR_OFFSET, IPI_MASK);
		/* clear old IPI interrupt */
		metal_io_write32(io, IPI_ISR_OFFSET, IPI_MASK);
	}

	ch->ipi_io = io;
	ch->ipi_mask = IPI_MASK;

	/*
	 * Buffer clean up. Do this at start in case a
	 * previous run was stopped midway.
	 */
	io = metal_device_io_region(shm_dev, 0);
	if (!io) {
		LPERROR("Failed to map io region for %s.\n", shm_dev->name);
	} else {
		metal_io_block_set(io, 0, 0, 0x400000);
	}

	ch->shm_io = io;

	/* Get TTC IO region */
	ch->ttc_io = metal_device_io_region(ttc_dev, 0);
	if (!ch->ttc_io) {
		LPERROR("Failed to map io region for %s.\n", ttc_dev->name);
		return -ENODEV;
	}

	/* Get the IPI IRQ from the opened IPI device */
	ch->ipi_irq = (intptr_t)ipi_dev->irq_info;

	/* Register IPI irq handler */
	metal_irq_register(ch->ipi_irq, ipi_irq_handler, ch);

	return 0;
}

void sys_cleanup(struct channel_s *ch)
{
	metal_irq_unregister(ch->ipi_irq);
	memset(&ch, 0, sizeof(ch));

	/* Close libmetal devices which have been opened */
	close_metal_devices();
	/* Finish libmetal environment */
	metal_finish();
	disable_caches();
}

int init_irq()
{
	int ret = 0;
	XScuGic_Config *IntcConfig;	/* The configuration parameters of
					 * the interrupt controller */

	Xil_ExceptionDisable();
	/*
	 * Initialize the interrupt controller driver
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return (int)XST_FAILURE;
	}

	ret = XScuGic_CfgInitialize(&xInterruptController, IntcConfig,
				       IntcConfig->CpuBaseAddress);
	if (ret != XST_SUCCESS) {
		return (int)XST_FAILURE;
	}

	/*
	 * Register the interrupt handler to the hardware interrupt handling
	 * logic in the ARM processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler)XScuGic_InterruptHandler,
			&xInterruptController);

	Xil_ExceptionEnable();
	/* Connect IPI Interrupt ID with libmetal ISR */
	XScuGic_Connect(&xInterruptController, IPI_IRQ_VECT_ID,
			   (Xil_ExceptionHandler)xlnx_irq_isr,
			   (void *)IPI_IRQ_VECT_ID);

	XScuGic_Enable(&xInterruptController, IPI_IRQ_VECT_ID);

	return 0;
}
