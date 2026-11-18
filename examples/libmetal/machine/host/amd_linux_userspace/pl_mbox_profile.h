/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Optional notification latency profiling of the PL Mailbox backend
 * (MBV_PROFILE). The host and the MicroBlaze-V remote read one shared AXI
 * Timer, so their timestamps compare directly. Each round trip gives:
 *   t0  host, before it writes its notification word
 *   t1  remote, when it reads that word
 *   t2  remote, before it writes its reply
 *   t3  host, when it reads the reply
 * The remote returns t1 and t2 with its reply (see amd_mbv_hw.h), and the
 * host prints min/avg/max of t1-t0, t2-t1, t3-t2 and t3-t0 at cleanup.
 *
 * It needs a PL design with the shared timer, exported to the host as the
 * UIO device "libmetal-mbv-timer" with its clock-frequency device-tree
 * property, a libmetal relation that names the timer (so the remote's
 * configuration gives its address), and both sides built with
 * MBV_PROFILE. Without MBV_PROFILE these calls compile to nothing.
 */

#ifndef __PL_MBOX_PROFILE_H__
#define __PL_MBOX_PROFILE_H__

#include <stdint.h>
#include <metal/io.h>

#ifdef MBV_PROFILE

/**
 * @brief mbv_prof_init() - open the shared timer and start it
 *
 * @return 0 for success, negative value for failure.
 */
int mbv_prof_init(void);

/**
 * @brief mbv_prof_kick() - record t0 of the host notification seq
 */
void mbv_prof_kick(uint32_t seq);

/**
 * @brief mbv_prof_reply() - record a timestamped remote reply
 *
 * Called from the mailbox handler with the first word of a reply, which
 * has MBV_PROF_REPLY set; reads the two timestamp words that follow it.
 */
void mbv_prof_reply(struct metal_io_region *mbox_io, uint32_t word);

/**
 * @brief mbv_prof_report() - print the latencies and close the timer
 */
void mbv_prof_report(uint32_t kicks);

#else

static inline int mbv_prof_init(void) { return 0; }
static inline void mbv_prof_kick(uint32_t seq) { (void)seq; }
static inline void mbv_prof_reply(struct metal_io_region *mbox_io,
				  uint32_t word)
{
	(void)mbox_io;
	(void)word;
}

static inline void mbv_prof_report(uint32_t kicks) { (void)kicks; }

#endif /* MBV_PROFILE */

#endif /* __PL_MBOX_PROFILE_H__ */
