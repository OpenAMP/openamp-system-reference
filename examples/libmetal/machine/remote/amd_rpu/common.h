 /*
  * Copyright (C) 2025, Advanced Micro Devices, Inc.
  *
  * SPDX-License-Identifier: BSD-3-Clause
  */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <metal/atomic.h>
#include <metal/alloc.h>
#include <metal/irq.h>
#include <metal/irq_controller.h>
#include <metal/errno.h>
#include <metal/sys.h>
#include <metal/cpu.h>
#include <metal/io.h>
#include <metal/device.h>
#include <sys/types.h>
#include "platform_init.h"

#include "xil_printf.h"
#include "xil_exception.h"

#include "amp_demo_os.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

#define TTC_CLK_FREQ_HZ	100000000

/*
 * For Versal and future SOCs, pm_defs.h exists today but will be
 * deprecated. Use xpm_defs.h for those board families.
 */
#if defined(PLATFORM_ZYNQMP)
#include "pm_defs.h"
#else
#include "xpm_nodeid.h"
#endif

#if defined(PLATFORM_ZYNQMP)

#define TTC_NODEID NODE_TTC_0
#define TTC0_BASE_ADDR 0xff110000
#define TTC_DEV_NAME "ff110000.ttc"
#define IPI_MASK 0x1000000

#if XPAR_CPU_ID == 0
#define IPI_DEV_NAME "ff310000.ipi"
#define IPI_BASE_ADDR 0xff310000
#define IPI_IRQ_VECT_ID 65
#else
#define IPI_DEV_NAME "ff320000.ipi"
#define IPI_BASE_ADDR 0xff320000
#define IPI_IRQ_VECT_ID 66
#endif

#elif defined(versal)
#define TTC0_BASE_ADDR 0xFF0E0000
#define IPI_BASE_ADDR 0xFF340000
#define IPI_IRQ_VECT_ID 63
#define IPI_MASK 0x0000020
#define TTC_DEV_NAME "ff0e0000.ttc"
#define IPI_DEV_NAME "ff340000.ipi"

#elif defined(VERSAL_NET)

#ifdef IS_VERSAL2

#define TTC0_BASE_ADDR 0xF1E60000
#define TTC_DEV_NAME "f1e60000.ttc"

#else  /* Versal NET Case */

#define TTC0_BASE_ADDR 0xFD1C0000
#define TTC_DEV_NAME "fd1c0000.ttc"

#endif /* IS_VERSAL2 */

#define IPI_BASE_ADDR 0xEB340000
#define IPI_IRQ_VECT_ID 90
#define IPI_MASK 0x0000020
#define IPI_DEV_NAME "eb340000.ipi"
#endif

/* Symbol name is same for all non-ZynqMP SOC's. */
#ifndef PLATFORM_ZYNQMP
#define TTC_NODEID PM_DEV_TTC_0
#endif

/* Devices names */
#define BUS_NAME        "generic"
#define SHM_DEV_NAME	"3ed80000.shm"
#define SHM_SIZE	0x200000
#define SHM_BASE_ADDR   0x3ED80000

/* IPI registers offset */
#define IPI_TRIG_OFFSET 0x0  /* IPI trigger reg offset */
#define IPI_OBS_OFFSET  0x4  /* IPI observation reg offset */
#define IPI_ISR_OFFSET  0x10 /* IPI interrupt status reg offset */
#define IPI_IMR_OFFSET  0x14 /* IPI interrupt mask reg offset */
#define IPI_IER_OFFSET  0x18 /* IPI interrupt enable reg offset */
#define IPI_IDR_OFFSET  0x1C /* IPI interrupt disable reg offset */

/* TTC counter offsets */
#define XTTCPS_CLK_CNTRL_OFFSET 0x0  /* TTC counter clock control reg offset */
#define XTTCPS_CNT_CNTRL_OFFSET 0xC  /* TTC counter control reg offset */
#define XTTCPS_CNT_VAL_OFFSET   0x18 /* TTC counter val reg offset */
#define XTTCPS_CNT_OFFSET(ID) ((ID) == 1 ? 0 : 1 << (ID))

/* TTC counter control masks */
#define XTTCPS_CNT_CNTRL_RST_MASK  0x10U /* TTC counter control reset mask */
#define XTTCPS_CNT_CNTRL_DIS_MASK  0x01U /* TTC counter control disable mask */

struct msg_hdr_s {
	uint32_t index;
	uint32_t len;
};

extern struct metal_device *ipi_dev; /* IPI metal device */
extern struct metal_device *shm_dev; /* SHM metal device */
extern struct metal_device *ttc_dev; /* TTC metal device */

/**
 * @brief reset_timer() - function to reset TTC counter
 *        Set the RST bit in the Count Control Reg.
 *
 * @param[in] ttc_io - TTC timer i/o region
 * @param[in] cnt_id - counter id
 */
static inline void reset_timer(struct metal_io_region *ttc_io,
			unsigned long cnt_id)
{
	uint32_t val;
	unsigned long offset = XTTCPS_CNT_CNTRL_OFFSET +
				XTTCPS_CNT_OFFSET(cnt_id);

	val = XTTCPS_CNT_CNTRL_RST_MASK;
	metal_io_write32(ttc_io, offset, val);
}

/**
 * @brief stop_timer() - function to stop TTC counter
 *        Set the disable bit in the Count Control Reg.
 *
 * @param[in] ttc_io - TTC timer i/o region
 * @param[in] cnt_id - counter id
 */
static inline void stop_timer(struct metal_io_region *ttc_io,
			unsigned long cnt_id)
{
	uint32_t val;
	unsigned long offset = XTTCPS_CNT_CNTRL_OFFSET +
				XTTCPS_CNT_OFFSET(cnt_id);

	val = XTTCPS_CNT_CNTRL_DIS_MASK;
	metal_io_write32(ttc_io, offset, val);
}

/**
 * @ AMD RPU port for IRQ notification
 * @param[in] irq_io - IO region used for IRQ kick
 */
static inline void irq_kick(struct channel_s *ch)
{
	metal_assert(ch);
	metal_io_write32(ch->ipi_io, IPI_TRIG_OFFSET, ch->ipi_mask);
}

int demo(void *arg);

#endif /* __COMMON_H__ */
