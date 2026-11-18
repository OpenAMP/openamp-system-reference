/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Notification between the Linux host and the remote: the host kicks the
 * remote, and the remote's kick clears remote_nkicked in the channel's
 * machine context from the interrupt handler.
 *
 * platform_init.c is common to every remote. It opens the notification
 * device by its stable UIO name, maps its first I/O region into
 * ch->ipi_io, takes its interrupt into ch->irq_vector_id and then hands the
 * channel to the backend for that device:
 *   notify_pl_mbox.c   "libmetal-mbox", AXI Mailbox, for PL remotes such as
 *                      a MicroBlaze-V
 *   notify_ipi.c       "libmetal-ipi", PS IPI, for RPU remotes
 * The first of these devices that exists is used. A MicroBlaze-V device
 * tree overlay adds "libmetal-mbox" to a base device tree that already has
 * "libmetal-ipi", so the mailbox is looked for first.
 */

#ifndef __NOTIFY_H__
#define __NOTIFY_H__

#include "irq_shmem_demo.h"

struct notify_ops {
	/* linux,uio-name of the notification device */
	const char *dev_name;
	/* program the device and register its interrupt handler */
	int (*init)(struct channel_s *ch);
	/* notify the remote */
	void (*kick)(struct channel_s *ch);
	/* disable and unregister the interrupt */
	void (*cleanup)(struct channel_s *ch);
};

extern const struct notify_ops notify_ipi_ops;
extern const struct notify_ops notify_pl_mbox_ops;

#endif /* __NOTIFY_H__ */
