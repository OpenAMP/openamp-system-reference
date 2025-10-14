/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * shmem_latency_demod.c - shared memory latency demo (remote side)
 *
 * Port of the legacy libmetal remote latency sample onto the platform helpers
 * shared with the IRQ demo.
 */

#include <stdbool.h>

#include <metal/alloc.h>
#include <metal/errno.h>
#include <metal/io.h>

#include "common.h"

#define TTC_CNT_APU_TO_RPU 2 /* APU to RPU TTC counter ID */
#define TTC_CNT_RPU_TO_APU 3 /* RPU to APU TTC counter ID */

/* Shared memory offsets */
#define SHM_DEMO_CNTRL_OFFSET 0x0 /* Shared memory for the demo status */
#define SHM_BUFF_OFFSET_RX 0x1000 /* Shared memory RX buffer start offset */
#define SHM_BUFF_OFFSET_TX 0x2000 /* Shared memory TX buffer start offset */

#define DEMO_STATUS_IDLE  0x0
#define DEMO_STATUS_START 0x1 /* Status value to indicate demo start */

#define BUF_SIZE_MAX 4096

/**
 * @brief   demo() - shared memory latency remote task
 *
 * @param[in] arg - OS specific task handle used for suspend/resume
 * @return 0 on success, otherwise error code.
 */
int demo(void *arg)
{
	struct channel_s ch_s = {0};
	struct channel_s *ch = &ch_s;
	struct msg_hdr_s *msg_hdr;
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
		metal_err("REMOTE: Failed to allocate latency buffer.\n");
		ret = -ENOMEM;
		goto out;
	}

	metal_info("REMOTE: Shared memory latency demo start.\n");

	while (1) {
		system_suspend(ch);

		if (metal_io_read32(ch->shm_io, SHM_DEMO_CNTRL_OFFSET) !=
		    DEMO_STATUS_START) {
			break;
		}

		metal_io_block_read(ch->shm_io, SHM_BUFF_OFFSET_RX,
				    lbuf, sizeof(struct msg_hdr_s));
		msg_hdr = (struct msg_hdr_s *)lbuf;

		if (msg_hdr->len > (BUF_SIZE_MAX - sizeof(*msg_hdr))) {
			metal_err("REMOTE: Invalid payload length %u.\n", msg_hdr->len);
			ret = -EINVAL;
			break;
		}

		metal_io_block_read(ch->shm_io,
				    SHM_BUFF_OFFSET_RX + sizeof(*msg_hdr),
				    lbuf + sizeof(*msg_hdr), msg_hdr->len);

		/* Stop APU->RPU TTC counter */
		stop_timer(ch->ttc_io, TTC_CNT_APU_TO_RPU);

		/* Reset RPU->APU TTC counter */
		reset_timer(ch->ttc_io, TTC_CNT_RPU_TO_APU);

		metal_io_block_write(ch->shm_io, SHM_BUFF_OFFSET_TX,
				     msg_hdr, sizeof(*msg_hdr) + msg_hdr->len);

		irq_kick(ch);
	}

	metal_info("REMOTE: Shared memory latency demo complete (%d).\n", ret);

out:
	if (lbuf)
		metal_free_memory(lbuf);

	if (platform_ready)
		platform_cleanup(&ch_s);

	return ret;
}
