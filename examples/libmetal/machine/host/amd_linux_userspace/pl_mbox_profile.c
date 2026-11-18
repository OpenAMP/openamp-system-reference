/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Notification latency profiling of the PL Mailbox backend; see
 * pl_mbox_profile.h. Built only with MBV_PROFILE.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <metal/device.h>
#include <metal/log.h>
#include <metal/time.h>
#include "amd_mbv_hw.h"
#include "pl_mbox_profile.h"
#include "platform_init.h"

#define APP_UIO_BUS_NAME "uio"
#define APP_TIMER_DEV_NAME "libmetal-mbv-timer"
#define APP_TIMER_CLK_PROP "clock-frequency"

/* Round trips kept for the statistics; later ones overwrite the first. */
#define PROF_ENTRIES	2048U
/* Bound on the wait for the timestamp words that follow a reply word. */
#define PROF_WORD_SPINS	100000U

static struct metal_device *timer_dev;
static struct metal_io_region *timer_io;
static uint32_t timer_clk_hz;
static uint32_t t0[PROF_ENTRIES], t1[PROF_ENTRIES];
static uint32_t t2[PROF_ENTRIES], t3[PROF_ENTRIES];
static uint32_t replies, lost_words;
static unsigned long long calib_ns;
static uint64_t calib_ticks;

static inline uint32_t timer_read32(void)
{
	return metal_io_read32(timer_io, TIMER_TCR0);
}

static uint64_t timer_read64(void)
{
	uint32_t hi, lo, hi2;

	do {
		hi = metal_io_read32(timer_io, TIMER_TCR1);
		lo = metal_io_read32(timer_io, TIMER_TCR0);
		hi2 = metal_io_read32(timer_io, TIMER_TCR1);
	} while (hi != hi2);

	return ((uint64_t)hi << 32) | lo;
}

/* Start the shared timer as a free-running 64-bit up counter if needed. */
static void timer_start(void)
{
	if (metal_io_read32(timer_io, TIMER_TCSR0) & TIMER_TCSR_ENT)
		return;

	metal_io_write32(timer_io, TIMER_TLR0, 0);
	metal_io_write32(timer_io, TIMER_TLR1, 0);
	metal_io_write32(timer_io, TIMER_TCSR0, TIMER_TCSR_LOAD);
	metal_io_write32(timer_io, TIMER_TCSR1, TIMER_TCSR_LOAD);
	metal_io_write32(timer_io, TIMER_TCSR0, 0);
	metal_io_write32(timer_io, TIMER_TCSR1, 0);
	metal_io_write32(timer_io, TIMER_TCSR0,
			 TIMER_TCSR_CASC | TIMER_TCSR_ARHT | TIMER_TCSR_ENT);
}

int mbv_prof_init(void)
{
	int ret;

	ret = app_uio_read_dt_u32(APP_TIMER_DEV_NAME, APP_TIMER_CLK_PROP,
				  &timer_clk_hz);
	if (ret || !timer_clk_hz) {
		metal_err("HOST: Failed to read %s of %s: %d\n",
			  APP_TIMER_CLK_PROP, APP_TIMER_DEV_NAME, ret);
		return ret ? ret : -EINVAL;
	}
	ret = metal_device_open(APP_UIO_BUS_NAME, APP_TIMER_DEV_NAME, &timer_dev);
	if (ret) {
		metal_err("HOST: Failed to open timer %s: %d\n",
			  APP_TIMER_DEV_NAME, ret);
		return ret;
	}
	timer_io = metal_device_io_region(timer_dev, 0);
	if (!timer_io) {
		metal_err("HOST: Failed to map io region for %s.\n",
			  APP_TIMER_DEV_NAME);
		metal_device_close(timer_dev);
		timer_dev = NULL;
		return -ENODEV;
	}
	timer_start();
	memset(t0, 0, sizeof(t0));
	memset(t1, 0, sizeof(t1));
	memset(t2, 0, sizeof(t2));
	memset(t3, 0, sizeof(t3));
	replies = 0;
	lost_words = 0;
	calib_ns = metal_get_timestamp();
	calib_ticks = timer_read64();
	return 0;
}

void mbv_prof_kick(uint32_t seq)
{
	t0[seq % PROF_ENTRIES] = timer_read32();
}

/* Read the next word of a reply, which the remote writes right after. */
static int next_word(struct metal_io_region *mbox_io, uint32_t *word)
{
	unsigned int spins;

	for (spins = 0; spins < PROF_WORD_SPINS; spins++) {
		if (!(metal_io_read32(mbox_io, MBOX_STATUS) &
		      MBOX_STATUS_EMPTY)) {
			*word = metal_io_read32(mbox_io, MBOX_RDDATA);
			return 0;
		}
	}
	return -ETIMEDOUT;
}

void mbv_prof_reply(struct metal_io_region *mbox_io, uint32_t word)
{
	uint32_t k = (word & MBV_PROF_SEQ_MASK) % PROF_ENTRIES;
	uint32_t now = timer_read32();
	uint32_t rt1, rt2;

	if (next_word(mbox_io, &rt1) || next_word(mbox_io, &rt2)) {
		lost_words++;
		return;
	}
	t1[k] = rt1;
	t2[k] = rt2;
	t3[k] = now;
	replies++;
}

struct lat_stat {
	const char *name;
	unsigned int cnt;
	uint64_t sum, min, max;
};

static void lat_add(struct lat_stat *st, uint32_t ticks)
{
	if (!st->cnt || ticks < st->min)
		st->min = ticks;
	if (ticks > st->max)
		st->max = ticks;
	st->sum += ticks;
	st->cnt++;
}

static unsigned long long ticks_to_ns(uint64_t ticks)
{
	return ticks * 1000000000ULL / timer_clk_hz;
}

static void lat_print(const struct lat_stat *st)
{
	if (!st->cnt) {
		metal_info("HOST: %-22s no samples\n", st->name);
		return;
	}
	metal_info("HOST: %-22s n=%u min=%lluns avg=%lluns max=%lluns\n",
		   st->name, st->cnt, ticks_to_ns(st->min),
		   ticks_to_ns(st->sum / st->cnt), ticks_to_ns(st->max));
}

void mbv_prof_report(uint32_t kicks)
{
	struct lat_stat h2r = { .name = "host->remote notify" };
	struct lat_stat proc = { .name = "remote read->reply" };
	struct lat_stat r2h = { .name = "remote->host notify" };
	struct lat_stat rtt = { .name = "round trip" };
	unsigned long long now_ns = metal_get_timestamp();
	uint64_t now_ticks = timer_read64();
	uint32_t k, last, read_ticks;
	int i;

	/* Cost of one host timer read, from 64 back-to-back reads. */
	read_ticks = timer_read32();
	for (i = 0; i < 64; i++)
		(void)timer_read32();
	read_ticks = (timer_read32() - read_ticks) / 65;

	metal_info("HOST: timer %llu Hz nominal, %llu Hz measured; read cost ~%lluns\n",
		   (unsigned long long)timer_clk_hz,
		   (now_ns > calib_ns) ?
		   (unsigned long long)((now_ticks - calib_ticks) * 1000000000ULL /
					(now_ns - calib_ns)) : 0ULL,
		   ticks_to_ns(read_ticks));
	metal_info("HOST: host kicks %u, timestamped replies %u, incomplete replies %u\n",
		   kicks, replies, lost_words);
	if (!replies)
		metal_info("HOST: no remote timestamps (remote built without MBV_PROFILE)\n");

	last = kicks < PROF_ENTRIES ? kicks : PROF_ENTRIES - 1;
	for (k = 1; k <= last; k++) {
		if (!t0[k] || !t3[k])
			continue;
		lat_add(&h2r, t1[k] - t0[k]);
		lat_add(&proc, t2[k] - t1[k]);
		lat_add(&r2h, t3[k] - t2[k]);
		lat_add(&rtt, t3[k] - t0[k]);
	}

	lat_print(&h2r);
	lat_print(&proc);
	lat_print(&r2h);
	lat_print(&rtt);

	if (timer_dev)
		metal_device_close(timer_dev);
	timer_dev = NULL;
	timer_io = NULL;
}
