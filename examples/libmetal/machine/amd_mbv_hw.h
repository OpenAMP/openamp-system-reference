/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Register maps of the AXI Mailbox and, for the optional latency profiling
 * (MBV_PROFILE), the AXI Timer, shared by the libmetal Linux host
 * (host/amd_linux_userspace, PL Mailbox backend) and the MicroBlaze-V
 * remote (remote/amd_mbv).
 *
 * Why these are constants: the register offsets and bit fields are fixed
 * by the IP core, the same for every instance in every design, so they are
 * not generated. Lopper and the device tree give only where an instance
 * is (its base, size and interrupt), never its register layout.
 *
 * Why they are here: both sides access these devices directly, with no
 * driver that would supply the offsets. The Linux host reaches them through
 * UIO, and the remote's libmetal device must use the same accessors. The
 * same pattern holds for the PS IPI offsets in
 * host/amd_linux_userspace/notify_ipi.c.
 *
 * References:
 *   AMD Mailbox v2.1 LogiCORE IP Product Guide (PG114), "Register Space";
 *   offsets are per processor-facing interface.
 *   AMD AXI Timer v2.0 LogiCORE IP Product Guide (PG079), "Register Space".
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

/*
 * AXI Timer v2.0 registers (PG079), used only by MBV_PROFILE. Counter 0
 * and counter 1 run in cascade mode as one free-running 64-bit up counter
 * (TCR1:TCR0), which the host starts and both sides read.
 */
#define TIMER_TCSR0		0x00 /* counter 0 control/status */
#define TIMER_TLR0		0x04 /* counter 0 load */
#define TIMER_TCR0		0x08 /* counter 0 value, low 32 bits */
#define TIMER_TCSR1		0x10 /* counter 1 control/status */
#define TIMER_TLR1		0x14 /* counter 1 load */
#define TIMER_TCR1		0x18 /* counter 1 value, high 32 bits */

#define TIMER_TCSR_ARHT		(1U << 4)  /* auto reload */
#define TIMER_TCSR_LOAD		(1U << 5)  /* load from TLR */
#define TIMER_TCSR_ENT		(1U << 7)  /* enable */
#define TIMER_TCSR_CASC		(1U << 11) /* cascade the two counters */

/*
 * Mailbox words with MBV_PROFILE. A notification is one word: a sequence
 * number. With profiling, the remote's reply to the host's k-th message is
 * three words, back to back:
 *   k | MBV_PROF_REPLY, t1, t2
 * where t1 is when the remote read the host's k-th word and t2 when it
 * sent the reply, both in low 32 bits of the shared timer. The flag lets a
 * host tell such a reply from a plain notification, so a host and remote
 * built differently do not misread each other's words.
 */
#define MBV_PROF_REPLY		0x80000000U
#define MBV_PROF_SEQ_MASK	0x7FFFFFFFU

#endif /* __AMD_MBV_HW_H__ */
