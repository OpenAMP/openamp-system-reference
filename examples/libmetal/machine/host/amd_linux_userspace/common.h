/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <sys/types.h>

#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/cpu.h>
#include <metal/irq.h>

#include <stdio.h>

#include "config.h"

/*
 * Apply this snippet to the device tree in an overlay so that Linux userspace can
 * see and use TTC0:
 *   &TTC0 {
 *         compatible = "ttc0_libmetal_demo";
 *         status = "okay";
 * };
 */

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

/**
 * @brief ipi_kick_register_handler() - register for IPI kick handler
 *
 * @param[in] hd - handler function
 * @param[in] priv - private data will be passed to the handler
 */
void ipi_kick_register_handler(metal_irq_handler hd, void *priv);

/**
 * @brief init_ipi() - Initialize IPI
 *
 * @return return 0 for success, negative value for failure.
 */
int init_ipi(void);

/**
 * @brief deinit_ipi() - Deinitialize IPI
 */
void deinit_ipi(void);

/**
 * @brief kick_ipi() - kick remote with IPI
 */
void kick_ipi(void *msg);

/**
 * @brief disable_ipi_kick() - disable IPI interrupt from remote kick
 */
void disable_ipi_kick(void);

/**
 * @brief enable_ipi_kick() - enable IPI interrupt from remote kick
 */
void enable_ipi_kick(void);

/**
 * @brief platform_gettime() - return platform-specific timestamp in ns
 */
unsigned long long platform_gettime(void);

/**
 * basic statistics
 */
struct metal_stat {
	uint64_t st_cnt;
	uint64_t st_sum;
	uint64_t st_min;
	uint64_t st_max;
};
#define STAT_INIT { .st_cnt = 0, .st_sum = 0, .st_min = ~0UL, .st_max = 0, }

/**
 * @brief update_stat() - update basic statistics
 *
 * @param[in] pst   - pointer to the struct stat
 * @param[in] val - the value for the update
 */
static inline void update_stat(struct metal_stat *pst, uint64_t val)
{
	pst->st_cnt++;
	pst->st_sum += val;
	if (pst->st_min > val)
		pst->st_min = val;
	if (pst->st_max < val)
		pst->st_max = val;
}

struct channel_s {
	struct metal_io_region *host_to_remote_desc_io; /* host to remote descriptors */
	struct metal_io_region *remote_to_host_desc_io; /* remote to host descriptors */
	struct metal_io_region *ipi_io; /* IPI metal i/o region */
	struct metal_io_region *shm_io; /* Shared memory metal i/o region */
	struct metal_io_region *ttc_io; /* TTC metal i/o region */
	atomic_flag remote_nkicked; /* IRQ kick flag */
	uint32_t ipi_mask; /* RPU IPI mask */
	int irq_vector_id; /* IRQ number. */
};

/**
 * @ AMD RPU port for IRQ notification
 * @param[in] irq_io - IO region used for IRQ kick
 */
static inline void irq_kick(struct channel_s *ch)
{
	metal_assert(ch);
	metal_io_write32(ch->ipi_io, IPI_TRIG_OFFSET, ch->ipi_mask);
}
#endif /* __COMMON_H__ */
