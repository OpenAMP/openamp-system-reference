/*
 * Copyright (c) 2022-2025, Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "common.h"

#include <metal/device.h>
#include <metal/irq.h>
#include <metal/io.h>
#include <metal/sys.h>

#include <pm_api_sys.h>
#include <xil_cache.h>
#include <xil_exception.h>
#include <xipipsu.h>
#include <xparameters.h>
#include <xreg_cortexr5.h>
#include <xscugic.h>
#include <xstatus.h>

#ifdef STDOUT_IS_16550
 #include <xuartns550_l.h>

 #define UART_BAUD 9600
#endif

/* Default generic I/O region page shift */
/*
 * Each I/O region can contain multiple pages. In FreeRTOS and bare-metal systems
 * the memory mapping is flat with no virtual memory, so we assume a single page.
 */
#define DEFAULT_PAGE_SHIFT (-1UL)
#define DEFAULT_PAGE_MASK  (-1UL)

/* Possible to control metal log build time */
#ifndef XLNX_METAL_LOG_LEVEL
#define XLNX_METAL_LOG_LEVEL METAL_LOG_INFO
#endif

void xlnx_log_handler(enum metal_log_level level, const char *format, ...);

#define XLNX_PLATFORM_METAL_INIT_PARAMS \
{ \
	.log_handler = xlnx_log_handler, \
	.log_level = XLNX_METAL_LOG_LEVEL, \
}

/**
 * Definition of the interrupt controller will be provided by
 * libmetal_xlnx_extention module
 **/
extern struct metal_irq_controller xlnx_irq_cntr;

const metal_phys_addr_t metal_phys[] = {
	IPI_BASE_ADDR, /**< base IPI address */
	SHM0_DESC_BASE, /**< host to remote descriptor base address */
	SHM1_DESC_BASE, /**< remote to host descriptor base address */
	SHM_PAYLOAD_BASE, /**< shared payload base address */
	TTC_BASE_ADDR, /**< base TTC address */
};

/*
 * Define the metal device table for IPI, descriptor, payload, and TTC devices.
 * Linux uses device trees, but remote relies on libmetal structures to
 * describe the peripherals. Because these devices are memory mapped, we must
 * expose their regions and interrupt information. The FreeRTOS memory map is
 * flat, so the virtual and physical addresses are identical.
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
		/* Host to remote descriptor device */
		.name = SHM0_DESC_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = {
			{
				.virt = (void *)SHM0_DESC_BASE,
				.physmap = &metal_phys[1],
				.size = SHM0_DESC_SIZE,
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
		/* Remote to host descriptor device */
		.name = SHM1_DESC_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = {
			{
				.virt = (void *)SHM1_DESC_BASE,
				.physmap = &metal_phys[2],
				.size = SHM1_DESC_SIZE,
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
		/* Shared payload device */
		.name = SHM_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = {
			{
				.virt = (void *)SHM_PAYLOAD_BASE,
				.physmap = &metal_phys[3],
				.size = SHM_PAYLOAD_SIZE,
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
		/* ttc */
		.name = TTC_DEV_NAME,
		.bus = NULL,
		.num_regions = 1,
		.regions = {
			{
				.virt = (void *)TTC_BASE_ADDR,
				.physmap = &metal_phys[4],
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
static struct metal_device *host_to_remote_desc_dev = NULL;
static struct metal_device *remote_to_host_desc_dev = NULL;
struct metal_device *shm_dev = NULL;
struct metal_device *ttc_dev = NULL;

/**
 * @brief enable_caches() - Enable caches
 */
void enable_caches(void)
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
void disable_caches(void)
{
	Xil_DCacheDisable();
	Xil_ICacheDisable();
}

/**
 * @brief init_uart() - Initialize UARTs
 */
void init_uart(void)
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
	if (vector >= (unsigned int)xlnx_irq_cntr.irq_num)
		return;

	/*
	 * The argument passed in here is to denote this is for our enabled interrupt.
	 * ipi_irq_handler will handle based on ISR register and determine source
	 * based on ISR.
	 */
	(void)metal_irq_handle(&xlnx_irq_cntr.irqs[IPI_IRQ_VECT_ID],
			       (int)IPI_IRQ_VECT_ID);
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
	struct metal_device *dev;
	unsigned int i;
	int ret;

	for (i = 0; i < sizeof(metal_dev_table) / sizeof(struct metal_device);
	     i++) {
		dev = &metal_dev_table[i];
		metal_info("REMOTE: registering: %d, name=%s\n", i, dev->name);
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

	/* Open payload device */
	ret = metal_device_open(BUS_NAME, SHM_DEV_NAME, &shm_dev);
	if (ret) {
		metal_err("REMOTE: Failed to open device %s.\n", SHM_DEV_NAME);
		goto out;
	}

	/* Open descriptor devices */
	ret = metal_device_open(BUS_NAME, SHM0_DESC_DEV_NAME,
				&host_to_remote_desc_dev);
	if (ret) {
		metal_err("REMOTE: Failed to open device %s.\n",
			  SHM0_DESC_DEV_NAME);
		goto out;
	}

	ret = metal_device_open(BUS_NAME, SHM1_DESC_DEV_NAME,
				&remote_to_host_desc_dev);
	if (ret) {
		metal_err("REMOTE: Failed to open device %s.\n",
			  SHM1_DESC_DEV_NAME);
		goto out;
	}

	/* Open IPI device */
	ret = metal_device_open(BUS_NAME, IPI_DEV_NAME, &ipi_dev);
	if (ret) {
		metal_err("REMOTE: Failed to open device %s.\n", IPI_DEV_NAME);
		goto out;
	}

	/* Open TTC device */
	ret = metal_device_open(BUS_NAME, TTC_DEV_NAME, &ttc_dev);
	if (ret) {
		metal_err("REMOTE: Failed to open device %s.\n", TTC_DEV_NAME);
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
	/* Close payload device */
	if (shm_dev)
		metal_device_close(shm_dev);

	/* Close descriptor devices */
	if (host_to_remote_desc_dev)
		metal_device_close(host_to_remote_desc_dev);

	if (remote_to_host_desc_dev)
		metal_device_close(remote_to_host_desc_dev);

	/* Close IPI device */
	if (ipi_dev)
		metal_device_close(ipi_dev);

	/* Close TTC device */
	if (ttc_dev)
		metal_device_close(ttc_dev);
}

static XStatus init_ttc(void)
{
	XPm_NodeStatus node_status = {0};
	XIpiPsu_Config *ipi_cfg_ptr;
	XIpiPsu ipi_inst;

	/* Look Up the config data */
	ipi_cfg_ptr = XIpiPsu_LookupConfig(XPAR_XIPIPSU_0_BASEADDR);
	if (!ipi_cfg_ptr) {
		metal_err("REMOTE: %s ERROR in getting CfgPtr\n", __func__);
		return -EINVAL;
	}

	/* Init with the Cfg Data */
	if (XIpiPsu_CfgInitialize(&ipi_inst, ipi_cfg_ptr,
				  ipi_cfg_ptr->BaseAddress) != XST_SUCCESS) {
		metal_err("REMOTE: Unable to configure IPI Instance for xilpm\n");
		return -EINVAL;
	}

	if (XPm_InitXilpm(&ipi_inst) != XST_SUCCESS) {
		metal_err("REMOTE: Failed to init xilpm\n");
		return -EINVAL;
	}

	if (XPm_GetNodeStatus(TTC_NODEID, &node_status) != XST_SUCCESS) {
		metal_err("REMOTE: XPm_GetNodeStatus failed\n");
		return -EINVAL;
	}

	/*
	 * If node status is 1, then TTC is powered on.
	 * Else attempt to power on TTC.
	 */
	if (!node_status.status == 0 &&
	    XPm_RequestNode(TTC_NODEID, PM_CAP_ACCESS, 100, 0) != XST_SUCCESS) {
		metal_err("REMOTE: Attempt to power on TTC failed.\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief ipi_irq_handler() - IPI interrupt handler
 *        It will clear the notified flag to mark it's got an IPI interrupt.
 *        It will stop the RPU->APU timer and will clear the notified
 *        flag to mark it's got an IPI interrupt
 *
 * @param[in] ch - channel to use
 *
 * @return - If the IPI interrupt is triggered by its remote, it returns
 *           METAL_IRQ_HANDLED. It returns METAL_IRQ_NOT_HANDLED, if it is
 *           not the interrupt it expected.
 *
 */
static inline int ipi_irq_handler(int vect_id, void *priv)
{
	struct channel_s *ch = (struct channel_s *)priv;
	uint32_t val;

	metal_assert(ch);

	(void)vect_id;

	if (ch) {
		val = metal_io_read32(ch->ipi_io, XIPIPSU_ISR_OFFSET);
		if (val & ch->ipi_mask) {
			/* stop RPU -> APU timer */
			if (ch->ttc_io) {
				stop_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);
				metal_io_write32(ch->ipi_io, XIPIPSU_ISR_OFFSET,
						 ch->ipi_mask);
				system_resume(ch);
				return METAL_IRQ_HANDLED;
			}
		}
	}

	return METAL_IRQ_NOT_HANDLED;
}

/**
 * @brief platform_init() - Register libmetal devices.
 *        This function register the libmetal generic bus, and then
 *        register the IPI, shared memory descriptor and shared memory
 *        devices to the libmetal generic bus.
 *
 * @return 0 - succeeded, non-zero for failures.
 */
int platform_init(struct channel_s *ch)
{
	struct metal_init_params metal_param = XLNX_PLATFORM_METAL_INIT_PARAMS;
	int ret;

	enable_caches();
	init_uart();

	if (init_irq()) {
		metal_err("REMOTE: Failed to initialize interrupt\n");
		return XST_FAILURE;
	}

	if (init_ttc() != XST_SUCCESS) {
		metal_err("REMOTE: Failed to init IPI for xilpm\n");
		return XST_FAILURE;
	}

	/* Initialize libmetal environment */
	metal_init(&metal_param);

	/* Initialize metal Xilinx IRQ controller */
	ret = metal_xlnx_irq_init();
	if (ret) {
		metal_err("REMOTE: %s: Xilinx metal IRQ controller init failed.\n",
			  __func__);
		return ret;
	}

	/* Register libmetal devices */
	ret = platform_register_metal_device();
	if (ret) {
		metal_err("REMOTE: %s: failed to register devices: %d\n", __func__, ret);
		return ret;
	}

	/* Open libmetal devices which have been registered */
	ret = open_metal_devices();
	if (ret) {
		metal_err("REMOTE: %s: failed to open devices: %d\n", __func__, ret);
		return ret;
	}

	/* wipe pending interrupts */
	ch->ipi_io = metal_device_io_region(ipi_dev, 0);
	if (!ch->ipi_io) {
		metal_err("REMOTE: Failed to map io region for %s.\n", ipi_dev->name);
	} else {
		/* disable IPI interrupt */
		metal_io_write32(ch->ipi_io, XIPIPSU_IDR_OFFSET, IPI_MASK);
		/* clear old IPI interrupt */
		metal_io_write32(ch->ipi_io, XIPIPSU_ISR_OFFSET, IPI_MASK);
	}

	ch->ipi_mask = IPI_MASK;
	if (!ch->ipi_io)
		return -ENODEV;

	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, XIPIPSU_IDR_OFFSET, IPI_MASK);
	/* clear old IPI interrupt */
	metal_io_write32(ch->ipi_io, XIPIPSU_ISR_OFFSET, IPI_MASK);
	/* Get the IPI IRQ from the opened IPI device */
	ch->irq_vector_id = (intptr_t)ipi_dev->irq_info;
	/* Register IPI irq handler */
	metal_irq_register(ch->irq_vector_id, ipi_irq_handler, ch);
	/* Enable IPI interrupt */
	metal_irq_enable(ch->irq_vector_id);
	metal_io_write32(ch->ipi_io, XIPIPSU_IER_OFFSET, ch->ipi_mask);

	/*
	 * Buffer clean up. Do this at start in case a
	 * previous run was stopped midway.
	 */
	ch->host_to_remote_desc_io = metal_device_io_region(host_to_remote_desc_dev, 0);
	if (!ch->host_to_remote_desc_io) {
		metal_err("REMOTE: Failed to map io region for %s.\n",
			  host_to_remote_desc_dev->name);
		return -ENODEV;
	}

	ch->remote_to_host_desc_io = metal_device_io_region(remote_to_host_desc_dev, 0);
	if (!ch->remote_to_host_desc_io) {
		metal_err("REMOTE: Failed to map io region for %s.\n",
			  remote_to_host_desc_dev->name);
		return -ENODEV;
	}

	ch->shm_io = metal_device_io_region(shm_dev, 0);
	if (!ch->shm_io) {
		metal_err("REMOTE: Failed to map io region for %s.\n", shm_dev->name);
		return -ENODEV;
	}

	ret = metal_io_block_set(ch->host_to_remote_desc_io, 0, 0, SHM0_DESC_SIZE);
	if (ret < 0)
		return ret;

	ret = metal_io_block_set(ch->remote_to_host_desc_io, 0, 0, SHM1_DESC_SIZE);
	if (ret < 0)
		return ret;

	ret = metal_io_block_set(ch->shm_io, 0, 0, SHM_PAYLOAD_SIZE);
	if (ret < 0)
		return ret;

	/* Get TTC IO region */
	ch->ttc_io = metal_device_io_region(ttc_dev, 0);
	if (!ch->ttc_io) {
		metal_err("REMOTE: Failed to map io region for %s.\n", ttc_dev->name);
		return -ENODEV;
	}

	return 0;
}

void platform_cleanup(struct channel_s *ch)
{
	metal_io_write32(ch->ipi_io, XIPIPSU_IDR_OFFSET, ch->ipi_mask);
	metal_irq_disable(ch->irq_vector_id);
	metal_irq_unregister(ch->irq_vector_id);
	memset(&ch, 0, sizeof(ch));

	/* Close libmetal devices which have been opened */
	close_metal_devices();
	/* Finish libmetal environment */
	metal_finish();
	disable_caches();
}

void xlnx_log_handler(enum metal_log_level level, const char *format, ...)
{
	char msg[1024];
	va_list args;

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);

	if (level > metal_get_log_level())
		return;

	xil_printf("RPU%d: %s", XPAR_CPU_ID, msg);
}
