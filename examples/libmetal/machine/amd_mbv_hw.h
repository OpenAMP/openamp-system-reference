/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Register map of the AXI Mailbox shared by the libmetal Linux host
 * (host/amd_linux_userspace, PL Mailbox backend) and the MicroBlaze-V
 * remote (remote/amd_mbv).
 *
 * Why these are constants: the register offsets and bit fields are fixed
 * by the IP core, the same for every instance in every design, so they are
 * not generated. Lopper and the device tree give only where an instance
 * is (its base, size and interrupt), never its register layout.
 *
 * Why they are here: both sides access the mailbox through a libmetal I/O
 * region, with no driver that would supply the offsets. The Linux host
 * reaches it through UIO, and the remote's libmetal device must use the
 * same accessors. The same pattern holds for the PS IPI offsets in
 * host/amd_linux_userspace/notify_ipi.c.
 *
 * Reference: AMD Mailbox v2.1 LogiCORE IP Product Guide (PG114),
 * "Register Space". Offsets are per processor-facing interface.
 */

#ifndef __AMD_MBV_HW_H__
#define __AMD_MBV_HW_H__

#include <stdint.h>

/* AXI Mailbox v2.1 registers (PG114) */
#define MBOX_WRDATA		0x00 /* write data: push a word to the send FIFO */
#define MBOX_RDDATA		0x08 /* read data: pop a word from the receive FIFO */
#define MBOX_STATUS		0x10 /* FIFO status */
#define MBOX_ERROR		0x14 /* error status */
#define MBOX_SIT		0x18 /* send interrupt threshold */
#define MBOX_RIT		0x1C /* receive interrupt threshold */
#define MBOX_IS			0x20 /* interrupt status, write 1 to clear */
#define MBOX_IE			0x24 /* interrupt enable */
#define MBOX_IP			0x28 /* interrupt pending */
#define MBOX_CTRL		0x2C /* control */

/* MBOX_STATUS bits */
#define MBOX_STATUS_EMPTY	0x1U /* receive FIFO empty */
#define MBOX_STATUS_FULL	0x2U /* send FIFO full */

/* MBOX_IS, MBOX_IE and MBOX_IP bits */
#define MBOX_IRQ_STI		0x1U /* send FIFO at or below threshold */
#define MBOX_IRQ_RTI		0x2U /* receive FIFO above threshold */
#define MBOX_IRQ_ERR		0x4U /* error */

/* MBOX_CTRL bits */
#define MBOX_CTRL_RESET_SEND	0x1U /* empty the send FIFO */
#define MBOX_CTRL_RESET_RECV	0x2U /* empty the receive FIFO */

#endif /* __AMD_MBV_HW_H__ */
