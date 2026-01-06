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
#include "xipipsu.h"
#include "xttcps.h"

#include "amp_demo_os.h"

/* This provides IPI_DEV_NAME, IPI_MASK, and carveout definitions */
#include "config.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

#define TTC_CLK_FREQ_HZ	100000000

/* TTC counter offsets */
#define XTTCPS_CNT_OFFSET(ID) ((ID) == 1 ? 0 : 1 << (ID))

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
	unsigned long offset = XTTCPS_CNT_CNTRL_OFFSET +
			       XTTCPS_CNT_OFFSET(cnt_id);
	uint32_t val = XTTCPS_CNT_CNTRL_RST_MASK;

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
	unsigned long offset = XTTCPS_CNT_CNTRL_OFFSET +
			       XTTCPS_CNT_OFFSET(cnt_id);
	uint32_t val = XTTCPS_CNT_CNTRL_DIS_MASK;

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
	metal_io_write32(ch->ipi_io, XIPIPSU_TRIG_OFFSET, ch->ipi_mask);
}

int demo(void *arg);

#endif /* __COMMON_H__ */
