/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * shmem_latency_demo.c - shared memory latency demo (host side)
 *
 * Port of the legacy libmetal latency sample onto the common platform helpers
 * used by the IRQ shared-memory demo.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <metal/errno.h>
#include <metal/atomic.h>
#include <metal/alloc.h>
#include <metal/cpu.h>
#include <metal/io.h>
#include <metal/device.h>
#include <metal/irq.h>

#include "common.h"
#include "platform_init.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

#define TTC_CLK_FREQ_HZ 100000000
#define NS_PER_SEC 1000000000
#define NS_PER_TTC_TICK (NS_PER_SEC / TTC_CLK_FREQ_HZ)

/* Shared memory offsets */
#define SHM_DEMO_CNTRL_OFFSET 0x0 /* Shared memory for the demo status */
#define SHM_BUFF_OFFSET_TX 0x1000 /* Shared memory TX buffer start offset */
#define SHM_BUFF_OFFSET_RX 0x2000 /* Shared memory RX buffer start offset */

#define DEMO_STATUS_IDLE  0x0
#define DEMO_STATUS_START 0x1 /* Status value to indicate demo start */

#define ITERATIONS 1000

#define BUF_SIZE_MAX 4096
#define PKG_SIZE_MIN 16
#define PKG_SIZE_MAX 1024

struct msg_hdr_s {
	uint32_t index;
	uint32_t len;
};

/**
 * @brief measure_shmem_latency() - Measure latency using shared memory + IPI.
 *
 * @param[in] ch - channel information, which contains the mapped I/O regions.
 * @return 0 on success, error code if failure.
 */
static int measure_shmem_latency(struct channel_s *ch)
{
	void *lbuf = NULL;
	struct msg_hdr_s *msg_hdr;
	size_t s;
	int ret = 0;

	metal_info("HOST: Shared memory latency demo start; TTC tick = %u ns\n",
		   NS_PER_TTC_TICK);

	lbuf = metal_allocate_memory(BUF_SIZE_MAX);
	if (!lbuf) {
		metal_err("HOST: Failed to allocate latency buffer.\n");
		return -ENOMEM;
	}
	memset(lbuf, 0xA5, BUF_SIZE_MAX);

	/* Write to shared memory to indicate demo has started */
	metal_io_write32(ch->shm_io, SHM_DEMO_CNTRL_OFFSET, DEMO_STATUS_START);

	for (s = PKG_SIZE_MIN; s <= PKG_SIZE_MAX; s <<= 1) {
		struct metal_stat a2r = STAT_INIT;
		struct metal_stat r2a = STAT_INIT;
		int i;

		metal_info("HOST: Payload size %zu\n", s);

		for (i = 1; i <= ITERATIONS; i++) {
			ssize_t len;

			/* Reset APU to RPU TTC counter */
			reset_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);

			msg_hdr = lbuf;
			msg_hdr->index = i;
			msg_hdr->len = s - sizeof(*msg_hdr);

			len = metal_io_block_write(ch->shm_io, SHM_BUFF_OFFSET_TX,
						   lbuf, s);
			if ((size_t)len != s) {
				metal_err("HOST: Write shm failure: %zu/%zd\n", s, len);
				ret = -EIO;
				goto out;
			}

			atomic_flag_test_and_set(&ch->remote_nkicked);
			irq_kick(ch);

			/* Wait for remote notification; ISR clears the flag. */
			wait_for_notified(&ch->remote_nkicked);

			metal_io_block_read(ch->shm_io, SHM_BUFF_OFFSET_RX, lbuf, s);
			msg_hdr = lbuf;
			if (msg_hdr->len != (s - sizeof(*msg_hdr))) {
				metal_err("HOST: Read shm failure: expected %zu got %u\n",
					  s, msg_hdr->len + sizeof(*msg_hdr));
				ret = -EINVAL;
				goto out;
			}

			/* Stop RPU to APU TTC counter */
			stop_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);

			update_stat(&a2r, read_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU));
			update_stat(&r2a, read_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU));
		}

		metal_info("HOST: A->R ticks min=%llu max=%llu avg=%llu\n",
			   a2r.st_min, a2r.st_max, a2r.st_sum / a2r.st_cnt);
		metal_info("HOST: R->A ticks min=%llu max=%llu avg=%llu\n",
			   r2a.st_min, r2a.st_max, r2a.st_sum / r2a.st_cnt);
		metal_info("HOST: Avg latency A->R = %llu ns, R->A = %llu ns\n",
			   (a2r.st_sum * NS_PER_TTC_TICK) / a2r.st_cnt,
			   (r2a.st_sum * NS_PER_TTC_TICK) / r2a.st_cnt);
	}

	/* Write to shared memory to indicate demo has finished */
	metal_io_write32(ch->shm_io, SHM_DEMO_CNTRL_OFFSET, DEMO_STATUS_IDLE);

	atomic_flag_test_and_set(&ch->remote_nkicked);
	irq_kick(ch);

	metal_info("HOST: Shared memory latency demo complete\n");

out:
	metal_free_memory(lbuf);
	return ret;
}

int main(void)
{
	struct channel_s ch_s = {0};
	int ret;

	ret = platform_init(&ch_s);
	if (ret) {
		metal_err("HOST: Failed to initialize platform (%d).\n", ret);
		return ret;
	}

	ret = measure_shmem_latency(&ch_s);

	platform_cleanup(&ch_s);
	return ret;
}
