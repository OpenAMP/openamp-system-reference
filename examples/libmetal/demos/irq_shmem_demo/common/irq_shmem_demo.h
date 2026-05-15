/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __IRQ_SHMEM_DEMO_H__
#define __IRQ_SHMEM_DEMO_H__

#include <stdint.h>

#include <metal/io.h>

/*
 * Common transport state shared by the host and remote implementations of the
 * IRQ shared-memory demo. Platform- or OS-private wait state is kept behind
 * machine_ctx to avoid duplicating this layout in each machine header.
 */
struct channel_s {
	struct metal_io_region *host_to_remote_desc_io; /* host to remote descriptors */
	struct metal_io_region *remote_to_host_desc_io; /* remote to host descriptors */
	struct metal_io_region *ipi_io; /* IPI metal i/o region */
	struct metal_io_region *shm_io; /* Shared memory metal i/o region */
	struct metal_io_region *ttc_io; /* TTC metal i/o region */
	void *machine_ctx; /* Platform- or OS-private channel state */
	uint32_t ipi_mask; /* RPU IPI mask */
	int irq_vector_id; /* IRQ number. */
};

#endif /* __IRQ_SHMEM_DEMO_H__ */
