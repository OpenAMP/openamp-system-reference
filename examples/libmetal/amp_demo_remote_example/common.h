 /*
  * Copyright (c) 2017 - 2022, Xilinx Inc. and Contributors. All rights reserved.
  * Copyright (C) 2023 - 2024, Advanced Micro Devices, Inc.
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
#include "sys_init.h"

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

#define INTC_DEVICE_ID	XPAR_SCUGIC_0_DEVICE_ID
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
#define XTTCPS_CNT_OFFSET(ID) ((ID) == 1 ? 0 : 1 << (ID)) /* TTC counter offset
							     ID is from 1 to 3 */

/* TTC counter control masks */
#define XTTCPS_CNT_CNTRL_RST_MASK  0x10U /* TTC counter control reset mask */
#define XTTCPS_CNT_CNTRL_DIS_MASK  0x01U /* TTC counter control disable mask */

#define LPRINTF(format, ...) \
  xil_printf("\r\nSERVER> " format, ##__VA_ARGS__)

#define LPERROR(format, ...) LPRINTF("ERROR: " format, ##__VA_ARGS__)

struct msg_hdr_s {
	uint32_t index;
	uint32_t len;
};

extern struct metal_device *ipi_dev; /* IPI metal device */
extern struct metal_device *shm_dev; /* SHM metal device */
extern struct metal_device *ttc_dev; /* TTC metal device */

extern struct channel_s ch;

/**
 * @brief   atomic_shmem_demod() - Shared memory atomic operation demo
 *          This task will:
 *          * Wait for the remote to write to shared memory.
 *          * Once it receives the notification via polling, start atomic add by
 *	      1 for 1000 times to first 32 bits of memory in the
 *	      shared memory location at 3ed00000 which is pointed to by shm_io.
 *          * Write to shared mem to notify the remote once it finishes
 *            calculation.
 * @param[in] ch - channel structure that will store information for demo
 *
 * @return - If setup failed, return the corresponding error number. Otherwise
 *          return 0 on success.
 */
int atomic_shmem_demod(struct channel_s *ch);

/**
 * @brief ipi_latency_demod() - Show performance of  IPI with Libmetal.
 *        Loop until APU tells RPU to stop via shared memory.
 *        In loop, wait for interrupt (interrupt handler stops APU to
 *        RPU timer). Then reset count on RPU to APU timer to 0, start
 *        counting and send interrupt to notify APU.
 * @param[in] ch - channel structure that will store information for demo
 *
 * @return - 0 on success, error code if failure.
 */
int ipi_latency_demod(struct channel_s *ch);

/**
 * @brief   ipi_shmem_demod() - shared memory IPI demo
 *          This task will:
 *          * Wait for IPI interrupt from the remote
 *          * Once it received the interrupt, copy the content from
 *            the ping buffer to the pong buffer.
 *          * Update the shared memory descriptor for the new available
 *            pong buffer.
 *          * Trigger IPI to notifty the remote.
 * @param[in] ch - channel structure that will store information for demo
 *
 * @return - 0 on success, error code if failure.
 */
int ipi_shmem_demod(struct channel_s *ch);

/**
 * @brief shmem_demod() - Show use of shared memory with Libmetal.
 *        Until KEEP_GOING signal is stopped, keep looping.
 *        In the loop, read message from remote, add one to message and
 *        then respond. After the loop, cleanup resources.
 * @param[in] ch - channel structure that will store information for demo
 *
 * @return - return 0 on success, otherwise return error number indicating
 *           type of error
 */
int shmem_demod(struct channel_s *ch);

/**
 * @brief shmem_latency_demod() - Show performance of shared mem.
 *        Loop until APU tells RPU to stop via shared memory.
 *        In loop, wait for interrupt (interrupt handler stops APU to
 *        RPU timer). Then reset count on RPU to APU timer to 0, start
 *        counting and send interrupt to notify APU.
 * @param[in] ch - channel structure that will store information for demo
 *
 * @return - 0 on success, error code if failure.
 */
int shmem_latency_demod(struct channel_s *ch);

/**
 * @brief shmem_throughput_demod() - Show throughput of shared mem.
 *        At signal of remote, record total time to do block read and write
 *        operation Loop until APU tells RPU to stop via shared memory.
 *        In loop, wait for interrupt (interrupt handler stops APU to
 *        RPU timer). Then reset count on RPU to APU timer to 0, start
 *        counting and send interrupt to notify APU.
 * @param[in] ch - channel structure that will store information for demo
 *
 * @return - 0 on success, error code if failure.
 */
int shmem_throughput_demod(struct channel_s *ch);

/**
 * @brief print_demo() - print demo string
 *
 * @param[in] name - demo name
 */
static inline void print_demo(char *name)
{
	LPRINTF("====== libmetal demo: %s ======\n", name);
}

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
		val = metal_io_read32(ch->ipi_io, IPI_ISR_OFFSET);
		if (val & ch->ipi_mask) {
			/* stop RPU -> APU timer */
			if (ch->ttc_io) {
				stop_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);
				metal_io_write32(ch->ipi_io, IPI_ISR_OFFSET, ch->ipi_mask);
				resume(ch);		
			   return METAL_IRQ_HANDLED;
			}
		}
	}
	return METAL_IRQ_NOT_HANDLED;
}

/**
 * demo_prepare() - prepare environment for demo before run
 * @param[in] ch - channel information.
 *
 * @return - return 0 on success. Non-zero on failure.
 */
static inline int demo_prepare(struct channel_s *ch)
{
	metal_assert(ch);
	metal_assert(ch->ipi_io);

	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, IPI_MASK);
	/* clear old IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_ISR_OFFSET, IPI_MASK);

	/* Enable IPI interrupt */
	metal_irq_enable(ch->ipi_irq);
	metal_io_write32(ch->ipi_io, IPI_IER_OFFSET, IPI_MASK);

	return 0;
}

/**
 * demo_unprepare() - unprepare environment for demo after run
 * @param[in] ch - channel information.
 *
 * @return - return 0 on success. Non-zero on failure.
 */
static inline int demo_unprepare(struct channel_s *ch)
{
	metal_assert(ch);
	metal_assert(ch->ipi_io);

	/* disable IPI interrupt */
	metal_io_write32(ch->ipi_io, IPI_IDR_OFFSET, IPI_MASK);
	metal_irq_disable(ch->ipi_irq);

	return 0;
}

int libmetal_amp_demo_remote(void *arg);

#endif /* __COMMON_H__ */
