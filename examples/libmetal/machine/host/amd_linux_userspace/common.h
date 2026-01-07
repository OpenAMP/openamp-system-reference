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

/* This symbol is provided in case a demo config .cmake file is provided. */
#ifdef LIBMETAL_CFG_PROVIDED
/* This provides IPI_DEV_NAME, IPI_MASK, and carveout definitions */
#include "config.h"
#endif /* LIBMETAL_CFG_PROVIDED */

#define BUS_NAME        "platform"

/*
 * Shared memory carveouts (override via config.h or toolchain flags).
 */
#ifndef SHM_DEV_NAME
#define SHM_DEV_NAME    "9868000.shm"
#endif /* SHM_DEV_NAME */

#ifndef SHM0_DESC_DEV_NAME
#define SHM0_DESC_DEV_NAME     "9860000.shm_desc"
#endif /* SHM0_DESC_DEV_NAME */

#ifndef SHM1_DESC_DEV_NAME
#define SHM1_DESC_DEV_NAME     "9864000.shm_desc"
#endif /* SHM1_DESC_DEV_NAME */

#ifndef SHM_IMAGE_BASE
#define SHM_IMAGE_BASE         0x09800000U
#endif /* SHM_IMAGE_BASE */

#ifndef SHM_IMAGE_SIZE
#define SHM_IMAGE_SIZE         0x00060000U
#endif /* SHM_IMAGE_SIZE */

#ifndef SHM0_DESC_BASE
#define SHM0_DESC_BASE         0x09860000U
#endif /* SHM0_DESC_BASE */

#ifndef SHM0_DESC_SIZE
#define SHM0_DESC_SIZE         0x00004000U
#endif /* SHM0_DESC_SIZE */

#ifndef SHM1_DESC_BASE
#define SHM1_DESC_BASE         0x09864000U
#endif /* SHM1_DESC_BASE */

#ifndef SHM1_DESC_SIZE
#define SHM1_DESC_SIZE         0x00004000U
#endif /* SHM1_DESC_SIZE */

#ifndef SHM_PAYLOAD_BASE
#define SHM_PAYLOAD_BASE       0x09868000U
#endif /* SHM_PAYLOAD_BASE */

#ifndef SHM_PAYLOAD_SIZE
#define SHM_PAYLOAD_SIZE       0x00040000U
#endif /* SHM_PAYLOAD_SIZE */

#ifndef SHM_PAYLOAD_HALF_SIZE
#define SHM_PAYLOAD_HALF_SIZE  (SHM_PAYLOAD_SIZE / 2U)
#endif /* SHM_PAYLOAD_HALF_SIZE */

#ifndef SHM_PAYLOAD_TX_OFFSET
#define SHM_PAYLOAD_TX_OFFSET  SHM_PAYLOAD_HALF_SIZE
#endif /* SHM_PAYLOAD_TX_OFFSET */

#ifndef SHM_PAYLOAD_RX_OFFSET
#define SHM_PAYLOAD_RX_OFFSET  0x00000000U
#endif /* SHM_PAYLOAD_RX_OFFSET */

/* IPI/TTC defaults per platform; config.h can override by predefining. */
#if defined(PLATFORM_ZYNQMP)

#ifndef IPI_DEV_NAME
#define IPI_DEV_NAME "ff340000.ipi"
#endif /* !IPI_DEV_NAME */

#ifndef TTC_DEV_NAME
#define TTC_DEV_NAME "ff110000.timer"
#endif /* !TTC_DEV_NAME */

#ifndef IPI_MASK
#define IPI_MASK 0x100
#endif /* !IPI_MASK */

#elif defined(versal)

#ifndef IPI_DEV_NAME
#define IPI_DEV_NAME "ff360000.ipi"
#endif /* !IPI_DEV_NAME */

#ifndef IPI_MASK
#define IPI_MASK 0x08
#endif /* !IPI_MASK */

#ifndef TTC_DEV_NAME
#define TTC_DEV_NAME "ff0e0000.ttc0"
#endif /* TTC_DEV_NAME */

#elif defined(VERSAL_NET)

#ifndef IPI_DEV_NAME
#define IPI_DEV_NAME "eb3600000.ipi"
#endif /* !IPI_DEV_NAME */

#ifndef IPI_MASK
#define IPI_MASK 0x08
#endif /* !IPI_MASK */

#ifdef IS_VERSALGEN2

#ifndef TTC_DEV_NAME
#define TTC_DEV_NAME "f1e60000.ttc0"
#endif /* !TTC_DEV_NAME */

#else /* Versal NET case */

#ifndef TTC_DEV_NAME
#define TTC_DEV_NAME "fd1c0000.ttc0"
#endif /* !TTC_DEV_NAME */

#endif /* IS_VERSALGEN2 */

#endif

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
	struct metal_io_region *ipi_io; /* IPI metal i/o region */
	struct metal_io_region *shm_io; /* Shared memory metal i/o region */
	struct metal_io_region *apu_to_rpu_desc_io; /* host to remote descriptors */
	struct metal_io_region *rpu_to_apu_desc_io; /* remote to host descriptors */
	struct metal_io_region *ttc_io; /* TTC metal i/o region */
	uint32_t ipi_mask; /* RPU IPI mask */
	int irq_vector_id; /* IRQ number. */
	atomic_flag remote_nkicked; /* IRQ kick flag */
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
