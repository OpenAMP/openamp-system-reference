/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * shmem_throughput_demo.c - shared memory throughput demo (host side)
 *
 * Port of the legacy libmetal throughput sample onto the common platform
 * helpers shared with the IRQ demo.
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
#include <metal/utilities.h>

#include "common.h"
#include "platform_init.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

#define TTC_CLK_FREQ_HZ 100000000
#define NS_PER_SEC 1000000000

/* Shared memory offsets */
#define SHM_DESC_OFFSET_TX 0x0
#define SHM_BUFF_OFFSET_TX 0x400000
#define SHM_DESC_OFFSET_RX 0x200000
#define SHM_BUFF_OFFSET_RX 0x800000

/* Shared memory descriptors offset */
#define SHM_DESC_AVAIL_OFFSET 0x00
#define SHM_DESC_ADDR_ARRAY_OFFSET 0x04

#define ITERATIONS 1000

#define BUF_SIZE_MAX 4096
#define PKG_SIZE_MAX 1024
#define PKG_SIZE_MIN 16
#define TOTAL_DATA_SIZE (1024 * 4096)
#define MB 1048576.0f /* Float constant so MB/s computation keeps fractional precision */

/**
 * @brief measure_shmem_throughput() - Show throughput of using shared memory.
 *
 * @param[in] ch - channel information with mapped shared memory, IPI, TTC.
 * @return 0 on success, error otherwise.
 */
static int measure_shmem_throughput(struct channel_s *ch)
{
	void *lbuf = NULL;
	uint32_t *apu_tx_count = NULL;
	uint32_t *apu_rx_count = NULL;
	uint32_t *rpu_tx_count = NULL;
	uint32_t *rpu_rx_count = NULL;
	size_t num_sizes = 0;
	size_t s, i;
	int ret = 0;

	lbuf = metal_allocate_memory(BUF_SIZE_MAX);
	if (!lbuf) {
		metal_err("HOST: Failed to allocate throughput buffer.\n");
		return -ENOMEM;
	}
	memset(lbuf, 0xA5, BUF_SIZE_MAX);

	for (s = PKG_SIZE_MIN; s <= PKG_SIZE_MAX; s <<= 1)
		num_sizes++;

	apu_tx_count = metal_allocate_memory(num_sizes * sizeof(uint32_t));
	apu_rx_count = metal_allocate_memory(num_sizes * sizeof(uint32_t));
	rpu_tx_count = metal_allocate_memory(num_sizes * sizeof(uint32_t));
	rpu_rx_count = metal_allocate_memory(num_sizes * sizeof(uint32_t));
	if (!apu_tx_count || !apu_rx_count || !rpu_tx_count || !rpu_rx_count) {
		metal_err("HOST: Failed to allocate counter buffers.\n");
		ret = -ENOMEM;
		goto out;
	}

	metal_io_block_set(ch->shm_io, 0, 0,
			   metal_io_region_size(ch->shm_io));

	metal_info("HOST: Shared memory throughput demo start.\n");

	/* Upload throughput measurement (host TX, remote RX). */
	for (s = PKG_SIZE_MIN, i = 0; s <= PKG_SIZE_MAX; s <<= 1, i++) {
		uint32_t tx_count = 0;
		uint32_t iterations = TOTAL_DATA_SIZE / s;
		unsigned long tx_avail_offset, tx_addr_offset, tx_data_offset;
		uint32_t buf_phy_addr_32;

		tx_avail_offset = SHM_DESC_OFFSET_TX + SHM_DESC_AVAIL_OFFSET;
		tx_addr_offset = SHM_DESC_OFFSET_TX + SHM_DESC_ADDR_ARRAY_OFFSET;
		tx_data_offset = SHM_DESC_OFFSET_TX + SHM_BUFF_OFFSET_TX;

		reset_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);

		while (tx_count < iterations) {
			metal_io_block_write(ch->shm_io, tx_data_offset, lbuf, s);

			buf_phy_addr_32 =
				(uint32_t)metal_io_phys(ch->shm_io, tx_data_offset);
			metal_io_write32(ch->shm_io, tx_addr_offset, buf_phy_addr_32);
			tx_data_offset += s;
			tx_addr_offset += sizeof(buf_phy_addr_32);

			tx_count++;
			metal_io_write32(ch->shm_io, tx_avail_offset, tx_count);

			atomic_flag_test_and_set(&ch->remote_nkicked);
			irq_kick(ch);
		}

		stop_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);

		wait_for_notified(&ch->remote_nkicked);

		apu_tx_count[i] = read_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);
		rpu_rx_count[i] = read_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);
	}

	atomic_flag_test_and_set(&ch->remote_nkicked);
	irq_kick(ch);

	/* Download throughput measurement (remote TX, host RX). */
	for (s = PKG_SIZE_MIN, i = 0; s <= PKG_SIZE_MAX; s <<= 1, i++) {
		uint32_t rx_count = 0;
		uint32_t iterations = TOTAL_DATA_SIZE / s;
		unsigned long rx_avail_offset, rx_addr_offset, rx_data_offset;
		uint32_t buf_phy_addr_32;

		rx_avail_offset = SHM_DESC_OFFSET_RX + SHM_DESC_AVAIL_OFFSET;
		rx_addr_offset = SHM_DESC_OFFSET_RX + SHM_DESC_ADDR_ARRAY_OFFSET;
		rx_data_offset = SHM_DESC_OFFSET_RX + SHM_BUFF_OFFSET_RX;

		wait_for_notified(&ch->remote_nkicked);

		reset_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);
		while (1) {
			uint32_t rx_avail =
				metal_io_read32(ch->shm_io, rx_avail_offset);

			while (rx_count != rx_avail) {
				buf_phy_addr_32 = metal_io_read32(ch->shm_io,
							  rx_addr_offset);
				rx_data_offset = metal_io_phys_to_offset(
					ch->shm_io, (metal_phys_addr_t)buf_phy_addr_32);
				if (rx_data_offset == METAL_BAD_OFFSET) {
					metal_err("HOST: [%u] bad RX offset 0x%x (base 0x%lx).\n",
						  rx_count, buf_phy_addr_32,
						  metal_io_phys(ch->shm_io, rx_addr_offset));
					ret = -EINVAL;
					goto out;
				}
				rx_addr_offset += sizeof(buf_phy_addr_32);

				metal_io_block_read(ch->shm_io, rx_data_offset, lbuf, s);
				rx_count++;
			}

			if (rx_count < iterations) {
				wait_for_notified(&ch->remote_nkicked);
			} else {
				break;
			}
		}

		stop_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);

		atomic_flag_clear(&ch->remote_nkicked);
		atomic_flag_test_and_set(&ch->remote_nkicked);
		irq_kick(ch);

		wait_for_notified(&ch->remote_nkicked);

		apu_rx_count[i] = read_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);
		rpu_tx_count[i] = read_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);

		atomic_flag_test_and_set(&ch->remote_nkicked);
		irq_kick(ch);
	}

	{
		float mbs = TTC_CLK_FREQ_HZ * (float)(TOTAL_DATA_SIZE / MB);

		for (s = PKG_SIZE_MIN, i = 0; s <= PKG_SIZE_MAX; s <<= 1, i++) {
			metal_info("HOST: pkg %zu -> Tx %u ticks (%.1f MB/s), "
				   "Rx %u ticks (%.1f MB/s)\n",
				   s,
				   apu_tx_count[i], mbs / apu_tx_count[i],
				   apu_rx_count[i], mbs / apu_rx_count[i]);
			metal_info("HOST: pkg %zu <- Rx %u ticks (%.1f MB/s), "
				   "Tx %u ticks (%.1f MB/s)\n",
				   s,
				   rpu_rx_count[i], mbs / rpu_rx_count[i],
				   rpu_tx_count[i], mbs / rpu_tx_count[i]);
		}
	}

	metal_info("HOST: Shared memory throughput demo complete.\n");

out:
	if (apu_tx_count)
		metal_free_memory(apu_tx_count);
	if (apu_rx_count)
		metal_free_memory(apu_rx_count);
	if (rpu_tx_count)
		metal_free_memory(rpu_tx_count);
	if (rpu_rx_count)
		metal_free_memory(rpu_rx_count);
	if (lbuf)
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

	ret = measure_shmem_throughput(&ch_s);

	platform_cleanup(&ch_s);
	return ret;
}
