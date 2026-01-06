/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * irq_shmem_demo.c - shared memory with IRQ demo
 *
 * This demo will:
 * 1. Open the shared memory device.
 * 2. Open the IRQ device.
 * 3. Register the IRQ interrupt handler.
 * 4. Write a message to the shared memory.
 * 5. Kick the IRQ to notify the remote there is a new message.
 * 6. Wait until the remote notifies that the message was echoed back.
 * 7. Read the message from shared memory.
 * 8. Verify the message.
 * 9. Repeat steps 4 to 8 for 100 iterations.
 * 10. Clean up: deregister the IRQ handler, close the IRQ device, and close the
 *     shared memory device.
 *
 * Shared-memory partitioning details are documented in machine/host/
 * amd_linux_userspace/README.md.
 */

#include <unistd.h>
#include <stdio.h>
#include <metal/sys.h>
#include <metal/io.h>
#include <metal/alloc.h>
#include <sys/types.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/errno.h>

#include "common.h"
#include "platform_init.h"

/* Shared memory offsets */
#define SHM_DESC_OFFSET_TX 0x0
#define SHM_BUFF_OFFSET_TX 0x04000
#define SHM_DESC_OFFSET_RX 0x02000
#define SHM_BUFF_OFFSET_RX 0x104000

/* Shared memory descriptors offset */
#define SHM_DESC_AVAIL_OFFSET 0x00
#define SHM_DESC_USED_OFFSET  0x04
#define SHM_DESC_ADDR_ARRAY_OFFSET 0x08

/* Descriptor regions for each direction. */
/* Note that H_TO_R_ is host to remote and R_TO_H_ is vice versa. */
#define H_TO_R_DESC_ADDR_START SHM_DESC_ADDR_ARRAY_OFFSET
#define H_TO_R_DESC_ADDR_END   SHM0_DESC_SIZE
#define R_TO_H_DESC_ADDR_START SHM_DESC_ADDR_ARRAY_OFFSET
#define R_TO_H_DESC_ADDR_END   SHM1_DESC_SIZE

/* Split of the data / payload area for each direction */
#define H_TO_R_PAYLOAD_START   SHM_PAYLOAD_RX_OFFSET
#define H_TO_R_PAYLOAD_END     (SHM_PAYLOAD_RX_OFFSET + SHM_PAYLOAD_HALF_SIZE)
#define R_TO_H_PAYLOAD_START   SHM_PAYLOAD_TX_OFFSET
#define R_TO_H_PAYLOAD_END     (SHM_PAYLOAD_TX_OFFSET + SHM_PAYLOAD_HALF_SIZE)

#define PKGS_TOTAL 1024

#define BUF_SIZE_MAX 512
#define SHUTDOWN "shutdown"

#define NS_PER_S  (1000 * 1000 * 1000)

struct msg_hdr_s {
	uint32_t index;
	uint32_t len;
};

/**
 * @brief wait_for_notified() - Loop until notified bit in channel is set.
 *
 * @param[in] notified - pointer to the notified variable
 */
static inline void wait_for_notified(atomic_flag *notified)
{
	unsigned int flags;

	do {
		flags = metal_irq_save_disable();
		if (!atomic_flag_test_and_set(notified)) {
			metal_irq_restore_enable(flags);
			break;
		}
		metal_cpu_yield();
		metal_irq_restore_enable(flags);
	} while (1);
}

/**
 * @brief dump_buffer() - print hex value of each byte in the buffer
 *
 * @param[in] buf - pointer to the buffer
 * @param[in] len - len of the buffer
 */
static inline void dump_buffer(void *buf, unsigned int len)
{
	unsigned int i;
	unsigned char *tmp = (unsigned char *)buf;

	for (i = 0; i < len; i++) {
		metal_info("HOST:  %02x", *(tmp++));
		if (!(i % 20))
			metal_info("HOST:\n");
	}
	metal_info("HOST:\n");
}

/**
 * @brief   irq_shmem_echo() - shared memory IRQ demo
 *          This task will:
 *          * Get the timestamp and put it into the ping shared memory
 *          * Update the shared memory descriptor for the new available
 *            ping buffer.
 *          * Trigger IRQ to notifty the remote.
 *          * Repeat the above steps until it sends out all the packages.
 *          * Monitor IRQ interrupt, verify every received package.
 *          * After all the packages are received, it sends out shutdown
 *            message to the remote.
 *
 * @param[in] ch - communication channel used
 * @return - return 0 on success, otherwise return error number indicating
 *           type of error.
 */
static int irq_shmem_echo(struct channel_s *ch)
{
	struct metal_io_region *desc_host_to_remote = ch->host_to_remote_desc_io;
	struct metal_io_region *desc_remote_to_host = ch->remote_to_host_desc_io;
	struct metal_io_region *payload_io = ch->shm_io;
	unsigned long tx_avail_offset, rx_avail_offset;
	unsigned long tx_addr_offset, rx_addr_offset;
	unsigned long tx_data_offset, rx_data_offset;
	void *txbuf = NULL, *rxbuf = NULL, *tmpptr;
	long long tdiff_avg_s = 0, tdiff_avg_ns = 0;
	unsigned long long tstart, tend;
	unsigned long rx_used_offset;
	struct msg_hdr_s *msg_hdr;
	uint32_t tx_phy_addr_32;
	uint32_t rx_avail;
	long long tdiff;
	uint32_t i;
	int ret;

	txbuf = metal_allocate_memory(BUF_SIZE_MAX);
	if (!txbuf) {
		metal_err("HOST: Failed to allocate local tx buffer for msg.\n");
		ret = -ENOMEM;
		goto out;
	}
	rxbuf = metal_allocate_memory(BUF_SIZE_MAX);
	if (!rxbuf) {
		metal_err("HOST: Failed to allocate local rx buffer for msg.\n");
		ret = -ENOMEM;
		goto out;
	}

	if (!ch || !ch->shm_io || !ch->host_to_remote_desc_io ||
	    !ch->remote_to_host_desc_io || !ch->ipi_io) {
		return -EINVAL;
	}

	/* Clear shared memory and descriptors */
	ret = metal_io_block_set(ch->shm_io, 0, 0, SHM_PAYLOAD_SIZE);
	if (ret < 0) {
		metal_err("HOST: Failed to clear payload area.\n");
		goto out;
	}

	ret = metal_io_block_set(ch->host_to_remote_desc_io, 0, 0, SHM0_DESC_SIZE);
	if (ret < 0) {
		metal_err("HOST: Failed to clear host to remote descriptor area.\n");
		goto out;
	}

	ret = metal_io_block_set(ch->remote_to_host_desc_io, 0, 0, SHM1_DESC_SIZE);
	if (ret < 0) {
		metal_err("HOST: Failed to clear remote to host descriptor area.\n");
		goto out;
	}

	/* Set tx/rx buffer address offset */
	tx_avail_offset = SHM_DESC_AVAIL_OFFSET;
	rx_avail_offset = SHM_DESC_AVAIL_OFFSET;
	rx_used_offset = SHM_DESC_USED_OFFSET;
	tx_addr_offset = H_TO_R_DESC_ADDR_START;
	rx_addr_offset = R_TO_H_DESC_ADDR_START;
	tx_data_offset = H_TO_R_PAYLOAD_START;
	rx_data_offset = R_TO_H_PAYLOAD_START;

	metal_info("HOST: Start echo flood testing....\n");
	metal_info("HOST: Sending msgs to the remote.\n");

	for (i = 0; i < PKGS_TOTAL; i++) {

		/* Construct a message to send */
		tmpptr = txbuf;
		msg_hdr = tmpptr;
		msg_hdr->index = i;
		msg_hdr->len = sizeof(tstart);
		tmpptr += sizeof(struct msg_hdr_s);
		tstart = platform_gettime();
		*(unsigned long long *)tmpptr = tstart;

		/* Copy message to shared buffer. */
		ret = metal_io_block_write(payload_io, tx_data_offset, msg_hdr,
					   sizeof(struct msg_hdr_s) + msg_hdr->len);
		if (ret < 0) {
			metal_err("HOST: Failed to copy message to shared buffer.\n");
			goto out;
		}

		/* Write to the address array to tell the other end the buffer address. */
		tx_phy_addr_32 = (uint32_t)metal_io_phys(payload_io,
							 tx_data_offset);
		if (tx_phy_addr_32 == (uint32_t)METAL_BAD_PHYS) {
			metal_err("HOST: Failed to get offset.\n");
			ret = -EINVAL;
			goto out;
		}

		metal_io_write32(desc_host_to_remote, tx_addr_offset, tx_phy_addr_32);
		tx_data_offset += sizeof(struct msg_hdr_s) + msg_hdr->len;
		tx_addr_offset += sizeof(uint32_t);
		if (tx_addr_offset >= H_TO_R_DESC_ADDR_END)
			tx_addr_offset = H_TO_R_DESC_ADDR_START;

		/* Increase number of available buffers */
		metal_io_write32(desc_host_to_remote, tx_avail_offset, (i + 1));
		/* Kick IRQ to notify data has been put to shared buffer */
		irq_kick(ch);
	}
	metal_info("HOST: Waiting for messages to echo back and verify.\n");
	i = 0;
	tx_data_offset = H_TO_R_PAYLOAD_START;

	while (i != PKGS_TOTAL) {

		wait_for_notified(&ch->remote_nkicked);

		rx_avail = metal_io_read32(desc_remote_to_host, rx_avail_offset);
		while (i != rx_avail) {
			uint32_t rx_phy_addr_32;

			/* Received pong from the other side */

			/*
			 * Get the buffer location from the shared memory RX address array.
			 */
			rx_phy_addr_32 = metal_io_read32(desc_remote_to_host,
							 rx_addr_offset);
			rx_data_offset = metal_io_phys_to_offset(payload_io,
								 (metal_phys_addr_t)rx_phy_addr_32);
			if (rx_data_offset == METAL_BAD_OFFSET) {
				metal_err("HOST: failed to get rx [%d] offset: 0x%x.\n",
					  i, rx_phy_addr_32);
				ret = -EINVAL;
				goto out;
			}
			rx_addr_offset += sizeof(rx_phy_addr_32);

			/* Read message header from shared memory */
			ret = metal_io_block_read(payload_io, rx_data_offset, rxbuf,
						  sizeof(struct msg_hdr_s));
			if (ret < 0) {
				metal_err("HOST: Failed to read from shared memory.\n");
				goto out;
			}

			msg_hdr = (struct msg_hdr_s *)rxbuf;

			/* Check if the message header is valid */
			if (msg_hdr->index != (uint32_t)i) {
				metal_err("HOST: wrong msg: expected: %d, actual: %d\n",
					  i, msg_hdr->index);
				ret = -EINVAL;
				goto out;
			}
			if (msg_hdr->len != sizeof(tstart)) {
				metal_err("HOST: wrong msg: length invalid: %lu, %u.\n",
					  sizeof(tstart), msg_hdr->len);
				ret = -EINVAL;
				goto out;
			}
			/* Read message */
			rx_data_offset += sizeof(*msg_hdr);
			ret = metal_io_block_read(payload_io,
						  rx_data_offset,
						  rxbuf + sizeof(*msg_hdr), msg_hdr->len);
			if (ret < 0) {
				metal_err("HOST: Failed to read from shared memory.\n");
				goto out;
			}

			rx_data_offset += msg_hdr->len;
			/*
			 * Increase RX used count to indicate it has consumed the received data.
			 */
			metal_io_write32(desc_remote_to_host, rx_used_offset, (i + 1));

			/* Verify message */
			/* Get tx message previously sent*/
			ret = metal_io_block_read(payload_io, tx_data_offset, txbuf,
						  sizeof(*msg_hdr) + sizeof(tstart));
			if (ret < 0) {
				metal_err("HOST: Failed to read tx data.\n");
				goto out;
			}

			tx_data_offset += sizeof(*msg_hdr) + sizeof(tstart);
			/* Compare the received message and the sent message */
			ret = memcmp(rxbuf, txbuf, sizeof(*msg_hdr) + sizeof(tstart));
			if (ret) {
				metal_err("HOST: data[%u] verification failed.\n", i);
				metal_info("HOST: Expected:");
				dump_buffer(txbuf,	sizeof(*msg_hdr) + sizeof(tstart));
				metal_info("HOST: Actual:");
				dump_buffer(rxbuf, sizeof(*msg_hdr) + sizeof(tstart));
				ret = -EINVAL;
				goto out;
			}

			i++;
		}
	}
	tend = platform_gettime();
	tdiff = tend - tstart;

	/* Send shutdown message */
	tmpptr = txbuf;
	msg_hdr = tmpptr;
	msg_hdr->index = i;
	msg_hdr->len = strlen(SHUTDOWN);
	tmpptr += sizeof(struct msg_hdr_s);
	sprintf(tmpptr, SHUTDOWN);
	/* copy message to shared buffer */
	ret = metal_io_block_write(payload_io, tx_data_offset, msg_hdr,
				   sizeof(struct msg_hdr_s) + msg_hdr->len);
	if (ret < 0) {
		metal_err("HOST: Failed to write to tx data.\n");
		goto out;
	}

	tx_phy_addr_32 = (uint32_t)metal_io_phys(payload_io, tx_data_offset);
	if (tx_phy_addr_32 == (uint32_t)METAL_BAD_PHYS) {
		metal_err("HOST: failed to get offset.\n");
		ret = -EINVAL;
		goto out;
	}

	metal_io_write32(desc_host_to_remote, tx_addr_offset, tx_phy_addr_32);
	metal_io_write32(desc_host_to_remote, tx_avail_offset, PKGS_TOTAL + 1);
	metal_info("HOST: Kick remote to notify shutdown message sent...\n");
	irq_kick(ch);

	tdiff /= PKGS_TOTAL;
	tdiff_avg_s = tdiff / NS_PER_S;
	tdiff_avg_ns = tdiff % NS_PER_S;
	metal_info("HOST: Total packages: %d, time_avg = %lds, %ldns\n",
		   i, (long int)tdiff_avg_s, (long int)tdiff_avg_ns);

	ret = 0;
out:
	if (txbuf)
		metal_free_memory(txbuf);
	if (rxbuf)
		metal_free_memory(rxbuf);

	return ret;
}

int main(void)
{
	struct channel_s ch_s;
	int ret = 0;

	/* platform_init will set the OS agnostic channel information */
	ret = platform_init(&ch_s);
	if (ret) {
		metal_err("HOST: Failed to initialize system.\n");
		return ret;
	}

	metal_info("HOST: IRQ and shared memory\n");
	ret = irq_shmem_echo(&ch_s);
	platform_cleanup(&ch_s);
	return ret;
}
