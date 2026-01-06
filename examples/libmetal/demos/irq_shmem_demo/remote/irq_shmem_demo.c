/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * irq_shmem_demo.c - shared memory with IRQ demo
 *
 * Demo sequence details are documented in README.md.
 *
 * Shared-memory partitioning details are documented in machine/remote/amd_rpu/
 * README.md.
 */

#include <stdbool.h>

#include "common.h"

#define BUF_SIZE_MAX 512
#define SHUTDOWN "shutdown"

/* Shared memory offsets */
#define SHM0_DESC_OFFSET	0x0
#define SHM1_DESC_OFFSET	SHM0_DESC_SIZE
#define SHM_PAYLOAD_OFFSET	(SHM0_DESC_SIZE + SHM1_DESC_SIZE)

/* Shared memory descriptors offset */
#define SHM_DESC_AVAIL_OFFSET 0x00
#define SHM_DESC_USED_OFFSET  0x04
#define SHM_DESC_ADDR_ARRAY_OFFSET 0x08

/* Descriptor 0 (Host to Remote) resides at SHM0_DESC_OFFSET.
 * Descriptor 1 (Remote to Host) resides at SHM1_DESC_OFFSET.
 * The payload carveout begins at SHM_PAYLOAD_OFFSET and is split evenly
 * between RX (lower half) and TX (upper half).
 *
 * Note that H_TO_R_ is host to remote and R_TO_H_ is vice versa.
 */
#define SHM_DESC_OFFSET_H_TO_R SHM0_DESC_OFFSET
#define SHM_DESC_OFFSET_R_TO_H SHM1_DESC_OFFSET
#define SHM_PAYLOAD_H_TO_R    SHM_PAYLOAD_RX_OFFSET
#define SHM_PAYLOAD_R_TO_H    SHM_PAYLOAD_TX_OFFSET

#define H_TO_R_DESC_ADDR_START \
	(SHM_DESC_OFFSET_H_TO_R + SHM_DESC_ADDR_ARRAY_OFFSET)
#define H_TO_R_DESC_ADDR_END \
	(SHM_DESC_OFFSET_H_TO_R + SHM0_DESC_SIZE)
#define R_TO_H_DESC_ADDR_START \
	(SHM_DESC_OFFSET_R_TO_H + SHM_DESC_ADDR_ARRAY_OFFSET)
#define R_TO_H_DESC_ADDR_END \
	(SHM_DESC_OFFSET_R_TO_H + SHM1_DESC_SIZE)

#define H_TO_R_PAYLOAD_START   SHM_PAYLOAD_H_TO_R
#define H_TO_R_PAYLOAD_END     (SHM_PAYLOAD_H_TO_R + SHM_PAYLOAD_HALF_SIZE)
#define R_TO_H_PAYLOAD_START   SHM_PAYLOAD_R_TO_H
#define R_TO_H_PAYLOAD_END     (SHM_PAYLOAD_R_TO_H + SHM_PAYLOAD_HALF_SIZE)
#define PKGS_TOTAL 1024

/**
 * @brief   demo() - shared memory IRQ demo
 *	  This task will:
 *	  * Wait for an IRQ interrupt from the host.
 *	  * Copy the ping buffer into the pong buffer after each notification.
 *	  * Update the shared memory descriptor for the new available pong buffer.
 *	  * Trigger an IRQ to notify the host.
 * @param[in] ch - channel structure
 * @return - return 0 on success, otherwise return error number indicating
 *		 type of error.
 */
int demo(void *arg)
{
	unsigned long tx_data_offset, rx_data_offset, rx_used_offset;
	unsigned long tx_avail_offset, rx_avail_offset;
	unsigned long tx_addr_offset, rx_addr_offset;
	struct channel_s ch_s = {0x0};
	struct channel_s *ch = &ch_s;
	bool platform_ready = false;
	uint32_t rx_count, rx_avail;
	struct msg_hdr_s *msg_hdr;
	void *lbuf = NULL;
	char *payload;
	int ret = 0;

	/* platform_init will set the OS agnostic channel information */
	ret = platform_init(ch);
	if (ret) {
		metal_err("REMOTE: Failed to initialize system.\n");
		goto out;
	}

	platform_ready = true;

	/* OS specific setup for system suspend/resume done here. */
	ret = amp_os_init(ch, arg);
	if (ret) {
		metal_err("REMOTE: Failed to set task to system.\n");
		goto out;
	}

	metal_info("REMOTE: IRQ and shared memory\n");

	lbuf = metal_allocate_memory(BUF_SIZE_MAX);
	if (!lbuf) {
		metal_err("REMOTE: Failed to allocate local buffer for msg.\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Set tx/rx buffer address offset */
	tx_addr_offset = R_TO_H_DESC_ADDR_START;
	rx_addr_offset = H_TO_R_DESC_ADDR_START;
	tx_data_offset = R_TO_H_PAYLOAD_START;

	metal_info("REMOTE: Wait for echo test to start.\n");
	rx_count = 0;
	while (1) {
		system_suspend(ch);

		rx_avail = metal_io_read32(ch->shm_io,
					   SHM_DESC_OFFSET_H_TO_R +
					   SHM_DESC_AVAIL_OFFSET);
		while (rx_count != rx_avail) {
			uint32_t buf_phy_addr;
			/* Get the buffer location from the rx addr array. */
			buf_phy_addr = metal_io_read32(ch->shm_io, rx_addr_offset);
			rx_data_offset = metal_io_phys_to_offset(ch->shm_io,
								 (metal_phys_addr_t)buf_phy_addr);
			if (rx_data_offset == METAL_BAD_OFFSET) {
				metal_err("REMOTE: [%u]failed to get rx offset: 0x%x, 0x%lx.\n",
					  rx_count, buf_phy_addr,
					  metal_io_phys(ch->shm_io, rx_addr_offset));
				ret = -EINVAL;
				goto out;
			}
			rx_addr_offset += sizeof(buf_phy_addr);

			/* Read message header from shared memory */
			ret = metal_io_block_read(ch->shm_io, rx_data_offset, lbuf,
						  sizeof(struct msg_hdr_s));
			if (ret < 0) {
				metal_err("REMOTE: failed to read message header\n");
				ret = -EINVAL;
				goto out;
			}

			msg_hdr = (struct msg_hdr_s *)lbuf;

			/* Check if the message header is valid */
			if (msg_hdr->len > (BUF_SIZE_MAX - sizeof(*msg_hdr))) {
				metal_err("REMOTE: wrong msg: length invalid: %u, %u.\n",
					  BUF_SIZE_MAX - sizeof(*msg_hdr), msg_hdr->len);
				ret = -EINVAL;
				goto out;
			}
			rx_data_offset += sizeof(*msg_hdr);
			/* Read message body. */
			ret = metal_io_block_read(ch->shm_io, rx_data_offset,
						  lbuf + sizeof(*msg_hdr),
						  msg_hdr->len);
			if (ret < 0) {
				metal_err("REMOTE: failed to read message body\n");
				ret = -EINVAL;
				goto out;
			}

			rx_data_offset += msg_hdr->len;
			payload = (char *)lbuf + sizeof(*msg_hdr);
			rx_count++;
			/* Increase rx used count to indicate received data was used. */
			metal_io_write32(ch->shm_io,
					 SHM_DESC_OFFSET_H_TO_R + SHM_DESC_USED_OFFSET,
					 rx_count);

			/* Check if it is the shutdown message. */
			if (msg_hdr->len == strlen(SHUTDOWN) && !strncmp(SHUTDOWN,
									 payload,
									 strlen(SHUTDOWN))) {
				metal_info("REMOTE: Received shutdown message\n");
				ret = 0;
				goto out;
			}
			/* Copy the message back to the other end. */
			ret = metal_io_block_write(ch->shm_io, tx_data_offset, msg_hdr,
						   sizeof(struct msg_hdr_s) +
						   msg_hdr->len);
			if (ret < 0) {
				metal_err("REMOTE: failed to send message\n");
				ret = -EINVAL;
				goto out;
			}

			/* Write to address array to tell host the buffer address. */
			buf_phy_addr = (uint32_t)metal_io_phys(ch->shm_io,
							       tx_data_offset);
			if (buf_phy_addr == METAL_BAD_PHYS) {
				metal_err("REMOTE: failed to get offset.\n");
				ret = -EINVAL;
				goto out;
			}

			metal_io_write32(ch->shm_io, tx_addr_offset, buf_phy_addr);
			tx_data_offset += sizeof(struct msg_hdr_s) + msg_hdr->len;
			tx_addr_offset += sizeof(uint32_t);

			/* Increase number of available buffers. */
			metal_io_write32(ch->shm_io,
					 SHM_DESC_OFFSET_R_TO_H + SHM_DESC_AVAIL_OFFSET,
					 rx_count);

			/* Kick IRQ to notify data is in shared buffer. */
			irq_kick(ch);
		}

	}

out:
	metal_info("REMOTE: IRQ shared memory demo finished with exit code: %i.\n", ret);

	if (lbuf)
		metal_free_memory(lbuf);

	if (platform_ready)
		platform_cleanup(&ch_s);

	return ret;
}
