/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * shmem_throughput_demod.c - shared memory throughput demo (remote side)
 *
 * Port of the legacy libmetal remote throughput sample onto the common
 * platform helpers used by the IRQ shared-memory demo.
 */

#include <stdbool.h>
#include <string.h>

#include <metal/alloc.h>
#include <metal/errno.h>
#include <metal/io.h>

#include "common.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

/* Shared memory offsets */
#define SHM_DESC_OFFSET_RX 0x0
#define SHM_BUFF_OFFSET_RX 0x400000
#define SHM_DESC_OFFSET_TX 0x200000
#define SHM_BUFF_OFFSET_TX 0x800000

/* Shared memory descriptors offset */
#define SHM_DESC_AVAIL_OFFSET 0x00
#define SHM_DESC_ADDR_ARRAY_OFFSET 0x04

#define BUF_SIZE_MAX 4096
#define PKG_SIZE_MAX 1024
#define PKG_SIZE_MIN 16
#define TOTAL_DATA_SIZE (1024 * 4096)

/**
 * @brief demo() - shared memory throughput remote task
 *
 * @param[in] arg - OS specific task handle used for suspend/resume
 * @return 0 on success, otherwise error code.
 */
int demo(void *arg)
{
	struct channel_s ch_s = {0};
	struct channel_s *ch = &ch_s;
	bool platform_ready = false;
	void *lbuf = NULL;
	int ret = 0;

	ret = platform_init(&ch_s);
	if (ret) {
		metal_err("REMOTE: Failed to initialize platform (%d).\n", ret);
		goto out;
	}
	platform_ready = true;

	ret = amp_os_init(&ch_s, arg);
	if (ret) {
		metal_err("REMOTE: Failed to initialise OS bindings (%d).\n", ret);
		goto out;
	}

	lbuf = metal_allocate_memory(BUF_SIZE_MAX);
	if (!lbuf) {
		metal_err("REMOTE: Failed to allocate throughput buffer.\n");
		ret = -ENOMEM;
		goto out;
	}
	memset(lbuf, 0xA5, BUF_SIZE_MAX);

	metal_io_block_set(ch->shm_io, 0, 0,
			   metal_io_region_size(ch->shm_io));

	metal_info("REMOTE: Shared memory throughput demo start.\n");

	/* Download throughput measurement (host TX, remote RX). */
	for (size_t s = PKG_SIZE_MIN; s <= PKG_SIZE_MAX; s <<= 1) {
		uint32_t rx_count = 0;
		uint32_t iterations = TOTAL_DATA_SIZE / s;
		unsigned long rx_avail_offset =
			SHM_DESC_OFFSET_RX + SHM_DESC_AVAIL_OFFSET;
		unsigned long rx_addr_offset =
			SHM_DESC_OFFSET_RX + SHM_DESC_ADDR_ARRAY_OFFSET;
		unsigned long rx_data_offset =
			SHM_DESC_OFFSET_RX + SHM_BUFF_OFFSET_RX;

		system_suspend(ch);
		reset_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);

		while (1) {
			uint32_t rx_avail =
				metal_io_read32(ch->shm_io, rx_avail_offset);

			while (rx_count != rx_avail) {
				uint32_t buf_phy_addr_32 =
					metal_io_read32(ch->shm_io, rx_addr_offset);
				rx_data_offset = metal_io_phys_to_offset(
					ch->shm_io, (metal_phys_addr_t)buf_phy_addr_32);
				if (rx_data_offset == METAL_BAD_OFFSET) {
					metal_err("REMOTE: [%u] bad RX offset 0x%x at 0x%lx.\n",
						  rx_count, buf_phy_addr_32,
						  metal_io_phys(ch->shm_io, rx_addr_offset));
					ret = -EINVAL;
					goto out;
				}
				rx_addr_offset += sizeof(buf_phy_addr_32);

				metal_io_block_read(ch->shm_io, rx_data_offset, lbuf, s);
				rx_count++;
			}

			if (rx_count < iterations)
				system_suspend(ch);
			else
				break;
		}

		stop_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);

		irq_kick(ch);
	}

	/* Upload throughput measurement (remote TX, host RX). */
	for (size_t s = PKG_SIZE_MIN; s <= PKG_SIZE_MAX; s <<= 1) {
		uint32_t tx_count = 0;
		uint32_t iterations = TOTAL_DATA_SIZE / s;
		unsigned long tx_avail_offset =
			SHM_DESC_OFFSET_TX + SHM_DESC_AVAIL_OFFSET;
		unsigned long tx_addr_offset =
			SHM_DESC_OFFSET_TX + SHM_DESC_ADDR_ARRAY_OFFSET;
		unsigned long tx_data_offset =
			SHM_DESC_OFFSET_TX + SHM_BUFF_OFFSET_TX;

		system_suspend(ch);
		reset_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);

		while (tx_count < iterations) {
			metal_io_block_write(ch->shm_io, tx_data_offset, lbuf, s);

			uint32_t buf_phy_addr_32 =
				(uint32_t)metal_io_phys(ch->shm_io, tx_data_offset);
			metal_io_write32(ch->shm_io, tx_addr_offset, buf_phy_addr_32);
			tx_data_offset += s;
			tx_addr_offset += sizeof(buf_phy_addr_32);

			tx_count++;
			metal_io_write32(ch->shm_io, tx_avail_offset, tx_count);
			irq_kick(ch);
		}

		stop_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);

		system_suspend(ch);
		irq_kick(ch);
	}

	metal_info("REMOTE: Shared memory throughput demo complete (%d).\n", ret);

out:
	if (lbuf)
		metal_free_memory(lbuf);

	if (platform_ready)
		platform_cleanup(&ch_s);

	return ret;
}
